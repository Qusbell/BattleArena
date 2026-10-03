#include "Camera/ShooterOutlineComponent.h"
#include "ShooterDisplayValidation.h"

#include "Camera/DeathCamActor.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Camera/ShooterRevengeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "MaterialShaderPrecompileMode.h"
#include "SceneView.h"
#include "SceneViewExtension.h"

namespace
{
	class FShooterOutlineViewExtension final : public FSceneViewExtensionBase
	{
	public:
		FShooterOutlineViewExtension(const FAutoRegister& Register, UShooterOutlineComponent* InOwner)
			: FSceneViewExtensionBase(Register), Owner(InOwner) {}
		virtual void SetupView(FSceneViewFamily& Family, FSceneView& View) override
		{
			UShooterOutlineComponent* Component = Owner.Get();
			APlayerController* PC = Component ? Cast<APlayerController>(Component->GetOwner()) : nullptr;
			if (!IsValid(Component) || Component->IsBeingDestroyed() || !IsValid(PC)
				|| PC->IsActorBeingDestroyed() || !PC->GetLocalPlayer()
				|| View.PlayerIndex != PC->GetLocalPlayer()->GetControllerId()) return;
			Component->ApplyViewVisibility(View);
			if (UMaterialInstanceDynamic* Material = Component->GetActiveMaterial())
				Material->OverrideBlendableSettings(View, 1.0f);
		}
	protected:
		virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
		{
			const UShooterOutlineComponent* Component = Owner.Get();
			const UWorld* World = Component ? Component->GetWorld() : nullptr;
			return World && !World->bIsTearingDown && !Component->IsBeingDestroyed()
				&& Context.GetWorld() == World && Component->GetActiveMaterial();
		}
	private:
		TWeakObjectPtr<UShooterOutlineComponent> Owner;
	};

	bool IsLiveMesh(const UMeshComponent* Mesh, const AActor* ViewActor)
	{
		if (!IsValid(Mesh) || Mesh->IsBeingDestroyed() || !Mesh->IsRegistered()
			|| !Mesh->IsVisible() || Mesh->bHiddenInGame || !Mesh->bRenderInMainPass) return false;
		const AActor* Owner = Mesh->GetOwner();
		if (!IsValid(Owner) || Owner->IsActorBeingDestroyed() || Owner->IsHidden()) return false;
		const bool bViewOwnsMesh = ViewActor && Owner->IsOwnedBy(ViewActor);
		if ((Mesh->bOnlyOwnerSee && !bViewOwnsMesh) || (Mesh->bOwnerNoSee && bViewOwnsMesh)) return false;
		if (const USkeletalMeshComponent* Skeletal = Cast<USkeletalMeshComponent>(Mesh))
			if (Skeletal->bHideSkin) return false;
		// Hidden collision roots do not hide their visible children. Visibility propagation is
		// applied to each child explicitly by the engine, so only inspect the mesh itself.
		return Cast<USkeletalMeshComponent>(Mesh) || Cast<UStaticMeshComponent>(Mesh);
	}
}

void UShooterOutlineComponent::ApplyViewVisibility(FSceneView& View) const
{
	const UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !World || World->bIsTearingDown) return;
	// Split-screen views have different revenge targets. Do not let another local
	// controller's depth proxies compete for the same pixel's stencil in this view.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* OtherPC = It->Get();
		const UShooterOutlineComponent* Other = IsValid(OtherPC) ? OtherPC->FindComponentByClass<UShooterOutlineComponent>() : nullptr;
		if (!IsValid(Other) || Other->IsBeingDestroyed() || Other == this) continue;
		for (const FShooterOutlineProxy& Proxy : Other->Proxies)
			if (IsValid(Proxy.Mesh)) View.HiddenPrimitives.Add(Proxy.Mesh->GetPrimitiveSceneId());
	}
	for (const FShooterOutlineProxy& Proxy : Proxies)
	{
		const UMeshComponent* Source = Proxy.Source.Get();
		if (!IsValid(Source) || !Source->IsRegistered() || !IsValid(Proxy.Mesh) || !Proxy.Mesh->IsRegistered()) continue;
		if (View.HiddenPrimitives.Contains(Source->GetPrimitiveSceneId()))
			View.HiddenPrimitives.Add(Proxy.Mesh->GetPrimitiveSceneId());
		if (View.ShowOnlyPrimitives.IsSet() && View.ShowOnlyPrimitives->Contains(Source->GetPrimitiveSceneId()))
			View.ShowOnlyPrimitives->Add(Proxy.Mesh->GetPrimitiveSceneId());
	}
}

UShooterOutlineComponent::UShooterOutlineComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	Settings = TSoftObjectPtr<UShooterOutlineSettings>(FSoftObjectPath(TEXT("/Game/QuakeLike_1_0/Data/Player/DA_ShooterOutline_Default.DA_ShooterOutline_Default")));
}

void UShooterOutlineComponent::BeginPlay()
{
	Super::BeginPlay();
	bEndingPlay = false;
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC) || PC->IsActorBeingDestroyed() || !PC->IsLocalController() || GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}
	ApplyOutlineSettings(Settings.LoadSynchronous());
	ViewExtension = FSceneViewExtensions::NewExtension<FShooterOutlineViewExtension>(this);
}

void UShooterOutlineComponent::ApplyOutlineSettings(UShooterOutlineSettings* NewSettings)
{
	if (bEndingPlay || IsBeingDestroyed()) return;
	ClearProxies();
	bPresentationActive = false;
	if (!IsValid(NewSettings))
	{
		Material = nullptr;
		ShooterClass = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("ShooterOutline: no settings asset assigned to %s."), *GetNameSafe(GetOwner()));
		return;
	}
	Settings = NewSettings;
	ShooterClass = NewSettings->ShooterClass.LoadSynchronous();
	RefreshInterval = ShooterDisplayValidation::FiniteClamp(NewSettings->RefreshInterval, 0.05f, MAX_flt, 0.2f);
	StencilValue = FMath::Clamp(NewSettings->StencilValue, 2, 255);
	RevengeStencilValue = FMath::Clamp(NewSettings->RevengeStencilValue, 2, 255);
	if (RevengeStencilValue == StencilValue) RevengeStencilValue = StencilValue == 255 ? 254 : StencilValue + 1;
	PrepareMaterial(NewSettings->Material);
	SetOutlineStyle(NewSettings->Style);
}

void UShooterOutlineComponent::PrepareMaterial(UMaterialInterface* SourceMaterial)
{
	if (bEndingPlay || IsBeingDestroyed()) return;
	if (!IsValid(SourceMaterial) || !IsValid(SourceMaterial->GetMaterial())
		|| SourceMaterial->GetMaterial()->MaterialDomain != MD_PostProcess)
	{
		Material = nullptr;
		return;
	}
#if WITH_EDITOR
	// Editor loads can leave post-process shaders in on-demand mode. Request the real
	// render permutations without changing the asset or blocking gameplay on compilation.
	if (SourceMaterial) SourceMaterial->CacheShaders(EMaterialShaderPrecompileMode::Background);
#endif
	Material = SourceMaterial ? UMaterialInstanceDynamic::Create(SourceMaterial, this) : nullptr;
}

bool UShooterOutlineComponent::IsMaterialReady() const
{
	if (!IsValid(Material) || !GetWorld() || GetWorld()->bIsTearingDown) return false;
	const FMaterialResource* Resource = Material->GetMaterialResource(GetWorld()->GetFeatureLevel());
	return Resource && Resource->IsGameThreadShaderMapComplete();
}

void UShooterOutlineComponent::SetOutlineStyle(const FShooterOutlineStyle& NewStyle)
{
	if (bEndingPlay || IsBeingDestroyed()) return;
	Style = NewStyle;
	Style.Thickness = ShooterDisplayValidation::FiniteClamp(Style.Thickness, 0.0f, 8.0f, 2.0f);
	Style.Opacity = ShooterDisplayValidation::FiniteClamp(Style.Opacity, 0.0f, 1.0f, 1.0f);
	Style.DepthTolerance = ShooterDisplayValidation::FiniteClamp(Style.DepthTolerance, 0.0f, 5.0f, 0.5f);
	Style.Color = ShooterDisplayValidation::FiniteColor(Style.Color, FLinearColor::White);
	Style.RevengeColor = ShooterDisplayValidation::FiniteColor(Style.RevengeColor, FLinearColor::Red);
	UpdateMaterial();
	RefreshElapsed = RefreshInterval;
	if (!Style.bEnabled || Style.Thickness <= 0.0f || Style.Opacity <= 0.0f || Style.Color.A <= 0.0f)
	{
		bPresentationActive = false;
		ClearProxies();
	}
}

void UShooterOutlineComponent::SetOutlineEnabled(bool bEnabled)
{
	FShooterOutlineStyle NewStyle = Style;
	NewStyle.bEnabled = bEnabled;
	SetOutlineStyle(NewStyle);
}

void UShooterOutlineComponent::ResetOutlineStyle()
{
	if (const UShooterOutlineSettings* Defaults = Settings.Get()) SetOutlineStyle(Defaults->Style);
}

void UShooterOutlineComponent::UpdateMaterial()
{
	if (!IsValid(Material) || bEndingPlay || IsBeingDestroyed()) return;
	Material->SetVectorParameterValue(TEXT("killerColor"), Style.Color);
	Material->SetVectorParameterValue(TEXT("revengeColor"), Style.RevengeColor);
	Material->SetScalarParameterValue(TEXT("OutlineThickness"), Style.Thickness);
	Material->SetScalarParameterValue(TEXT("OutlineOpacity"), Style.Opacity * FMath::Clamp(Style.Color.A, 0.0f, 1.0f));
	Material->SetScalarParameterValue(TEXT("DepthTolerance"), Style.DepthTolerance);
	Material->SetScalarParameterValue(TEXT("StencilValue"), StencilValue);
	Material->SetScalarParameterValue(TEXT("RevengeStencilValue"), RevengeStencilValue);
}

int32 UShooterOutlineComponent::GetShooterStencil(const APawn* Shooter) const
{
	const AActor* Owner = GetOwner();
	const UShooterRevengeComponent* Revenge = IsValid(Owner) ? Owner->FindComponentByClass<UShooterRevengeComponent>() : nullptr;
	return IsValid(Revenge) && Revenge->IsRevengeTarget(Shooter) ? RevengeStencilValue : StencilValue;
}

void UShooterOutlineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	const UWorld* World = GetWorld();
	const bool bCanPresent = !bEndingPlay && !IsBeingDestroyed() && World && !World->bIsTearingDown
		&& IsValid(PC) && !PC->IsActorBeingDestroyed() && PC->IsLocalController() && Style.bEnabled && IsValid(Material) && ShooterClass
		&& IsMaterialReady()
		&& Style.Thickness > 0.0f && Style.Opacity > 0.0f && Style.Color.A > 0.0f
		&& !Cast<ADeathCamActor>(PC->GetViewTarget());
	if (!bCanPresent)
	{
		if (bPresentationActive) ClearProxies();
		bPresentationActive = false;
		return;
	}
	RefreshElapsed += FMath::IsFinite(DeltaTime) ? FMath::Max(0.0f, DeltaTime) : 0.0f;
	if (!bPresentationActive || RefreshElapsed >= RefreshInterval)
	{
		bPresentationActive = true;
		RefreshElapsed = 0.0f;
		RefreshShooters();
	}
	UpdateProxies();
}

void UShooterOutlineComponent::RefreshShooters()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !World || World->bIsTearingDown || !ShooterClass) return;
	TMap<UMeshComponent*, APawn*> Wanted;
	auto CollectMeshes = [&Wanted, PC](AActor* Actor, APawn* Shooter)
	{
		TInlineComponentArray<UMeshComponent*> Meshes(Actor);
		for (UMeshComponent* Mesh : Meshes)
			if (IsLiveMesh(Mesh, PC->GetViewTarget())) Wanted.Add(Mesh, Shooter);
	};
	for (TActorIterator<APawn> It(World, ShooterClass); It; ++It)
	{
		APawn* Shooter = *It;
		if (!IsValid(Shooter) || Shooter->IsActorBeingDestroyed() || Shooter == PC->GetPawn()
			|| Shooter == PC->GetViewTarget() || Shooter->IsHidden() || !UShooterDisplayLibrary::IsShooterAlive(Shooter)) continue;
		CollectMeshes(Shooter, Shooter);
		TArray<AActor*> Attached;
		Shooter->GetAttachedActors(Attached, true, true);
		for (AActor* Actor : Attached)
			if (IsValid(Actor) && !Actor->IsHidden() && Actor->ActorHasTag(TEXT("DeathCamWeapon"))) CollectMeshes(Actor, Shooter);
	}
	for (int32 Index = Proxies.Num() - 1; Index >= 0; --Index)
	{
		FShooterOutlineProxy& Proxy = Proxies[Index];
		if (!IsValid(Proxy.Mesh) || Proxy.Mesh->IsBeingDestroyed() || !Proxy.Mesh->IsRegistered()
			|| !Wanted.Contains(Proxy.Source.Get()))
		{
			if (IsValid(Proxy.Mesh)) Proxy.Mesh->DestroyComponent();
			Proxies.RemoveAtSwap(Index);
		}
		else Wanted.Remove(Proxy.Source.Get());
	}
	for (const auto& Entry : Wanted)
	{
		UMeshComponent* Source = Entry.Key;
		if (!IsLiveMesh(Source, PC->GetViewTarget())) continue;
		UMeshComponent* ProxyMesh = nullptr;
		if (USkeletalMeshComponent* SkeletalSource = Cast<USkeletalMeshComponent>(Source))
		{
			if (!IsValid(SkeletalSource->GetSkeletalMeshAsset())) continue;
			// Controllers are hidden actors: their mesh components cannot render, even in CustomDepth.
			USkeletalMeshComponent* SkeletalProxy = NewObject<USkeletalMeshComponent>(Source->GetOwner(), NAME_None, RF_Transient);
			SkeletalProxy->SetSkeletalMeshAsset(SkeletalSource->GetSkeletalMeshAsset());
			SkeletalProxy->SetLeaderPoseComponent(SkeletalSource);
			ProxyMesh = SkeletalProxy;
		}
		else if (UStaticMeshComponent* StaticSource = Cast<UStaticMeshComponent>(Source))
		{
			if (!IsValid(StaticSource->GetStaticMesh())) continue;
			UStaticMeshComponent* StaticProxy = NewObject<UStaticMeshComponent>(Source->GetOwner(), NAME_None, RF_Transient);
			StaticProxy->SetStaticMesh(StaticSource->GetStaticMesh());
			ProxyMesh = StaticProxy;
		}
		if (!ProxyMesh) continue;
		// Stencil-only proxies never alter the source materials, collision, or DeathCam stencil.
		ProxyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProxyMesh->SetGenerateOverlapEvents(false);
		ProxyMesh->SetCastShadow(false);
		ProxyMesh->SetVisibleInRayTracing(false);
		ProxyMesh->SetRenderInMainPass(false);
		ProxyMesh->SetRenderInDepthPass(false);
		ProxyMesh->SetRenderCustomDepth(true);
		ProxyMesh->SetCustomDepthStencilValue(GetShooterStencil(Entry.Value));
		ProxyMesh->SetCanEverAffectNavigation(false);
		ProxyMesh->SetComponentTickEnabled(false);
		// FP meshes and FP weapons remain owner-only; TP meshes retain OwnerNoSee.
		ProxyMesh->SetOnlyOwnerSee(Source->bOnlyOwnerSee);
		ProxyMesh->SetOwnerNoSee(Source->bOwnerNoSee);
		ProxyMesh->SetupAttachment(Source);
		for (int32 Slot = 0; Slot < Source->GetNumMaterials(); ++Slot) ProxyMesh->SetMaterial(Slot, Source->GetMaterial(Slot));
		ProxyMesh->RegisterComponent();
		FShooterOutlineProxy& Proxy = Proxies.AddDefaulted_GetRef();
		Proxy.Source = Source;
		Proxy.Shooter = Entry.Value;
		Proxy.Mesh = ProxyMesh;
	}
}

void UShooterOutlineComponent::UpdateProxies()
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !GetWorld() || GetWorld()->bIsTearingDown) return;
	for (FShooterOutlineProxy& Proxy : Proxies)
	{
		UMeshComponent* Source = Proxy.Source.Get();
		APawn* Shooter = Proxy.Shooter.Get();
		const bool bVisible = Source && Shooter && !Shooter->IsHidden() && IsLiveMesh(Source, PC->GetViewTarget()) && UShooterDisplayLibrary::IsShooterAlive(Shooter);
		if (!IsValid(Proxy.Mesh) || Proxy.Mesh->IsBeingDestroyed()) continue;
		Proxy.Mesh->SetRenderCustomDepth(bVisible);
		Proxy.Mesh->SetCustomDepthStencilValue(GetShooterStencil(Shooter));
		if (!bVisible) continue;
		Proxy.Mesh->SetOnlyOwnerSee(Source->bOnlyOwnerSee);
		Proxy.Mesh->SetOwnerNoSee(Source->bOwnerNoSee);
		if (USkeletalMeshComponent* SkeletalProxy = Cast<USkeletalMeshComponent>(Proxy.Mesh))
		{
			USkeletalMeshComponent* SkeletalSource = Cast<USkeletalMeshComponent>(Source);
			if (!SkeletalSource) { Proxy.Mesh->SetRenderCustomDepth(false); continue; }
			if (SkeletalProxy->GetSkeletalMeshAsset() != SkeletalSource->GetSkeletalMeshAsset())
			{
				SkeletalProxy->SetSkeletalMeshAsset(SkeletalSource->GetSkeletalMeshAsset());
				SkeletalProxy->SetLeaderPoseComponent(SkeletalSource, true);
			}
			// Section visibility belongs to each component, rather than its shared pose.
			// Copy only hidden-material flags, never the LOD buffers or source materials.
			SkeletalSource->InitLODInfos();
			SkeletalProxy->InitLODInfos();
			bool bSectionsChanged = false;
			for (int32 LOD = 0; LOD < SkeletalSource->LODInfo.Num() && LOD < SkeletalProxy->LODInfo.Num(); ++LOD)
			{
				if (SkeletalProxy->LODInfo[LOD].HiddenMaterials != SkeletalSource->LODInfo[LOD].HiddenMaterials)
				{
					SkeletalProxy->LODInfo[LOD].HiddenMaterials = SkeletalSource->LODInfo[LOD].HiddenMaterials;
					bSectionsChanged = true;
				}
			}
			if (bSectionsChanged) SkeletalProxy->MarkRenderStateDirty();
			SkeletalProxy->SetForcedLOD(SkeletalSource->GetForcedLOD());
		}
		else if (UStaticMeshComponent* StaticProxy = Cast<UStaticMeshComponent>(Proxy.Mesh))
		{
			UStaticMeshComponent* StaticSource = Cast<UStaticMeshComponent>(Source);
			if (!StaticSource) { Proxy.Mesh->SetRenderCustomDepth(false); continue; }
			UStaticMesh* Mesh = StaticSource->GetStaticMesh();
			if (StaticProxy->GetStaticMesh() != Mesh) StaticProxy->SetStaticMesh(Mesh);
		}
		for (int32 Slot = 0; Slot < Source->GetNumMaterials(); ++Slot)
			if (Proxy.Mesh->GetMaterial(Slot) != Source->GetMaterial(Slot)) Proxy.Mesh->SetMaterial(Slot, Source->GetMaterial(Slot));
	}
}

void UShooterOutlineComponent::ClearProxies()
{
	for (FShooterOutlineProxy& Proxy : Proxies)
		if (IsValid(Proxy.Mesh)) Proxy.Mesh->DestroyComponent();
	Proxies.Reset();
}

void UShooterOutlineComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	bEndingPlay = true;
	bPresentationActive = false;
	ViewExtension.Reset();
	ClearProxies();
	Material = nullptr;
	Super::EndPlay(Reason);
}

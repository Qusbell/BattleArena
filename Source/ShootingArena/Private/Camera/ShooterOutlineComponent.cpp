#include "Camera/ShooterOutlineComponent.h"

#include "Camera/DeathCamActor.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
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
			if (!PC || !PC->GetLocalPlayer() || View.PlayerIndex != PC->GetLocalPlayer()->GetControllerId()) return;
			Component->ApplyViewVisibility(View);
			if (UMaterialInstanceDynamic* Material = Component->GetActiveMaterial())
				Material->OverrideBlendableSettings(View, 1.0f);
		}
	protected:
		virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
		{
			return Owner.IsValid() && Context.GetWorld() == Owner->GetWorld() && Owner->GetActiveMaterial();
		}
	private:
		TWeakObjectPtr<UShooterOutlineComponent> Owner;
	};

	bool IsLiveMesh(const UMeshComponent* Mesh, const AActor* ViewActor)
	{
		if (!IsValid(Mesh) || !Mesh->IsVisible() || Mesh->bHiddenInGame || !Mesh->bRenderInMainPass) return false;
		const AActor* Owner = Mesh->GetOwner();
		if (!IsValid(Owner) || Owner->IsHidden()) return false;
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
	for (const FShooterOutlineProxy& Proxy : Proxies)
	{
		const UMeshComponent* Source = Proxy.Source.Get();
		if (!Source || !IsValid(Proxy.Mesh)) continue;
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
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController() || GetNetMode() == NM_DedicatedServer)
	{
		SetComponentTickEnabled(false);
		return;
	}
	ApplyOutlineSettings(Settings.LoadSynchronous());
	ViewExtension = FSceneViewExtensions::NewExtension<FShooterOutlineViewExtension>(this);
}

void UShooterOutlineComponent::ApplyOutlineSettings(UShooterOutlineSettings* NewSettings)
{
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
	RefreshInterval = FMath::Max(0.05f, NewSettings->RefreshInterval);
	StencilValue = FMath::Clamp(NewSettings->StencilValue, 2, 255);
	PrepareMaterial(NewSettings->Material);
	SetOutlineStyle(NewSettings->Style);
}

void UShooterOutlineComponent::PrepareMaterial(UMaterialInterface* SourceMaterial)
{
#if WITH_EDITOR
	// Editor loads can leave post-process shaders in on-demand mode. Request the real
	// render permutations without changing the asset or blocking gameplay on compilation.
	if (SourceMaterial) SourceMaterial->CacheShaders(EMaterialShaderPrecompileMode::Background);
#endif
	Material = SourceMaterial ? UMaterialInstanceDynamic::Create(SourceMaterial, this) : nullptr;
}

bool UShooterOutlineComponent::IsMaterialReady() const
{
	if (!Material || !GetWorld()) return false;
	const FMaterialResource* Resource = Material->GetMaterialResource(GetWorld()->GetFeatureLevel());
	return Resource && Resource->IsGameThreadShaderMapComplete();
}

void UShooterOutlineComponent::SetOutlineStyle(const FShooterOutlineStyle& NewStyle)
{
	Style = NewStyle;
	Style.Thickness = FMath::Clamp(Style.Thickness, 0.0f, 8.0f);
	Style.Opacity = FMath::Clamp(Style.Opacity, 0.0f, 1.0f);
	Style.DepthTolerance = FMath::Clamp(Style.DepthTolerance, 0.0f, 5.0f);
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
	if (!Material) return;
	Material->SetVectorParameterValue(TEXT("killerColor"), Style.Color);
	Material->SetScalarParameterValue(TEXT("OutlineThickness"), Style.Thickness);
	Material->SetScalarParameterValue(TEXT("OutlineOpacity"), Style.Opacity * FMath::Clamp(Style.Color.A, 0.0f, 1.0f));
	Material->SetScalarParameterValue(TEXT("DepthTolerance"), Style.DepthTolerance);
	Material->SetScalarParameterValue(TEXT("StencilValue"), StencilValue);
}

void UShooterOutlineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	const bool bCanPresent = PC && PC->IsLocalController() && Style.bEnabled && Material && ShooterClass
		&& IsMaterialReady()
		&& Style.Thickness > 0.0f && Style.Opacity > 0.0f && Style.Color.A > 0.0f
		&& !Cast<ADeathCamActor>(PC->GetViewTarget());
	if (!bCanPresent)
	{
		if (bPresentationActive) ClearProxies();
		bPresentationActive = false;
		return;
	}
	RefreshElapsed += DeltaTime;
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
	APlayerController* PC = CastChecked<APlayerController>(GetOwner());
	TMap<UMeshComponent*, APawn*> Wanted;
	auto CollectMeshes = [&Wanted, PC](AActor* Actor, APawn* Shooter)
	{
		TInlineComponentArray<UMeshComponent*> Meshes(Actor);
		for (UMeshComponent* Mesh : Meshes)
			if (IsLiveMesh(Mesh, PC->GetViewTarget())) Wanted.Add(Mesh, Shooter);
	};
	for (TActorIterator<APawn> It(GetWorld(), ShooterClass); It; ++It)
	{
		APawn* Shooter = *It;
		if (Shooter == PC->GetPawn() || Shooter == PC->GetViewTarget() || Shooter->IsHidden() || !UShooterDisplayLibrary::IsShooterAlive(Shooter)) continue;
		CollectMeshes(Shooter, Shooter);
		TArray<AActor*> Attached;
		Shooter->GetAttachedActors(Attached, true, true);
		for (AActor* Actor : Attached)
			if (IsValid(Actor) && !Actor->IsHidden() && Actor->ActorHasTag(TEXT("DeathCamWeapon"))) CollectMeshes(Actor, Shooter);
	}
	for (int32 Index = Proxies.Num() - 1; Index >= 0; --Index)
	{
		FShooterOutlineProxy& Proxy = Proxies[Index];
		if (!Wanted.Contains(Proxy.Source.Get()))
		{
			if (IsValid(Proxy.Mesh)) Proxy.Mesh->DestroyComponent();
			Proxies.RemoveAtSwap(Index);
		}
		else Wanted.Remove(Proxy.Source.Get());
	}
	for (const auto& Entry : Wanted)
	{
		UMeshComponent* Source = Entry.Key;
		UMeshComponent* ProxyMesh = nullptr;
		if (USkeletalMeshComponent* SkeletalSource = Cast<USkeletalMeshComponent>(Source))
		{
			// Controllers are hidden actors: their mesh components cannot render, even in CustomDepth.
			USkeletalMeshComponent* SkeletalProxy = NewObject<USkeletalMeshComponent>(Source->GetOwner(), NAME_None, RF_Transient);
			SkeletalProxy->SetSkeletalMeshAsset(SkeletalSource->GetSkeletalMeshAsset());
			SkeletalProxy->SetLeaderPoseComponent(SkeletalSource);
			ProxyMesh = SkeletalProxy;
		}
		else if (UStaticMeshComponent* StaticSource = Cast<UStaticMeshComponent>(Source))
		{
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
		ProxyMesh->SetCustomDepthStencilValue(StencilValue);
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
	const APlayerController* PC = CastChecked<APlayerController>(GetOwner());
	for (FShooterOutlineProxy& Proxy : Proxies)
	{
		UMeshComponent* Source = Proxy.Source.Get();
		APawn* Shooter = Proxy.Shooter.Get();
		const bool bVisible = Source && Shooter && !Shooter->IsHidden() && IsLiveMesh(Source, PC->GetViewTarget()) && UShooterDisplayLibrary::IsShooterAlive(Shooter);
		if (!IsValid(Proxy.Mesh)) continue;
		Proxy.Mesh->SetRenderCustomDepth(bVisible);
		if (!bVisible) continue;
		Proxy.Mesh->SetOnlyOwnerSee(Source->bOnlyOwnerSee);
		Proxy.Mesh->SetOwnerNoSee(Source->bOwnerNoSee);
		if (USkeletalMeshComponent* SkeletalProxy = Cast<USkeletalMeshComponent>(Proxy.Mesh))
		{
			USkeletalMeshComponent* SkeletalSource = CastChecked<USkeletalMeshComponent>(Source);
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
			UStaticMesh* Mesh = CastChecked<UStaticMeshComponent>(Source)->GetStaticMesh();
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
	bPresentationActive = false;
	ViewExtension.Reset();
	ClearProxies();
	Super::EndPlay(Reason);
}

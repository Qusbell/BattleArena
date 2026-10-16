#include "Weapon/ProjectileFirstPersonRenderingComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "SceneViewExtension.h"
#include "UObject/UnrealType.h"
#include "Weapon/FirstPersonRenderingMath.h"

namespace
{
bool LiveComponent(const UActorComponent* Component)
{
	return IsValid(Component) && !Component->IsBeingDestroyed();
}

UObject* Field(const UObject* Object, FName Name)
{
	const FObjectPropertyBase* P = IsValid(Object) ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
	UObject* Value = P ? P->GetObjectPropertyValue_InContainer(Object) : nullptr;
	return IsValid(Value) ? Value : nullptr;
}
bool Eligible(AActor* Weapon)
{
	return IsValid(Weapon) && !Weapon->IsActorBeingDestroyed() && !Weapon->IsHidden()
		&& LiveComponent(Cast<UPrimitiveComponent>(Field(Weapon, TEXT("FP_Mesh"))));
}

class FFirstPersonNearClipViewExtension final : public FSceneViewExtensionBase
{
public:
	FFirstPersonNearClipViewExtension(const FAutoRegister& Register, UProjectileFirstPersonRenderingComponent* Component)
		: FSceneViewExtensionBase(Register), Owner(Component) {}
	virtual void SetupViewPoint(APlayerController* Player, FMinimalViewInfo& View) override
	{
		const auto* Component = Owner.Get();
		if (Component) Component->ApplyFirstPersonView(Player, View);
	}
protected:
	virtual bool IsActiveThisFrame_Internal(const FSceneViewExtensionContext& Context) const override
	{
		const auto* Component = Owner.Get();
		return IsValid(Component) && !Component->IsBeingDestroyed() && Component->GetWorld()
			&& Component->IsRegistered() && Component->IsComponentTickEnabled() && Component->bEnabled
			&& !Component->GetWorld()->bIsTearingDown && Context.GetWorld() == Component->GetWorld();
	}
private:
	TWeakObjectPtr<UProjectileFirstPersonRenderingComponent> Owner;
};
}

UProjectileFirstPersonRenderingComponent::UProjectileFirstPersonRenderingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

bool UProjectileFirstPersonRenderingComponent::CanUpdateRendering() const
{
	const UWorld* World = GetWorld();
	return !IsBeingDestroyed() && IsRegistered() && HasBegunPlay() && IsComponentTickEnabled()
		&& IsValid(World) && !World->bIsTearingDown && World->GetNetMode() != NM_DedicatedServer;
}

void UProjectileFirstPersonRenderingComponent::Track(UPrimitiveComponent* Primitive)
{
	if (!LiveComponent(Primitive)) return;
	if (!PreviousTypes.Contains(Primitive)) PreviousTypes.Add(Primitive, Primitive->FirstPersonPrimitiveType);
	if (Primitive->FirstPersonPrimitiveType != EFirstPersonPrimitiveType::FirstPerson)
		Primitive->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
}

void UProjectileFirstPersonRenderingComponent::RestoreView()
{
	// Reject late spawn callbacks before releasing or restoring any render state.
	bAcceptMuzzleEffects = false;
	for (const auto& Pair : PreviousTypes)
		if (UPrimitiveComponent* Primitive = Pair.Key.Get(); LiveComponent(Primitive)) Primitive->SetFirstPersonPrimitiveType(Pair.Value);
	PreviousTypes.Reset();
	if (UCameraComponent* SavedCamera = Camera.Get(); LiveComponent(SavedCamera))
	{
		SavedCamera->SetFirstPersonScale(FirstPersonRendering::SafeScale(PreviousScale, 1.f));
		SavedCamera->SetEnableFirstPersonScale(bPreviousEnableScale);
	}
	Camera.Reset(); ActiveWeapon.Reset(); ActiveRenderScale = 1.f;
}

void UProjectileFirstPersonRenderingComponent::Restore()
{
	RestoreView();
	for (const auto& Pair : PreparedGunTypes)
		if (UPrimitiveComponent* Gun = Pair.Key.Get(); LiveComponent(Gun)) Gun->SetFirstPersonPrimitiveType(Pair.Value);
	PreparedGunTypes.Reset(); PreparedMeshes.Reset(); NextTextureRefreshTime = 0.f;
}

void UProjectileFirstPersonRenderingComponent::PrepareWeapons(USkeletalMeshComponent* Arms)
{
	UWorld* World = GetWorld();
	if (!CanUpdateRendering() || !LiveComponent(Arms) || Arms->GetWorld() != World) return;
	TArray<USceneComponent*> Children; Arms->GetChildrenComponents(false, Children);
	TSet<UPrimitiveComponent*> RetainedGuns;
	TSet<USkeletalMeshComponent*> RetainedMeshes;
	const float Now = World->GetTimeSeconds();
	const bool bRefreshTextures = Now >= NextTextureRefreshTime;
	for (USceneComponent* Child : Children)
	{
		AActor* Weapon = IsValid(Child) ? Child->GetOwner() : nullptr;
		USkeletalMeshComponent* Gun = Cast<USkeletalMeshComponent>(Child);
		if (!IsValid(Weapon) || Weapon->IsActorBeingDestroyed() || !LiveComponent(Gun)
			|| Gun->GetWorld() != World || Gun != Field(Weapon, TEXT("FP_Mesh"))) continue;
		RetainedGuns.Add(Gun);
		if (!PreparedGunTypes.Contains(Gun)) PreparedGunTypes.Add(Gun, Gun->FirstPersonPrimitiveType);
		Gun->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		RetainedMeshes.Add(Gun);
		USkeletalMesh* Mesh = Gun->GetSkeletalMeshAsset();
		const bool bMeshChanged = !PreparedMeshes.Contains(Gun) || PreparedMeshes[Gun].Get() != Mesh;
		if (bMeshChanged)
		{
			PreparedMeshes.Add(Gun, Mesh);
			if (IsValid(Mesh)) Gun->PrecachePSOs();
		}
		// Common preparation for every owned FP gun, including hitscan weapons.
		// Rendering and prewarming both cover hitscan and projectile guns.
		// Skeletal mesh LOD buffers also stream; warming only textures leaves geometry cold.
		if (IsValid(Mesh) && (bMeshChanged || bRefreshTextures)) Mesh->SetForceMipLevelsToBeResident(5.f);
		// Hidden inventory weapons are prepared without displaying or equipping them.
		// Do not prioritize character textures: that would pause world streaming for 30 frames.
		if (IsValid(Mesh) && bPrestreamOwnedWeaponTextures && (bMeshChanged || bRefreshTextures)) Gun->PrestreamTextures(5.f, false);
	}
	if (bRefreshTextures) NextTextureRefreshTime = Now + 3.f;
	for (auto It = PreparedGunTypes.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Gun = It.Key().Get();
		if (!Gun || !RetainedGuns.Contains(Gun))
		{
			if (LiveComponent(Gun)) Gun->SetFirstPersonPrimitiveType(It.Value());
			It.RemoveCurrent();
		}
	}
	for (auto It = PreparedMeshes.CreateIterator(); It; ++It)
		if (!It.Key().IsValid() || !RetainedMeshes.Contains(It.Key().Get())) It.RemoveCurrent();
}

void UProjectileFirstPersonRenderingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bEnabled || !CanUpdateRendering())
	{
		Restore();
		return;
	}
	APawn* Pawn = Cast<APawn>(GetOwner());
	USkeletalMeshComponent* Arms = Cast<USkeletalMeshComponent>(Field(Pawn, TEXT("FP_Mesh")));
	UCameraComponent* View = Cast<UCameraComponent>(Field(Pawn, TEXT("FP_Camera")));
	APlayerController* Controller = IsValid(Pawn) && !Pawn->IsActorBeingDestroyed() ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	AActor* Weapon = nullptr;
	UPrimitiveComponent* Gun = nullptr;
	if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || !Pawn->IsLocallyControlled() || Pawn->IsHidden()
		|| !IsValid(Controller) || Controller->IsActorBeingDestroyed() || !Controller->IsLocalController() || Controller->GetViewTarget() != Pawn
		|| !LiveComponent(Arms) || !LiveComponent(View) || !View->IsActive()
		|| Arms->GetWorld() != GetWorld() || View->GetWorld() != GetWorld())
	{
		Restore();
		return;
	}
	PrepareWeapons(Arms);
	{
		TArray<USceneComponent*> Children; Arms->GetChildrenComponents(false, Children);
		for (USceneComponent* Child : Children)
		{
			AActor* Candidate = IsValid(Child) ? Child->GetOwner() : nullptr;
			if (Eligible(Candidate) && Child->IsVisible() && !Child->bHiddenInGame && Child == Field(Candidate, TEXT("FP_Mesh")))
			{ Weapon = Candidate; Gun = Cast<UPrimitiveComponent>(Child); break; }
		}
	}
	// Keep the arm/camera state across weapon switches. Recreating the
	// same render state at every swap discards renderer history unnecessarily.
	if (!Weapon || View != Camera.Get()) RestoreView();
	if (!Weapon || !Gun) return;
	if (!Camera.IsValid())
	{
		Camera = View; ActiveWeapon = Weapon;
		PreviousScale = View->FirstPersonScale; bPreviousEnableScale = View->bEnableFirstPersonScale;
	}
	ActiveWeapon = Weapon;
	if (!ViewExtension) ViewExtension = FSceneViewExtensions::NewExtension<FFirstPersonNearClipViewExtension>(this);
	float Scale = FirstPersonRendering::SafeScale(FirstPersonScale);
	if (bFitInsideCollisionCapsule)
	{
		if (const auto* Capsule = Pawn->FindComponentByClass<UCapsuleComponent>(); LiveComponent(Capsule)
			&& !Gun->Bounds.ContainsNaN() && !Arms->Bounds.ContainsNaN()
			&& Gun->Bounds.SphereRadius >= 0.f && Arms->Bounds.SphereRadius >= 0.f)
		{
			FVector Eye; FRotator EyeRotation;
			Controller->GetPlayerViewPoint(Eye, EyeRotation);
			const double BoundDistance = Eye.ContainsNaN() ? 0.0 : FMath::Max(
				FVector::Distance(Eye, Gun->Bounds.Origin) + Gun->Bounds.SphereRadius,
				FVector::Distance(Eye, Arms->Bounds.Origin) + Arms->Bounds.SphereRadius);
			double FOVExpansion = 1.0;
			if (IsValid(Controller->PlayerCameraManager))
			{
				const FMinimalViewInfo& CachedView = Controller->PlayerCameraManager->GetCameraCacheView();
				if (FirstPersonRendering::IsValidFOV(CachedView.FOV) && FirstPersonRendering::IsValidFOV(CachedView.FirstPersonFOV))
					FOVExpansion = CachedView.CalculateFirstPersonFOVCorrectionFactor();
			}
			Scale = FirstPersonRendering::FitScaleToCapsule(Scale, Eye, BoundDistance,
				Capsule->GetComponentLocation(), Capsule->GetUpVector(), Capsule->GetScaledCapsuleRadius(),
				Capsule->GetScaledCapsuleHalfHeight(), CapsuleSafetyMargin, FOVExpansion);
		}
	}
	View->SetEnableFirstPersonScale(true);
	ActiveRenderScale = Scale;
	View->SetFirstPersonScale(Scale);
	bAcceptMuzzleEffects = true;
	Track(Arms);
	TArray<USceneComponent*> Effects; Gun->GetChildrenComponents(true, Effects);
	for (USceneComponent* Effect : Effects) Track(Cast<UPrimitiveComponent>(Effect));
	// Detached/pooled effects and replaced arms must not retain the local FP mode.
	for (auto It = PreviousTypes.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Primitive = It.Key().Get();
		if (!LiveComponent(Primitive) || (Primitive != Arms && !Primitive->IsAttachedTo(Gun)))
		{
			if (LiveComponent(Primitive)) Primitive->SetFirstPersonPrimitiveType(It.Value());
			It.RemoveCurrent();
		}
	}
}

void UProjectileFirstPersonRenderingComponent::ApplyFirstPersonView(const APlayerController* Controller, FMinimalViewInfo& InView) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const UCameraComponent* View = Camera.Get();
	if (!bEnabled || !bAcceptMuzzleEffects || !CanUpdateRendering()
		|| !IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || Pawn->IsHidden() || !Pawn->IsLocallyControlled()
		|| !IsValid(Controller) || Controller->IsActorBeingDestroyed() || !Controller->IsLocalController()
		|| Controller != Pawn->GetController() || Controller->GetViewTarget() != Pawn
		|| !LiveComponent(View) || !View->IsActive() || View != Field(Pawn, TEXT("FP_Camera"))
		|| !Eligible(ActiveWeapon.Get()) || !FirstPersonRendering::IsValidFOV(InView.FOV)) return;
	const auto* Gun = Cast<UPrimitiveComponent>(Field(ActiveWeapon.Get(), TEXT("FP_Mesh")));
	if (!LiveComponent(Gun) || Gun->GetAttachParent() != Field(Pawn, TEXT("FP_Mesh"))
		|| !Gun->IsVisible() || Gun->bHiddenInGame) return;
	InView.PerspectiveNearClipPlane = FirstPersonRendering::SafeNearClip(FirstPersonNearClipPlane,
		InView.PerspectiveNearClipPlane, GNearClippingPlane);
	// CameraManager cached its POV before the late component tick. Update the
	// view directly so a long-gun swap cannot render one frame using the old scale.
	InView.FirstPersonScale = FirstPersonRendering::SafeScale(ActiveRenderScale);
	InView.FirstPersonFOV = FirstPersonRendering::SafeFirstPersonFOV(
		View->bEnableFirstPersonFieldOfView ? View->FirstPersonFieldOfView : InView.FOV, InView.FOV);
	InView.bUseFirstPersonParameters = true;
}

void UProjectileFirstPersonRenderingComponent::ApplyFirstPersonMuzzleEffect(AActor* Weapon, UPrimitiveComponent* Effect)
{
	if (!Eligible(Weapon) || !LiveComponent(Effect)) return;
	UPrimitiveComponent* Gun = Cast<UPrimitiveComponent>(Field(Weapon, TEXT("FP_Mesh")));
	if (!LiveComponent(Gun) || Effect->GetAttachParent() != Gun) return;
	const USceneComponent* Parent = Gun->GetAttachParent();
	APawn* Pawn = Cast<APawn>(LiveComponent(Parent) ? Parent->GetOwner() : nullptr);
	if (IsValid(Pawn) && !Pawn->IsActorBeingDestroyed() && !Pawn->IsHidden() && Pawn->IsLocallyControlled())
	{
		const auto* Controller = Cast<APlayerController>(Pawn->GetController());
		const auto* View = Cast<UCameraComponent>(Field(Pawn, TEXT("FP_Camera")));
		if (!IsValid(Controller) || Controller->IsActorBeingDestroyed() || Controller->GetViewTarget() != Pawn
			|| !LiveComponent(View) || !View->IsActive()) return;
		if (auto* Rendering = Pawn->FindComponentByClass<UProjectileFirstPersonRenderingComponent>())
			if (Rendering->bEnabled && Rendering->bAcceptMuzzleEffects && Rendering->CanUpdateRendering()
				&& Parent == Field(Pawn, TEXT("FP_Mesh")) && Effect->GetWorld() == Rendering->GetWorld()) Rendering->Track(Effect);
	}
}

void UProjectileFirstPersonRenderingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Restore();
	ViewExtension.Reset();
	Super::EndPlay(EndPlayReason);
}

void UProjectileFirstPersonRenderingComponent::OnUnregister()
{
	Restore();
	ViewExtension.Reset();
	Super::OnUnregister();
}

void UProjectileFirstPersonRenderingComponent::Deactivate()
{
	Restore();
	ViewExtension.Reset();
	Super::Deactivate();
	// This component can tick without auto-activation; base Deactivate alone may
	// leave that tick enabled when the component was already marked inactive.
	SetComponentTickEnabled(false);
}

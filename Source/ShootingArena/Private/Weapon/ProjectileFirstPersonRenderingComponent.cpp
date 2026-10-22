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

namespace FirstPersonEffectHelpers
{
bool ReadView(const APlayerController* Controller, const UCameraComponent* Camera, FMinimalViewInfo& View)
{
	if (!IsValid(Controller) || !LiveComponent(Camera)) return false;
	if (IsValid(Controller->PlayerCameraManager)) View = Controller->PlayerCameraManager->GetCameraCacheView();
	else
	{
		View.Location = Camera->GetComponentLocation();
		View.Rotation = Camera->GetComponentRotation();
		View.FOV = Camera->FieldOfView;
		View.ProjectionMode = Camera->ProjectionMode;
	}
	View.FirstPersonFOV = FirstPersonRendering::SafeFirstPersonFOV(
		Camera->bEnableFirstPersonFieldOfView ? Camera->FirstPersonFieldOfView : View.FOV, View.FOV);
	return View.ProjectionMode == ECameraProjectionMode::Perspective && !View.Location.ContainsNaN()
		&& !View.Rotation.ContainsNaN() && FirstPersonRendering::IsValidFOV(View.FOV);
}
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

float UProjectileFirstPersonRenderingComponent::CalculateRenderScale(const APawn* Pawn,
	const USkeletalMeshComponent* Arms, const UPrimitiveComponent* Gun, const FMinimalViewInfo& View) const
{
	const float Scale = FirstPersonRendering::SafeScale(FirstPersonScale);
	if (!bFitInsideCollisionCapsule || !IsValid(Pawn) || !LiveComponent(Arms) || !LiveComponent(Gun)) return Scale;
	const auto* Capsule = Pawn->FindComponentByClass<UCapsuleComponent>();
	if (!LiveComponent(Capsule) || Gun->Bounds.ContainsNaN() || Arms->Bounds.ContainsNaN()
		|| Gun->Bounds.SphereRadius < 0.f || Arms->Bounds.SphereRadius < 0.f) return Scale;
	const double BoundDistance = View.Location.ContainsNaN() ? 0.0 : FMath::Max(
		FVector::Distance(View.Location, Gun->Bounds.Origin) + Gun->Bounds.SphereRadius,
		FVector::Distance(View.Location, Arms->Bounds.Origin) + Arms->Bounds.SphereRadius);
	return FirstPersonRendering::FitScaleToCapsule(Scale, View.Location, BoundDistance,
		Capsule->GetComponentLocation(), Capsule->GetUpVector(), Capsule->GetScaledCapsuleRadius(),
		Capsule->GetScaledCapsuleHalfHeight(), CapsuleSafetyMargin, View.CalculateFirstPersonFOVCorrectionFactor());
}

bool UProjectileFirstPersonRenderingComponent::IsEffectMuzzleOccluded(UWorld* World,
	FVector Eye, FVector Muzzle, const AActor* Weapon, const AActor* Shooter)
{
	if (!IsValid(World) || World->bIsTearingDown || Eye.ContainsNaN() || Muzzle.ContainsNaN()
		|| Eye.Equals(Muzzle, UE_SMALL_NUMBER)) return false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(FirstPersonBeamMuzzleOcclusion), false);
	Query.AddIgnoredActor(Weapon);
	Query.AddIgnoredActor(Shooter);
	if (IsValid(Weapon)) Query.AddIgnoredActor(Weapon->GetOwner());
	FCollisionResponseParams Responses(ECR_Ignore);
	Responses.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_WorldDynamic, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_PhysicsBody, ECR_Block);
	Responses.CollisionResponse.SetResponse(ECC_Destructible, ECR_Block);
	return World->LineTraceTestByChannel(Eye, Muzzle, ECC_Visibility, Query, Responses);
}

bool UProjectileFirstPersonRenderingComponent::TryTransformEffectPosition(const AActor* Weapon,
	FVector Position, FVector& OutPosition)
{
	OutPosition = Position;
	if (Position.ContainsNaN()) return false;
	FMinimalViewInfo View;
	FVector Muzzle;
	return TryGetOccludedEffectView(Weapon, View, Muzzle)
		&& FirstPersonRendering::TryTransformEffectPosition(View, Muzzle, OutPosition);
}

bool UProjectileFirstPersonRenderingComponent::TryGetOccludedEffectView(const AActor* Weapon,
	FMinimalViewInfo& OutView, FVector& OutMuzzle)
{
	if (!IsValid(Weapon) || Weapon->IsActorBeingDestroyed() || Weapon->IsHidden()) return false;
	const auto* Gun = Cast<USkeletalMeshComponent>(Field(Weapon, TEXT("FP_Mesh")));
	if (!LiveComponent(Gun) || !Gun->IsVisible() || Gun->bHiddenInGame) return false;
	const FName MuzzleSocket(TEXT("Muzzle"));
	if (!Gun->DoesSocketExist(MuzzleSocket)) return false;
	const FVector PhysicalMuzzle = Gun->GetSocketLocation(MuzzleSocket);
	if (PhysicalMuzzle.ContainsNaN()) return false;
	const USceneComponent* Parent = Gun->GetAttachParent();
	const auto* Pawn = Cast<APawn>(LiveComponent(Parent) ? Parent->GetOwner() : nullptr);
	if (!IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || Pawn->IsHidden() || !Pawn->IsLocallyControlled()) return false;
	const auto* Arms = Cast<USkeletalMeshComponent>(Field(Pawn, TEXT("FP_Mesh")));
	const auto* Camera = Cast<UCameraComponent>(Field(Pawn, TEXT("FP_Camera")));
	const auto* Controller = Cast<APlayerController>(Pawn->GetController());
	const auto* Rendering = Pawn->FindComponentByClass<UProjectileFirstPersonRenderingComponent>();
	if (!LiveComponent(Rendering) || !Rendering->bEnabled || !Rendering->CanUpdateRendering()
		|| !LiveComponent(Arms) || Parent != Arms || !LiveComponent(Camera) || !Camera->IsActive()
		|| !IsValid(Controller) || Controller->IsActorBeingDestroyed() || !Controller->IsLocalController()
		|| Controller->GetViewTarget() != Pawn || Gun->GetWorld() != Rendering->GetWorld()
		|| Arms->GetWorld() != Rendering->GetWorld() || Camera->GetWorld() != Rendering->GetWorld()) return false;
	FMinimalViewInfo View;
	if (!FirstPersonEffectHelpers::ReadView(Controller, Camera, View)) return false;
	// Rendering can use FP scale continuously, but beam source correction is
	// needed only when geometry hides the physical muzzle from this camera.
	if (!IsEffectMuzzleOccluded(Rendering->GetWorld(), View.Location, PhysicalMuzzle, Weapon, Pawn)) return false;
	// Use the same committed scale/FOV as the weapon renderer. Re-fitting the
	// bounds here can produce a stronger correction than the visible gun uses.
	// A newly swapped weapon without valid FP render state must not borrow the
	// previous weapon's transform or invent a predicted one.
	if (Rendering->ActiveWeapon.Get() != Weapon || Rendering->Camera.Get() != Camera
		|| !Rendering->ApplyFirstPersonView(Controller, View)) return false;
	OutView = View;
	OutMuzzle = PhysicalMuzzle;
	return true;
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
	FMinimalViewInfo RenderView;
	if (!FirstPersonEffectHelpers::ReadView(Controller, View, RenderView)) { RestoreView(); return; }
	const float Scale = CalculateRenderScale(Pawn, Arms, Gun, RenderView);
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

bool UProjectileFirstPersonRenderingComponent::ApplyFirstPersonView(const APlayerController* Controller, FMinimalViewInfo& InView) const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const UCameraComponent* View = Camera.Get();
	if (!bEnabled || !bAcceptMuzzleEffects || !CanUpdateRendering()
		|| !IsValid(Pawn) || Pawn->IsActorBeingDestroyed() || Pawn->IsHidden() || !Pawn->IsLocallyControlled()
		|| !IsValid(Controller) || Controller->IsActorBeingDestroyed() || !Controller->IsLocalController()
		|| Controller != Pawn->GetController() || Controller->GetViewTarget() != Pawn
		|| !LiveComponent(View) || !View->IsActive() || View != Field(Pawn, TEXT("FP_Camera"))
		|| !Eligible(ActiveWeapon.Get()) || !FirstPersonRendering::IsValidFOV(InView.FOV)) return false;
	const auto* Gun = Cast<UPrimitiveComponent>(Field(ActiveWeapon.Get(), TEXT("FP_Mesh")));
	if (!LiveComponent(Gun) || Gun->GetAttachParent() != Field(Pawn, TEXT("FP_Mesh"))
		|| !Gun->IsVisible() || Gun->bHiddenInGame) return false;
	InView.PerspectiveNearClipPlane = FirstPersonRendering::SafeNearClip(FirstPersonNearClipPlane,
		InView.PerspectiveNearClipPlane, GNearClippingPlane);
	// CameraManager cached its POV before the late component tick. Update the
	// view directly so a long-gun swap cannot render one frame using the old scale.
	InView.FirstPersonScale = FirstPersonRendering::SafeScale(ActiveRenderScale);
	InView.FirstPersonFOV = FirstPersonRendering::SafeFirstPersonFOV(
		View->bEnableFirstPersonFieldOfView ? View->FirstPersonFieldOfView : InView.FOV, InView.FOV);
	InView.bUseFirstPersonParameters = true;
	return true;
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

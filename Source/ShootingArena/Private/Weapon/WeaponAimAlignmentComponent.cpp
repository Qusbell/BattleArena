#include "Weapon/WeaponAimAlignmentComponent.h"
#include "Weapon/WeaponAimAlignmentMath.h"

#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

UWeaponAimAlignmentComponent::UWeaponAimAlignmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetIsReplicatedByDefault(true);
}

APawn* UWeaponAimAlignmentComponent::GetShooter() const
{
	AActor* Actor = GetOwner();
	for (int32 Depth = 0; IsValid(Actor) && Depth < 4; ++Depth)
	{
		if (APawn* Pawn = Cast<APawn>(Actor)) return Pawn;
		AActor* Next = Actor->GetOwner();
		Actor = Next ? Next : Actor->GetAttachParentActor();
	}
	return nullptr;
}

USkeletalMeshComponent* UWeaponAimAlignmentComponent::GetThirdPersonWeaponMesh() const
{
	AActor* Weapon = GetOwner();
	if (!IsValid(Weapon)) return nullptr;
	const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("TP_Mesh"));
	return Property ? Cast<USkeletalMeshComponent>(Property->GetObjectPropertyValue_InContainer(Weapon)) : nullptr;
}

bool UWeaponAimAlignmentComponent::RefreshLocalTarget()
{
	bLocalTargetValid = false;
	AActor* Weapon = GetOwner();
	APawn* Shooter = GetShooter();
	if (!bEnabled || !IsValid(Settings) || !Settings->bEnabled || !IsValid(Weapon)
		|| Weapon->IsHidden() || !IsValid(Shooter) || (!Shooter->HasAuthority() && !Shooter->IsLocallyControlled())) return false;
	USkeletalMeshComponent* Mesh = GetThirdPersonWeaponMesh();
	if (!IsValid(Mesh) || !Mesh->GetAttachParent() || Mesh->GetAttachParent()->GetOwner() != Shooter
		|| Mesh->bHiddenInGame || !Mesh->DoesSocketExist(Settings->MuzzleSocket)) return false;
	// Initialization/holstering may precede itemInfo and weaponData replication.
	const FObjectPropertyBase* DataProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData"));
	if (!DataProperty || !IsValid(DataProperty->GetObjectPropertyValue_InContainer(Weapon))) return false;
	const FBoolProperty* MuzzleFlag = FindFProperty<FBoolProperty>(Weapon->GetClass(), TEXT("bUseMuzzleProjectileAim"));
	if (MuzzleFlag && !MuzzleFlag->GetPropertyValue_InContainer(Weapon)) return false;
	UFunction* Function = Weapon->FindFunction(TEXT("GetProjectileAim"));
	const FStructProperty* TargetProperty = Function ? FindFProperty<FStructProperty>(Function, TEXT("aimEnd")) : nullptr;
	if (!TargetProperty || TargetProperty->Struct != TBaseStructure<FVector>::Get()) return false;
	FStructOnScope Parameters(Function);
	// Existing GetProjectileAim only resolves coordinates and traces; it does not spawn, consume ammo or increment shot IDs.
	Weapon->ProcessEvent(Function, Parameters.GetStructMemory());
	LocalTarget = *TargetProperty->ContainerPtrToValuePtr<FVector>(Parameters.GetStructMemory());
	const FStructProperty* ForwardProperty = FindFProperty<FStructProperty>(Function, TEXT("FlightForward"));
	LocalViewForward = ForwardProperty && ForwardProperty->Struct == TBaseStructure<FVector>::Get()
		? ForwardProperty->ContainerPtrToValuePtr<FVector>(Parameters.GetStructMemory())->GetSafeNormal()
		: Shooter->GetBaseAimRotation().Vector().GetSafeNormal();
	bLocalTargetValid = !LocalTarget.ContainsNaN() && !LocalViewForward.ContainsNaN() && !LocalViewForward.IsNearlyZero();
	return bLocalTargetValid;
}

bool UWeaponAimAlignmentComponent::GetAlignmentTarget(FVector& OutTarget) const
{
	FVector ViewForward;
	return GetAlignmentAim(OutTarget, ViewForward);
}

bool UWeaponAimAlignmentComponent::GetAlignmentAim(FVector& OutTarget, FVector& OutViewForward) const
{
	const APawn* Shooter = GetShooter();
	const bool bUseLocal = IsValid(Shooter) && (Shooter->HasAuthority() || Shooter->IsLocallyControlled());
	OutTarget = bUseLocal ? LocalTarget : FVector(ReplicatedTarget.Location);
	OutViewForward = (bUseLocal ? LocalViewForward : FVector(ReplicatedTarget.ViewForward)).GetSafeNormal();
	return bEnabled && IsValid(Settings) && Settings->bEnabled
		&& (bUseLocal ? bLocalTargetValid : ReplicatedTarget.bValid) && !OutTarget.ContainsNaN()
		&& !OutViewForward.ContainsNaN() && !OutViewForward.IsNearlyZero();
}

FVector UWeaponAimAlignmentComponent::ResolvePoseAimTarget(FVector MuzzleLocation, FVector FlightTarget,
	FVector ViewForward, float MinimumForwardDistance, float MaximumViewDeviation, bool& bLimited)
{
	bLimited = false;
	const FVector Forward = ViewForward.GetSafeNormal();
	if (MuzzleLocation.ContainsNaN() || FlightTarget.ContainsNaN() || Forward.ContainsNaN() || Forward.IsNearlyZero()) return FlightTarget;
	const FVector Delta = FlightTarget - MuzzleLocation;
	const double Projection = FVector::DotProduct(Delta, Forward);
	const FVector Lateral = Delta - Forward * Projection;
	const double Minimum = FMath::IsFinite(MinimumForwardDistance) ? FMath::Max(1.f, MinimumForwardDistance) : 200.f;
	const double Degrees = FMath::IsFinite(MaximumViewDeviation) ? FMath::Clamp(MaximumViewDeviation, 1.f, 89.f) : 15.f;
	const double RequiredProjection = Lateral.Size() / FMath::Tan(FMath::DegreesToRadians(Degrees));
	const double SafeProjection = FMath::Max3(Projection, Minimum, RequiredProjection);
	bLimited = SafeProjection > Projection + UE_KINDA_SMALL_NUMBER;
	// Preserve an already-safe target exactly. Near, coincident and rear targets
	// keep their lateral relation to the aim line but are extended visually forward.
	return bLimited ? MuzzleLocation + Lateral + Forward * SafeProjection : FlightTarget;
}

float UWeaponAimAlignmentComponent::ClampSpineAimAngle(UAnimInstance* AnimInstance, float Angle)
{
	const float Limit = GetSpineAimLimit(AnimInstance);
	return FMath::IsFinite(Angle) ? FMath::Clamp(Angle, -Limit, Limit) : 0.f;
}

namespace
{
UWeaponAimAlignmentComponent* FindActiveAlignment(UAnimInstance* AnimInstance)
{
	USkeletalMeshComponent* Body = AnimInstance ? AnimInstance->GetSkelMeshComponent() : nullptr;
	if (!IsValid(Body)) return nullptr;
	TArray<USceneComponent*> Children;
	Body->GetChildrenComponents(false, Children);
	for (USceneComponent* Child : Children)
	{
		AActor* Weapon = IsValid(Child) ? Child->GetOwner() : nullptr;
		UWeaponAimAlignmentComponent* Component = Weapon ? Weapon->FindComponentByClass<UWeaponAimAlignmentComponent>() : nullptr;
		USkeletalMeshComponent* Mesh = Component ? Component->GetThirdPersonWeaponMesh() : nullptr;
		if (!IsValid(Weapon) || Weapon->IsHidden() || !Component || !Component->bEnabled
			|| !IsValid(Component->Settings) || !Component->Settings->bEnabled || !Mesh || Mesh->bHiddenInGame
			|| Mesh->GetAttachParent() != Body) continue;
		return Component;
	}
	return nullptr;
}
}

float UWeaponAimAlignmentComponent::GetSpineAimLimit(UAnimInstance* AnimInstance)
{
	if (const UWeaponAimAlignmentComponent* Alignment = FindActiveAlignment(AnimInstance))
	{
		// The procedural node owns spine_02 pitch for this weapon. Prevent the old
		// upstream pitch and the new chest solve from being added together.
		if (Alignment->Settings->bUseSpinePitch) return 0.f;
		const float Limit = Alignment->Settings->MaximumSpineAimAngle;
		return FMath::IsFinite(Limit) ? FMath::Clamp(Limit, 0.f, 89.f) : 45.f;
	}
	return 89.f;
}

float UWeaponAimAlignmentComponent::GetFreeHandPoseWeight(UAnimInstance* AnimInstance)
{
	const UWeaponAimAlignmentComponent* Alignment = FindActiveAlignment(AnimInstance);
	if (!Alignment || !Alignment->Settings->bKeepFreeHandOnLookDown || Alignment->Settings->Strength <= 0.f) return 0.f;
	for (const UAnimMontage* Montage : Alignment->Settings->SuppressedMontages)
	{
		if (Montage && AnimInstance->Montage_IsActive(Montage)) return 0.f;
	}
	FVector Target, View;
	if (!Alignment->GetAlignmentAim(Target, View)) return 0.f;
	return WeaponAimAlignmentMath::FreeHandWeight(View, Alignment->Settings->FreeHandDownBlendAngle);
}

void UWeaponAimAlignmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APawn* Shooter = GetShooter();
	if (TickShooter.Get() != Shooter)
	{
		if (TickShooter.IsValid()) RemoveTickPrerequisiteActor(TickShooter.Get());
		TickShooter = Shooter;
		if (Shooter) AddTickPrerequisiteActor(Shooter);
	}
	const bool bAuthority = IsValid(Shooter) && Shooter->HasAuthority();
	const bool bValid = RefreshLocalTarget();
	USkeletalMeshComponent* WeaponMesh = GetThirdPersonWeaponMesh();
	USkeletalMeshComponent* Body = WeaponMesh ? Cast<USkeletalMeshComponent>(WeaponMesh->GetAttachParent()) : nullptr;
	if (!bAuthority || !bValid || !bRefreshAuthorityPose || !IsValid(Body)) RestorePoseTick();
	else
	{
		if (ForcedPoseMesh.Get() != Body)
		{
			RestorePoseTick();
			ForcedPoseMesh = Body;
			PreviousPoseTickOption = static_cast<uint8>(Body->VisibilityBasedAnimTickOption);
			bPreviousUpdateRateOptimizations = Body->bEnableUpdateRateOptimizations;
			Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			Body->bEnableUpdateRateOptimizations = false;
			Body->AddTickPrerequisiteComponent(this);
		}
	}
	if (!bAuthority)
	{
		// An actor can lose its owner during unequip. Clear stale server state as well.
		if (GetOwner() && GetOwner()->HasAuthority()) ReplicatedTarget.bValid = false;
		return;
	}
	PublishElapsed += FMath::Max(0.f, DeltaTime);
	if (ReplicatedTarget.bValid != bValid || PublishElapsed >= 1.f / FMath::Clamp(TargetReplicationRate, 1.f, 60.f))
	{
		ReplicatedTarget.bValid = bValid;
		if (bValid)
		{
			ReplicatedTarget.Location = LocalTarget;
			ReplicatedTarget.ViewForward = LocalViewForward;
		}
		PublishElapsed = 0.f;
	}
}

void UWeaponAimAlignmentComponent::RestorePoseTick()
{
	if (USkeletalMeshComponent* Mesh = ForcedPoseMesh.Get())
	{
		Mesh->VisibilityBasedAnimTickOption = static_cast<EVisibilityBasedAnimTickOption>(PreviousPoseTickOption);
		Mesh->bEnableUpdateRateOptimizations = bPreviousUpdateRateOptimizations;
		Mesh->RemoveTickPrerequisiteComponent(this);
	}
	ForcedPoseMesh.Reset();
}

void UWeaponAimAlignmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestorePoseTick();
	Super::EndPlay(EndPlayReason);
}

void UWeaponAimAlignmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponAimAlignmentComponent, ReplicatedTarget);
}

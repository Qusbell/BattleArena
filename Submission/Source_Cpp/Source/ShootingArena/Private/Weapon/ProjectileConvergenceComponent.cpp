#include "Weapon/ProjectileConvergenceComponent.h"

#include "GameFramework/ProjectileMovementComponent.h"

UProjectileConvergenceComponent::UProjectileConvergenceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

bool UProjectileConvergenceComponent::Initialize(UProjectileMovementComponent* InMovement,
	FVector InJoin, FVector InForward)
{
	// These weapons use straight, non-homing movement. Other projectile models retain their original movement.
	if (!IsValid(InMovement) || !InMovement->UpdatedComponent || InMovement->bIsHomingProjectile
		|| !FMath::IsNearlyZero(InMovement->ProjectileGravityScale)
		|| InJoin.ContainsNaN() || InForward.ContainsNaN() || InForward.IsNearlyZero()
		|| InMovement->UpdatedComponent->IsSimulatingPhysics()) return false;
	Movement = InMovement;
	Join = InJoin;
	Forward = InForward.GetSafeNormal();
	const FVector ToJoin = Join - Movement->UpdatedComponent->GetComponentLocation();
	const double Speed = Movement->Velocity.Size();
	if (Speed <= SMALL_NUMBER) return false;
	bJoined = ToJoin.IsNearlyZero(0.01);
	Movement->Velocity = (bJoined ? Forward : ToJoin.GetSafeNormal()) * Speed;
	// Convergence changes heading after spawn. The projectile's local +X must follow
	// both flight legs, including pitch; the legacy path never installs this driver.
	Movement->bRotationFollowsVelocity = true;
	Movement->bRotationRemainsVertical = false;
	Movement->UpdatedComponent->SetWorldRotation(Movement->Velocity.Rotation());
	Movement->UpdateComponentVelocity();
	Movement->SetComponentTickEnabled(false);
	return true;
}

void UProjectileConvergenceComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsValid(Movement) || !Movement->IsActive() || !Movement->UpdatedComponent) return;
	if (bJoined)
	{
		Movement->TickComponent(DeltaTime, TickType, &Movement->PrimaryComponentTick);
		return;
	}
	const double Speed = Movement->Velocity.Size();
	if (Speed <= SMALL_NUMBER) return;
	const double TimeToJoin = FVector::Distance(Movement->UpdatedComponent->GetComponentLocation(), Join) / Speed;
	const float FirstTime = FMath::Min(DeltaTime, static_cast<float>(TimeToJoin));
	if (FirstTime > 0.f) Movement->TickComponent(FirstTime, TickType, &Movement->PrimaryComponentTick);
	// A blocking hit may have stopped movement or destroyed the actor. Never resume through that hit.
	if (!IsValid(GetOwner()) || !Movement->IsActive() || !Movement->UpdatedComponent) return;
	if (TimeToJoin <= DeltaTime && Movement->UpdatedComponent->GetComponentLocation().Equals(Join, 0.1))
	{
		bJoined = true;
		Movement->Velocity = Forward * Movement->Velocity.Size();
		Movement->UpdateComponentVelocity();
		if (Movement->bRotationFollowsVelocity)
			Movement->UpdatedComponent->SetWorldRotation(Forward.Rotation());
		const float RemainingTime = DeltaTime - FirstTime;
		if (RemainingTime > 0.f) Movement->TickComponent(RemainingTime, TickType, &Movement->PrimaryComponentTick);
	}
}

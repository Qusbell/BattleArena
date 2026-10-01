#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectileConvergenceComponent.generated.h"

class UProjectileMovementComponent;

UENUM(BlueprintType)
enum class EProjectileConvergenceMode : uint8
{
	FixedDistance,
	MuzzleRayIntersection
};

/** Per-weapon settings, sampled by the server once for each shot. Distances are in cm. */
UCLASS(ClassGroup = (Weapon), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UProjectileConvergenceSettings : public UActorComponent
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Convergence")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Convergence")
	EProjectileConvergenceMode Mode = EProjectileConvergenceMode::FixedDistance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Convergence", meta = (ClampMin = "1", Units = "cm"))
	float Distance = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Convergence", meta = (ClampMin = "1", Units = "cm"))
	float MaxIntersectionDistance = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Convergence", meta = (ClampMin = "0", Units = "cm"))
	float IntersectionTolerance = 2.f;
	/** Socket local +X is the default barrel direction. Adjust for each weapon's socket axes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile Convergence")
	FRotator MuzzleDirectionOffset = FRotator::ZeroRotator;
};

/** Drives the existing movement component, splitting a frame exactly at the join point.
 * Movement sweeps and the projectile's existing hit/overlap callbacks remain in charge.
 */
UCLASS()
class SHOOTINGARENA_API UProjectileConvergenceComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UProjectileConvergenceComponent();
	bool Initialize(UProjectileMovementComponent* InMovement, FVector InJoin, FVector InForward);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	UPROPERTY(Transient)
	TObjectPtr<UProjectileMovementComponent> Movement;
	FVector Join = FVector::ZeroVector;
	FVector Forward = FVector::ForwardVector;
	bool bJoined = false;
};

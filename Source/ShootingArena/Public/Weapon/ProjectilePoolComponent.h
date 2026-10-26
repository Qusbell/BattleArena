#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UObject/ObjectKey.h"
#include "ProjectilePoolComponent.generated.h"

class UProjectilePoolSubsystem;
class UProjectileMovementComponent;
class UProjectileWorldCollisionComponent;
class UProjectileConvergenceComponent;
class USceneComponent;
class FStructProperty;
class FObjectPropertyBase;
class FBoolProperty;
class FIntProperty;

enum class EProjectilePoolState : uint8 { Idle, Active, Returning, Destroyed };

/** Created once on a pooled Actor, before its first BeginPlay. No per-shot UObject allocations. */
UCLASS()
class SHOOTINGARENA_API UProjectilePoolComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UProjectilePoolComponent();
	bool Capture(UProjectilePoolSubsystem* InPool, bool bInRetained);
	bool StartShot(const FTransform& Transform, AActor* Weapon, UObject* Data, APawn* Instigator,
		bool bCanDealDamage, int32 ShotId);
	bool Release();
	bool IsShotActive() const { return State == EProjectilePoolState::Active; }
	uint64 GetGeneration() const { return Generation; }
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	EProjectilePoolState State = EProjectilePoolState::Idle;
	bool bRetained = false;
	TWeakObjectPtr<AActor> ShotWeapon;
	FObjectKey ShotWeaponKey;
	int32 ShotId = 0;
	bool bDamaging = false;
private:
	friend class UProjectilePoolSubsystem;
	TWeakObjectPtr<UProjectilePoolSubsystem> Pool;
	UPROPERTY(Transient) TObjectPtr<UProjectileMovementComponent> Movement;
	UPROPERTY(Transient) TObjectPtr<UProjectileWorldCollisionComponent> Collision;
	UPROPERTY(Transient) TObjectPtr<UProjectileConvergenceComponent> Convergence;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> DelayedMesh;
	struct FComponentDefault
	{
		TWeakObjectPtr<UActorComponent> Component;
		bool bTick = false, bActive = false, bVisible = true, bHidden = false;
	};
	TArray<FComponentDefault> Defaults;
	FStructProperty* ConfigProperty = nullptr;
	FObjectPropertyBase* ConfigWeapon = nullptr;
	FObjectPropertyBase* ConfigData = nullptr;
	FBoolProperty* DamageProperty = nullptr;
	FIntProperty* IdProperty = nullptr;
	bool bRotationFollowsVelocity = false, bRotationRemainsVertical = false;
	uint64 Generation = 0;
	FTimerHandle ExpiryTimer, VisibilityTimer;
	void SetConfig(AActor* Weapon, UObject* Data);
	void StopShot();
	void ResetBlueprintState();
};

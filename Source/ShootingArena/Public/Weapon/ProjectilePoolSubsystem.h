#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "Weapon/ProjectilePoolSettings.h"
#include "ProjectilePoolSubsystem.generated.h"

class UProjectilePoolComponent;

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FProjectilePoolStats
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Created = 0;
	UPROPERTY(BlueprintReadOnly) int32 Reused = 0;
	UPROPERTY(BlueprintReadOnly) int32 Returned = 0;
	UPROPERTY(BlueprintReadOnly) int32 OverflowCreated = 0;
	UPROPERTY(BlueprintReadOnly) int32 Active = 0;
	UPROPERTY(BlueprintReadOnly) int32 Idle = 0;
	UPROPERTY(BlueprintReadOnly) int32 Returning = 0;
	UPROPERTY(BlueprintReadOnly) int32 PeakActive = 0;
};

USTRUCT()
struct FProjectilePoolBucket
{
	GENERATED_BODY()
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Free;
	FProjectilePoolStats Stats;
	int32 Retained = 0;
	int32 PrewarmRemaining = 0;
};

struct FProjectilePoolShotKey
{
	FObjectKey Weapon;
	int32 Id = 0;
	bool operator==(const FProjectilePoolShotKey& Other) const { return Weapon == Other.Weapon && Id == Other.Id; }
	friend uint32 GetTypeHash(const FProjectilePoolShotKey& Key) { return HashCombine(GetTypeHash(Key.Weapon), GetTypeHash(Key.Id)); }
};

/** Separate pools for each game/PIE world. Only the two arena projectile classes opt in. */
UCLASS()
class SHOOTINGARENA_API UProjectilePoolSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	static bool SupportsClass(const UClass* Class);
	AActor* Acquire(TSubclassOf<AActor> Class, const FTransform& Transform, AActor* Weapon,
		UObject* WeaponData, APawn* Instigator, bool bCanDealDamage, int32 ShotId);
	void Return(UProjectilePoolComponent* Component);
	void Forget(UProjectilePoolComponent* Component);
	bool TracksWeapon(const AActor* Weapon) const;
	int32 RetireVisuals(AActor* Weapon, int32 ShotId);
	/** Old shots continue flying, but must not alias the shooter's next life. */
	void DetachInstigatorForLife(APawn* Instigator);
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile Pool")
	FProjectilePoolStats GetPoolStats(TSubclassOf<AActor> Class) const;
	/** Project defaults are captured at world initialization; explicit console overrides remain live. */
	FProjectilePoolConfiguration GetConfiguration() const;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
private:
	UPROPERTY(Transient) TMap<TObjectPtr<UClass>, FProjectilePoolBucket> Pools;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> Owned;
	TArray<TWeakObjectPtr<UProjectilePoolComponent>> PendingReturns;
	TMap<FProjectilePoolShotKey, TArray<TWeakObjectPtr<UProjectilePoolComponent>>> Shots;
	TSet<TWeakObjectPtr<AActor>> TrackedWeapons;
	bool bShuttingDown = false;
	FProjectilePoolConfiguration WorldConfiguration;
	AActor* Create(UClass* Class, const FTransform& Transform, bool bRetained);
	void Untrack(UProjectilePoolComponent* Component);
};

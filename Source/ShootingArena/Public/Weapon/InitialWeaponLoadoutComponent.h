#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "InitialWeaponLoadoutComponent.generated.h"

USTRUCT(BlueprintType)
struct FInitialWeaponEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSoftClassPtr<AActor> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSoftObjectPtr<UPrimaryDataAsset> WeaponData;
};

/** Configured on BP_ShooterBase. Each array can contain any number of weapons. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UInitialWeaponLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInitialWeaponLoadoutComponent();

	/** Granted on every character spawn. Ammo starts at the data asset's maximum and is never consumed. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial Weapons")
	TArray<FInitialWeaponEntry> DefaultWeapons;

	/** Granted on every character spawn. Ammo follows the normal weapon rules. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial Weapons")
	TArray<FInitialWeaponEntry> GrantedWeapons;

	/** Call from BP_ShooterBase's authority BeginPlay after its inventory is ready. */
	UFUNCTION(BlueprintCallable, Category="Initial Weapons")
	void GrantLoadout();

	bool IsDefaultWeapon(const AActor* Weapon) const;

private:
	bool bGrantedThisSpawn = false;
};

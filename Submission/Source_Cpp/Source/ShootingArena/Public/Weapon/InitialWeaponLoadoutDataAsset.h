#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InitialWeaponLoadoutDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FInitialWeaponEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSoftClassPtr<AActor> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	TSoftObjectPtr<UPrimaryDataAsset> WeaponData;
};

/** Initial weapons shared by ShooterBase instances and editable through JsonAssetSync. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UInitialWeaponLoadoutDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UInitialWeaponLoadoutDataAsset();

	/** Always owned on spawn; ammo starts at maximum and is never consumed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Initial Weapons")
	TArray<FInitialWeaponEntry> DefaultWeapons;

	/** Given on spawn; ammo follows the normal weapon rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Initial Weapons")
	TArray<FInitialWeaponEntry> GrantedWeapons;
};

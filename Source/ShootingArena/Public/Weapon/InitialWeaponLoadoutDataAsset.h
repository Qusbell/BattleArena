#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InitialWeaponLoadoutDataAsset.generated.h"

class UDataTable;

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FInitialWeaponEntry
{
	GENERATED_BODY()

	/** Select a row from the loadout's shared weapon table. None leaves a new entry empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon", meta=(GetOptions="GetWeaponOptions"))
	FName WeaponName;

	/** Serialized legacy pair; retained so older loadout assets continue to work. */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	TSoftClassPtr<AActor> WeaponClass;

	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	TSoftObjectPtr<UPrimaryDataAsset> WeaponData;
};

/** Initial weapons shared by ShooterBase instances and editable through JsonAssetSync. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UInitialWeaponLoadoutDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UInitialWeaponLoadoutDataAsset();

	/** Reuses the class/data pairs already maintained for weapon pickups. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Catalog", AdvancedDisplay)
	TSoftObjectPtr<UDataTable> WeaponTable;

	/** Always owned on spawn; ammo starts at maximum and is never consumed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Initial Weapons", meta=(TitleProperty="WeaponName"))
	TArray<FInitialWeaponEntry> DefaultWeapons;

	/** Given on spawn; ammo follows the normal weapon rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Initial Weapons", meta=(TitleProperty="WeaponName"))
	TArray<FInitialWeaponEntry> GrantedWeapons;

	/** Default weapons are a fallback while no regular weapon has ammunition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon Selection", meta=(DisplayName="일반 총기 사용 가능 시 기본 총기 선택 제한"))
	bool bRestrictDefaultWeaponSelection = true;

	/** Shared by granting, unlimited-ammo checks and death-drop substitution. */
	bool ResolveWeaponEntry(const FInitialWeaponEntry& Entry, FInitialWeaponEntry& OutEntry) const;

	UFUNCTION()
	TArray<FString> GetWeaponOptions() const;
};

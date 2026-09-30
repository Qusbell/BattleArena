#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InitialWeaponLoadoutComponent.generated.h"

class UInitialWeaponLoadoutDataAsset;

/** Grants the weapons configured by a JsonAssetSync-backed data asset. */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UInitialWeaponLoadoutComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInitialWeaponLoadoutComponent();

	/** Initial weapon list applied to BP_ShooterBase. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Initial Weapons")
	TObjectPtr<UInitialWeaponLoadoutDataAsset> LoadoutData = nullptr;

	/** Call from BP_ShooterBase's authority BeginPlay after its inventory is ready. */
	UFUNCTION(BlueprintCallable, Category="Initial Weapons")
	void GrantLoadout();

	bool IsDefaultWeapon(const AActor* Weapon) const;

private:
	bool bGrantedThisSpawn = false;
};

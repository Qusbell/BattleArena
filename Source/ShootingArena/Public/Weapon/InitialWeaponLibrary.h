#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InitialWeaponLibrary.generated.h"

class UActorComponent;

/** Shared default-weapon check for the firing logic. */
UCLASS()
class SHOOTINGARENA_API UInitialWeaponLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Initial Weapons", meta=(DefaultToSelf="Weapon"))
	static bool IsDefaultWeapon(const AActor* Weapon);

	UFUNCTION(BlueprintPure, Category="Initial Weapons")
	static bool CanSelectInventoryWeapon(UActorComponent* Inventory, int32 Index);

	/** Keeps the current regular weapon when a restricted default is requested. */
	UFUNCTION(BlueprintPure, Category="Initial Weapons")
	static int32 ResolveInventoryWeaponSelection(UActorComponent* Inventory, int32 RequestedIndex);

	/** Called after ammo recovery; authority switches away from a now-restricted default. */
	UFUNCTION(BlueprintCallable, Category="Initial Weapons", meta=(DefaultToSelf="ChangedWeapon"))
	static void RefreshDefaultWeaponSelection(AActor* ChangedWeapon);

};

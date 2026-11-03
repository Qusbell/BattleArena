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

	/** One-based HUD/hotkey number; show only defaults when regular ammo is exhausted, otherwise retain empty regular slots. */
	UFUNCTION(BlueprintPure, Category="Initial Weapons")
	static int32 GetInventoryWeaponSlotNumber(UActorComponent* Inventory, int32 Index);

	/** Maps a one-based numeric-key input to the real inventory index; slot zero is unused. Wheel swaps keep raw indices. */
	UFUNCTION(BlueprintPure, Category="Initial Weapons")
	static int32 ResolveInventoryWeaponShortcut(UActorComponent* Inventory, int32 ShortcutIndex);

	/** Keep slot ammo updates connected after a weapon swap; unbind when the HUD changes its displayed pawn. */
	UFUNCTION(BlueprintCallable, Category="Initial Weapons", meta=(DefaultToSelf="HUD"))
	static void SetWeaponSlotConsumptionBinding(UObject* HUD, AActor* Weapon, bool bBind);

	/** Keeps the current regular weapon when a restricted default is requested. */
	UFUNCTION(BlueprintPure, Category="Initial Weapons")
	static int32 ResolveInventoryWeaponSelection(UActorComponent* Inventory, int32 RequestedIndex);

	/** Called after ammo recovery; authority switches away from a now-restricted default. */
	UFUNCTION(BlueprintCallable, Category="Initial Weapons", meta=(DefaultToSelf="ChangedWeapon"))
	static void RefreshDefaultWeaponSelection(AActor* ChangedWeapon);

};

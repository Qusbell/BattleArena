#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemSoundMigrationLibrary.generated.h"

class UBlueprint;
class UDataTable;
class UUserDefinedStruct;

/** Explicit, editor-only migration. Never runs automatically or saves maps/packages. */
UCLASS()
class UItemSoundMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool ConfigureSoundFields(UUserDefinedStruct* Struct, const FString& Kind);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool ExtendSpawnFeedback(UBlueprint* Interface);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool WireSpawnerSound(UBlueprint* Blueprint, UDataTable* Table);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool WirePickupSound(UBlueprint* Blueprint, UDataTable* Table);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool WireSpawnVolume(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool MoveSpawnSoundToSpawn(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool SeparateWeaponPickupSound(UBlueprint* HeldItem, UBlueprint* HoldableItem);

	UFUNCTION(BlueprintCallable, Category="Editor|Item Sound Migration")
	static bool RefreshAndCompile(UBlueprint* Blueprint);
};

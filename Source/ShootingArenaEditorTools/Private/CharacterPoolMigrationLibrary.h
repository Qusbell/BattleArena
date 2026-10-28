#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterPoolMigrationLibrary.generated.h"

class UBlueprint;

UCLASS()
class UCharacterPoolMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Editor|Character Pool") static bool ConnectShooter(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Character Pool") static bool ConnectRespawnHelper(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Character Pool") static bool ConnectGameMode(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Character Pool") static bool ConnectPlayer(UBlueprint* Blueprint);
	/** Includes delegate function names and pin defaults not exposed by the Python node wrappers. */
	UFUNCTION(BlueprintCallable, Category="Editor|Character Pool") static FString ExportGraphs(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Character Pool") static FString ValidateContracts();
};

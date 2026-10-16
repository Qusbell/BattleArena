#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectileBlueprintMigrationLibrary.generated.h"

class UBlueprint;

/** Editor support for Tools/projectile_muzzle_migration.py. */
UCLASS()
class UProjectileBlueprintMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static bool AddMuzzleAimSwitch(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static bool ConfigureProjectileMulticast(UBlueprint* Blueprint, const FString& NodeName);

	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static bool ExtendProjectileAimOutputs(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static void RefreshProjectileNodes(UBlueprint* Blueprint);
};

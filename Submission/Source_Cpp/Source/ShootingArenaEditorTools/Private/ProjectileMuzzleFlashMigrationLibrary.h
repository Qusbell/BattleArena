#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectileMuzzleFlashMigrationLibrary.generated.h"

class UBlueprint;

UCLASS()
class UProjectileMuzzleFlashMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile Migration")
	static bool AddRocketProjectileMuzzleFlash(UBlueprint* Base, UBlueprint* Rocket);

	/** Actor-free direction/absolute-rotation verification; no gameplay world. */
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile Migration")
	static bool VerifyMuzzleFlashDirections();
};

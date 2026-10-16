#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlasmaExplosionMigrationLibrary.generated.h"

class UBlueprint;
class AActor;
class ACharacter;

/** Copies the existing rocket explosion functions, rebinding self references to the plasma Blueprint. */
UCLASS()
class UPlasmaExplosionMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static bool CopyRocketExplosionFunctions(UBlueprint* Rocket, UBlueprint* Plasma);

	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static bool RestoreRocketExplosionDefaults(UBlueprint* Rocket, UBlueprint* Plasma);

	/** Initialize the Blueprint user-defined config struct on transient actors for functional validation. */
	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static bool PrepareExplosionValidationActor(AActor* Projectile, AActor* Weapon, UObject* Config);

	UFUNCTION(BlueprintCallable, Category = "Editor|Projectile Migration")
	static FVector GetExplosionValidationLaunchVelocity(ACharacter* Character);
};

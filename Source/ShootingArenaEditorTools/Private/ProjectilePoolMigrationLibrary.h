#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectilePoolMigrationLibrary.generated.h"

class UBlueprint;

/** Narrow, reproducible edits to the two existing weapon/projectile pairs. */
UCLASS()
class UProjectilePoolMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile Pool")
	static bool ConnectProjectile(UBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile Pool")
	static bool ConnectWeapon(UBlueprint* Blueprint);
	/** Isolated real Game-world fixture, including BeginPlay, timers, collision and repeated BP reuse. */
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile Pool")
	static FString VerifyPool();
};

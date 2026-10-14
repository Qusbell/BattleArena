#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectileCollisionMigrationLibrary.generated.h"

class UBlueprint;

UCLASS()
class UProjectileCollisionMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile")
	static bool AddWorldCollision(UBlueprint* Blueprint, float WorldRadius);
	/** Compile after adding the component, then redirect the existing hit pins. */
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile")
	static bool ConnectResolvedImpact(UBlueprint* Blueprint);
	/** Read-only EditorPreview construction; does not BeginPlay or tick a world. */
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile")
	static FString InspectCollisionInstance(UBlueprint* Blueprint);
	/** Isolated asset fixture; no Actor BeginPlay, game tick, or PIE. */
	UFUNCTION(BlueprintCallable, Category="Editor|Projectile")
	static FString VerifyVisualRetirement(UBlueprint* Blueprint);
};

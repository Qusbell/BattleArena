#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectileMuzzleFlashLibrary.generated.h"

class AActor;
class USceneComponent;

/** Cosmetic shot effects only; never changes projectile movement or damage. */
UCLASS()
class SHOOTINGARENA_API UProjectileMuzzleFlashLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Opt-in weapon class default, muzzle mode and a remote shooter are all required. */
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile")
	static bool ShouldUseProjectileMuzzleFlash(AActor* Weapon);

	/** Call immediately after convergence setup, before any projectile tick.
	 * Follows the selected muzzle location while retaining this shot's world rotation.
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon|Projectile")
	static bool SpawnProjectileMuzzleFlash(AActor* Weapon, USceneComponent* SelectedMesh,
		AActor* Projectile, FVector ShotStart, FVector ShotTarget);

	/** Finite, nonzero velocity wins; a failed spawn uses the same shot endpoints. */
	static FVector ResolveFlashDirection(FVector Velocity, FVector ShotStart, FVector ShotTarget);
};

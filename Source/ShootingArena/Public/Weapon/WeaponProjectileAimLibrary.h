#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponProjectileAimLibrary.generated.h"

class AActor;
class USceneComponent;
class UProjectileConvergenceSettings;

UCLASS()
class SHOOTINGARENA_API UWeaponProjectileAimLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Each client's cosmetic projectile starts at its locally selected FP/TP muzzle.
	 * Authority retains the server-resolved start; every peer retains the server flight target.
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Projectile")
	static FVector ResolveProjectileViewStart(AActor* Shooter, USceneComponent* SelectedMesh,
		FVector AuthoritativeStart);

	/** Resolve the camera's first blocking hit, then aim at it from the muzzle.
	 * Call on the server before multicasting the resolved coordinates.
	 * Missing muzzle sockets fall back to the supplied camera origin.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Projectile", meta = (WorldContext = "WorldContextObject"))
	static void ResolveMuzzleProjectileAim(const UObject* WorldContextObject, AActor* Weapon,
		AActor* Shooter, USceneComponent* MuzzleMesh, FVector CameraStart, FVector CameraEnd,
		TEnumAsByte<ETraceTypeQuery> TraceChannel, FVector& ProjectileStart, FVector& AimTarget);

	UFUNCTION(BlueprintPure, Category = "Weapon|Projectile")
	static void ResolveProjectileConvergence(UProjectileConvergenceSettings* Settings,
		USceneComponent* MuzzleMesh, FVector CameraStart, FVector CameraEnd,
		FVector ProjectileStart, FVector AimTarget, FVector& FlightTarget,
		FVector& FlightForward, bool& bConverge);

	/** Called on every peer with the server-resolved path after the existing spawn/BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Projectile")
	static bool ConfigureProjectileConvergence(AActor* Projectile, FVector JoinPoint,
		FVector FlightForward, bool bConverge);

	/** Finds a forward near-intersection of two rays. Never accepts intersections behind either origin. */
	static bool FindMuzzleRayJoin(FVector CameraStart, FVector CameraForward, FVector MuzzleStart,
		FVector MuzzleForward, double MaxDistance, double Tolerance, FVector& JoinPoint);
};

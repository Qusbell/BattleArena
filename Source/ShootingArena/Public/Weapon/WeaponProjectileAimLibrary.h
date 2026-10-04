#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponProjectileAimLibrary.generated.h"

class AActor;
class USceneComponent;
class UProjectileConvergenceSettings;
class USphereComponent;
class UWorld;

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
		FVector AuthoritativeStart, bool bCameraFallback = false);
	/** Keep the Muzzle position but optionally take rotation from a separate physical barrel socket. */
	static bool TryGetProjectileBoreTransform(USceneComponent* Mesh, FName MuzzleSocket,
		FName AimDirectionSocket, FTransform& OutTransform);

	/** Existing muzzle collision volume plus a zero-radius camera-to-muzzle line; no enlarged sweep. */
	UFUNCTION(BlueprintPure, Category = "Weapon|Projectile")
	static FVector ResolveSafeProjectileStart(AActor* Weapon, AActor* Shooter,
		UProjectileConvergenceSettings* Settings, FVector CameraStart, FVector MuzzleStart,
		bool& bCameraFallback);

	/** Uses inherited/overridden Blueprint collision templates without spawning an actor. */
	static const USphereComponent* GetProjectileCollisionTemplate(AActor* Weapon);
	static bool IsProjectileStartBlocked(UWorld* World, const USphereComponent* Collision,
		FVector Start, AActor* Weapon, AActor* Shooter);
	/** Detects both an overlapping muzzle and a muzzle that has passed completely through a wall. */
	static bool IsProjectileMuzzleObstructed(UWorld* World, const USphereComponent* Collision,
		FVector CameraStart, FVector MuzzleStart, AActor* Weapon, AActor* Shooter);
	/** Resolve only an existing eye overlap, using its penetration depth; never cross a wall. */
	static bool TryResolveCameraProjectileStart(UWorld* World, const USphereComponent* Collision,
		FVector CameraStart, AActor* Weapon, AActor* Shooter, FVector& SafeStart);

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
		FVector& FlightForward, bool& bConverge, bool bCameraFallback = false);

	/** Called on every peer with the server-resolved path after the existing spawn/BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Projectile")
	static bool ConfigureProjectileConvergence(AActor* Projectile, FVector JoinPoint,
		FVector FlightForward, bool bConverge);

	/** Finds a forward near-intersection of two rays. Never accepts intersections behind either origin. */
	static bool FindMuzzleRayJoin(FVector CameraStart, FVector CameraForward, FVector MuzzleStart,
		FVector MuzzleForward, double MaxDistance, double Tolerance, FVector& JoinPoint);
};

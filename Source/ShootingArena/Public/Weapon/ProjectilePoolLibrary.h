#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectilePoolLibrary.generated.h"

UCLASS()
class SHOOTINGARENA_API UProjectilePoolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Used only by PlasmaGun/RocketLauncher. Keeps the existing multicast payload and authority source. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Projectile Pool", meta=(WorldContext="WorldContextObject"))
	static AActor* SpawnPooledProjectile(const UObject* WorldContextObject, TSubclassOf<AActor> ProjectileClass,
		FTransform SpawnTransform, AActor* Weapon, UObject* WeaponData, APawn* Instigator,
		bool bCanDealDamage, int32 ProjectileId);
	/** Existing unmanaged/legacy instances retain normal Destroy behavior. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Projectile Pool", meta=(DefaultToSelf="Projectile"))
	static void ReleaseProjectile(AActor* Projectile);
	/** BP BeginPlay retains its legacy initialization only for actors created outside the pool factory. */
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile Pool", meta=(DefaultToSelf="Projectile"))
	static bool IsPooledProjectile(AActor* Projectile);
};

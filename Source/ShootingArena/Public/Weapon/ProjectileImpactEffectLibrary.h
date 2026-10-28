#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ProjectileImpactEffectLibrary.generated.h"

class UParticleSystem;
class UNiagaraSystem;
class UProjectileWorldCollisionComponent;

/** Cosmetic selection only. The server's impact-kind snapshot travels through the existing multicast. */
UCLASS()
class SHOOTINGARENA_API UProjectileImpactEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Inactive pooled components retain their snapshot; removed components safely read as false. */
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile|Effects")
	static bool GetProjectileImpactWasCharacter(const UProjectileWorldCollisionComponent* Collision);
	/** Guard the entire BP effect/sound branch before reading config fields or spawning. */
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile|Effects", meta=(WorldContext="WorldContextObject"))
	static bool CanPlayProjectileImpactEffects(const UObject* WorldContextObject, const UObject* WeaponConfig);
	/** Select a complete pair, preserving Cascade-first/Niagara-fallback playback in the existing BP. */
	UFUNCTION(BlueprintPure, Category="Weapon|Projectile|Effects")
	static void SelectProjectileImpactEffects(UObject* WeaponConfig, bool bHitCharacter,
		UParticleSystem*& ParticleEffect, UNiagaraSystem*& NiagaraEffect);
};

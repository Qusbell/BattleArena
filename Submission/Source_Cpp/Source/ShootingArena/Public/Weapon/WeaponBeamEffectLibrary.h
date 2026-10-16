#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "WeaponBeamEffectLibrary.generated.h"

class UNiagaraSystem;
class UParticleSystem;

/** Both weapons use instant-beam playback, with separate duration settings and rail-gun-only secondary effects. */
UENUM(BlueprintType)
enum class EWeaponBeamPlayback : uint8
{
	MachineGun UMETA(DisplayName = "Machine Gun (Instant Beam)"),
	RailGun UMETA(DisplayName = "Rail Gun (Instant Beam)")
};

UCLASS()
class SHOOTINGARENA_API UWeaponBeamEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Plays the cosmetic beam on this machine. Call from the existing weapon multicast, using
	 * the firing weapon's world-space muzzle and the authoritative trace's world-space endpoint.
	 * This function does not replicate, trace, or read the receiving client's camera.
	 * Niagara and Cascade can be supplied together; either can also be left empty.
	 * Both weapons render the full trace immediately. MachineGunBeamLifeTime and
	 * RailGunLifeTime independently control how long each weapon's beam remains visible.
	 * MachineGunSpeed and MachineGunFrontOffset remain as compatible Blueprint pins
	 * but are ignored. The optional trail and sonic boom remain rail-gun-only.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Effects",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Play Hit Scan Beam",
			AdvancedDisplay = "MachineGunSpeed,MachineGunFrontOffset,RailGunTrailEffect,RailGunSonicBombEffect"))
	static bool PlayHitScanBeam(
		const UObject* WorldContextObject,
		FVector BeamStart,
		FVector BeamEnd,
		EWeaponBeamPlayback Playback,
		UNiagaraSystem* NiagaraEffect,
		UParticleSystem* CascadeEffect,
		float MachineGunSpeed = 12000.0f,
		float RailGunLifeTime = 0.2f,
		/** Distance from the Niagara particle position to the tracer's visible leading edge, in cm. */
		float MachineGunFrontOffset = 1000.0f,
		UNiagaraSystem* RailGunTrailEffect = nullptr,
		UNiagaraSystem* RailGunSonicBombEffect = nullptr,
		float MachineGunBeamLifeTime = 0.2f);
};

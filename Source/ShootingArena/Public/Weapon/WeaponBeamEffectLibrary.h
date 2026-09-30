#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "WeaponBeamEffectLibrary.generated.h"

class UNiagaraSystem;
class UParticleSystem;

/** The two hit-scan weapons use different visual timing, even though both share the same trace endpoints. */
UENUM(BlueprintType)
enum class EWeaponBeamPlayback : uint8
{
	MachineGun UMETA(DisplayName = "Machine Gun (Travelling Tracer)"),
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
	 * The optional rail-gun trail and sonic boom are separate Niagara systems with
	 * their own authored lifetimes. The sonic boom reads world-space beam endpoints.
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Effects",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Play Hit Scan Beam",
			AdvancedDisplay = "MachineGunSpeed,RailGunLifeTime,MachineGunFrontOffset,RailGunTrailEffect,RailGunSonicBombEffect"))
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
		UNiagaraSystem* RailGunSonicBombEffect = nullptr);
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/DataAsset.h"

#include "WeaponBeamEffectLibrary.generated.h"

class UNiagaraSystem;
class UParticleSystem;
class UMaterialInterface;

USTRUCT()
struct FBeamWaveEmitterBinding
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) FName Emitter;
	UPROPERTY(EditAnywhere) FName CountParameter;
	UPROPERTY(EditAnywhere) int32 AuthoredCount = 0;
	UPROPERTY(EditAnywhere) FName WidthParameter;
	UPROPERTY(EditAnywhere) FName JitterParameter;
	UPROPERTY(EditAnywhere) float AuthoredWidth = 0.f;
	UPROPERTY(EditAnywhere) float AuthoredJitter = 0.f;
};

USTRUCT()
struct FBeamFirstPersonMaterialBinding
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) FName Parameter;
	UPROPERTY(EditAnywhere) TObjectPtr<UMaterialInterface> Material;
};

USTRUCT()
struct FBeamFirstPersonSystemVariant
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) TObjectPtr<UNiagaraSystem> Source;
	UPROPERTY(EditAnywhere) TObjectPtr<UNiagaraSystem> System;
	UPROPERTY(EditAnywhere) TArray<FBeamFirstPersonMaterialBinding> Materials;
	UPROPERTY(EditAnywhere) TArray<FBeamWaveEmitterBinding> Waves;
};

/** Hard references keep the optional local-view variants available in cooked builds. */
UCLASS()
class SHOOTINGARENA_API UBeamFirstPersonAssets : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere) TArray<FBeamFirstPersonSystemVariant> Systems;
	/** Used only by a local rail shot whose real muzzle is occluded by geometry. */
	UPROPERTY(EditAnywhere, Category="Near Waves") bool bLimitNearWaves = true;
	/** Fit the wave to short beams instead of removing it with fixed clearances. */
	UPROPERTY(EditAnywhere, Category="Near Waves") bool bFitWavesToBeam = true;
	UPROPERTY(EditAnywhere, Category="Near Waves", meta=(ClampMin="1", Units="cm")) float WaveFullSizeDistance = 600.f;
	UPROPERTY(EditAnywhere, Category="Near Waves", meta=(ClampMin="0", Units="cm")) float WaveCameraClearance = 120.f;
	UPROPERTY(EditAnywhere, Category="Near Waves", meta=(ClampMin="0", Units="cm")) float WaveMuzzleClearance = 80.f;
	UPROPERTY(EditAnywhere, Category="Near Waves", meta=(ClampMin="0", Units="cm")) float WaveHitClearance = 30.f;
	UPROPERTY(EditAnywhere, Category="Near Waves", meta=(ClampMin="1", Units="cm")) float WaveSpacing = 120.f;
};

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
	UWeaponBeamEffectLibrary(const FObjectInitializer& ObjectInitializer);
	/**
	 * Plays the cosmetic beam on this machine. Call from the existing weapon multicast, using
	 * the firing weapon's world-space muzzle and the authoritative trace's world-space endpoint.
	 * Does not replicate or change hit traces. Only a local FP muzzle hidden by
	 * blocking world geometry receives source correction (camera-to-muzzle visibility check).
	 * A prepared material variant transforms the near beam's full geometry with FPR,
	 * fading to world space at the unchanged endpoint. Authored width/color are retained.
	 * Remote views retain the supplied source. The beam is never split into extra segments.
	 * Pass the weapon (or its component) as world context. The endpoint is unchanged.
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
private:
	UPROPERTY() TObjectPtr<UBeamFirstPersonAssets> FirstPersonAssets;
};

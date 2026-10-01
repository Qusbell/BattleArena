#include "Weapon/WeaponBeamEffectLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/TypeData/ParticleModuleTypeDataBeam2.h"

namespace
{
	bool HasNiagaraParameter(const UNiagaraSystem* System, const FNiagaraTypeDefinition& Type, FName Name)
	{
		return System->GetExposedParameters().IndexOf(FNiagaraVariableBase(Type, Name)) != INDEX_NONE;
	}

	void SetNiagaraPositionIfPresent(UNiagaraComponent* Component, const UNiagaraSystem* System, FName Name, const FVector& Value)
	{
		if (HasNiagaraParameter(System, FNiagaraTypeDefinition::GetPositionDef(), Name))
		{
			Component->SetVariablePosition(Name, Value);
		}
		else if (HasNiagaraParameter(System, FNiagaraTypeDefinition::GetVec3Def(), Name))
		{
			Component->SetVariableVec3(Name, Value);
		}
	}

	void SetNiagaraFloatIfPresent(UNiagaraComponent* Component, const UNiagaraSystem* System, FName Name, float Value)
	{
		if (HasNiagaraParameter(System, FNiagaraTypeDefinition::GetFloatDef(), Name))
		{
			Component->SetVariableFloat(Name, Value);
		}
	}

	bool SpawnNiagaraBeam(
		UWorld* World, UNiagaraSystem* System, const FVector& BeamStart, const FVector& BeamEnd,
		const FRotator& BeamRotation, float BeamDistance, float Duration,
		bool bTrail, bool bSuppressSonicBomb)
	{
		if (!System)
		{
			return false;
		}

		UNiagaraComponent* Niagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, System, BeamStart, BeamRotation, FVector::OneVector,
			true, false, ENCPoolMethod::None, false);
		if (!Niagara)
		{
			return false;
		}
		if (bSuppressSonicBomb)
		{
			// Older rail-gun beam/trail systems also contain this emitter. Suppress it
			// for the machine gun or when a dedicated sonic-boom system is supplied.
			Niagara->SetEmitterEnable(TEXT("SonicBomb"), false);
		}

		// Instant beam effects are authored along local +X. The component is
		// already positioned at the muzzle and rotated toward the trace endpoint.
		// Passing a world-space endpoint here would apply that transform a second time.
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.BeamStart"), FVector::ZeroVector);
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.BeamEnd"), FVector(BeamDistance, 0.0f, 0.0f));

		if (!bTrail)
		{
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.BeamDistance"), BeamDistance);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.BeamLifeTime"), Duration);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.Speed"), 0.0f);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.LifeTime"), Duration);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.MaxDistance"), BeamDistance);
		}
		Niagara->Activate(true);
		return true;
	}

	bool SpawnNiagaraSonicBomb(UWorld* World, UNiagaraSystem* System,
		const FVector& BeamStart, const FVector& BeamEnd, const FRotator& BeamRotation)
	{
		if (!System)
		{
			return false;
		}

		// The sonic boom is a separate world-space emitter. Keeping the system near
		// the beam midpoint also keeps its initial component bounds near the effect.
		UNiagaraComponent* Niagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, System, FMath::Lerp(BeamStart, BeamEnd, 0.5), BeamRotation,
			FVector::OneVector, true, false, ENCPoolMethod::None, false);
		if (!Niagara)
		{
			return false;
		}

		// Both naming schemes are supported so a SonicBomb-only system can reuse
		// either the original Lerp bindings or the newer world-endpoint bindings.
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.BeamStart"), BeamStart);
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.BeamEnd"), BeamEnd);
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.SonicBombWorldStart"), BeamStart);
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.SonicBombWorldEnd"), BeamEnd);
		Niagara->Activate(true);
		return true;
	}
}

bool UWeaponBeamEffectLibrary::PlayHitScanBeam(
	const UObject* WorldContextObject,
	FVector BeamStart,
	FVector BeamEnd,
	EWeaponBeamPlayback Playback,
	UNiagaraSystem* NiagaraEffect,
	UParticleSystem* CascadeEffect,
	float MachineGunSpeed,
	float RailGunLifeTime,
	float MachineGunFrontOffset,
	UNiagaraSystem* RailGunTrailEffect,
	UNiagaraSystem* RailGunSonicBombEffect,
	float MachineGunBeamLifeTime)
{
	if (!WorldContextObject || BeamStart.ContainsNaN() || BeamEnd.ContainsNaN())
	{
		return false;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const FVector BeamDelta = BeamEnd - BeamStart;
	const double Distance = BeamDelta.Size();
	if (!FMath::IsFinite(Distance) || Distance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FRotator BeamRotation = BeamDelta.Rotation();
	const float BeamDistance = static_cast<float>(Distance);
	const bool bMachineGunWeapon = Playback == EWeaponBeamPlayback::MachineGun;
	const float ConfiguredLifeTime = bMachineGunWeapon ? MachineGunBeamLifeTime : RailGunLifeTime;
	const float BeamDuration = FMath::IsFinite(ConfiguredLifeTime) && ConfiguredLifeTime > 0.0f
		? ConfiguredLifeTime : 0.2f;
	// Deprecated travelling-tracer pins remain for existing Blueprint node connections.
	(void)MachineGunSpeed;
	(void)MachineGunFrontOffset;
	const bool bSeparateSonicBomb = !bMachineGunWeapon && RailGunSonicBombEffect != nullptr;
	bool bSpawnedAny = false;

	bSpawnedAny = SpawnNiagaraBeam(
		World, NiagaraEffect, BeamStart, BeamEnd, BeamRotation,
		BeamDistance, BeamDuration, false, bMachineGunWeapon || bSeparateSonicBomb);
	if (!bMachineGunWeapon)
	{
		// Keep the trail system's authored lifetime so it can linger after the beam.
		bSpawnedAny |= SpawnNiagaraBeam(
			World, RailGunTrailEffect, BeamStart, BeamEnd, BeamRotation,
			BeamDistance, BeamDuration, true, bSeparateSonicBomb);
		bSpawnedAny |= SpawnNiagaraSonicBomb(
			World, RailGunSonicBombEffect, BeamStart, BeamEnd, BeamRotation);
	}

	if (CascadeEffect)
	{
		UParticleSystemComponent* Cascade = UGameplayStatics::SpawnEmitterAtLocation(
			World, CascadeEffect, BeamStart, BeamRotation, FVector::OneVector,
			true, EPSCPoolMethod::None, false);
		if (Cascade)
		{
			// Cascade distributions can read these named instance parameters when authored to do so.
			Cascade->SetVectorParameter(TEXT("BeamStart"), BeamStart);
			Cascade->SetVectorParameter(TEXT("BeamEnd"), BeamEnd);
			Cascade->SetFloatParameter(TEXT("BeamDistance"), BeamDistance);
			Cascade->SetFloatParameter(TEXT("BeamLifeTime"), BeamDuration);
			Cascade->SetFloatParameter(TEXT("Speed"), 0.0f);
			Cascade->SetFloatParameter(TEXT("MaxDistance"), BeamDistance);
			Cascade->ActivateSystem(true);

			// Beam emitters configured for User Set source/target receive the exact trace segment.
			for (int32 EmitterIndex = 0; EmitterIndex < CascadeEffect->Emitters.Num(); ++EmitterIndex)
			{
				const UParticleEmitter* Emitter = CascadeEffect->Emitters[EmitterIndex];
				const UParticleLODLevel* LOD = Emitter && !Emitter->LODLevels.IsEmpty() ? Emitter->LODLevels[0] : nullptr;
				if (LOD && Cast<UParticleModuleTypeDataBeam2>(LOD->TypeDataModule))
				{
					Cascade->SetBeamSourcePoint(EmitterIndex, BeamStart, 0);
					Cascade->SetBeamTargetPoint(EmitterIndex, BeamEnd, 0);
				}
			}

			bSpawnedAny = true;
		}
	}

	return bSpawnedAny;
}

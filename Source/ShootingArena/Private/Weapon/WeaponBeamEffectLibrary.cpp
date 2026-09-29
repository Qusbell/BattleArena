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
		const FRotator& BeamRotation, float BeamDistance, float Duration, float Speed,
		float MaxDistance, bool bMachineGun, bool bTrail)
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

		// The rail-gun beam and trail are authored along local +X. The component is
		// already positioned at the muzzle and rotated toward the trace endpoint.
		// Passing a world-space endpoint here would apply that transform a second time.
		const FVector ParameterStart = bMachineGun ? BeamStart : FVector::ZeroVector;
		const FVector ParameterEnd = bMachineGun ? BeamEnd : FVector(BeamDistance, 0.0f, 0.0f);
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.BeamStart"), ParameterStart);
		SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.BeamEnd"), ParameterEnd);
		if (!bMachineGun && !bTrail)
		{
			// SonicBomb lerps these world endpoints in Niagara so its authored Alpha
			// controls the position without changing the rail beam's local endpoints.
			SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.SonicBombWorldStart"), BeamStart);
			SetNiagaraPositionIfPresent(Niagara, System, TEXT("User.SonicBombWorldEnd"), BeamEnd);
		}

		if (!bTrail)
		{
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.BeamDistance"), BeamDistance);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.BeamLifeTime"), Duration);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.Speed"), bMachineGun ? Speed : 0.0f);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.LifeTime"), Duration);
			SetNiagaraFloatIfPresent(Niagara, System, TEXT("User.MaxDistance"), MaxDistance);
		}
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
	UNiagaraSystem* RailGunTrailEffect)
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
	const float Speed = FMath::IsFinite(MachineGunSpeed) && MachineGunSpeed > 0.0f
		? MachineGunSpeed : 12000.0f;
	const float RailDuration = FMath::IsFinite(RailGunLifeTime) && RailGunLifeTime > 0.0f
		? RailGunLifeTime : 0.2f;
	const bool bMachineGun = Playback == EWeaponBeamPlayback::MachineGun;
	const float FrontOffset = FMath::IsFinite(MachineGunFrontOffset)
		? FMath::Max(MachineGunFrontOffset, 0.0f) : 0.0f;
	// Niagara moves the particle's origin, but renders the tracer ahead of that origin.
	// Stop when its visible leading edge reaches the hit point, not when the origin does.
	const float MachineGunTravelDistance = FMath::Max(BeamDistance - FrontOffset, 0.0f);
	const float Duration = bMachineGun ? MachineGunTravelDistance / Speed : RailDuration;
	bool bSpawnedAny = false;

	bSpawnedAny = SpawnNiagaraBeam(
		World, NiagaraEffect, BeamStart, BeamEnd, BeamRotation,
		BeamDistance, Duration, Speed, bMachineGun ? MachineGunTravelDistance : BeamDistance,
		bMachineGun, false);
	if (!bMachineGun)
	{
		// Keep the trail system's authored lifetime so it can linger after the beam.
		bSpawnedAny |= SpawnNiagaraBeam(
			World, RailGunTrailEffect, BeamStart, BeamEnd, BeamRotation,
			BeamDistance, Duration, Speed, BeamDistance, false, true);
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
			Cascade->SetFloatParameter(TEXT("BeamLifeTime"), Duration);
			Cascade->SetFloatParameter(TEXT("Speed"), bMachineGun ? Speed : 0.0f);
			Cascade->SetFloatParameter(TEXT("MaxDistance"), bMachineGun ? MachineGunTravelDistance : BeamDistance);
			Cascade->ActivateSystem(true);

			if (!bMachineGun)
			{
				// Activation creates the emitter instances that SetBeamSource/TargetPoint addresses.
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
			}

			bSpawnedAny = true;
		}
	}

	return bSpawnedAny;
}

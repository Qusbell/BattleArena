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
#include "Components/ActorComponent.h"
#include "Weapon/ProjectileFirstPersonRenderingComponent.h"
#include "Camera/CameraTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UWeaponBeamEffectLibrary::UWeaponBeamEffectLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinderOptional<UBeamFirstPersonAssets> Assets(
		TEXT("/Game/QuakeLike_1_0/HeldItem/NiagaraSystem/FPR/DA_BeamFirstPersonAssets"));
	FirstPersonAssets = Assets.Get();
}

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
		bool bTrail, bool bSuppressSonicBomb, const FBeamFirstPersonSystemVariant* FirstPersonVariant)
	{
		if (!System)
		{
			return false;
		}

		const bool bFirstPerson = FirstPersonVariant && IsValid(FirstPersonVariant->System)
			&& !FirstPersonVariant->Materials.IsEmpty();
		UNiagaraSystem* PlaybackSystem = bFirstPerson ? FirstPersonVariant->System.Get() : System;
		UNiagaraComponent* Niagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, PlaybackSystem, BeamStart, BeamRotation, FVector::OneVector,
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

		// Simulate one full-size beam in its original space. The material transforms
		// the complete near geometry, not only its centreline, and reaches zero FPR
		// at the real endpoint. Non-beam renderers in the variant opt out of FPR.
		const FTransform EffectTransform(BeamRotation, BeamStart);
		SetNiagaraPositionIfPresent(Niagara, PlaybackSystem, TEXT("User.BeamStart"), FVector::ZeroVector);
		SetNiagaraPositionIfPresent(Niagara, PlaybackSystem, TEXT("User.BeamEnd"), EffectTransform.InverseTransformPosition(BeamEnd));
		if (bFirstPerson)
		{
			const FVector LocalEnd = EffectTransform.InverseTransformPosition(BeamEnd);
			for (const FBeamFirstPersonMaterialBinding& Binding : FirstPersonVariant->Materials)
			{
				if (!IsValid(Binding.Material) || Binding.Parameter.IsNone()) { Niagara->DestroyComponent(); return false; }
				UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Binding.Material, Niagara);
				if (!Material) { Niagara->DestroyComponent(); return false; }
				Material->SetVectorParameterValue(TEXT("FPRBeamStartLocal"), FLinearColor::Black);
				Material->SetVectorParameterValue(TEXT("FPRBeamEndLocal"), FLinearColor(LocalEnd.X, LocalEnd.Y, LocalEnd.Z));
				Niagara->SetVariableMaterial(Binding.Parameter, Material);
			}
			Niagara->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		}

		if (!bTrail)
		{
			SetNiagaraFloatIfPresent(Niagara, PlaybackSystem, TEXT("User.BeamDistance"), BeamDistance);
			SetNiagaraFloatIfPresent(Niagara, PlaybackSystem, TEXT("User.BeamLifeTime"), Duration);
			SetNiagaraFloatIfPresent(Niagara, PlaybackSystem, TEXT("User.Speed"), 0.0f);
			SetNiagaraFloatIfPresent(Niagara, PlaybackSystem, TEXT("User.LifeTime"), Duration);
			SetNiagaraFloatIfPresent(Niagara, PlaybackSystem, TEXT("User.MaxDistance"), BeamDistance);
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
	if (!IsValid(WorldContextObject) || BeamStart.ContainsNaN() || BeamEnd.ContainsNaN())
	{
		return false;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const bool bMachineGunWeapon = Playback == EWeaponBeamPlayback::MachineGun;
	const float ConfiguredLifeTime = bMachineGunWeapon ? MachineGunBeamLifeTime : RailGunLifeTime;
	const float BeamDuration = FMath::IsFinite(ConfiguredLifeTime) && ConfiguredLifeTime > 0.0f
		? ConfiguredLifeTime : 0.2f;
	// Deprecated travelling-tracer pins remain for existing Blueprint node connections.
	(void)MachineGunSpeed;
	(void)MachineGunFrontOffset;
	const bool bSeparateSonicBomb = !bMachineGunWeapon && RailGunSonicBombEffect != nullptr;
	bool bSpawnedAny = false;
	const AActor* Weapon = Cast<AActor>(WorldContextObject);
	if (const auto* Component = Cast<UActorComponent>(WorldContextObject)) Weapon = Component->GetOwner();
	FMinimalViewInfo FirstPersonView;
	FVector PhysicalMuzzle;
	const auto* Assets = GetDefault<UWeaponBeamEffectLibrary>()->FirstPersonAssets.Get();
	const bool bOccluded = IsValid(Assets)
		&& UProjectileFirstPersonRenderingComponent::TryGetOccludedEffectView(Weapon, FirstPersonView, PhysicalMuzzle);
	auto FindVariant = [&](UNiagaraSystem* Source) -> const FBeamFirstPersonSystemVariant*
	{
		if (!bOccluded || !IsValid(Source)) return nullptr;
		return Assets->Systems.FindByPredicate([&](const FBeamFirstPersonSystemVariant& Variant)
		{
			return Variant.Source == Source && IsValid(Variant.System) && !Variant.Materials.IsEmpty();
		});
	};
	const FBeamFirstPersonSystemVariant* BeamVariant = FindVariant(NiagaraEffect);
	const FBeamFirstPersonSystemVariant* TrailVariant = FindVariant(RailGunTrailEffect);
	// The FP socket remains in physical space; the material performs its only FPR
	// transformation. Never pre-transform it or the near beam would be corrected twice.
	const FVector NiagaraStart = BeamVariant || TrailVariant ? PhysicalMuzzle : BeamStart;
	const double Distance = FVector::Distance(NiagaraStart, BeamEnd);
	if (!FMath::IsFinite(Distance) || Distance <= KINDA_SMALL_NUMBER || Distance > MAX_flt) return false;
	const FRotator BeamRotation = (BeamEnd - BeamStart).Rotation();
	const FRotator NiagaraRotation = (BeamEnd - NiagaraStart).Rotation();
	const float BeamDistance = static_cast<float>(Distance);

	bSpawnedAny |= SpawnNiagaraBeam(
		World, NiagaraEffect, NiagaraStart, BeamEnd, NiagaraRotation,
		BeamDistance, BeamDuration, false, bMachineGunWeapon || bSeparateSonicBomb, BeamVariant);
	if (!bMachineGunWeapon)
	{
		// Keep the trail system's authored lifetime so it can linger after the beam.
		bSpawnedAny |= SpawnNiagaraBeam(
			World, RailGunTrailEffect, NiagaraStart, BeamEnd, NiagaraRotation,
			BeamDistance, BeamDuration, true, bSeparateSonicBomb, TrailVariant);
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

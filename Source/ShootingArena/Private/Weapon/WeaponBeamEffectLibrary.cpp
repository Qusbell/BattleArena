#include "Weapon/WeaponBeamEffectLibrary.h"
#include "Weapon/BeamEffectPlaybackData.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemInstanceController.h"
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
#include "Weapon/RailBeamWaveMath.h"
#include "Weapon/FirstPersonRenderingMath.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/UObjectIterator.h"
#endif

UMaterialInstanceDynamic* UBeamEffectPlaybackData::GetOrCreateMaterial(FName Parameter, UMaterialInterface* Parent)
{
	TObjectPtr<UMaterialInstanceDynamic>& Material = Materials.FindOrAdd(Parameter);
	if (!IsValid(Material) || Material->Parent != Parent)
	{
		Material = UMaterialInstanceDynamic::Create(Parent, GetOuter());
	}
	return Material;
}

void UBeamEffectPlaybackData::EnableFirstPerson(UNiagaraComponent* Component)
{
	Component->OnSystemFinished.AddUniqueDynamic(this, &ThisClass::OnSystemFinished);
	Component->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
}

void UBeamEffectPlaybackData::OnSystemFinished(UNiagaraComponent* Component)
{
	// The pool does not reset the primitive type when another caller acquires this component.
	Component->OnSystemFinished.RemoveDynamic(this, &ThisClass::OnSystemFinished);
	Component->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
}

UWeaponBeamEffectLibrary::UWeaponBeamEffectLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinderOptional<UBeamFirstPersonAssets> Assets(
		TEXT("/Game/QuakeLike_1_0/HeldItem/NiagaraSystem/FPR/DA_BeamFirstPersonAssets"));
	FirstPersonAssets = Assets.Get();
}

namespace
{
	struct FNearWavePlayback
	{
		RailBeamWaves::FSegment Segment;
		double Spacing = 120.0;
		float SizeScale = 1.f;
		bool bFit = false;
	};
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

	UNiagaraComponent* AcquireNiagaraBeam(UWorld* World, UNiagaraSystem* System,
		const FVector& Location, const FRotator& Rotation)
	{
		UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World, System, Location, Rotation, FVector::OneVector, true, false, ENCPoolMethod::AutoRelease, false);
		if (Component)
		{
			Component->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
			if (const auto Controller = Component->GetSystemInstanceController(); Controller && Controller->IsValid())
			{
				for (const FNiagaraEmitterHandle& Emitter : System->GetEmitterHandles())
				{
					if (Emitter.GetIsEnabled())
						Controller->SetEmitterEnable(Emitter.GetName(), true);
				}
			}
		}
		return Component;
	}

	void SuppressNiagaraEmitter(UNiagaraComponent* Component, FName Emitter)
	{
		const UNiagaraSystem* System = Component->GetAsset();
		if (!System || !System->GetEmitterHandles().ContainsByPredicate([Emitter](const FNiagaraEmitterHandle& Handle)
		{
			return Handle.GetName() == Emitter;
		})) return;

		Component->SetEmitterEnable(Emitter, false);
		// SetEmitterEnable only records the override on a completed instance. An existing
		// pooled controller also needs the flag before activation and any warmup tick.
		if (const auto Controller = Component->GetSystemInstanceController();
			Controller && Controller->IsValid() && Controller->IsComplete())
		{
			Controller->SetEmitterEnable(Emitter, false);
		}
	}

	void ConfigureNearWaves(UNiagaraComponent* Component, const UNiagaraSystem* System,
		const TArray<FBeamWaveEmitterBinding>& Bindings, const FNearWavePlayback* Playback,
		const FTransform& Transform, FVector OriginalEnd)
	{
		const FVector Start = Playback && Playback->Segment.bValid ? Playback->Segment.Start : Transform.GetLocation();
		const FVector End = Playback ? (Playback->Segment.bValid ? Playback->Segment.End : Start) : OriginalEnd;
		SetNiagaraPositionIfPresent(Component, System, TEXT("User.FPRWaveStartLocal"), Transform.InverseTransformPosition(Start));
		SetNiagaraPositionIfPresent(Component, System, TEXT("User.FPRWaveEndLocal"), Transform.InverseTransformPosition(End));
		for (const auto& Binding : Bindings)
		{
			const int32 Count = Playback ? (Playback->bFit
				? RailBeamWaves::FittedCount(Playback->Segment, Playback->Spacing, Binding.AuthoredCount)
				: RailBeamWaves::Count(Playback->Segment, Playback->Spacing, Binding.AuthoredCount)) : Binding.AuthoredCount;
			const float SizeScale = Playback ? Playback->SizeScale : 1.f;
			if (!Binding.WidthParameter.IsNone() && FMath::IsFinite(Binding.AuthoredWidth) && Binding.AuthoredWidth >= 0)
				SetNiagaraFloatIfPresent(Component, System, Binding.WidthParameter, Binding.AuthoredWidth * SizeScale);
			if (!Binding.JitterParameter.IsNone() && FMath::IsFinite(Binding.AuthoredJitter) && Binding.AuthoredJitter >= 0)
				SetNiagaraFloatIfPresent(Component, System, Binding.JitterParameter, Binding.AuthoredJitter * SizeScale);
			if (!Binding.CountParameter.IsNone()
				&& HasNiagaraParameter(System, FNiagaraTypeDefinition::GetIntDef(), Binding.CountParameter))
				Component->SetVariableInt(Binding.CountParameter, Count);
			if (Count == 0 && !Binding.Emitter.IsNone()) SuppressNiagaraEmitter(Component, Binding.Emitter);
		}
	}

	bool SpawnNiagaraBeam(
		UWorld* World, UNiagaraSystem* System, const FVector& BeamStart, const FVector& BeamEnd,
		const FRotator& BeamRotation, float BeamDistance, float Duration,
		bool bTrail, bool bSuppressSonicBomb, const FBeamFirstPersonSystemVariant* FirstPersonVariant,
		const FNearWavePlayback* NearWaves)
	{
		if (!System)
		{
			return false;
		}

		const bool bFirstPerson = FirstPersonVariant && IsValid(FirstPersonVariant->System)
			&& !FirstPersonVariant->Materials.IsEmpty();
		if (bFirstPerson && FirstPersonVariant->Materials.ContainsByPredicate([](const FBeamFirstPersonMaterialBinding& Binding)
		{
			return !IsValid(Binding.Material) || Binding.Parameter.IsNone();
		}))
		{
			return false;
		}
		UNiagaraSystem* PlaybackSystem = bFirstPerson ? FirstPersonVariant->System.Get() : System;
		UNiagaraComponent* Niagara = AcquireNiagaraBeam(World, PlaybackSystem, BeamStart, BeamRotation);
		if (!Niagara)
		{
			return false;
		}
		if (bSuppressSonicBomb)
		{
			// Older rail-gun beam/trail systems also contain this emitter. Suppress it
			// for the machine gun or when a dedicated sonic-boom system is supplied.
			SuppressNiagaraEmitter(Niagara, TEXT("SonicBomb"));
		}

		// Simulate one full-size beam in its original space. The material transforms
		// the complete near geometry, not only its centreline, and reaches zero FPR
		// at the real endpoint. Non-beam renderers in the variant opt out of FPR.
		const FTransform EffectTransform(BeamRotation, BeamStart);
		SetNiagaraPositionIfPresent(Niagara, PlaybackSystem, TEXT("User.BeamStart"), FVector::ZeroVector);
		const FVector LocalEnd = EffectTransform.InverseTransformPosition(BeamEnd);
		SetNiagaraPositionIfPresent(Niagara, PlaybackSystem, TEXT("User.BeamEnd"), LocalEnd);
		if (bFirstPerson)
		{
			ConfigureNearWaves(Niagara, PlaybackSystem, FirstPersonVariant->Waves, NearWaves, EffectTransform, BeamEnd);
			auto* PlaybackData = Cast<UBeamEffectPlaybackData>(Niagara->GetAssetUserDataOfClass(UBeamEffectPlaybackData::StaticClass()));
			if (!PlaybackData)
			{
				PlaybackData = NewObject<UBeamEffectPlaybackData>(Niagara);
				Niagara->AddAssetUserData(PlaybackData);
			}
			for (const FBeamFirstPersonMaterialBinding& Binding : FirstPersonVariant->Materials)
			{
				UMaterialInstanceDynamic* Material = PlaybackData->GetOrCreateMaterial(Binding.Parameter, Binding.Material);
				Material->SetVectorParameterValue(TEXT("FPRBeamStartLocal"), FLinearColor::Black);
				Material->SetVectorParameterValue(TEXT("FPRBeamEndLocal"), FLinearColor(LocalEnd.X, LocalEnd.Y, LocalEnd.Z));
				Niagara->SetVariableMaterial(Binding.Parameter, Material);
			}
			PlaybackData->EnableFirstPerson(Niagara);
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
		UNiagaraComponent* Niagara = AcquireNiagaraBeam(World, System, FMath::Lerp(BeamStart, BeamEnd, 0.5), BeamRotation);
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
	FVector PhysicalMuzzle = BeamStart;
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
	const bool bLimitWaves = bOccluded && !bMachineGunWeapon && Assets->bLimitNearWaves;
	FNearWavePlayback NearWaves;
	if (bLimitWaves)
	{
		NearWaves.bFit = Assets->bFitWavesToBeam;
		if (NearWaves.bFit)
		{
			NearWaves.Segment = RailBeamWaves::FitSegment(PhysicalMuzzle, BeamEnd,
				Assets->WaveMuzzleClearance, Assets->WaveHitClearance);
			FVector RenderedMuzzle;
			const bool bRendered = FirstPersonRendering::TryTransformEffectPosition(FirstPersonView, PhysicalMuzzle, RenderedMuzzle);
			NearWaves.SizeScale = RailBeamWaves::SizeScale(NearWaves.Segment.Length,
				bRendered ? FVector::Distance(RenderedMuzzle, BeamEnd) : NearWaves.Segment.Length, Assets->WaveFullSizeDistance);
		}
		else NearWaves.Segment = RailBeamWaves::MakeSegment(FirstPersonView.Location, PhysicalMuzzle, BeamEnd,
			Assets->WaveCameraClearance, Assets->WaveMuzzleClearance, Assets->WaveHitClearance);
		NearWaves.Spacing = Assets->WaveSpacing;
	}
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
		BeamDistance, BeamDuration, false, bMachineGunWeapon || bSeparateSonicBomb, BeamVariant, bLimitWaves ? &NearWaves : nullptr);
	if (!bMachineGunWeapon)
	{
		// Keep the trail system's authored lifetime so it can linger after the beam.
		bSpawnedAny |= SpawnNiagaraBeam(
			World, RailGunTrailEffect, NiagaraStart, BeamEnd, NiagaraRotation,
			BeamDistance, BeamDuration, true, bSeparateSonicBomb, TrailVariant, bLimitWaves ? &NearWaves : nullptr);
		bSpawnedAny |= SpawnNiagaraSonicBomb(
			World, RailGunSonicBombEffect, BeamStart, BeamEnd, BeamRotation);
	}

	if (CascadeEffect)
	{
		UParticleSystemComponent* Cascade = UGameplayStatics::SpawnEmitterAtLocation(
			World, CascadeEffect, BeamStart, BeamRotation, FVector::OneVector,
			true, EPSCPoolMethod::AutoRelease, false);
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

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBeamPoolReuseTest, "ShootingArena.Weapon.BeamPoolReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponBeamPoolReuseTest::RunTest(const FString&)
{
	if (!FApp::CanEverRender())
	{
		AddInfo(TEXT("Beam pool reuse requires a rendering-enabled editor."));
		return true;
	}

	const auto* Assets = LoadObject<UBeamFirstPersonAssets>(nullptr,
		TEXT("/Game/QuakeLike_1_0/HeldItem/NiagaraSystem/FPR/DA_BeamFirstPersonAssets"));
	if (!TestNotNull(TEXT("Authored first-person beam assets"), Assets)) return false;
	const FBeamFirstPersonSystemVariant* Variant = Assets->Systems.FindByPredicate([](const FBeamFirstPersonSystemVariant& Entry)
	{
		return Entry.Source && Entry.System && !Entry.Materials.IsEmpty();
	});
	if (!TestNotNull(TEXT("Beam variant to replay"), Variant)) return false;
	Variant->System->WaitForCompilationComplete(true, false);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Isolated Niagara test world"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	auto FindBeam = [World, Variant]()
	{
		for (UNiagaraComponent* Component : TObjectRange<UNiagaraComponent>())
			if (Component->GetWorld() == World && Component->GetAsset() == Variant->System
				&& Component->PoolingMethod == ENCPoolMethod::AutoRelease) return Component;
		return static_cast<UNiagaraComponent*>(nullptr);
	};
	const FBeamFirstPersonMaterialBinding& Binding = Variant->Materials[0];
	const FNiagaraVariable MaterialParameter(FNiagaraTypeDefinition::GetUMaterialDef(), Binding.Parameter);
	const FVector FirstEnd(500, 0, 0), SecondEnd(1500, 0, 0);
	if (!TestTrue(TEXT("First-person beam starts"), SpawnNiagaraBeam(World, Variant->Source, FVector::ZeroVector,
		FirstEnd, FRotator::ZeroRotator, FirstEnd.X, 0.2f, false, false, Variant, nullptr))) return false;
	UNiagaraComponent* First = FindBeam();
	if (!TestNotNull(TEXT("First shot has an active pooled component"), First)) return false;
	UMaterialInstanceDynamic* FirstMaterial = Cast<UMaterialInstanceDynamic>(First->GetOverrideParameters().GetUObject(MaterialParameter));
	if (!TestNotNull(TEXT("First shot bound its dynamic material"), FirstMaterial)) return false;
	TestTrue(TEXT("First shot uses first-person rendering"), First->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::FirstPerson);
	First->DeactivateImmediate();
	if (!TestTrue(TEXT("The same component returned to the engine pool"), First->PoolingMethod == ENCPoolMethod::FreeInPool)) return false;
	TestTrue(TEXT("First-person type clears on return"), First->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::None);

	if (!TestTrue(TEXT("World-space playback can borrow the same system"), SpawnNiagaraBeam(World, Variant->System,
		FVector::ZeroVector, SecondEnd, FRotator::ZeroRotator, SecondEnd.X, 0.2f, false, false, nullptr, nullptr))) return false;
	UNiagaraComponent* WorldBeam = FindBeam();
	if (!TestEqual(TEXT("World shot reuses the first component"), WorldBeam, First)) return false;
	TestTrue(TEXT("World shot has no previous first-person override"), WorldBeam->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::None
		&& WorldBeam->GetOverrideParameters().GetUObject(MaterialParameter).Get() != FirstMaterial);
	WorldBeam->DeactivateImmediate();

	if (!TestTrue(TEXT("Second first-person beam starts"), SpawnNiagaraBeam(World, Variant->Source, FVector::ZeroVector,
		SecondEnd, FRotator::ZeroRotator, SecondEnd.X, 0.2f, false, false, Variant, nullptr))) return false;
	UNiagaraComponent* Second = FindBeam();
	if (!TestEqual(TEXT("Second first-person shot reuses the same component"), Second, First)) return false;
	TestTrue(TEXT("The dynamic material is reused and carries the new endpoint"),
		Second->GetOverrideParameters().GetUObject(MaterialParameter).Get() == FirstMaterial
		&& FMath::IsNearlyEqual(FirstMaterial->K2_GetVectorParameterValue(TEXT("FPRBeamEndLocal")).R, static_cast<float>(SecondEnd.X)));
	Second->DeactivateImmediate();
	return !HasAnyErrors();
}
#endif

#include "Weapon/CascadeEffectPoolSubsystem.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/WorldPSCPool.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Weapon/CascadeEffectPoolSettings.h"

namespace
{
TAutoConsoleVariable<float> PlasmaMuzzleSeconds(TEXT("sa.ProjectileFX.PlasmaMuzzleSeconds"), 0.5f,
	TEXT("Maximum playback time for the Plasma Niagara muzzle flash. Zero keeps the asset's full lifetime."));
}

void UCascadeEffectPoolSubsystem::LimitPlasmaMuzzleLifetime(UNiagaraComponent* Effect)
{
	const float Lifetime = PlasmaMuzzleSeconds.GetValueOnGameThread();
	if (Lifetime <= 0.f || !IsValid(Effect) || !Effect->IsActive() || Effect->GetWorld() != GetWorld()) return;
	const UNiagaraSystem* System = Effect->GetAsset();
	if (!IsValid(System) || System->GetOutermost()->GetFName()
		!= TEXT("/Game/MuzzleFlashes/Particles/NiagaraSystems/Plasma_Flash")) return;

	FTimerHandle& Timer = MuzzleTimers.FindOrAdd(Effect);
	Effect->OnSystemFinished.AddUniqueDynamic(this, &ThisClass::OnMuzzleFinished);
	const TWeakObjectPtr<UNiagaraComponent> WeakEffect(Effect);
	GetWorld()->GetTimerManager().SetTimer(Timer, FTimerDelegate::CreateWeakLambda(this, [this, WeakEffect]
	{
		if (UNiagaraComponent* LiveEffect = WeakEffect.Get()) LiveEffect->DeactivateImmediate();
		MuzzleTimers.Remove(WeakEffect);
	}), Lifetime, false);
}

void UCascadeEffectPoolSubsystem::OnMuzzleFinished(UNiagaraComponent* Effect)
{
	FTimerHandle Timer;
	if (MuzzleTimers.RemoveAndCopyValue(Effect, Timer)) GetWorld()->GetTimerManager().ClearTimer(Timer);
	Effect->OnSystemFinished.RemoveDynamic(this, &ThisClass::OnMuzzleFinished);
}

void UCascadeEffectPoolSubsystem::Deinitialize()
{
	for (auto& Pair : MuzzleTimers)
	{
		GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
		if (UNiagaraComponent* Effect = Pair.Key.Get()) Effect->OnSystemFinished.RemoveDynamic(this, &ThisClass::OnMuzzleFinished);
	}
	MuzzleTimers.Reset();
	Super::Deinitialize();
}

bool UCascadeEffectPoolSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UCascadeEffectPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	const auto* Settings = GetDefault<UCascadeEffectPoolSettings>();
	if (bPrewarmed || !Settings->bEnablePrewarm || InWorld.bIsTearingDown
		|| InWorld.GetNetMode() == NM_DedicatedServer || !FApp::CanEverRender())
	{
		return;
	}

	const FString WorldPackage = UWorld::RemovePIEPrefix(InWorld.GetOutermost()->GetName());
	if (!Settings->MatchMaps.ContainsByPredicate([&WorldPackage](const TSoftObjectPtr<UWorld>& Map)
	{
		return Map.ToSoftObjectPath().GetLongPackageName() == WorldPackage;
	}))
	{
		return;
	}

	bPrewarmed = true;
	TRACE_CPUPROFILER_EVENT_SCOPE(CascadeEffectPool_Prewarm);

#if WITH_EDITOR
	if (InWorld.WorldType == EWorldType::PIE)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(WeaponEffects_RequestNiagaraCompile);
		for (const TSoftObjectPtr<UNiagaraSystem>& Asset : Settings->PIECompileSystems)
		{
			if (UNiagaraSystem* System = Asset.LoadSynchronous(); IsValid(System) && System->NeedsRequestCompile())
			{
				System->RequestCompile(false);
			}
		}
	}
#endif

	const IConsoleVariable* PoolEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("FX.ParticleSystemPool.Enable"));
	if (!PoolEnabled || PoolEnabled->GetInt() == 0) return;

	TMap<FSoftObjectPath, int32> Counts;
	for (const FCascadeEffectPrewarmEntry& Entry : Settings->Effects)
	{
		if (Entry.Effect.IsNull() || Entry.Count <= 0) continue;
		int32& Count = Counts.FindOrAdd(Entry.Effect.ToSoftObjectPath());
		Count = FMath::Max(Count, Entry.Count);
	}

	FWorldPSCPool& Pool = InWorld.GetPSCPool();
	for (const auto& Entry : Counts)
	{
		UParticleSystem* Template = TSoftObjectPtr<UParticleSystem>(Entry.Key).LoadSynchronous();
		if (!IsValid(Template) || Template->MaxPoolSize == 0) continue;

		const int32 Count = static_cast<int32>(FMath::Min(static_cast<uint32>(Entry.Value), Template->MaxPoolSize));
		TArray<UParticleSystemComponent*> Components;
		Components.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			UParticleSystemComponent* Component = Pool.CreateWorldParticleSystem(Template, &InWorld, EPSCPoolMethod::ManualRelease);
			if (!Component) break;
			if (Component->PoolingMethod != EPSCPoolMethod::ManualRelease)
			{
				Component->DestroyComponent();
				break;
			}
			Components.Add(Component);
		}
		// Hold all checkouts until the target is met. Inactive components have no completion event.
		for (UParticleSystemComponent* Component : Components)
		{
			Pool.ReclaimWorldParticleSystem(Component);
		}
	}
}

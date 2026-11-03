#include "Weapon/CascadeEffectPoolSubsystem.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/WorldPSCPool.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Weapon/CascadeEffectPoolSettings.h"

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

	const IConsoleVariable* PoolEnabled = IConsoleManager::Get().FindConsoleVariable(TEXT("FX.ParticleSystemPool.Enable"));
	if (!PoolEnabled || PoolEnabled->GetInt() == 0) return;
	bPrewarmed = true;

	TRACE_CPUPROFILER_EVENT_SCOPE(CascadeEffectPool_Prewarm);
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

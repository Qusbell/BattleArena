#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Weapon/CascadeEffectPoolSettings.h"
#include "Weapon/CascadeEffectPoolSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/UObjectIterator.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCascadeEffectPoolPrewarmTest, "ShootingArena.Weapon.CascadeEffectPoolPrewarm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCascadeEffectPoolPrewarmTest::RunTest(const FString&)
{
	if (!FApp::CanEverRender())
	{
		AddInfo(TEXT("Cascade effect pool prewarm requires a rendering-enabled editor."));
		return true;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Isolated world"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	auto* Prewarm = World->GetSubsystem<UCascadeEffectPoolSubsystem>();
	if (!TestNotNull(TEXT("World prewarmer"), Prewarm)) return false;

	auto* Effect = NewObject<UParticleSystem>();
	Effect->MaxPoolSize = 2;
	const TSoftObjectPtr<UParticleSystem> EffectReference(Effect);
	auto* Settings = GetMutableDefault<UCascadeEffectPoolSettings>();
	TGuardValue<bool> Enabled(Settings->bEnablePrewarm, true);
	TGuardValue<TArray<TSoftObjectPtr<UWorld>>> Maps(Settings->MatchMaps, {});
	TGuardValue<TArray<FCascadeEffectPrewarmEntry>> Entries(Settings->Effects,
		{{EffectReference, 1}, {EffectReference, 3}});

	auto FindIdleComponents = [World, Effect]()
	{
		TSet<UParticleSystemComponent*> Result;
		for (UParticleSystemComponent* Component : TObjectRange<UParticleSystemComponent>())
		{
			if (Component->GetWorld() == World && Component->Template == Effect
				&& Component->PoolingMethod == EPSCPoolMethod::FreeInPool)
			{
				Result.Add(Component);
			}
		}
		return Result;
	};

	Prewarm->OnWorldBeginPlay(*World);
	TestEqual(TEXT("A world outside the configured maps does not prewarm"), FindIdleComponents().Num(), 0);

	Settings->MatchMaps.Add(TSoftObjectPtr<UWorld>(World));
	Prewarm->OnWorldBeginPlay(*World);
	const TSet<UParticleSystemComponent*> Idle = FindIdleComponents();
	if (!TestEqual(TEXT("Duplicate rows use the larger target, capped by the asset limit"), Idle.Num(), 2)) return false;
	for (const UParticleSystemComponent* Component : Idle)
	{
		TestFalse(TEXT("Prewarmed components are never activated"), Component->IsActive());
		TestFalse(TEXT("Idle components are not registered for rendering"), Component->IsRegistered());
	}

	TSet<UParticleSystemComponent*> CheckedOut;
	for (int32 Index = 0; Index < Idle.Num(); ++Index)
	{
		UParticleSystemComponent* Shot = UGameplayStatics::SpawnEmitterAtLocation(World, Effect,
			FTransform::Identity, true, EPSCPoolMethod::AutoRelease, false);
		if (!TestNotNull(TEXT("The normal spawn path returns a component"), Shot)) return false;
		TestTrue(TEXT("Normal AutoRelease playback borrows a distinct prewarmed component"),
			Idle.Contains(Shot) && !CheckedOut.Contains(Shot));
		CheckedOut.Add(Shot);
	}
	Prewarm->OnWorldBeginPlay(*World);
	TestEqual(TEXT("Prewarming only runs once per world"), FindIdleComponents().Num(), 0);
	for (UParticleSystemComponent* Component : CheckedOut)
	{
		World->GetPSCPool().ReclaimWorldParticleSystem(Component);
	}
	return !HasAnyErrors();
}

#endif

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Weapon/ProjectilePoolSettings.h"
#include "Weapon/ProjectilePoolSubsystem.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectilePoolWorldSettingsTest,
	"ShootingArena.ProjectilePool.ProjectSettingsWorldSnapshotAndOverrides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectilePoolWorldSettingsTest::RunTest(const FString&)
{
	UProjectilePoolSettings* Defaults = GetMutableDefault<UProjectilePoolSettings>();
	const FProjectilePoolConfiguration Original{Defaults->bEnabled, Defaults->PlasmaPrewarm, Defaults->PlasmaCapacity,
		Defaults->RocketPrewarm, Defaults->RocketCapacity, Defaults->PrewarmPerTick};
	const TCHAR* Names[] = {TEXT("sa.ProjectilePool.Enabled"), TEXT("sa.ProjectilePool.PlasmaPrewarm"),
		TEXT("sa.ProjectilePool.RocketPrewarm"), TEXT("sa.ProjectilePool.PlasmaCapacity"),
		TEXT("sa.ProjectilePool.RocketCapacity"), TEXT("sa.ProjectilePool.PrewarmPerTick")};
	TArray<TPair<IConsoleVariable*, int32>> Previous;
	ON_SCOPE_EXIT
	{
		Defaults->bEnabled = Original.bEnabled;
		Defaults->PlasmaPrewarm = Original.PlasmaPrewarm; Defaults->PlasmaCapacity = Original.PlasmaCapacity;
		Defaults->RocketPrewarm = Original.RocketPrewarm; Defaults->RocketCapacity = Original.RocketCapacity;
		Defaults->PrewarmPerTick = Original.PrewarmPerTick;
		for (auto& Pair : Previous) Pair.Key->SetWithCurrentPriority(Pair.Value);
	};
	for (const TCHAR* Name : Names)
	{
		IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Name);
		if (!TestNotNull(Name, Variable)) return false;
		Previous.Emplace(Variable, Variable->GetInt());
		Variable->SetWithCurrentPriority(-1);
	}
	Defaults->bEnabled = true;
	Defaults->PlasmaPrewarm = 10; Defaults->PlasmaCapacity = 3;
	Defaults->RocketPrewarm = -2; Defaults->RocketCapacity = -1;
	Defaults->PrewarmPerTick = 0;
	// World initialization creates the subsystem, but no BeginPlay or projectile BP runs.
	UWorld* First = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false);
	if (!TestNotNull(TEXT("First isolated world created"), First)) return false;
	ON_SCOPE_EXIT { First->DestroyWorld(false); };
	UProjectilePoolSubsystem* FirstPool = First->GetSubsystem<UProjectilePoolSubsystem>();
	if (!TestNotNull(TEXT("Pool initialized in an isolated game world"), FirstPool)) return false;
	const FProjectilePoolConfiguration Initial = FirstPool->GetConfiguration();
	TestEqual(TEXT("Initial Plasma size is capped by retained limit"), Initial.PlasmaPrewarm, 3);
	TestEqual(TEXT("Negative Rocket size becomes zero"), Initial.RocketPrewarm, 0);
	TestEqual(TEXT("Negative Rocket capacity is clamped to zero"), Initial.RocketCapacity, 0);
	TestEqual(TEXT("Configured prewarm budget cannot stall forever"), Initial.PrewarmPerTick, 1);
	Defaults->PlasmaPrewarm = 7; Defaults->PlasmaCapacity = 9;
	Defaults->RocketPrewarm = 2; Defaults->RocketCapacity = 5;
	TestEqual(TEXT("Editing settings does not resize an existing world"), FirstPool->GetConfiguration().PlasmaCapacity, 3);
	UWorld* Second = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, false);
	if (!TestNotNull(TEXT("Second isolated world created"), Second)) return false;
	ON_SCOPE_EXIT { Second->DestroyWorld(false); };
	UProjectilePoolSubsystem* SecondPool = Second->GetSubsystem<UProjectilePoolSubsystem>();
	if (!TestNotNull(TEXT("Second world's pool initialized"), SecondPool)) return false;
	const FProjectilePoolConfiguration NewWorld = SecondPool->GetConfiguration();
	TestEqual(TEXT("New world uses changed Plasma size"), NewWorld.PlasmaPrewarm, 7);
	TestEqual(TEXT("Rocket settings remain independent"), NewWorld.RocketCapacity, 5);
	IConsoleVariable* Capacity = IConsoleManager::Get().FindConsoleVariable(TEXT("sa.ProjectilePool.PlasmaCapacity"));
	Capacity->SetWithCurrentPriority(1);
	TestEqual(TEXT("Explicit console override remains supported"), SecondPool->GetConfiguration().PlasmaCapacity, 1);
	TestEqual(TEXT("Override also bounds prewarming"), SecondPool->GetConfiguration().PlasmaPrewarm, 1);
	Capacity->SetWithCurrentPriority(-1);
	TestEqual(TEXT("Removing override returns to this world's snapshot"), SecondPool->GetConfiguration().PlasmaCapacity, 9);
	return !HasAnyErrors();
}
#endif

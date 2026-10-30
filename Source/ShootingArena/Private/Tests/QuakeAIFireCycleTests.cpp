#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "AI/QuakeAIFireCycleComponent.h"
#include "AI/QuakeTargetSelectionAIController.h"
#include "../AI/QuakeAIFireCycleState.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeAIFireCycleFormulaTest,
	"ShootingArena.AI.FireCycle.LegacyFormulaAndDelayedFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeAIFireCycleFormulaTest::RunTest(const FString&)
{
	FQuakeAIFireCycleState State;
	State.BeginNext(10., .8, .5);
	TestTrue(TEXT("Legacy firing probability comparison"), State.bFiring);
	TestEqual(TEXT("Legacy firing duration is 1-throttle"), State.EndTime, 10.2);
	TestFalse(TEXT("Strict greater-than deadline is preserved"), State.IsExpired(10.2));
	State.BeginNext(12., .8, .9);
	TestFalse(TEXT("Legacy rest probability comparison"), State.bFiring);
	TestEqual(TEXT("Rest starts at actual update, without catch-up"), State.EndTime, 12.8);
	State.BeginNext(20., 0., .5);
	TestFalse(TEXT("Throttle zero retains legacy rest state"), State.bFiring);
	TestEqual(TEXT("Zero-duration rest is finite"), State.EndTime, 20.);
	State.BeginNext(21., 1., .5);
	TestTrue(TEXT("Throttle one retains legacy fire state"), State.bFiring);
	TestEqual(TEXT("Zero-duration fire is finite"), State.EndTime, 21.);
	State.BeginNext(30., .5, .5);
	TestFalse(TEXT("Equality selects rest, as in the Blueprint"), State.bFiring);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeAIFireCycleLifecycleTest,
	"ShootingArena.AI.FireCycle.BranchTargetAndLifeBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeAIFireCycleLifecycleTest::RunTest(const FString&)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated regression world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Controller = World->SpawnActor<AQuakeTargetSelectionAIController>();
	auto* Pawn = World->SpawnActor<APawn>();
	Controller->Possess(Pawn);
	auto* Data = NewObject<UBlackboardData>();
	FBlackboardEntry Enemy; Enemy.EntryName = TEXT("Enemy"); Enemy.KeyType = NewObject<UBlackboardKeyType_Object>(Data);
	FBlackboardEntry Kill; Kill.EntryName = TEXT("Debug_KillSwitch"); Kill.KeyType = NewObject<UBlackboardKeyType_Bool>(Data);
	Data->Keys.Add(Enemy); Data->Keys.Add(Kill);
	UBlackboardComponent* BB = nullptr;
	if (!TestTrue(TEXT("Native blackboard initialized"), Controller->UseBlackboard(Data, BB))) return false;
	auto* Cycle = Controller->FireCycle.Get();
	// This isolated world never runs BeginPlay; activate the component explicitly.
	Cycle->Activate(true);
	Cycle->InitializeForPawn(Pawn);
	Cycle->ObserveBlackboard();
	TestFalse(TEXT("No timer without Enemy"), World->GetTimerManager().IsTimerActive(Cycle->UpdateTimer));
	AActor* FirstEnemy = World->SpawnActor<AActor>();
	AActor* NextEnemy = World->SpawnActor<AActor>();
	BB->SetValueAsObject(TEXT("Enemy"), FirstEnemy);
	TestFalse(TEXT("First combat waits for Activation to supply settings"), Cycle->bCombatActive);
	TestFalse(TEXT("No update timer before settings arrive"), World->GetTimerManager().IsTimerActive(Cycle->UpdateTimer));
	UObject* FirstService = FirstEnemy;
	UObject* NextService = NextEnemy;
	Cycle->ActivateFireGateForController(Controller, FirstService, 1., .8);
	TestTrue(TEXT("Activation with settings starts the first cycle"), Cycle->bCombatActive);
	TestEqual(TEXT("AimSkill is supplied directly"), Cycle->CachedAimSkill, 1.);
	TestEqual(TEXT("Throttle is supplied directly"), Cycle->CachedFireThrottle, .8);
	TestTrue(TEXT("Combat starts the update timer"), World->GetTimerManager().IsTimerActive(Cycle->UpdateTimer));
	Cycle->bFirePhase = true; Cycle->PhaseEndTime = 100.;
	Cycle->SetFireGate(FirstService, false);
	TestFalse(TEXT("Branch exit immediately releases gate"), Cycle->ActiveService.IsValid());
	TestEqual(TEXT("Branch exit preserves deadline"), Cycle->PhaseEndTime, 100.);
	Cycle->ActivateFireGateForController(Controller, NextService, 1., .8);
	TestEqual(TEXT("Activation settings do not restart an existing phase"), Cycle->PhaseEndTime, 100.);
	Cycle->SetFireGate(FirstService, false);
	TestTrue(TEXT("A stale service cannot release a newer gate"), Cycle->ActiveService.Get() == NextService);
	BB->SetValueAsObject(TEXT("Enemy"), NextEnemy);
	TestTrue(TEXT("Target change preserves fire phase"), Cycle->bFirePhase);
	TestEqual(TEXT("Target change preserves deadline"), Cycle->PhaseEndTime, 100.);
	BB->SetValueAsBool(TEXT("Debug_KillSwitch"), true);
	TestFalse(TEXT("Kill switch ends combat"), Cycle->bCombatActive);
	TestFalse(TEXT("Kill switch removes timer"), World->GetTimerManager().IsTimerActive(Cycle->UpdateTimer));
	BB->SetValueAsBool(TEXT("Debug_KillSwitch"), false);
	TestTrue(TEXT("Re-enable starts fresh combat"), Cycle->bCombatActive);
	BB->ClearValue(TEXT("Enemy"));
	TestFalse(TEXT("Forgetting Enemy resets cycle"), Cycle->bCombatActive);
	TestTrue(TEXT("Fixed settings survive between combats in the same life"), Cycle->bHasCharacteristic);
	BB->SetValueAsObject(TEXT("Enemy"), FirstEnemy);
	Cycle->OnPawnDeath(nullptr, nullptr);
	Cycle->ObserveBlackboard();
	TestFalse(TEXT("Dead life cannot restart from a remaining Enemy"), Cycle->bCombatActive);
	Cycle->InitializeForPawn(Pawn); Cycle->ObserveBlackboard();
	TestFalse(TEXT("New life waits for its first settings delivery"), Cycle->bCombatActive);
	Cycle->ActivateFireGateForController(Controller, FirstService, 1., .8);
	TestTrue(TEXT("New life can start a fresh cycle"), Cycle->bCombatActive);
	Cycle->ResetForLife();
	BB->SetValueAsObject(TEXT("Enemy"), NextEnemy);
	TestFalse(TEXT("Pool reset detaches observers and clears cycle"), Cycle->bCombatActive);
	TestFalse(TEXT("Pool reset clears pawn reference"), Cycle->LifePawn.IsValid());
	TestFalse(TEXT("Pool reset removes update timer"), World->GetTimerManager().IsTimerActive(Cycle->UpdateTimer));
	return !HasAnyErrors();
}

#endif

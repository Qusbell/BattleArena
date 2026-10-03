#if WITH_DEV_AUTOMATION_TESTS
#include "Camera/ShooterRevengeComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRevengePlayerStateBridgeTest, "ShootingArena.CharacterDisplay.RevengePlayerStateBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRevengePlayerStateBridgeTest::RunTest(const FString& Parameters)
{
	UClass* StateClass = LoadClass<APlayerState>(nullptr,
		TEXT("/Game/QuakeLike_1_0/GameMode/BP_QuakePlayerState.BP_QuakePlayerState_C"));
	if (!TestNotNull(TEXT("Actual main PlayerState Blueprint"), StateClass)) return false;
	FObjectPropertyBase* Canonical = FindFProperty<FObjectPropertyBase>(StateClass, TEXT("RevengeTarget"));
	if (!TestNotNull(TEXT("Canonical Blueprint target"), Canonical)) return false;
	AddInfo(FString::Printf(TEXT("Blueprint RevengeTarget replication flag: %s; display bridge uses owner-only replication."),
		Canonical->HasAnyPropertyFlags(CPF_Net) ? TEXT("ON") : TEXT("OFF")));
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated bridge regression world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* PC = World->SpawnActor<APlayerController>();
	FActorSpawnParameters Spawn; Spawn.Owner = PC;
	APlayerState* OwnerState = World->SpawnActor<APlayerState>(StateClass, Spawn);
	APlayerState* Killer = World->SpawnActor<APlayerState>(StateClass);
	APlayerState* Other = World->SpawnActor<APlayerState>(StateClass);
	FObjectPropertyBase* PCState = FindFProperty<FObjectPropertyBase>(PC->GetClass(), TEXT("PlayerState"));
	PCState->SetObjectPropertyValue_InContainer(PC, OwnerState);
	UShooterRevengeComponent* Bridge = NewObject<UShooterRevengeComponent>(PC);
	PC->AddInstanceComponent(Bridge);
	Bridge->RegisterComponent();
	World->InitializeActorsForPlay(FURL());
	FObjectPropertyBase* Snapshot = FindFProperty<FObjectPropertyBase>(Bridge->GetClass(), TEXT("TargetPlayerState"));
	auto Event = [&](APlayerState* State, const TCHAR* Name, const TCHAR* Parameter, APlayerState* Value)
	{
		UFunction* Function = State->FindFunction(FName(Name));
		if (!TestNotNull(Name, Function)) return;
		FStructOnScope Args(Function);
		FObjectPropertyBase* Input = FindFProperty<FObjectPropertyBase>(Function, Parameter);
		if (!TestNotNull(Parameter, Input)) return;
		Input->SetObjectPropertyValue_InContainer(Args.GetStructMemory(), Value);
		State->ProcessEvent(Function, Args.GetStructMemory());
	};
	auto Expect = [&](const TCHAR* Label, APlayerState* Target)
	{
		TestEqual(Label, Cast<APlayerState>(Canonical->GetObjectPropertyValue_InContainer(OwnerState)), Target);
		TestEqual(TEXT("Authority display reads the same canonical target immediately"), Bridge->GetRevengeTargetPlayerState(), Target);
		Bridge->RefreshRevengeTargetFromPlayerState();
		TestEqual(TEXT("Owner replication snapshot matches Blueprint"), Cast<APlayerState>(Snapshot->GetObjectPropertyValue_InContainer(Bridge)), Target);
	};
	Expect(TEXT("Initial target is empty"), nullptr);
	Event(OwnerState, TEXT("Play On Death"), TEXT("Killer Player State"), Killer);
	Expect(TEXT("Existing death event designates the direct killer"), Killer);
	APawn* OldPawn = World->SpawnActor<APawn>();
	FObjectPropertyBase* PawnState = FindFProperty<FObjectPropertyBase>(OldPawn->GetClass(), TEXT("PlayerState"));
	PawnState->SetObjectPropertyValue_InContainer(OldPawn, Killer);
	TestTrue(TEXT("Target pawn is recognized"), Bridge->IsRevengeTarget(OldPawn));
	Event(Killer, TEXT("Play On Death"), TEXT("Killer Player State"), Other);
	Expect(TEXT("Third-party kill does not clear my revenge target"), Killer);
	OldPawn->Destroy();
	APawn* Respawn = World->SpawnActor<APawn>();
	PawnState->SetObjectPropertyValue_InContainer(Respawn, Killer);
	TestTrue(TEXT("Target remains recognized after pawn respawn"), Bridge->IsRevengeTarget(Respawn));
	Event(OwnerState, TEXT("Play On Kill"), TEXT("Death Player State"), Other);
	Expect(TEXT("Killing a different player preserves revenge"), Killer);
	Event(OwnerState, TEXT("Play On Kill"), TEXT("Death Player State"), Killer);
	Expect(TEXT("Existing revenge kill event clears display and voice target together"), nullptr);
	TestFalse(TEXT("Cleared target uses normal presentation"), Bridge->IsRevengeTarget(Respawn));
	Event(OwnerState, TEXT("Play On Death"), TEXT("Killer Player State"), Killer);
	Event(OwnerState, TEXT("Play On Death"), TEXT("Killer Player State"), Other);
	Expect(TEXT("Last killer replaces the previous target"), Other);
	Event(OwnerState, TEXT("Play On Death"), TEXT("Killer Player State"), nullptr);
	Expect(TEXT("Existing non-player death policy preserves target"), Other);
	Event(OwnerState, TEXT("Play On Kill"), TEXT("Death Player State"), nullptr);
	Expect(TEXT("Missing victim does not clear revenge"), Other);
	Bridge->SetRevengeTargetPlayerState(Killer);
	Expect(TEXT("External display API writes canonical PlayerState"), Killer);
	PC->SetRole(ROLE_AutonomousProxy);
	Canonical->SetObjectPropertyValue_InContainer(OwnerState, Other);
	TestEqual(TEXT("Owning client reads replicated snapshot, not a nonreplicated Blueprint field"),
		Bridge->GetRevengeTargetPlayerState(), Killer);
	Bridge->ClearRevengeTarget();
	TestEqual(TEXT("Client setters cannot change authoritative target"),
		Cast<APlayerState>(Canonical->GetObjectPropertyValue_InContainer(OwnerState)), Other);
	PC->SetRole(ROLE_Authority);
	Bridge->RefreshRevengeTargetFromPlayerState();
	Expect(TEXT("Authority polling discovers external Blueprint changes"), Other);
	Bridge->ClearRevengeTarget();
	Expect(TEXT("External clear API clears canonical PlayerState"), nullptr);
	Canonical->SetObjectPropertyValue_InContainer(OwnerState, OwnerState);
	TestNull(TEXT("Malformed self target is never presented"), Bridge->GetRevengeTargetPlayerState());
	Bridge->SetRevengeTargetPlayerState(OwnerState);
	Expect(TEXT("External self target is sanitized"), nullptr);
	UWorld* ForeignWorld = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Values);
	if (TestNotNull(TEXT("Separate world for travel boundary checks"), ForeignWorld))
	{
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(ForeignWorld);
		APlayerState* ForeignState = ForeignWorld->SpawnActor<APlayerState>(StateClass);
		APawn* ForeignPawn = ForeignWorld->SpawnActor<APawn>();
		Bridge->SetRevengeTargetPlayerState(Killer);
		Bridge->SetRevengeTargetPlayerState(ForeignState);
		Expect(TEXT("Cross-world target input preserves the current valid target"), Killer);
		PawnState->SetObjectPropertyValue_InContainer(ForeignPawn, Killer);
		TestFalse(TEXT("A pawn from another world cannot match revenge"), Bridge->IsRevengeTarget(ForeignPawn));
		Canonical->SetObjectPropertyValue_InContainer(OwnerState, ForeignState);
		Bridge->RefreshRevengeTargetFromPlayerState();
		TestNull(TEXT("Cross-world Blueprint target is never presented"), Bridge->GetRevengeTargetPlayerState());
		PCState->SetObjectPropertyValue_InContainer(PC, ForeignState);
		Bridge->SetRevengeTargetPlayerState(Killer);
		TestNull(TEXT("Foreign PlayerState is never modified through this controller"), Canonical->GetObjectPropertyValue_InContainer(ForeignState));
		PCState->SetObjectPropertyValue_InContainer(PC, OwnerState);
		ForeignWorld->DestroyWorld(false);
		GEngine->DestroyWorldContext(ForeignWorld);
	}
	Bridge->SetRevengeTargetPlayerState(Killer);
	World->bIsTearingDown = true;
	Bridge->ClearRevengeTarget();
	TestNull(TEXT("World shutdown suppresses revenge presentation"), Bridge->GetRevengeTargetPlayerState());
	TestEqual(TEXT("World shutdown prevents gameplay target mutation"), Canonical->GetObjectPropertyValue_InContainer(OwnerState), static_cast<UObject*>(Killer));
	World->bIsTearingDown = false;
	Bridge->SetRevengeTargetPlayerState(Other);
	Other->Destroy();
	Bridge->RefreshRevengeTargetFromPlayerState();
	TestNull(TEXT("Destroyed target is ignored safely"), Bridge->GetRevengeTargetPlayerState());
	TestNull(TEXT("Destroyed target is removed from replication snapshot"), Snapshot->GetObjectPropertyValue_InContainer(Bridge));
	APlayerState* PlainState = World->SpawnActor<APlayerState>();
	PCState->SetObjectPropertyValue_InContainer(PC, PlainState);
	Bridge->SetRevengeTargetPlayerState(Killer);
	Bridge->RefreshRevengeTargetFromPlayerState();
	TestNull(TEXT("Missing canonical property has no independent gameplay fallback"), Bridge->GetRevengeTargetPlayerState());
	PCState->SetObjectPropertyValue_InContainer(PC, nullptr);
	Bridge->ClearRevengeTarget();
	TestNull(TEXT("Uninitialized PlayerState is safe"), Bridge->GetRevengeTargetPlayerState());
	Bridge->DestroyComponent();
	Bridge->SetRevengeTargetPlayerState(Killer);
	TestNull(TEXT("Calls after component destruction remain safe"), Bridge->GetRevengeTargetPlayerState());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif

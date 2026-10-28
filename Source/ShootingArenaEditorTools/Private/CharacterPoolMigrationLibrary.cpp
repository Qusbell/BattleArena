#include "CharacterPoolMigrationLibrary.h"

#include "Character/CharacterPoolComponent.h"
#include "Character/CharacterPoolLibrary.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Event.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Self.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

namespace CharacterPoolMigration
{
UEdGraph* Graph(UBlueprint* BP, FName Name)
{
	TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
	for (UEdGraph* G : Graphs) if (G->GetFName() == Name) return G;
	return nullptr;
}
bool Link(UEdGraphPin* A, UEdGraphPin* B)
{
	return A && B && GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(A, B);
}
bool Move(UEdGraphPin* From, UEdGraphPin* To)
{
	if (!From || !To) return false;
	const auto Links = From->LinkedTo; From->BreakAllPinLinks();
	for (auto* P : Links) if (!Link(To, P)) return false;
	return true;
}
UK2Node_CallFunction* Call(UEdGraph* G, FName Name, int32 X, int32 Y)
{
	FGraphNodeCreator<UK2Node_CallFunction> Creator(*G);
	auto* N = Creator.CreateNode(false);
	N->SetFromFunction(UCharacterPoolLibrary::StaticClass()->FindFunctionByName(Name));
	N->NodePosX = X; N->NodePosY = Y; Creator.Finalize();
	return N;
}
UK2Node_Self* Self(UEdGraph* G, int32 X, int32 Y)
{
	FGraphNodeCreator<UK2Node_Self> C(*G); auto* N = C.CreateNode(false);
	N->NodePosX = X; N->NodePosY = Y; C.Finalize(); return N;
}
UK2Node_IfThenElse* Branch(UEdGraph* G, int32 X, int32 Y)
{
	FGraphNodeCreator<UK2Node_IfThenElse> C(*G); auto* N = C.CreateNode(false);
	N->NodePosX = X; N->NodePosY = Y; C.Finalize(); return N;
}
bool HasCall(UEdGraph* G, FName Name)
{
	for (UEdGraphNode* N : G->Nodes) if (auto* C = Cast<UK2Node_CallFunction>(N); C && C->GetFunctionName() == Name) return true;
	return false;
}
}

bool UCharacterPoolMigrationLibrary::ConnectShooter(UBlueprint* BP)
{
	using namespace CharacterPoolMigration;
	if (!BP || BP->GetName() != TEXT("BP_ShooterBase") || !BP->SimpleConstructionScript) return false;
	UEdGraph* G = Graph(BP, TEXT("EventGraph")); if (!G) return false;
	if (HasCall(G, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, ShouldApplyPossessionAppearance))) return true;
	UK2Node_Event* Possessed = nullptr;
	for (UEdGraphNode* N : G->Nodes) if (auto* E = Cast<UK2Node_Event>(N); E && E->EventReference.GetMemberName() == TEXT("ReceivePossessed")) Possessed = E;
	if (!Possessed) return false;
	BP->Modify(); G->Modify();
	USCS_Node* PoolNode = BP->SimpleConstructionScript->CreateNode(UCharacterPoolComponent::StaticClass(), TEXT("CharacterPool"));
	BP->SimpleConstructionScript->AddNode(PoolNode);
	auto* B = Branch(G, Possessed->NodePosX + 200, Possessed->NodePosY - 100);
	auto* P = Call(G, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, ShouldApplyPossessionAppearance), Possessed->NodePosX - 40, Possessed->NodePosY - 240);
	auto* S = Self(G, Possessed->NodePosX - 240, Possessed->NodePosY - 240);
	B->NodeComment = TEXT("A participant keeps its selected mesh across respawns and reconnection. Explicit appearance changes remain available.");
	if (!Move(Possessed->FindPin(TEXT("then")), B->GetThenPin()) || !Link(Possessed->FindPin(TEXT("then")), B->GetExecPin())
		|| !Link(S->FindPin(TEXT("self")), P->FindPin(TEXT("Pawn"))) || !Link(P->GetReturnValuePin(), B->GetConditionPin())) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UCharacterPoolMigrationLibrary::ConnectPlayer(UBlueprint* BP)
{
	using namespace CharacterPoolMigration;
	if (!BP || BP->GetName() != TEXT("BP_PlayerShooter")) return false;
	UEdGraph* G = Graph(BP, TEXT("EventGraph")); if (!G) return false;
	UK2Node_MacroInstance* Once = nullptr;
	for (UEdGraphNode* N : G->Nodes)
	{
		if (auto* E = Cast<UK2Node_CustomEvent>(N); E && E->CustomFunctionName == TEXT("ResetPooledPlayerPresentation")) return true;
		if (auto* M = Cast<UK2Node_MacroInstance>(N); M && M->GetMacroGraph() && M->GetMacroGraph()->GetFName() == TEXT("DoOnce")) Once = M;
	}
	if (!Once || !Once->FindPin(TEXT("Reset"))) return false;
	BP->Modify(); G->Modify();
	FGraphNodeCreator<UK2Node_CustomEvent> C(*G);
	auto* Event = C.CreateNode(false); Event->CustomFunctionName = TEXT("ResetPooledPlayerPresentation");
	Event->NodePosX = Once->NodePosX - 280; Event->NodePosY = Once->NodePosY - 180; C.Finalize();
	if (!Link(Event->FindPin(TEXT("then")), Once->FindPin(TEXT("Reset")))) return false;
	UK2Node_Event* Movement = nullptr;
	for (UEdGraphNode* N : G->Nodes)
		if (auto* E = Cast<UK2Node_Event>(N); E && E->EventReference.GetMemberName() == TEXT("K2_OnMovementModeChanged")) Movement = E;
	if (!Movement) return false;
	auto* Predicate = Call(G, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, CanPlayMovementEffects), Movement->NodePosX - 80, Movement->NodePosY - 260);
	auto* S = Self(G, Movement->NodePosX - 280, Movement->NodePosY - 260);
	auto* B = Branch(G, Movement->NodePosX + 200, Movement->NodePosY - 100);
	if (!Move(Movement->FindPin(TEXT("then")), B->GetThenPin()) || !Link(Movement->FindPin(TEXT("then")), B->GetExecPin())
		|| !Link(Predicate->GetReturnValuePin(), B->GetConditionPin()) || !Link(S->FindPin(TEXT("self")), Predicate->FindPin(TEXT("Pawn")))) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UCharacterPoolMigrationLibrary::ConnectRespawnHelper(UBlueprint* BP)
{
	using namespace CharacterPoolMigration;
	if (!BP || BP->GetName() != TEXT("BPC_RespawnHelper")) return false;
	UEdGraph* G = Graph(BP, TEXT("EventGraph")); UEdGraph* Set = Graph(BP, TEXT("Set Pawn"));
	if (!G || !Set) return false;
	if (HasCall(G, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, StartPooledRespawn))) return true;
	UK2Node_CustomEvent* Start = nullptr; UK2Node_AddDelegate* Destroyed = nullptr;
	for (UEdGraphNode* N : G->Nodes) if (auto* E = Cast<UK2Node_CustomEvent>(N); E && E->CustomFunctionName == TEXT("Start Timer")) Start = E;
	for (UEdGraphNode* N : Set->Nodes) if (auto* A = Cast<UK2Node_AddDelegate>(N); A && A->DelegateReference.GetMemberName() == TEXT("OnDestroyed")) Destroyed = A;
	if (!Start || !Destroyed || Destroyed->FindPin(TEXT("self"))->LinkedTo.Num() != 1) return false;
	BP->Modify(); G->Modify(); Set->Modify();
	auto* C = Call(G, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, StartPooledRespawn), Start->NodePosX + 200, Start->NodePosY - 200);
	auto* S = Self(G, Start->NodePosX, Start->NodePosY - 300);
	auto* B = Branch(G, Start->NodePosX + 460, Start->NodePosY - 200);
	B->NodeComment = TEXT("Dedicated pawn owns the server respawn deadline, including while its controller disconnects. Legacy/client UI timers remain below.");
	if (!Move(Start->FindPin(TEXT("then")), B->GetElsePin()) || !Link(Start->FindPin(TEXT("then")), C->GetExecPin())
		|| !Link(C->GetThenPin(), B->GetExecPin()) || !Link(C->GetReturnValuePin(), B->GetConditionPin())
		|| !Link(S->FindPin(TEXT("self")), C->FindPin(TEXT("RespawnHelper")))) return false;
	// A real final Destroy must never recursively request another pawn for a dedicated participant.
	auto* IsPool = Call(Set, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, IsDedicatedPawn), Destroyed->NodePosX - 220, Destroyed->NodePosY - 220);
	auto* Skip = Branch(Set, Destroyed->NodePosX - 220, Destroyed->NodePosY - 80);
	if (!Link(Destroyed->FindPin(TEXT("self"))->LinkedTo[0], IsPool->FindPin(TEXT("Pawn")))
		|| !Move(Destroyed->FindPin(TEXT("execute")), Skip->GetExecPin()) || !Link(IsPool->GetReturnValuePin(), Skip->GetConditionPin())
		|| !Link(Skip->GetElsePin(), Destroyed->FindPin(TEXT("execute")))) return false;
	const auto Following = Destroyed->FindPin(TEXT("then"))->LinkedTo;
	for (auto* Next : Following) if (!Link(Skip->GetThenPin(), Next)) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UCharacterPoolMigrationLibrary::ConnectGameMode(UBlueprint* BP)
{
	using namespace CharacterPoolMigration;
	if (!BP || BP->GetName() != TEXT("BP_QuakeGameMode")) return false;
	UEdGraph* Set = Graph(BP, TEXT("Set Waiting Pawn")); UEdGraph* Get = Graph(BP, TEXT("Get Waiting Pawn"));
	if (!Set || !Get) return false;
	if (HasCall(Set, GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, ReserveWaitingPawn))) return true;
	for (UEdGraph* G : {Set, Get})
	{
		UK2Node_FunctionEntry* Entry = nullptr; UK2Node_FunctionResult* Result = nullptr;
		for (UEdGraphNode* N : G->Nodes)
		{
			if (auto* E = Cast<UK2Node_FunctionEntry>(N)) Entry = E;
			if (auto* R = Cast<UK2Node_FunctionResult>(N); R && R->FindPin(TEXT("Pawn"))) Result = R;
		}
		if (!Entry || (G == Get && !Result)) return false;
		BP->Modify(); G->Modify();
		const auto Nodes = G->Nodes;
		for (UEdGraphNode* N : Nodes) if (N != Entry && (G == Set || N != Result)) FBlueprintEditorUtils::RemoveNode(BP, N, true);
		Entry->BreakAllNodeLinks(); if (Result) Result->BreakAllNodeLinks();
		auto* C = Call(G, G == Set ? GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, ReserveWaitingPawn)
			: GET_FUNCTION_NAME_CHECKED(UCharacterPoolLibrary, GetWaitingPawn), Entry->NodePosX + 300, Entry->NodePosY);
		if (!Link(Entry->FindPin(TEXT("then")), C->GetExecPin())) return false;
		if (G == Set)
		{
			if (!Link(Entry->FindPin(TEXT("Player Controller")), C->FindPin(TEXT("PlayerController")))
				|| !Link(Entry->FindPin(TEXT("Waiting Time")), C->FindPin(TEXT("WaitingTime")))) return false;
		}
		else
		{
			auto* S = Self(G, Entry->NodePosX + 40, Entry->NodePosY + 250);
			if (!Link(Entry->FindPin(TEXT("UID")), C->FindPin(TEXT("UID")))
				|| !Link(S->FindPin(TEXT("self")), C->FindPin(TEXT("WorldContextObject")))
				|| !Link(C->GetThenPin(), Result->FindPin(TEXT("execute"))) || !Link(C->GetReturnValuePin(), Result->FindPin(TEXT("Pawn")))) return false;
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

FString UCharacterPoolMigrationLibrary::ExportGraphs(UBlueprint* BP)
{
	if (!BP) return FString();
	TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
	FString Output;
	for (auto* G : Graphs)
	{
		TSet<UObject*> Nodes; for (UEdGraphNode* N : G->Nodes) Nodes.Add(N);
		FString Text; FEdGraphUtilities::ExportNodesToText(Nodes, Text);
		Output += TEXT("GRAPH ") + G->GetName() + TEXT("\n") + Text + TEXT("\n");
	}
	return Output;
}

FString UCharacterPoolMigrationLibrary::ValidateContracts()
{
	TArray<TSharedPtr<FJsonValue>> Failures;
	int32 Checks = 0;
	auto CheckClass = [&](const TCHAR* Path, std::initializer_list<const TCHAR*> Functions, std::initializer_list<const TCHAR*> Properties)
	{
		UClass* Class = LoadObject<UClass>(nullptr, Path);
		++Checks;
		if (!Class) { Failures.Add(MakeShared<FJsonValueString>(FString(TEXT("Missing class: ")) + Path)); return; }
		for (const TCHAR* Name : Functions)
		{
			++Checks;
			if (!Class->FindFunctionByName(Name)) Failures.Add(MakeShared<FJsonValueString>(Class->GetName() + TEXT(" missing function: ") + Name));
		}
		for (const TCHAR* Name : Properties)
		{
			++Checks;
			if (!Class->FindPropertyByName(Name)) Failures.Add(MakeShared<FJsonValueString>(Class->GetName() + TEXT(" missing property: ") + Name));
		}
	};
	CheckClass(TEXT("/Game/QuakeLike_1_0/Life/BPC_Life.BPC_Life_C"),
		{TEXT("Init Info"), TEXT("OnRep_LifeInfo")}, {TEXT("LifeInfo"), TEXT("LifeData"), TEXT("OnDeath"), TEXT("DamageWeaponData"), TEXT("OverLifeTimerHandle")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/Inventory/BPC_Inventory.BPC_Inventory_C"),
		{TEXT("Clear")}, {TEXT("All_Items"), TEXT("ItemCache"), TEXT("CachedItemCount"), TEXT("IndexInfo"), TEXT("OnSwapItem")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/Spawner/Component/BPC_RespawnHelper.BPC_RespawnHelper_C"),
		{TEXT("Start Timer")}, {TEXT("Controller"), TEXT("Now Pawn"), TEXT("Respawn Time")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/Buff/BPC_Buff.BPC_Buff_C"),
		{TEXT("OnBuffTimerFinished")}, {TEXT("ActiveBuffTags"), TEXT("CurrentBuffItem"), TEXT("BuffMap"), TEXT("TagsToRemove"), TEXT("currentBuffRowName"), TEXT("currentModifierType")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/Skill/BPC_TeleportSkill.BPC_TeleportSkill_C"),
		{TEXT("StopTravelLoopSound"), TEXT("OnRep_skillCooldownEndTime")}, {TEXT("activeSkillProjectile"), TEXT("skillCooldownEndTime"), TEXT("travelLoopAudio"), TEXT("skillData")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/HeldItem/Component/BPC_Zoom.BPC_Zoom_C"),
		{TEXT("StopZoom")}, {TEXT("cameraRef"), TEXT("defaultFov")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/AI/AIC/BP_QuakeAIController2.BP_QuakeAIController2_C"),
		{TEXT("Death"), TEXT("Clear Target Lost Timer"), TEXT("TargetEnemyDeath")},
		{TEXT("STG"), TEXT("LTG"), TEXT("MyCharacter"), TEXT("CurrentTarget"), TEXT("Chase Points"), TEXT("Cached Aggression Score"), TEXT("LTG Setting Timer")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/AI/VirtualInputMovement/BPC_VirtualInputAIMovement.BPC_VirtualInputAIMovement_C"), {},
		{TEXT("Way Route Queue"), TEXT("Path Link Route"), TEXT("Path Points"), TEXT("Path Length"), TEXT("Path Points Index"), TEXT("Has Goal"),
		TEXT("Final Goal"), TEXT("Final Goal Location"), TEXT("Possessed Character"), TEXT("Input Direction"), TEXT("Area Route Result"), TEXT("Start Point"), TEXT("Is Stuck"), TEXT("Input Direction When Stuck")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/Controller/BP_QuakePlayerController.BP_QuakePlayerController_C"),
		{TEXT("OnPawnDeath"), TEXT("ClientShowDeathScreen")}, {TEXT("LastDeathInfo "), TEXT("LastRespawnDuration")});
	CheckClass(TEXT("/Game/QuakeLike_1_0/GameMode/BP_QuakePlayerState.BP_QuakePlayerState_C"), {TEXT("GetCharacterModeling")}, {});
	if (UClass* Health = LoadObject<UClass>(nullptr, TEXT("/Game/QuakeLike_1_0/Life/BPC_Life.BPC_Life_C")))
	{
		const auto* Death = FindFProperty<FMulticastDelegateProperty>(Health, TEXT("OnDeath"));
		const auto* Handler = UCharacterPoolComponent::StaticClass()->FindFunctionByName(TEXT("OnPawnDeath"));
		++Checks;
		if (!Death || !Handler || !Death->SignatureFunction->IsSignatureCompatibleWith(Handler)) Failures.Add(MakeShared<FJsonValueString>(TEXT("Life death dispatcher signature mismatch")));
	}
	UBlueprint* Base = LoadObject<UBlueprint>(nullptr, TEXT("/Game/QuakeLike_1_0/Character/BP_ShooterBase.BP_ShooterBase"));
	bool bPoolReplicates = false, bHealthReplicates = false;
	if (Base && Base->SimpleConstructionScript)
		for (USCS_Node* Node : Base->SimpleConstructionScript->GetAllNodes())
		{
			if (Node->ComponentClass == UCharacterPoolComponent::StaticClass()) bPoolReplicates = Node->ComponentTemplate->GetIsReplicated();
			if (Node->ComponentClass->GetName() == TEXT("BPC_Life_C")) bHealthReplicates = Node->ComponentTemplate->GetIsReplicated();
		}
	Checks += 2;
	if (!bPoolReplicates) Failures.Add(MakeShared<FJsonValueString>(TEXT("Shooter pool component is missing or not replicated")));
	if (!bHealthReplicates) Failures.Add(MakeShared<FJsonValueString>(TEXT("Shooter Life component is not replicated")));
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	Report->SetBoolField(TEXT("success"), Failures.IsEmpty()); Report->SetNumberField(TEXT("checks"), Checks); Report->SetArrayField(TEXT("failures"), Failures);
	FString Output; FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Output)); return Output;
}

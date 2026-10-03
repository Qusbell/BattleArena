#include "ItemSoundMigrationLibrary.h"

#include "DataTableUtils.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_GetDataTableRow.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/StructureEditorUtils.h"
#include "Sound/SoundBase.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

namespace
{
FEdGraphPinType FloatType()
{
	FEdGraphPinType Type;
	Type.PinCategory = UEdGraphSchema_K2::PC_Real;
	Type.PinSubCategory = UEdGraphSchema_K2::PC_Float;
	return Type;
}

UEdGraph* Graph(UBlueprint* BP, FName Name)
{
	TArray<UEdGraph*> Graphs;
	BP->GetAllGraphs(Graphs); // Includes BPI implementations, unlike FunctionGraphs alone.
	for (UEdGraph* G : Graphs) if (G->GetFName() == Name) return G;
	return nullptr;
}

template<typename T> T* First(UEdGraph* G)
{
	for (UEdGraphNode* N : G->Nodes) if (T* Typed = Cast<T>(N)) return Typed;
	return nullptr;
}

UEdGraphPin* Pin(UEdGraph* G, const TCHAR* Node, const TCHAR* Name)
{
	for (UEdGraphNode* N : G->Nodes) if (N->GetFName() == Node) return N->FindPin(Name);
	return nullptr;
}

bool Connect(UEdGraphPin* A, UEdGraphPin* B)
{
	if (!A || !B) return false;
	return A->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(A, B);
}

template<typename T> T* NewNode(UEdGraph* G, int32 X, int32 Y)
{
	T* N = NewObject<T>(G, NAME_None, RF_Transactional);
	G->AddNode(N, false, false);
	N->CreateNewGuid();
	N->NodePosX = X; N->NodePosY = Y;
	return N;
}

void Allocate(UK2Node* N)
{
	N->PostPlacedNewNode(); N->AllocateDefaultPins();
}

UEdGraphPin* FieldPin(UK2Node_BreakStruct* N, const TCHAR* Field)
{
	FProperty* P = FStructureEditorUtils::GetPropertyByFriendlyName(Cast<UUserDefinedStruct>(N->StructType), Field);
	if (!P) return nullptr;
	for (FOptionalPinFromProperty& Option : N->ShowPinForProperties)
		if (Option.PropertyName == P->GetFName()) Option.bShowPin = true;
	N->ReconstructNode();
	return N->FindPin(P->GetFName());
}

bool AddField(UUserDefinedStruct* S, const TCHAR* Name, const FEdGraphPinType& Type,
	const TCHAR* Default, const TCHAR* Tooltip)
{
	if (!FStructureEditorUtils::GetPropertyByFriendlyName(S, Name))
	{
		if (!FStructureEditorUtils::AddVariable(S, Type))
		{
			UE_LOG(LogTemp, Error, TEXT("Item sound: cannot add %s to %s"), Name, *S->GetPathName()); return false;
		}
		const FGuid Guid = FStructureEditorUtils::GetVarDesc(S).Last().VarGuid;
		if (!FStructureEditorUtils::RenameVariable(S, Guid, Name))
		{
			UE_LOG(LogTemp, Error, TEXT("Item sound: cannot name %s"), Name); return false;
		}
		if (FStructureEditorUtils::GetVarDescByGuid(S, Guid)->DefaultValue != Default
			&& !FStructureEditorUtils::ChangeVariableDefaultValue(S, Guid, Default))
		{
			UE_LOG(LogTemp, Error, TEXT("Item sound: cannot default %s to %s"), Name, Default); return false;
		}
	}
	FProperty* P = FStructureEditorUtils::GetPropertyByFriendlyName(S, Name);
	if (!P) { UE_LOG(LogTemp, Error, TEXT("Item sound: cannot find compiled field %s"), Name); return false; }
	const FGuid Guid = FStructureEditorUtils::GetGuidForProperty(P);
	return FStructureEditorUtils::GetVariableTooltip(S, Guid) == Tooltip
		|| FStructureEditorUtils::ChangeVariableTooltip(S, Guid, Tooltip);
}

bool RenameField(UUserDefinedStruct* S, const TCHAR* Old, const TCHAR* Name)
{
	if (FStructureEditorUtils::GetPropertyByFriendlyName(S, Name)) return true;
	return FStructureEditorUtils::RenameVariable(S, Old, Name); // Existing member GUID is preserved.
}
}

bool UItemSoundMigrationLibrary::ConfigureSoundFields(UUserDefinedStruct* S, const FString& Kind)
{
	if (!S || !S->GetPathName().StartsWith(TEXT("/Game/QuakeLike_1_0/Spawner/DataTable/ST_"))) return false;
	FEdGraphPinType SoundType;
	SoundType.PinCategory = UEdGraphSchema_K2::PC_Object;
	SoundType.PinSubCategoryObject = USoundBase::StaticClass();
	if (Kind == TEXT("HeldItem"))
	{
		if (!RenameField(S, TEXT("sound"), TEXT("SpawnSound"))) return false;
	}
	else if (Kind == TEXT("Buff"))
	{
		if (!RenameField(S, TEXT("Sound"), TEXT("PickupSound"))
			|| !RenameField(S, TEXT("soundVolume"), TEXT("PickupVolume"))) return false;
	}
	else if (Kind == TEXT("AmmoItem"))
	{
		if (!RenameField(S, TEXT("sound"), TEXT("PickupSound"))) return false;
	}
	else if (Kind != TEXT("LifeItem")) return false;
	// Friendly-name matching is case insensitive; existing Life/Ammo pickup fields remain compatible.
	return AddField(S, TEXT("SpawnSound"), SoundType, TEXT("None"),
		TEXT("생성 사전 피드백에 사용하는 위치 사운드. 최초 생성과 재생성에 적용하며, None이면 재생하지 않습니다."))
		&& AddField(S, TEXT("SpawnVolume"), FloatType(), TEXT("1.0"), TEXT("생성음 볼륨 배율. 1은 원래 볼륨, 0은 음소거입니다."))
		&& AddField(S, TEXT("PickupSound"), SoundType, TEXT("None"), TEXT("획득 성공 시 먹은 플레이어에게만 한 번 재생합니다. 생성음과 별개입니다."))
		&& AddField(S, TEXT("PickupVolume"), FloatType(), TEXT("1.0"), TEXT("획득음 볼륨 배율. 1은 원래 볼륨, 0은 음소거입니다."));
}

bool UItemSoundMigrationLibrary::ExtendSpawnFeedback(UBlueprint* BP)
{
	if (!BP || BP->GetName() != TEXT("BPI_ItemSpawner")) return false;
	UEdGraph* G = Graph(BP, TEXT("GetSpawnFeedback"));
	if (!G) return false;
	UK2Node_FunctionResult* Result = First<UK2Node_FunctionResult>(G);
	if (!Result) return false;
	if (!Result->FindPin(TEXT("Spawn Volume"))) Result->CreateUserDefinedPin(TEXT("Spawn Volume"), FloatType(), EGPD_Input);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UItemSoundMigrationLibrary::WireSpawnerSound(UBlueprint* BP, UDataTable* Table)
{
	if (!BP || !Table || !BP->GetPathName().StartsWith(TEXT("/Game/QuakeLike_1_0/Spawner/Component/BPC_"))) return false;
	FBlueprintEditorUtils::RefreshAllNodes(BP);
	UEdGraph* G = Graph(BP, TEXT("GetSpawnFeedback"));
	if (!G) return false;
	auto* Entry = First<UK2Node_FunctionEntry>(G);
	auto* Result = First<UK2Node_FunctionResult>(G);
	auto* Row = First<UK2Node_GetDataTableRow>(G);
	if (!Entry || !Result) return false;
	if (!Row)
	{
		Row = NewNode<UK2Node_GetDataTableRow>(G, 304, 0); Allocate(Row);
		G->GetSchema()->TrySetDefaultObject(*Row->FindPin(TEXT("DataTable")), Table);
		Entry->GetThenPin()->BreakAllPinLinks();
		if (!Connect(Entry->GetThenPin(), Row->GetExecPin())
			|| !Connect(Entry->FindPin(TEXT("Row Name")), Row->FindPin(TEXT("RowName")))
			|| !Connect(Row->GetThenPin(), Result->GetExecPin())) return false;
	}
	UK2Node_BreakStruct* Break = nullptr;
	for (UEdGraphNode* N : G->Nodes)
		if (auto* Candidate = Cast<UK2Node_BreakStruct>(N); Candidate && Candidate->StructType == Table->GetRowStruct()) Break = Candidate;
	if (!Break)
	{
		Break = NewNode<UK2Node_BreakStruct>(G, 592, 128);
		Break->StructType = const_cast<UScriptStruct*>(Table->GetRowStruct()); Allocate(Break);
		if (!Connect(Row->FindPin(TEXT("ReturnValue")), Break->FindPin(Break->StructType->GetFName()))) return false;
	}
	// Expose both before wiring: reconstructing after a connection would invalidate cached pins.
	FieldPin(Break, TEXT("SpawnSound")); FieldPin(Break, TEXT("SpawnVolume"));
	if (!Result->FindPin(TEXT("Spawn Volume"))) Result->CreateUserDefinedPin(TEXT("Spawn Volume"), FloatType(), EGPD_Input);
	Result->FindPin(TEXT("Spawn Sound"))->BreakAllPinLinks();
	const FProperty* Sound = FStructureEditorUtils::GetPropertyByFriendlyName(Cast<UUserDefinedStruct>(Break->StructType), TEXT("SpawnSound"));
	const FProperty* Volume = FStructureEditorUtils::GetPropertyByFriendlyName(Cast<UUserDefinedStruct>(Break->StructType), TEXT("SpawnVolume"));
	const bool OK = Sound && Volume
		&& Connect(Break->FindPin(Sound->GetFName()), Result->FindPin(TEXT("Spawn Sound")))
		&& Connect(Break->FindPin(Volume->GetFName()), Result->FindPin(TEXT("Spawn Volume")));
	Break->NodeComment = TEXT("이번 CurrentSpawnItem 행의 생성음/볼륨. 획득음과 독립적으로 설정합니다.");
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return OK;
}

bool UItemSoundMigrationLibrary::WirePickupSound(UBlueprint* BP, UDataTable* Table)
{
	if (!BP || !Table || !BP->GetPathName().StartsWith(TEXT("/Game/QuakeLike_1_0/DropItem/BP_"))) return false;
	FBlueprintEditorUtils::RefreshAllNodes(BP);
	UEdGraph* G = Graph(BP, TEXT("GetPickSound"));
	if (!G)
	{
		UFunction* ParentFunction = BP->ParentClass->FindFunctionByName(TEXT("GetPickSound"));
		if (!ParentFunction) return false;
		G = FBlueprintEditorUtils::CreateNewGraph(BP, TEXT("GetPickSound"), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
		// Class signature creates a real override. The UFunction overload creates a new
		// user-signature graph and can duplicate inherited result pins during reconstruction.
		FBlueprintEditorUtils::AddFunctionGraph(BP, G, false, BP->ParentClass.Get());
		auto* Handle = NewNode<UK2Node_VariableGet>(G, 0, 160);
		Handle->VariableReference.SetSelfMember(TEXT("DataTable Handle")); Allocate(Handle);
		auto* Split = NewNode<UK2Node_BreakStruct>(G, 240, 160);
		Split->StructType = FDataTableRowHandle::StaticStruct(); Allocate(Split);
		auto* Row = NewNode<UK2Node_GetDataTableRow>(G, 528, 0); Allocate(Row);
		G->GetSchema()->TrySetDefaultObject(*Row->FindPin(TEXT("DataTable")), Table);
		auto* Break = NewNode<UK2Node_BreakStruct>(G, 800, 160);
		Break->StructType = const_cast<UScriptStruct*>(Table->GetRowStruct()); Allocate(Break);
		auto* Entry = First<UK2Node_FunctionEntry>(G);
		auto* Result = First<UK2Node_FunctionResult>(G);
		if (!Entry || !Result) return false;
		Result->NodePosX = 1120;
		Entry->GetThenPin()->BreakAllPinLinks();
		Result->GetExecPin()->BreakAllPinLinks();
		if (!Connect(Handle->GetValuePin(), Split->FindPin(TEXT("DataTableRowHandle")))
			|| !Connect(Split->FindPin(TEXT("DataTable")), Row->FindPin(TEXT("DataTable")))
			|| !Connect(Split->FindPin(TEXT("RowName")), Row->FindPin(TEXT("RowName")))
			|| !Connect(Entry->GetThenPin(), Row->GetExecPin())
			|| !Connect(Row->FindPin(TEXT("ReturnValue")), Break->FindPin(Break->StructType->GetFName()))
			|| !Connect(Row->GetThenPin(), Result->GetExecPin())) return false;
		// Failed lookup deliberately falls through with null/zero outputs, like the existing item getters.
	}
	// The override wizard inserts a parent call with null outputs. This getter fully
	// implements the DT lookup, so discard that default call and its result links.
	TArray<UEdGraphNode*> ParentCalls;
	for (UEdGraphNode* N : G->Nodes) if (N->IsA<UK2Node_CallParentFunction>()) ParentCalls.Add(N);
	for (UEdGraphNode* N : ParentCalls) FBlueprintEditorUtils::RemoveNode(BP, N, false);
	UK2Node_BreakStruct* Break = nullptr;
	for (UEdGraphNode* N : G->Nodes)
		if (auto* Candidate = Cast<UK2Node_BreakStruct>(N); Candidate && Candidate->StructType == Table->GetRowStruct()) Break = Candidate;
	if (!Break) return false;
	FieldPin(Break, TEXT("PickupSound")); FieldPin(Break, TEXT("PickupVolume"));
	const FProperty* Sound = FStructureEditorUtils::GetPropertyByFriendlyName(Cast<UUserDefinedStruct>(Break->StructType), TEXT("PickupSound"));
	const FProperty* Volume = FStructureEditorUtils::GetPropertyByFriendlyName(Cast<UUserDefinedStruct>(Break->StructType), TEXT("PickupVolume"));
	if (!Sound || !Volume) return false;
	for (UEdGraphNode* N : G->Nodes)
	{
		auto* Result = Cast<UK2Node_FunctionResult>(N);
		if (!Result || Result->GetExecPin()->LinkedTo.IsEmpty()) continue;
		const UK2Node_GetDataTableRow* Row = Cast<UK2Node_GetDataTableRow>(Result->GetExecPin()->LinkedTo[0]->GetOwningNode());
		if (!Row || Result->GetExecPin()->LinkedTo[0]->PinName != TEXT("then")) continue;
		UEdGraphPin* SoundOutput = Result->FindPin(TEXT("sound"));
		UEdGraphPin* VolumeOutput = Result->FindPin(TEXT("pickupVolume"));
		if (!SoundOutput || !VolumeOutput) return false;
		SoundOutput->BreakAllPinLinks(); VolumeOutput->BreakAllPinLinks();
		if (!Connect(Break->FindPin(Sound->GetFName()), SoundOutput)
			|| !Connect(Break->FindPin(Volume->GetFName()), VolumeOutput)) return false;
	}
	Break->NodeComment = TEXT("획득 성공 후 BP_DropItem이 이 행의 PickupSound/PickupVolume을 소유 클라이언트에게 전달합니다.");
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UItemSoundMigrationLibrary::WireSpawnVolume(UBlueprint* BP)
{
	if (!BP || BP->GetName() != TEXT("BP_ItemSpawnPoint")) return false;
	UEdGraph* G = Graph(BP, TEXT("EventGraph"));
	if (!G) return false;
	UK2Node_CustomEvent* Event = nullptr;
	for (UEdGraphNode* N : G->Nodes)
		if (auto* E = Cast<UK2Node_CustomEvent>(N); E && E->CustomFunctionName == TEXT("Multicast_PlaySpawnFeedback")) Event = E;
	if (!Event) return false;
	if (!Event->FindPin(TEXT("SpawnVolume"))) Event->CreateUserDefinedPin(TEXT("SpawnVolume"), FloatType(), EGPD_Output);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FBlueprintEditorUtils::RefreshAllNodes(BP);
	const bool OK = Connect(Pin(G, TEXT("K2Node_Message_0"), TEXT("Spawn Volume")), Pin(G, TEXT("K2Node_CallFunction_8"), TEXT("SpawnVolume")))
		&& Connect(Event->FindPin(TEXT("SpawnVolume")), Pin(G, TEXT("K2Node_CallFunction_4"), TEXT("VolumeMultiplier")));
	Event->NodeComment = TEXT("서버가 선택한 생성음과 볼륨을 전달합니다. 기존 위치/감쇠, FX, 사전 피드백 타이밍은 유지합니다.");
	return OK;
}

bool UItemSoundMigrationLibrary::SeparateWeaponPickupSound(UBlueprint* Held, UBlueprint* Drop)
{
	if (!Held || !Drop || Held->GetName() != TEXT("BP_HeldItem") || Drop->GetName() != TEXT("BP_HoldableItem")) return false;
	const FName Flag(TEXT("bSuppressEquipSoundForPickup"));
	FEdGraphPinType Type; Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	if (!FBlueprintEditorUtils::AddMemberVariable(Held, Flag, Type, TEXT("false"))) return false;
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Held, Flag, nullptr, TEXT("ToolTip"),
		TEXT("서버의 맵 아이템 획득 처리 동안만 장착음을 생략합니다. 획득음은 DT를 사용하며 초기 지급/이후 무기 교체 장착음은 유지합니다."));
	FBlueprintEditorUtils::SetBlueprintVariableCategory(Held, Flag, nullptr, FText::FromString(TEXT("Audio|Runtime")));
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Held, Flag, nullptr, TEXT("Private"), TEXT("false"));
	if (uint64* Flags = FBlueprintEditorUtils::GetBlueprintVariablePropertyFlags(Held, Flag)) *Flags |= CPF_DisableEditOnInstance;
	if (!RefreshAndCompile(Held)) return false;
	UEdGraph* HG = Graph(Held, TEXT("ChangeAnim"));
	UEdGraph* DG = Graph(Drop, TEXT("ApplyItemEffect"));
	if (!HG || !DG) return false;
	auto* Get = NewNode<UK2Node_VariableGet>(HG, 1952, 2048);
	Get->VariableReference.SetSelfMember(Flag); Allocate(Get);
	auto* Branch = NewNode<UK2Node_IfThenElse>(HG, 2208, 1920); Allocate(Branch);
	UEdGraphPin* Authority = Pin(HG, TEXT("K2Node_IfThenElse_0"), TEXT("then"));
	UEdGraphPin* Original = Pin(HG, TEXT("K2Node_DynamicCast_0"), TEXT("execute"));
	if (!Authority || !Original) return false;
	Authority->BreakAllPinLinks();
	if (!Connect(Authority, Branch->GetExecPin()) || !Connect(Get->GetValuePin(), Branch->GetConditionPin())
		|| !Connect(Branch->GetElsePin(), Original)) return false;
	Branch->NodeComment = TEXT("맵 획득 중의 자동 장착음만 생략. 실제 획득음은 BP_DropItem의 성공 경로에서 1회 재생.");
	auto Setter = [&](int32 X, bool Value)
	{
		auto* Set = NewNode<UK2Node_VariableSet>(DG, X, -128);
		Set->VariableReference.SetExternalMember(Flag, Held->GeneratedClass); Allocate(Set);
		DG->GetSchema()->TrySetDefaultValue(*Set->FindPin(Flag), Value ? TEXT("true") : TEXT("false"));
		return Set;
	};
	auto* Before = Setter(2304, true);
	auto* After = Setter(2816, false);
	UEdGraphPin* PickupExec = Pin(DG, TEXT("K2Node_Message_0"), TEXT("execute"));
	UEdGraphPin* PickupThen = Pin(DG, TEXT("K2Node_Message_0"), TEXT("then"));
	UEdGraphPin* Previous = Pin(DG, TEXT("K2Node_CallFunction_2"), TEXT("then"));
	UEdGraphPin* Next = Pin(DG, TEXT("K2Node_FunctionResult_1"), TEXT("execute"));
	UEdGraphPin* Weapon = Pin(DG, TEXT("K2Node_SpawnActorFromClass_0"), TEXT("ReturnValue"));
	if (!PickupExec || !PickupThen || !Previous || !Next || !Weapon) return false;
	Previous->BreakAllPinLinks(); PickupThen->BreakAllPinLinks();
	const bool OK = Connect(Previous, Before->GetExecPin()) && Connect(Before->GetThenPin(), PickupExec)
		&& Connect(PickupThen, After->GetExecPin()) && Connect(After->GetThenPin(), Next)
		&& Connect(Weapon, Before->FindPin(TEXT("self"))) && Connect(Weapon, After->FindPin(TEXT("self")));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Held);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Drop);
	return OK;
}

bool UItemSoundMigrationLibrary::RefreshAndCompile(UBlueprint* BP)
{
	if (!BP) return false;
	FBlueprintEditorUtils::RefreshAllNodes(BP);
	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::SkipSave, &Results);
	return Results.NumErrors == 0 && BP->Status != BS_Error;
}

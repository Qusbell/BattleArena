#include "ProjectileMuzzleFlashMigrationLibrary.h"

#include "Components/SceneComponent.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Weapon/ProjectileMuzzleFlashLibrary.h"

namespace
{
UEdGraph* EventGraph(UBlueprint* BP)
{
	for (UEdGraph* Graph : BP->UbergraphPages) if (Graph->GetFName() == TEXT("EventGraph")) return Graph;
	return nullptr;
}

UEdGraphPin* Pin(UEdGraph* Graph, const TCHAR* NodeName, const TCHAR* PinName)
{
	for (UEdGraphNode* Node : Graph->Nodes)
		if (Node->GetFName() == NodeName) return Node->FindPin(PinName);
	return nullptr;
}

template<typename T> T* Add(UEdGraph* Graph, int32 X, int32 Y)
{
	T* Node = NewObject<T>(Graph, NAME_None, RF_Transactional);
	Graph->AddNode(Node, false, false);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->NodePosX = X;
	Node->NodePosY = Y;
	return Node;
}

UK2Node_CallFunction* Call(UEdGraph* Graph, FName Function, int32 X, int32 Y)
{
	auto* Node = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional);
	Node->SetFromFunction(UProjectileMuzzleFlashLibrary::StaticClass()->FindFunctionByName(Function));
	Graph->AddNode(Node, false, false);
	Node->CreateNewGuid();
	Node->PostPlacedNewNode();
	Node->AllocateDefaultPins();
	Node->NodePosX = X;
	Node->NodePosY = Y;
	return Node;
}
}

bool UProjectileMuzzleFlashMigrationLibrary::AddRocketProjectileMuzzleFlash(UBlueprint* Base, UBlueprint* Rocket)
{
	const FString Root(TEXT("/Game/QuakeLike_1_0/HeldItem/Gun/"));
	if (!Base || !Rocket || Base->GetPathName() != Root + TEXT("BP_WeaponBase.BP_WeaponBase")
		|| Rocket->GetPathName() != Root + TEXT("BP_Weapon_RocketLauncher.BP_Weapon_RocketLauncher")) return false;
	UEdGraph* BG = EventGraph(Base);
	UEdGraph* RG = EventGraph(Rocket);
	if (!BG || !RG) return false;
	for (UEdGraph* Graph : {BG, RG}) for (UEdGraphNode* Node : Graph->Nodes)
		if (auto* Fn = Cast<UK2Node_CallFunction>(Node))
			if (Fn->GetFunctionName() == TEXT("ShouldUseProjectileMuzzleFlash")
				|| Fn->GetFunctionName() == TEXT("SpawnProjectileMuzzleFlash")) return false;
	UEdGraphPin* FlashExec = Pin(BG, TEXT("K2Node_ExecutionSequence_2"), TEXT("then_0"));
	UEdGraphPin* OriginalFlash = Pin(BG, TEXT("K2Node_MacroInstance_7"), TEXT("exec"));
	UEdGraphPin* ConfigThen = Pin(RG, TEXT("K2Node_CallFunction_9"), TEXT("then"));
	UEdGraphPin* OriginalNext = Pin(RG, TEXT("K2Node_MacroInstance_1"), TEXT("exec"));
	UEdGraphPin* Mesh = Pin(RG, TEXT("K2Node_CallFunction_11"), TEXT("mesh"));
	UEdGraphPin* Projectile = Pin(RG, TEXT("K2Node_SpawnActorFromClass_0"), TEXT("ReturnValue"));
	UEdGraphPin* Start = Pin(RG, TEXT("K2Node_CallFunction_0"), TEXT("ReturnValue"));
	UEdGraphPin* Target = Pin(RG, TEXT("K2Node_CallFunction_6"), TEXT("ReturnValue"));
	if (!FlashExec || !OriginalFlash || !ConfigThen || !OriginalNext || !Mesh || !Projectile || !Start || !Target
		|| FlashExec->LinkedTo.Num() != 1 || FlashExec->LinkedTo[0] != OriginalFlash
		|| OriginalFlash->LinkedTo.Num() != 1 || ConfigThen->LinkedTo.Num() != 1
		|| ConfigThen->LinkedTo[0] != OriginalNext || OriginalNext->LinkedTo.Num() != 1) return false;
	FEdGraphPinType BoolType;
	BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	const FName Flag(TEXT("bAlignProjectileMuzzleFlash"));
	if (!FBlueprintEditorUtils::AddMemberVariable(Rocket, Flag, BoolType, TEXT("true"))) return false;
	FBlueprintEditorUtils::SetBlueprintVariableCategory(Rocket, Flag, nullptr, FText::FromString(TEXT("Weapon|Projectile")));
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Rocket, Flag, nullptr, TEXT("ToolTip"),
		TEXT("Remote TP muzzle flash uses this shot's initial projectile direction. Disable to restore socket rotation. FP and legacy firing remain unchanged."));
	Base->Modify(); Rocket->Modify(); BG->Modify(); RG->Modify();
	FlashExec->GetOwningNode()->Modify(); OriginalFlash->GetOwningNode()->Modify();
	ConfigThen->GetOwningNode()->Modify(); OriginalNext->GetOwningNode()->Modify();
	auto* Predicate = Call(BG, TEXT("ShouldUseProjectileMuzzleFlash"), 5184, 2688);
	auto* BSelf = Add<UK2Node_Self>(BG, 4960, 2768);
	auto* Branch = Add<UK2Node_IfThenElse>(BG, 5504, 2432);
	Branch->NodeComment = TEXT("Remote opt-in rocket flash is emitted after its projectile direction is initialized. False preserves the original muzzle flash chain; smoke and sound stay on the other sequence outputs.");
	auto* SpawnFlash = Call(RG, TEXT("SpawnProjectileMuzzleFlash"), 1344, -80);
	auto* RSelf = Add<UK2Node_Self>(RG, 1136, 480);
	SpawnFlash->NodeComment = TEXT("Cosmetic TP flash only: velocity after convergence setup, same shot endpoints if spawn failed. Original projectile initialization continues afterward.");
	FlashExec->BreakAllPinLinks(); OriginalFlash->BreakAllPinLinks();
	ConfigThen->BreakAllPinLinks(); OriginalNext->BreakAllPinLinks();
	auto Connect = [](UEdGraphPin* A, UEdGraphPin* B)
	{
		return A && B && A->GetOwningNode()->GetGraph()->GetSchema()->TryCreateConnection(A, B);
	};
	const bool bOK = Connect(BSelf->FindPin(TEXT("self")), Predicate->FindPin(TEXT("Weapon")))
		&& Connect(Predicate->GetReturnValuePin(), Branch->GetConditionPin())
		&& Connect(FlashExec, Branch->GetExecPin()) && Connect(Branch->GetElsePin(), OriginalFlash)
		&& Connect(ConfigThen, SpawnFlash->GetExecPin()) && Connect(SpawnFlash->GetThenPin(), OriginalNext)
		&& Connect(RSelf->FindPin(TEXT("self")), SpawnFlash->FindPin(TEXT("Weapon")))
		&& Connect(Mesh, SpawnFlash->FindPin(TEXT("SelectedMesh")))
		&& Connect(Projectile, SpawnFlash->FindPin(TEXT("Projectile")))
		&& Connect(Start, SpawnFlash->FindPin(TEXT("ShotStart")))
		&& Connect(Target, SpawnFlash->FindPin(TEXT("ShotTarget")));
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Base);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Rocket);
	return bOK;
}

bool UProjectileMuzzleFlashMigrationLibrary::VerifyMuzzleFlashDirections()
{
	for (const FVector Direction : {FVector(1,0,0), FVector(0,0,1), FVector(0,0,-1), FVector(0,-1,0), FVector(1,-2,3).GetSafeNormal()})
	{
		const FVector Start(50, -30, 150), Target = Start + Direction * 1000;
		if (!UProjectileMuzzleFlashLibrary::ResolveFlashDirection(Direction * 4800, Start, Start - Direction * 1000).Equals(Direction, 1.e-6)
			|| !UProjectileMuzzleFlashLibrary::ResolveFlashDirection(FVector::ZeroVector, Start, Target).Equals(Direction, 1.e-6)) return false;
		// An unregistered component is enough to verify socket-parent rotation cannot rotate the shot's flash.
		USceneComponent* Parent = NewObject<USceneComponent>();
		USceneComponent* Effect = NewObject<USceneComponent>();
		Parent->SetWorldRotation(FRotator(25, 40, 60));
		Effect->AttachToComponent(Parent, FAttachmentTransformRules::KeepRelativeTransform);
		Effect->SetAbsolute(false, true, false);
		Effect->SetWorldRotation(Direction.Rotation());
		Parent->SetWorldRotation(FRotator(-45, -90, 30));
		if (!Effect->GetForwardVector().Equals(Direction, 1.e-6)) return false;
	}
	return UProjectileMuzzleFlashLibrary::ResolveFlashDirection(FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector).IsZero();
}

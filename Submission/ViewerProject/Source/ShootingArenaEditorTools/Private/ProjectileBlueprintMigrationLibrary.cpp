#include "ProjectileBlueprintMigrationLibrary.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_FunctionResult.h"
#include "Kismet2/BlueprintEditorUtils.h"

bool UProjectileBlueprintMigrationLibrary::AddMuzzleAimSwitch(UBlueprint* Blueprint)
{
	if (!Blueprint) return false;
	FEdGraphPinType Type;
	Type.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	const FName Name(TEXT("bUseMuzzleProjectileAim"));
	if (!FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, Type, TEXT("true"))) return false;
	FBlueprintEditorUtils::SetBlueprintVariableCategory(Blueprint, Name, nullptr, FText::FromString(TEXT("Weapon|Projectile")));
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, Name, nullptr, TEXT("ToolTip"),
		TEXT("FIX-26-0025: aim from Muzzle at the camera trace hit. Disable to restore the original firing path. Set the same class default on server and clients."));
	return true;
}

bool UProjectileBlueprintMigrationLibrary::ConfigureProjectileMulticast(UBlueprint* Blueprint,
	const FString& NodeName)
{
	if (!Blueprint) return false;
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_CustomEvent* Event = Cast<UK2Node_CustomEvent>(Node);
			if (!Event || Event->GetName() != NodeName) continue;
			Event->Modify();
			Event->FunctionFlags |= FUNC_Net | FUNC_NetReliable | FUNC_NetMulticast;
			FEdGraphPinType VectorType;
			VectorType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			VectorType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			FEdGraphPinType IntType;
			IntType.PinCategory = UEdGraphSchema_K2::PC_Int;
			FEdGraphPinType BoolType;
			BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			if (!Event->FindPin(TEXT("ShotStart"))) Event->CreateUserDefinedPin(TEXT("ShotStart"), VectorType, EGPD_Output);
			if (!Event->FindPin(TEXT("ShotEnd"))) Event->CreateUserDefinedPin(TEXT("ShotEnd"), VectorType, EGPD_Output);
			if (!Event->FindPin(TEXT("ShotId"))) Event->CreateUserDefinedPin(TEXT("ShotId"), IntType, EGPD_Output);
			if (!Event->FindPin(TEXT("FlightForward"))) Event->CreateUserDefinedPin(TEXT("FlightForward"), VectorType, EGPD_Output);
			if (!Event->FindPin(TEXT("bConverge"))) Event->CreateUserDefinedPin(TEXT("bConverge"), BoolType, EGPD_Output);
			Event->NodeComment = TEXT("FIX-26-0025: server-resolved muzzle origin, camera hit target and projectile ID.");
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			return true;
		}
	}
	return false;
}

bool UProjectileBlueprintMigrationLibrary::ExtendProjectileAimOutputs(UBlueprint* Blueprint)
{
	if (!Blueprint) return false;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetFName() != TEXT("GetProjectileAim")) continue;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_FunctionResult* Result = Cast<UK2Node_FunctionResult>(Node);
			if (!Result) continue;
			Result->Modify();
			FEdGraphPinType VectorType;
			VectorType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			VectorType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
			FEdGraphPinType BoolType;
			BoolType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			if (!Result->FindPin(TEXT("FlightForward"))) Result->CreateUserDefinedPin(TEXT("FlightForward"), VectorType, EGPD_Input);
			if (!Result->FindPin(TEXT("bConverge"))) Result->CreateUserDefinedPin(TEXT("bConverge"), BoolType, EGPD_Input);
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		return true;
	}
	return false;
}

void UProjectileBlueprintMigrationLibrary::RefreshProjectileNodes(UBlueprint* Blueprint)
{
	if (Blueprint) FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
}

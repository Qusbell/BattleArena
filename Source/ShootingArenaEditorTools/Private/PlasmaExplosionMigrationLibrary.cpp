#include "PlasmaExplosionMigrationLibrary.h"

#include "EdGraphUtilities.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionTerminator.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_EditablePinBase.h"
#include "Kismet/KismetMathLibrary.h"
#include "K2Node_Variable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "UObject/UnrealType.h"

bool UPlasmaExplosionMigrationLibrary::CopyRocketExplosionFunctions(UBlueprint* Rocket, UBlueprint* Plasma)
{
	if (!Rocket || !Plasma || Rocket == Plasma || !Rocket->GeneratedClass || !Plasma->GeneratedClass) return false;
	// Refuse collisions before making changes, including previously applied migrations.
	for (const FBPVariableDescription& Variable : Rocket->NewVariables)
		if (FBlueprintEditorUtils::FindNewVariableIndex(Plasma, Variable.VarName) != INDEX_NONE) return false;
	for (UEdGraph* Graph : Rocket->FunctionGraphs)
	{
		if (Graph->GetFName() == UEdGraphSchema_K2::FN_UserConstructionScript) continue;
		for (UEdGraph* Existing : Plasma->FunctionGraphs)
			if (Existing->GetFName() == Graph->GetFName()) return false;
	}
	Plasma->Modify();
	TSet<FName> Functions;
	for (UEdGraph* Graph : Rocket->FunctionGraphs)
		if (Graph->GetFName() != UEdGraphSchema_K2::FN_UserConstructionScript) Functions.Add(Graph->GetFName());
	for (const FBPVariableDescription& Variable : Rocket->NewVariables)
	{
		FString Default = Variable.DefaultValue;
		if (const FProperty* Property = Rocket->GeneratedClass->FindPropertyByName(Variable.VarName))
		{
			Default.Reset();
			Property->ExportText_InContainer(0, Default, Rocket->GeneratedClass->GetDefaultObject(), nullptr,
				Rocket->GeneratedClass->GetDefaultObject(), PPF_None);
		}
		if (!FBlueprintEditorUtils::AddMemberVariable(Plasma, Variable.VarName, Variable.VarType, Default)) return false;
		FBlueprintEditorUtils::SetBlueprintVariableCategory(Plasma, Variable.VarName, nullptr,
			FText::FromString(TEXT("Projectile|Explosion")));
	}
	FEdGraphPinType Bool;
	Bool.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	if (!FBlueprintEditorUtils::AddMemberVariable(Plasma, TEXT("bUseExplosionDamage"), Bool, TEXT("true"))) return false;
	FBlueprintEditorUtils::SetBlueprintVariableCategory(Plasma, TEXT("bUseExplosionDamage"), nullptr,
		FText::FromString(TEXT("Projectile|Explosion")));
	FBlueprintEditorUtils::SetBlueprintVariableMetaData(Plasma, TEXT("bUseExplosionDamage"), nullptr, TEXT("ToolTip"),
		TEXT("Use the rocket-style impact/explosion path. Disable to restore the original direct-hit plasma path."));
	for (UEdGraph* Original : Rocket->FunctionGraphs)
	{
		if (Original->GetFName() == UEdGraphSchema_K2::FN_UserConstructionScript) continue;
		UEdGraph* Copy = FEdGraphUtilities::CloneGraph(Original, Plasma);
		if (!Copy) return false;
		Copy->Rename(*Original->GetName(), Plasma, REN_DontCreateRedirectors | REN_DoNotDirty);
		Copy->GraphGuid = FGuid::NewGuid();
		Plasma->FunctionGraphs.Add(Copy);
		for (UEdGraphNode* Node : Copy->Nodes)
		{
			Node->CreateNewGuid();
			if (UK2Node_FunctionTerminator* Terminator = Cast<UK2Node_FunctionTerminator>(Node))
				Terminator->FunctionReference.SetSelfMember(Copy->GetFName());
			if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
			{
				if (Functions.Contains(Call->FunctionReference.GetMemberName()) &&
					(Call->FunctionReference.IsSelfContext() || Call->FunctionReference.GetMemberParentClass() == Rocket->GeneratedClass
					 || Call->FunctionReference.GetMemberParentClass() == Rocket->SkeletonGeneratedClass))
					Call->FunctionReference.SetSelfMember(Call->FunctionReference.GetMemberName());
			}
			if (UK2Node_Variable* Get = Cast<UK2Node_Variable>(Node))
			{
				const int32 Index = FBlueprintEditorUtils::FindNewVariableIndex(Plasma, Get->VariableReference.GetMemberName());
				if (Index != INDEX_NONE && (Get->VariableReference.IsSelfContext()
					|| Get->VariableReference.GetMemberParentClass() == Rocket->GeneratedClass
					|| Get->VariableReference.GetMemberParentClass() == Rocket->SkeletonGeneratedClass))
					Get->VariableReference.SetSelfMember(Get->VariableReference.GetMemberName(), Plasma->NewVariables[Index].VarGuid);
			}
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin->PinType.PinSubCategoryObject.Get() == Rocket->GeneratedClass.Get()) Pin->PinType.PinSubCategoryObject = Plasma->GeneratedClass.Get();
				if (Pin->PinType.PinSubCategoryObject.Get() == Rocket->SkeletonGeneratedClass.Get()) Pin->PinType.PinSubCategoryObject = Plasma->SkeletonGeneratedClass.Get();
			}
		}
	}
	RestoreRocketExplosionDefaults(Rocket, Plasma);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Plasma);
	return true;
}

bool UPlasmaExplosionMigrationLibrary::RestoreRocketExplosionDefaults(UBlueprint* Rocket, UBlueprint* Plasma)
{
	if (!Rocket || !Plasma) return false;
	for (UEdGraph* Original : Rocket->FunctionGraphs)
	{
		if (Original->GetFName() == UEdGraphSchema_K2::FN_UserConstructionScript) continue;
		UEdGraph* Copy = nullptr;
		for (UEdGraph* Graph : Plasma->FunctionGraphs) if (Graph->GetFName() == Original->GetFName()) Copy = Graph;
		if (!Copy) return false;
		for (UEdGraphNode* SourceNode : Original->Nodes)
		{
			UEdGraphNode* TargetNode = nullptr;
			for (UEdGraphNode* Node : Copy->Nodes) if (Node->GetFName() == SourceNode->GetFName()) TargetNode = Node;
			if (!TargetNode) return false;
			for (UEdGraphPin* SourcePin : SourceNode->Pins)
			{
				if (SourcePin->Direction != EGPD_Input || SourcePin->LinkedTo.Num() != 0) continue;
				UEdGraphPin* TargetPin = TargetNode->FindPin(SourcePin->PinName, EGPD_Input);
				if (!TargetPin) return false;
				TargetPin->DefaultValue = SourcePin->DefaultValue;
				TargetPin->AutogeneratedDefaultValue = SourcePin->AutogeneratedDefaultValue;
				TargetPin->DefaultObject = SourcePin->DefaultObject;
				TargetPin->DefaultTextValue = SourcePin->DefaultTextValue;
				// Function-result user-pin metadata must retain each return node's literal value.
				if (UK2Node_EditablePinBase* Editable = Cast<UK2Node_EditablePinBase>(TargetNode))
					for (const TSharedPtr<FUserPinInfo>& Info : Editable->UserDefinedPins)
						if (Info->PinName == TargetPin->PinName) Info->PinDefaultValue = SourcePin->GetDefaultAsString();
				// UE synchronizes output defaults across return nodes when rebuilding a new function.
				// An explicit constant connection keeps the successful visibility result true after rebuild/reload.
				if (Cast<UK2Node_FunctionResult>(TargetNode) && SourcePin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean
					&& SourcePin->DefaultValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) && TargetPin->LinkedTo.IsEmpty())
				{
					FGraphNodeCreator<UK2Node_CallFunction> Creator(*Copy);
					UK2Node_CallFunction* Constant = Creator.CreateNode(false);
					Constant->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, BooleanOR), UKismetMathLibrary::StaticClass());
					Constant->NodePosX = TargetNode->NodePosX - 256;
					Constant->NodePosY = TargetNode->NodePosY - 160;
					Constant->NodeComment = TEXT("Preserve the successful visibility return across Blueprint signature reconstruction.");
					Creator.Finalize();
					Constant->FindPinChecked(TEXT("A"))->DefaultValue = TEXT("true");
					Constant->FindPinChecked(TEXT("B"))->DefaultValue = TEXT("false");
					const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
					if (!Schema->TryCreateConnection(Constant->FindPinChecked(TEXT("ReturnValue")), TargetPin)) return false;
				}
			}
		}
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(Plasma);
	return true;
}

bool UPlasmaExplosionMigrationLibrary::PrepareExplosionValidationActor(AActor* Projectile, AActor* Weapon, UObject* Config)
{
	if (!Projectile || !Weapon || !Config) return false;
	const FStructProperty* Struct = FindFProperty<FStructProperty>(Projectile->GetClass(), TEXT("wConfig"));
	if (!Struct) return false;
	void* Data = Struct->ContainerPtrToValuePtr<void>(Projectile);
	bool bConfig = false, bWeapon = false;
	for (TFieldIterator<FObjectPropertyBase> It(Struct->Struct); It; ++It)
	{
		FObjectPropertyBase* Property = *It;
		if (Property->GetName().StartsWith(TEXT("config_")) && Config->IsA(Property->PropertyClass))
		{
			Property->SetObjectPropertyValue_InContainer(Data, Config);
			bConfig = true;
		}
		if (Property->GetName().StartsWith(TEXT("weapon_")) && Weapon->IsA(Property->PropertyClass))
		{
			Property->SetObjectPropertyValue_InContainer(Data, Weapon);
			bWeapon = true;
		}
	}
	return bConfig && bWeapon;
}

FVector UPlasmaExplosionMigrationLibrary::GetExplosionValidationLaunchVelocity(ACharacter* Character)
{
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const FStructProperty* Property = Movement
		? FindFProperty<FStructProperty>(Movement->GetClass(), TEXT("PendingLaunchVelocity")) : nullptr;
	return Property ? *Property->ContainerPtrToValuePtr<FVector>(Movement) : FVector::ZeroVector;
}

#include "MCPythonHelper.h"
#include "MCPythonHelperInternal.h"
#include "EdGraphUtilities.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Variable.h"
#include "Kismet2/BlueprintEditorUtils.h"

FString UMCPythonHelper::CopyBlueprintNodes(UBlueprint* Source, const FString& SourceGraphName,
    UBlueprint* Target, const FString& TargetGraphName, const TArray<FString>& NodeNames, int32 OffsetY)
{
    if (!Source || !Target || Source == Target || NodeNames.IsEmpty())
        return MakeJsonError(TEXT("Two different Blueprints and at least one node are required."));

    UEdGraph* SourceGraph = FindGraphByName(Source, SourceGraphName);
    UEdGraph* TargetGraph = FindGraphByName(Target, TargetGraphName);
    if (!SourceGraph || !TargetGraph)
        return MakeJsonError(TEXT("Source or target graph not found."));

    TSet<UObject*> Selection;
    TMap<FGuid, FString> SourceNames;
    for (const FString& Name : NodeNames)
    {
        UEdGraphNode* Node = FindBPNodeByName(SourceGraph, Name);
        if (!Node || !Node->CanDuplicateNode())
            return MakeJsonError(FString::Printf(TEXT("Cannot copy node '%s'."), *Name));
        Selection.Add(Node);
        SourceNames.Add(Node->NodeGuid, Name);
    }

    FString ClipboardText;
    FEdGraphUtilities::ExportNodesToText(Selection, ClipboardText);
    if (!FEdGraphUtilities::CanImportNodesFromText(TargetGraph, ClipboardText))
        return MakeJsonError(TEXT("The target graph does not accept these nodes."));

    Target->Modify();
    TargetGraph->Modify();
    TSet<UEdGraphNode*> Copied;
    FEdGraphUtilities::ImportNodesFromText(TargetGraph, ClipboardText, Copied);
    if (Copied.Num() != Selection.Num())
    {
        for (UEdGraphNode* Node : Copied)
            Node->DestroyNode();
        return MakeJsonError(FString::Printf(TEXT("Imported %d of %d nodes."), Copied.Num(), Selection.Num()));
    }

    TSharedPtr<FJsonObject> Mapping = MakeShared<FJsonObject>();
    for (UEdGraphNode* Node : Copied)
    {
        if (const FString* Original = SourceNames.Find(Node->NodeGuid))
            Mapping->SetStringField(*Original, Node->GetName());
        Node->CreateNewGuid();
        Node->NodePosY += OffsetY;

        if (UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node))
        {
            if (Variable->VariableReference.IsSelfContext())
            {
                const FName Name = Variable->VariableReference.GetMemberName();
                Variable->VariableReference.SetSelfMember(Name, FBlueprintEditorUtils::FindMemberVariableGuidByName(Target, Name));
            }
        }
        if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node))
        {
            if (Call->FunctionReference.GetMemberName() == FName(*TargetGraphName)
                || FindGraphByName(Target, Call->FunctionReference.GetMemberName().ToString()))
                Call->FunctionReference.SetSelfMember(Call->FunctionReference.GetMemberName());
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsModified(Target);
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("count"), Copied.Num());
    Result->SetObjectField(TEXT("nodes"), Mapping);
    return SerializeJsonObj(Result);
}


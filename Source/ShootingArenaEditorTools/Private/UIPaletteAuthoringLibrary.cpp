#include "UIPaletteAuthoringLibrary.h"

#include "UI/UIPalette.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphNodeUtils.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeStruct.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/DefaultValueHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Styling/SlateColor.h"
#include "UObject/UnrealType.h"

namespace
{
struct FColorLiteral
{
    UEdGraph* Graph = nullptr;
    UEdGraphNode* Node = nullptr;
    UEdGraphPin* Pin = nullptr;
    FLinearColor Color;
    bool bMakeColor = false;
    bool bSlateColor = false;

    FString Key() const
    {
        return Graph->GetName() + TEXT("|") + Node->GetName() + TEXT("|") + Pin->PinName.ToString();
    }
};

FString SerializePaletteJson(const TSharedRef<FJsonObject>& Object)
{
    FString Result;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Result));
    return Result;
}

bool UsablePin(const UEdGraphPin* Pin)
{
    return Pin && !Pin->bHidden && !Pin->bOrphanedPin;
}

bool IsLinearColor(const UEdGraphPin* Pin)
{
    return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
        && Pin->PinType.PinSubCategoryObject == TBaseStructure<FLinearColor>::Get();
}

bool IsSlateColor(const UEdGraphPin* Pin)
{
    return Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct
        && Pin->PinType.PinSubCategoryObject == FSlateColor::StaticStruct();
}

UEdGraphPin* FindPin(UEdGraphNode* Node, FName Name, EEdGraphPinDirection Direction)
{
    for (UEdGraphPin* Pin : Node->Pins)
        if (UsablePin(Pin) && Pin->PinName == Name && Pin->Direction == Direction) return Pin;
    return nullptr;
}

bool ParseColorInput(const UEdGraphPin* Pin, FLinearColor& Color, bool& bSlate)
{
    if (!UsablePin(Pin) || Pin->Direction != EGPD_Input || !Pin->LinkedTo.IsEmpty()
        || !Pin->SubPins.IsEmpty() || Pin->DefaultValue.IsEmpty()) return false;

    if (IsLinearColor(Pin))
    {
        bSlate = false;
        return FDefaultValueHelper::ParseLinearColor(Pin->DefaultValue, Color)
            || TBaseStructure<FLinearColor>::Get()->ImportText(*Pin->DefaultValue, &Color,
                nullptr, PPF_None, nullptr, TEXT("LinearColor")) != nullptr;
    }
    if (IsSlateColor(Pin))
    {
        FSlateColor Value;
        if (!FSlateColor::StaticStruct()->ImportText(*Pin->DefaultValue, &Value,
            nullptr, PPF_None, nullptr, TEXT("SlateColor")) || !Value.IsColorSpecified()) return false;
        Color = Value.GetSpecifiedColor();
        bSlate = true;
        return true;
    }
    return false;
}

bool ParseMakeColor(UEdGraphNode* Node, FColorLiteral& Literal)
{
    const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
    const UK2Node_MakeStruct* Make = Cast<UK2Node_MakeStruct>(Node);
    const FName Function = Call ? Call->FunctionReference.GetMemberName() : NAME_None;
    const bool bColorFunction = Function == TEXT("MakeColor") || Function == TEXT("MakeLinearColor");
    const bool bColorStruct = Make && Make->StructType == TBaseStructure<FLinearColor>::Get();
    if (!bColorFunction && !bColorStruct) return false;

    UEdGraphPin* Output = nullptr;
    for (UEdGraphPin* Pin : Node->Pins)
        if (UsablePin(Pin) && Pin->Direction == EGPD_Output && IsLinearColor(Pin))
        {
            if (Output) return false;
            Output = Pin;
        }
    if (!Output || !Output->SubPins.IsEmpty()) return false;

    float Components[4];
    const FName Names[] = {TEXT("R"), TEXT("G"), TEXT("B"), TEXT("A")};
    for (int32 Index = 0; Index < 4; ++Index)
    {
        UEdGraphPin* Pin = FindPin(Node, Names[Index], EGPD_Input);
        if (!Pin || !Pin->LinkedTo.IsEmpty()
            || !FDefaultValueHelper::ParseFloat(Pin->DefaultValue, Components[Index])) return false;
    }
    Literal.Node = Node;
    Literal.Pin = Output;
    Literal.Color = FLinearColor(Components[0], Components[1], Components[2], Components[3]);
    Literal.bMakeColor = true;
    return true;
}

bool IsSpecifiedSlateMember(UEdGraphNode* Node, const UEdGraphPin* Pin)
{
    const UK2Node_MakeStruct* Make = Cast<UK2Node_MakeStruct>(Node);
    if (!Make || Make->StructType != FSlateColor::StaticStruct()
        || Pin->PinName != TEXT("SpecifiedColor")) return true;
    const UEdGraphPin* Rule = FindPin(Node, TEXT("ColorUseRule"), EGPD_Input);
    // A dynamic rule cannot safely be replaced by a specified palette color.
    return !Rule || (Rule->LinkedTo.IsEmpty()
        && (Rule->DefaultValue.IsEmpty() || Rule->DefaultValue == TEXT("UseColor_Specified")));
}

void CollectColors(UBlueprint* Blueprint, TArray<UEdGraph*>& Graphs, TArray<FColorLiteral>& Literals)
{
    Blueprint->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        if (!Graph) continue;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;
            FColorLiteral Make;
            if (ParseMakeColor(Node, Make))
            {
                Make.Graph = Graph;
                Literals.Add(Make);
                continue;
            }
            for (UEdGraphPin* Pin : Node->Pins)
            {
                FColorLiteral Input;
                if (ParseColorInput(Pin, Input.Color, Input.bSlateColor)
                    && IsSpecifiedSlateMember(Node, Pin))
                {
                    Input.Graph = Graph;
                    Input.Node = Node;
                    Input.Pin = Pin;
                    Literals.Add(Input);
                }
            }
        }
    }
}

UK2Node_CallFunction* MakePaletteCall(const FColorLiteral& Literal, FName Role)
{
    FGraphNodeCreator<UK2Node_CallFunction> Creator(*Literal.Graph);
    UK2Node_CallFunction* Call = Creator.CreateNode(false);
    Call->SetFromFunction(UUIThemePaletteLibrary::StaticClass()->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UUIThemePaletteLibrary, GetColor)));
    Call->NodePosX = Literal.Node->NodePosX - 360;
    Call->NodePosY = Literal.Node->NodePosY;
    Creator.Finalize();
    if (UEdGraphPin* RolePin = Call->FindPin(TEXT("Role")))
        Literal.Graph->GetSchema()->TrySetDefaultValue(*RolePin, Role.ToString());
    return Call;
}

UK2Node_MakeStruct* MakeSlateWrapper(const FColorLiteral& Literal)
{
    FGraphNodeCreator<UK2Node_MakeStruct> Creator(*Literal.Graph);
    UK2Node_MakeStruct* Make = Creator.CreateNode(false);
    Make->StructType = FSlateColor::StaticStruct();
    Make->NodePosX = Literal.Node->NodePosX - 180;
    Make->NodePosY = Literal.Node->NodePosY;
    Creator.Finalize();
    if (UEdGraphPin* Rule = Make->FindPin(TEXT("ColorUseRule")))
        Literal.Graph->GetSchema()->TrySetDefaultValue(*Rule, TEXT("UseColor_Specified"));
    return Make;
}

bool BindOne(UBlueprint* Blueprint, const FColorLiteral& Literal, FName Role, FString& Error)
{
    const UEdGraphSchema* Schema = Literal.Graph->GetSchema();
    if (!Schema || Role.IsNone())
    {
        Error = TEXT("A graph schema and a palette role are required.");
        return false;
    }
    if (!Literal.bMakeColor && !Literal.Pin->LinkedTo.IsEmpty())
    {
        Error = TEXT("The target color input is already connected.");
        return false;
    }

    UK2Node_CallFunction* Call = MakePaletteCall(Literal, Role);
    UEdGraphPin* ColorOutput = Call->GetReturnValuePin();
    UK2Node_MakeStruct* Slate = nullptr;
    if (Literal.bSlateColor)
    {
        Slate = MakeSlateWrapper(Literal);
        UEdGraphPin* Specified = FindPin(Slate, TEXT("SpecifiedColor"), EGPD_Input);
        UEdGraphPin* SlateOutput = nullptr;
        for (UEdGraphPin* Pin : Slate->Pins)
            if (UsablePin(Pin) && Pin->Direction == EGPD_Output && IsSlateColor(Pin)) SlateOutput = Pin;
        if (!ColorOutput || !Specified || !SlateOutput || !Schema->TryCreateConnection(ColorOutput, Specified))
        {
            Error = TEXT("Could not construct the specified Slate color wrapper.");
            FBlueprintEditorUtils::RemoveNode(Blueprint, Slate, true);
            FBlueprintEditorUtils::RemoveNode(Blueprint, Call, true);
            return false;
        }
        ColorOutput = SlateOutput;
    }

    bool bConnected = ColorOutput != nullptr;
    if (bConnected && Literal.bMakeColor)
    {
        const TArray<UEdGraphPin*> OldLinks = Literal.Pin->LinkedTo;
        for (UEdGraphPin* Link : OldLinks)
            if (!UsablePin(Link) || Schema->CanCreateConnection(ColorOutput, Link).Response == CONNECT_RESPONSE_DISALLOW)
                bConnected = false;
        if (bConnected)
        {
            const FPinConnectionResponse Response = Schema->MovePinLinks(*Literal.Pin, *ColorOutput, false, true);
            bConnected = Response.Response != CONNECT_RESPONSE_DISALLOW;
            if (!bConnected)
            {
                ColorOutput->BreakAllPinLinks(true);
                for (UEdGraphPin* Link : OldLinks) Literal.Pin->MakeLinkTo(Link);
            }
        }
        if (bConnected) FBlueprintEditorUtils::RemoveNode(Blueprint, Literal.Node, true);
    }
    else if (bConnected)
        bConnected = Schema->TryCreateConnection(ColorOutput, Literal.Pin);

    if (!bConnected)
    {
        Error = TEXT("Palette color could not connect without changing existing graph wiring.");
        if (Slate) FBlueprintEditorUtils::RemoveNode(Blueprint, Slate, true);
        FBlueprintEditorUtils::RemoveNode(Blueprint, Call, true);
    }
    return bConnected;
}
}

FString UUIPaletteAuthoringLibrary::InspectBlueprintColors(UBlueprint* Blueprint)
{
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), IsValid(Blueprint));
    if (!IsValid(Blueprint))
    {
        Result->SetStringField(TEXT("error"), TEXT("Invalid Blueprint."));
        return SerializePaletteJson(Result);
    }
    TArray<UEdGraph*> Graphs;
    TArray<FColorLiteral> Literals;
    CollectColors(Blueprint, Graphs, Literals);
    TArray<TSharedPtr<FJsonValue>> Entries;
    for (const FColorLiteral& Literal : Literals)
    {
        TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("key"), Literal.Key());
        Entry->SetStringField(TEXT("graph"), Literal.Graph->GetName());
        Entry->SetStringField(TEXT("graph_path"), Literal.Graph->GetPathName());
        Entry->SetStringField(TEXT("node"), Literal.Node->GetName());
        Entry->SetStringField(TEXT("pin"), Literal.Pin->PinName.ToString());
        Entry->SetStringField(TEXT("kind"), Literal.bMakeColor ? TEXT("make_color") : TEXT("input"));
        Entry->SetStringField(TEXT("type"), Literal.bSlateColor ? TEXT("SlateColor") : TEXT("LinearColor"));
        Entry->SetStringField(TEXT("default_value"), Literal.Pin->DefaultValue);
        TArray<TSharedPtr<FJsonValue>> Color;
        for (int32 Index = 0; Index < 4; ++Index) Color.Add(MakeShared<FJsonValueNumber>(Literal.Color.Component(Index)));
        Entry->SetArrayField(TEXT("color"), Color);
        Entries.Add(MakeShared<FJsonValueObject>(Entry));
    }
    Result->SetNumberField(TEXT("graph_count"), Graphs.Num());
    Result->SetNumberField(TEXT("literal_count"), Literals.Num());
    Result->SetArrayField(TEXT("literals"), Entries);
    return SerializePaletteJson(Result);
}

FString UUIPaletteAuthoringLibrary::BindBlueprintColors(UBlueprint* Blueprint, const TMap<FString, FName>& Bindings)
{
    TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
    if (!IsValid(Blueprint))
    {
        Result->SetBoolField(TEXT("success"), false);
        Result->SetStringField(TEXT("error"), TEXT("Invalid Blueprint."));
        return SerializePaletteJson(Result);
    }
    TArray<UEdGraph*> Graphs;
    TArray<FColorLiteral> Literals;
    CollectColors(Blueprint, Graphs, Literals);
    TMap<FString, int32> KeyCounts;
    for (const FColorLiteral& Literal : Literals) ++KeyCounts.FindOrAdd(Literal.Key());

    int32 Bound = 0;
    TSet<FString> Found;
    TArray<TSharedPtr<FJsonValue>> Errors;
    for (const FColorLiteral& Literal : Literals)
    {
        const FString Key = Literal.Key();
        const FName* Role = Bindings.Find(Key);
        if (!Role) continue;
        Found.Add(Key);
        FString Error;
        if (KeyCounts.FindRef(Key) != 1) Error = TEXT("The graph/node/pin key is ambiguous.");
        else if (BindOne(Blueprint, Literal, *Role, Error))
        {
            ++Bound;
            Literal.Graph->NotifyGraphChanged();
            continue;
        }
        TSharedRef<FJsonObject> Failure = MakeShared<FJsonObject>();
        Failure->SetStringField(TEXT("key"), Key);
        Failure->SetStringField(TEXT("error"), Error);
        Errors.Add(MakeShared<FJsonValueObject>(Failure));
    }
    for (const auto& Pair : Bindings)
        if (!Found.Contains(Pair.Key))
        {
            TSharedRef<FJsonObject> Failure = MakeShared<FJsonObject>();
            Failure->SetStringField(TEXT("key"), Pair.Key);
            Failure->SetStringField(TEXT("error"), TEXT("No unconnected literal matches this key; an existing connection is preserved."));
            Errors.Add(MakeShared<FJsonValueObject>(Failure));
        }
    Result->SetBoolField(TEXT("success"), Errors.IsEmpty());
    Result->SetNumberField(TEXT("requested"), Bindings.Num());
    Result->SetNumberField(TEXT("bound"), Bound);
    Result->SetNumberField(TEXT("skipped"), Bindings.Num() - Bound);
    Result->SetArrayField(TEXT("errors"), Errors);
    return SerializePaletteJson(Result);
}

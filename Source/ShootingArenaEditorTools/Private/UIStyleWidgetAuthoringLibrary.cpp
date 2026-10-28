#include "UIStyleWidgetAuthoringLibrary.h"
#include "UI/ReusableStyleHost.h"
#include "Blueprint/WidgetTree.h"
#include "Components/NamedSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"

UWidget* UUIStyleWidgetAuthoringLibrary::CreateWidgetInTree(UBlueprint* Blueprint,
    FName WidgetName, TSubclassOf<UWidget> WidgetClass, FName ParentName)
{
    UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(Blueprint);
    if (!BP || !BP->WidgetTree || !WidgetClass) return nullptr;
    UPanelWidget* Parent = Cast<UPanelWidget>(BP->WidgetTree->FindWidget(ParentName));
    if (!Parent) return nullptr;
    UWidget* Widget = BP->WidgetTree->ConstructWidget<UWidget>(WidgetClass, WidgetName);
    Parent->AddChild(Widget);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return Widget;
}

bool UUIStyleWidgetAuthoringLibrary::CreateStyleHostTemplate(UBlueprint* Blueprint)
{
    UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(Blueprint);
    if (!BP || !BP->WidgetTree || BP->WidgetTree->RootWidget) return false;
    USizeBox* Bounds = BP->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StyleBounds"));
    UNamedSlot* Content = BP->WidgetTree->ConstructWidget<UNamedSlot>(UNamedSlot::StaticClass(), TEXT("Content"));
    Bounds->AddChild(Content);
    BP->WidgetTree->RootWidget = Bounds;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return true;
}

UReusableStyleHost* UUIStyleWidgetAuthoringLibrary::WrapWithStyleHost(UBlueprint* Blueprint,
    FName WidgetName, TSubclassOf<UUserWidget> HostClass, FName HostName)
{
    UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(Blueprint);
    if (!BP || !BP->WidgetTree || !HostClass || !HostClass->IsChildOf(UReusableStyleHost::StaticClass())) return nullptr;
    UWidget* Widget = BP->WidgetTree->FindWidget(WidgetName);
    if (!Widget) return nullptr;
    UPanelWidget* Parent = Widget->GetParent();
    if (!Parent && BP->WidgetTree->RootWidget != Widget) return nullptr;
    BP->WidgetTree->Modify();
    UReusableStyleHost* Host = BP->WidgetTree->ConstructWidget<UReusableStyleHost>(HostClass.Get(), HostName);
    if (Parent) Parent->ReplaceChildAt(Parent->GetChildIndex(Widget), Host);
    else BP->WidgetTree->RootWidget = Host;
    Host->SetContentForSlot(TEXT("Content"), Widget);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return Host;
}

bool UUIStyleWidgetAuthoringLibrary::UpgradeStyleableCombo(UBlueprint* Blueprint, FName WidgetName)
{
    UWidgetBlueprint* BP = Cast<UWidgetBlueprint>(Blueprint);
    UComboBoxString* Old = BP && BP->WidgetTree ? Cast<UComboBoxString>(BP->WidgetTree->FindWidget(WidgetName)) : nullptr;
    if (!Old || Old->IsA<UStyleableComboBoxString>()) return Old != nullptr;
    UPanelWidget* Parent = Old->GetParent();
    if (!Parent) return false;
    const int32 Index = Parent->GetChildIndex(Old);
    UStyleableComboBoxString* New = BP->WidgetTree->ConstructWidget<UStyleableComboBoxString>(
        UStyleableComboBoxString::StaticClass(), MakeUniqueObjectName(BP->WidgetTree, UStyleableComboBoxString::StaticClass()));
    UEngine::CopyPropertiesForUnrelatedObjects(Old, New);
    Old->Rename(*MakeUniqueObjectName(BP->WidgetTree, Old->GetClass(), TEXT("RetiredCombo")).ToString(), BP->WidgetTree, REN_DontCreateRedirectors);
    New->Rename(*WidgetName.ToString(), BP->WidgetTree, REN_DontCreateRedirectors);
    Parent->ReplaceChildAt(Index, New);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return true;
}

bool UUIStyleWidgetAuthoringLibrary::RemoveUnusedStyleVariable(UBlueprint* Blueprint, FName VariableName)
{
    if (!Blueprint || (VariableName != FName(TEXT("ReusableTheme")) && VariableName != FName(TEXT("SettingTheme"))))
        return false;

    const bool bHasVariable = Blueprint->NewVariables.ContainsByPredicate(
        [VariableName](const FBPVariableDescription& Variable) { return Variable.VarName == VariableName; });
    if (!bHasVariable) return true;
    if (FBlueprintEditorUtils::IsVariableUsed(Blueprint, VariableName)) return false;

    FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, VariableName);
    return true;
}

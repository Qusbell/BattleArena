#include "ArenaAnalyzerMigrationLibrary.h"
#include "Analyzer/ArenaCombatLibrary.h"
#include "Analyzer/ArenaAnalyzeControls.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/ComboBoxString.h"
#include "Components/ActorComponent.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_Self.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "WidgetBlueprint.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
const UEdGraphSchema_K2* Schema() { return GetDefault<UEdGraphSchema_K2>(); }
bool Link(UEdGraphPin* A, UEdGraphPin* B) { return A && B && Schema()->TryCreateConnection(A, B); }
bool Move(UEdGraphPin* From, UEdGraphPin* To)
{
    if (!From || !To) return false;
    const auto Links = From->LinkedTo; From->BreakAllPinLinks();
    To->DefaultValue = From->DefaultValue; To->DefaultObject = From->DefaultObject;
    for (auto* Pin : Links) if (!Link(Pin, To)) return false;
    return true;
}
bool Copy(UEdGraphPin* From, UEdGraphPin* To)
{
    if (!From || !To) return false;
    To->DefaultValue = From->DefaultValue; To->DefaultObject = From->DefaultObject;
    for (auto* Pin : From->LinkedTo) if (!Link(Pin, To)) return false;
    return true;
}
UK2Node_CallFunction* Call(UEdGraph* G, FName Name, UClass* Class = nullptr)
{
    FGraphNodeCreator<UK2Node_CallFunction> C(*G); auto* N = C.CreateNode(false);
    N->SetFromFunction((Class ? Class : UArenaCombatLibrary::StaticClass())->FindFunctionByName(Name)); C.Finalize(); return N;
}
UEdGraphPin* Self(UEdGraph* G)
{
    FGraphNodeCreator<UK2Node_Self> C(*G); auto* N = C.CreateNode(false); C.Finalize(); return N->FindPin(TEXT("self"));
}
bool Before(UEdGraphNode* Target, UK2Node_CallFunction* New)
{
    New->NodePosX = Target->NodePosX - 280; New->NodePosY = Target->NodePosY - 160;
    return Move(Target->FindPin(TEXT("execute")), New->GetExecPin()) && Link(New->GetThenPin(), Target->FindPin(TEXT("execute")));
}
bool After(UEdGraphNode* Target, UK2Node_CallFunction* New)
{
    New->NodePosX = Target->NodePosX + 280; New->NodePosY = Target->NodePosY + 160;
    return Move(Target->FindPin(TEXT("then")), New->GetThenPin()) && Link(Target->FindPin(TEXT("then")), New->GetExecPin());
}
bool Configure(UBlueprint* BP, UEdGraph* G, UK2Node_CallFunction* Node)
{
    const FName Name = Node->GetFunctionName();
    if (Name == TEXT("ApplyAnalyzedDamage"))
    {
        if (!Node->FindPin(TEXT("FiringWeapon"))) Node->ReconstructNode();
        auto* Pin = Node->FindPin(TEXT("FiringWeapon"));
        return Pin && (!Pin->LinkedTo.IsEmpty() || Link(Self(G), Pin));
    }
    if ((Name == TEXT("RecordTrace") || Name == TEXT("RecordTraces")) && BP->GetName().StartsWith(TEXT("BP_Weapon_")))
    {
        if (!Node->FindPin(TEXT("AimStart"))) Node->ReconstructNode();
        UK2Node_CallFunction* Aim = nullptr;
        for (UEdGraphNode* N : G->Nodes)
            if (auto* C = Cast<UK2Node_CallFunction>(N); C && C->GetFunctionName() == TEXT("GetHitScanAim")) Aim = C;
        if (!Aim) return false;
        for (const auto& Pair : {TPair<FName,FName>(TEXT("aimStart"),TEXT("AimStart")), TPair<FName,FName>(TEXT("aimEnd"),TEXT("AimEnd"))})
        {
            auto* Pin = Node->FindPin(Pair.Value);
            if (!Pin || (Pin->LinkedTo.IsEmpty() && !Link(Aim->FindPin(Pair.Key), Pin))) return false;
        }
    }
    return true;
}

}

FString UArenaAnalyzerMigrationLibrary::ConnectAnalyzer(UBlueprint* BP)
{
    if (!BP || !BP->GetPathName().StartsWith(TEXT("/Game/QuakeLike_1_0/"))) return TEXT("ERROR: asset outside active root");
    BP->Modify();
    if (BP->GetName() == TEXT("EUW_ArenaAnalyzer"))
    {
        auto* Widget = Cast<UWidgetBlueprint>(BP);
        auto* Root = Widget && Widget->WidgetTree ? Cast<UPanelWidget>(Widget->WidgetTree->FindWidget(TEXT("StackBox_0"))) : nullptr;
        if (!Root) return TEXT("ERROR: missing utility root");
        if (!Widget->WidgetTree->FindWidget(TEXT("AnalyzerScroll")))
        {
            UWidget* Content = Widget->WidgetTree->RootWidget;
            auto* Scroll = Widget->WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AnalyzerScroll"));
            Scroll->SetAlwaysShowScrollbar(true); Scroll->SetAlwaysShowScrollbarTrack(true);
            Scroll->SetScrollbarThickness(FVector2D(12,12)); Scroll->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
            Widget->WidgetTree->RootWidget = Scroll; Scroll->AddChild(Content);
            Widget->OnVariableAdded(TEXT("AnalyzerScroll"));
        }
        UArenaAnalyzeControls::ConfigureCombo(Cast<UComboBoxString>(Widget->WidgetTree->FindWidget(TEXT("Files"))));
        if (!Widget->WidgetTree->FindWidget(TEXT("CombatRecords")))
        {
            auto* Controls = Widget->WidgetTree->ConstructWidget<UArenaAnalyzeControls>(UArenaAnalyzeControls::StaticClass(), TEXT("CombatRecords"));
            Root->AddChild(Controls);
            Widget->OnVariableAdded(TEXT("CombatRecords"));
        }
        if (!Widget->WidgetVariableNameToGuidMap.Contains(TEXT("CombatRecords"))) Widget->OnVariableAdded(TEXT("CombatRecords"));
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        return TEXT("OK: combat controls");
    }
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    int32 Changes = 0;
    for (UEdGraph* G : Graphs)
    {
        // Per-graph marker makes repeated migration safe.
        bool Connected = false;
        for (UEdGraphNode* N : G->Nodes) if (N->NodeComment == TEXT("ArenaAnalyzer.v2")) Connected = true;
        if (Connected)
        {
            const auto Existing = G->Nodes;
            for (UEdGraphNode* N : Existing)
                if (auto* C = Cast<UK2Node_CallFunction>(N); C && !Configure(BP, G, C)) return TEXT("ERROR: upgrade pins");
            continue;
        }
        const auto Nodes = G->Nodes;
        for (UEdGraphNode* N : Nodes)
        {
            auto* C = Cast<UK2Node_CallFunction>(N);
            if (C && BP->GetName() == TEXT("BP_WeaponBase") && C->GetFunctionName() == TEXT("StartFire"))
            {
                auto* Begin = Call(G, TEXT("BeginWeaponShot"));
                FGraphNodeCreator<UK2Node_VariableGet> V(*G); auto* Data = V.CreateNode(false);
                Data->VariableReference.SetSelfMember(TEXT("weaponData")); V.Finalize();
                if (!Link(Self(G), Begin->FindPin(TEXT("Weapon"))) || !Link(Data->GetValuePin(), Begin->FindPin(TEXT("WeaponData"))) || !Before(C, Begin)) return TEXT("ERROR: fire pins");
                Begin->NodeComment = TEXT("ArenaAnalyzer.v2"); ++Changes;
            }
            if (C && BP->GetName() == TEXT("BP_WeaponBase") && C->GetFunctionName() == TEXT("ApplyDamage"))
            {
                auto* Apply = Call(G, TEXT("ApplyAnalyzedDamage")); Apply->NodePosX = C->NodePosX; Apply->NodePosY = C->NodePosY;
                for (const TCHAR* Pin : {TEXT("execute"),TEXT("then"),TEXT("DamagedActor"),TEXT("BaseDamage"),TEXT("EventInstigator"),TEXT("DamageCauser"),TEXT("DamageTypeClass"),TEXT("ReturnValue")})
                    if (!Move(C->FindPin(Pin), Apply->FindPin(Pin))) return TEXT("ERROR: damage pins");
                if (!Configure(BP, G, Apply)) return TEXT("ERROR: firing source pin");
                Apply->NodeComment = TEXT("ArenaAnalyzer.v2"); FBlueprintEditorUtils::RemoveNode(BP, C, true); ++Changes;
            }
            if (C && BP->GetName().StartsWith(TEXT("BP_Weapon_"))
                && (C->GetFunctionName() == TEXT("LineTraceSingle") || C->GetFunctionName() == TEXT("LineTraceMulti")))
            {
                const bool Multi = C->GetFunctionName() == TEXT("LineTraceMulti");
                auto* Trace = Call(G, Multi ? TEXT("RecordTraces") : TEXT("RecordTrace"));
                if (!Link(Self(G), Trace->FindPin(TEXT("Weapon"))) || !Link(C->FindPin(Multi ? TEXT("OutHits") : TEXT("OutHit")), Trace->FindPin(Multi ? TEXT("Hits") : TEXT("Hit"))) || !After(C, Trace)) return TEXT("ERROR: trace pins");
                if (!Configure(BP, G, Trace)) return TEXT("ERROR: aim pins");
                Trace->NodeComment = TEXT("ArenaAnalyzer.v2"); ++Changes;
            }
            if ((BP->GetName() == TEXT("BP_PlasmaProjectile") || BP->GetName() == TEXT("BP_RocketLauncherProjectile"))
                && N->FindPin(TEXT("Hit")) && N->FindPin(TEXT("Hit"))->Direction == EGPD_Output && N->FindPin(TEXT("then")))
            {
                auto* Hit = Call(G, TEXT("RecordTrace"));
                if (!Link(Self(G), Hit->FindPin(TEXT("Weapon"))) || !Link(N->FindPin(TEXT("Hit")), Hit->FindPin(TEXT("Hit"))) || !After(N, Hit)) return TEXT("ERROR: impact pins");
                Hit->NodeComment = TEXT("ArenaAnalyzer.v2"); ++Changes;
            }
            if (C && (BP->GetName() == TEXT("BP_PlasmaProjectile") || BP->GetName() == TEXT("BP_RocketLauncherProjectile")) && C->GetFunctionName() == TEXT("TakeDamage"))
            {
                auto* Begin = Call(G, TEXT("BeginProjectileDamage")); auto* End = Call(G, TEXT("EndProjectileDamage"));
                auto* Me = Self(G);
                if (!Link(Me, Begin->FindPin(TEXT("Projectile"))) || !Link(Me, End->FindPin(TEXT("Projectile"))) || !Before(C, Begin) || !After(C, End)) return TEXT("ERROR: projectile context pins");
                Begin->NodeComment = TEXT("ArenaAnalyzer.v2"); ++Changes;
            }
            if (auto* Delegate = Cast<UK2Node_CallDelegate>(N);
                Delegate && BP->GetName() == TEXT("BPC_Life") && Delegate->GetPropertyName() == TEXT("OnDeath"))
            {
                auto* Death = Call(G, TEXT("ConfirmDeath"));
                auto* Owner = Call(G, TEXT("GetOwner"), UActorComponent::StaticClass());
                if (!Link(Owner->GetReturnValuePin(), Death->FindPin(TEXT("Victim")))
                    || !Copy(Delegate->FindPin(TEXT("InstigatedBy")), Death->FindPin(TEXT("InstigatedBy")))
                    || !Copy(Delegate->FindPin(TEXT("DamageCauser")), Death->FindPin(TEXT("DamageCauser"))) || !Before(Delegate, Death)) return TEXT("ERROR: death pins");
                Death->NodeComment = TEXT("ArenaAnalyzer.v2"); ++Changes;
            }
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    return FString::Printf(TEXT("OK: %d connections"), Changes);
}

AActor* UArenaAnalyzerMigrationLibrary::SpawnAnalyzerProbeWeapon(APawn* Shooter, TSubclassOf<AActor> Class)
{
    if (!IsValid(Shooter) || !Shooter->HasAuthority() || !Class || Shooter->GetWorld()->WorldType != EWorldType::PIE
        || !Class->GetPathName().StartsWith(TEXT("/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_"))) return nullptr;
    for (TActorIterator<AActor> It(Shooter->GetWorld(), Class); It; ++It)
        if (It->GetOwner() == Shooter) return *It;
    FString Name = Class->GetName(); Name.RemoveFromStart(TEXT("BP_Weapon_")); Name.RemoveFromEnd(TEXT("_C"));
    const FString DataPath = FString::Printf(TEXT("/Game/QuakeLike_1_0/Data/Item/DA_%s.DA_%s"), *Name, *Name);
    UObject* Data = LoadObject<UObject>(nullptr, *DataPath);
    auto* DataProperty = FindFProperty<FObjectPropertyBase>(Class.Get(), TEXT("weaponData"));
    auto* InfoProperty = FindFProperty<FStructProperty>(Class.Get(), TEXT("itemInfo"));
    if (!Data || !DataProperty || !InfoProperty) return nullptr;
    AActor* Weapon = Shooter->GetWorld()->SpawnActorDeferred<AActor>(Class, Shooter->GetActorTransform(), Shooter, Shooter,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Weapon) return nullptr;
    DataProperty->SetObjectPropertyValue_InContainer(Weapon, Data);
    void* Info = InfoProperty->ContainerPtrToValuePtr<void>(Weapon);
    for (TFieldIterator<FObjectPropertyBase> It(InfoProperty->Struct); It; ++It)
        if (It->GetName().StartsWith(TEXT("ItemData"))) It->SetObjectPropertyValue_InContainer(Info, Data);
    Weapon->FinishSpawning(Shooter->GetActorTransform());
    TInlineComponentArray<UActorComponent*> Components(Shooter);
    for (auto* Component : Components)
        if (Component->GetClass()->GetFName() == TEXT("BPC_Inventory_C"))
            if (UFunction* Pickup = Component->FindFunction(TEXT("Pickup")))
            {
                FStructOnScope Params(Pickup);
                if (auto* Item = FindFProperty<FObjectPropertyBase>(Pickup, TEXT("item")))
                { Item->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Weapon); Component->ProcessEvent(Pickup, Params.GetStructMemory()); }
                break;
            }
    return IsValid(Weapon) ? Weapon : nullptr;
}

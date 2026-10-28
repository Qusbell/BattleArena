#include "Analyzer/ArenaAnalyzeControls.h"
#include "Analyzer/ArenaAnalyzeLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "DrawDebugHelpers.h"
#include "Misc/Paths.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

bool UArenaAnalyzeControls::Initialize()
{
    const bool Result = Super::Initialize();
    if (!Result || !WidgetTree || Summary) return Result;
    auto* Root = WidgetTree->ConstructWidget<UVerticalBox>(); WidgetTree->RootWidget = Root;
    auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
    Label->SetText(FText::FromString(TEXT("Combat records — selected file above"))); Root->AddChild(Label);
    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChild(Row);
    auto Button = [&](const TCHAR* Caption)
    {
        auto* B = WidgetTree->ConstructWidget<UButton>(); auto* T = WidgetTree->ConstructWidget<UTextBlock>();
        T->SetText(FText::FromString(Caption)); B->AddChild(T); Row->AddChild(B); return B;
    };
    Button(TEXT("Load combat"))->OnClicked.AddDynamic(this, &ThisClass::LoadSession);
    Actors = WidgetTree->ConstructWidget<UComboBoxString>(); Row->AddChild(Actors);
    for (const TCHAR* Option : {TEXT("All actors"), TEXT("Players"), TEXT("AI")}) Actors->AddOption(Option);
    Actors->SetSelectedIndex(0);
    Weapons = WidgetTree->ConstructWidget<UComboBoxString>(); Root->AddChild(Weapons);
    Weapons->AddOption(TEXT("All weapons")); Weapons->SetSelectedIndex(0);
    Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChild(Row);
    Events = WidgetTree->ConstructWidget<UComboBoxString>(); Row->AddChild(Events);
    for (const TCHAR* Option : {TEXT("All events"), TEXT("Shots"), TEXT("Hits"), TEXT("Deaths"), TEXT("Movement"), TEXT("Damage")}) Events->AddOption(Option);
    Events->SetSelectedIndex(0);
    Button(TEXT("Draw combat"))->OnClicked.AddDynamic(this, &ThisClass::DrawRecords);
    Button(TEXT("Clear combat"))->OnClicked.AddDynamic(this, &ThisClass::ClearRecords);
    Summary = WidgetTree->ConstructWidget<UTextBlock>(); Summary->SetAutoWrapText(true); Summary->SetWrapTextAt(400); Root->AddChild(Summary);
    return Result;
}
FString UArenaAnalyzeControls::SelectedFile() const
{
    if (const UUserWidget* Owner = GetTypedOuter<UUserWidget>())
        if (const auto* Files = Cast<UComboBoxString>(Owner->GetWidgetFromName(TEXT("Files")))) return Files->GetSelectedOption();
    return FString();
}
void UArenaAnalyzeControls::LoadSession()
{
    const FString File = SelectedFile();
    FArenaAnalyzeSession Loaded;
    if (File.IsEmpty() || !UArenaAnalyzeLibrary::LoadAnalyzeFileByName(File, Loaded))
    { Summary->SetText(FText::FromString(TEXT("Select a valid analyzer file above."))); return; }
    Session = MoveTemp(Loaded); LoadedFile = File;
    Weapons->ClearOptions(); Weapons->AddOption(TEXT("All weapons"));
    TSet<FString> Ids; for (const auto& Shot : Session.ShotSamples) Ids.Add(Shot.WeaponId);
    WeaponIds = Ids.Array(); WeaponIds.Sort();
    for (int32 Index = 0; Index < WeaponIds.Num(); ++Index)
        Weapons->AddOption(FString::Printf(TEXT("%s [%d]"), *FPaths::GetBaseFilename(WeaponIds[Index]), Index + 1));
    Weapons->SetSelectedIndex(0);
    Summary->SetText(FText::FromString(FString::Printf(TEXT("v%d | %d shots | %d contacts | %d damage | %d deaths%s"),
        Session.SchemaVersion, Session.ShotSamples.Num(), Session.HitSamples.Num(), Session.DamageSamples.Num(), Session.DeathSamples.Num(),
        Session.SchemaVersion == 1 ? TEXT(" | legacy: actor type / combat identity unavailable") : TEXT(""))));
}
void UArenaAnalyzeControls::ClearRecords()
{
#if WITH_EDITOR
    if (GEditor) FlushPersistentDebugLines(GEditor->GetEditorWorldContext().World());
#endif
}
void UArenaAnalyzeControls::DrawRecords()
{
    if (LoadedFile != SelectedFile() || LoadedFile.IsEmpty()) LoadSession();
    if (LoadedFile.IsEmpty() || LoadedFile != SelectedFile()) return;
#if WITH_EDITOR
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr; if (!World) return;
    const auto S = UArenaAnalyzeLibrary::FilterSession(Session, WeaponIds.IsValidIndex(Weapons->GetSelectedIndex() - 1) ? WeaponIds[Weapons->GetSelectedIndex() - 1] : FString(), Actors->GetSelectedIndex());
    const int32 Event = Events->GetSelectedIndex();
    TSet<int32> Players; for (const auto& P : S.Participants) if (P.bPlayer) Players.Add(P.ControllerId);
    auto Color = [&](int32 Id) { return Players.Contains(Id) ? FColor::Cyan : FColor::Orange; };
    TMap<int32, FArenaShotSample> Shots; for (const auto& Shot : Session.ShotSamples) Shots.Add(Shot.ShotId, Shot);
    if (Event == 0 || Event == 1) for (const auto& Shot : S.ShotSamples)
        DrawDebugDirectionalArrow(World, Shot.Location, Shot.Location + Shot.Direction * 150, 20, Color(Shot.ControllerId), true, -1, 0, 2);
    if (Event == 0 || Event == 2) for (const auto& Hit : S.HitSamples)
    {
        DrawDebugSphere(World, Hit.Location, Hit.bDamageContact ? 7 : 12, 8, FColor::Green, true);
        if (const auto* Shot = Shots.Find(Hit.ShotId)) DrawDebugLine(World, Shot->Location, Hit.Location, FColor::Green, true, -1, 0, 0.5);
    }
    if (Event == 0 || Event == 3) for (const auto& Death : S.DeathSamples)
    {
        DrawDebugBox(World, Death.VictimLocation, FVector(20), FColor::Red, true);
        if (Death.bHasKillerLocation) DrawDebugLine(World, Death.KillerLocation, Death.VictimLocation, FColor::Red, true, -1, 0, 2);
    }
    if (Event == 0 || Event == 4) for (const auto& Segment : UArenaAnalyzeLibrary::BuildMovementLineSegments(S.MovementSamples))
        DrawDebugLine(World, Segment.StartLocation, Segment.EndLocation, Color(Segment.ControllerId), true, -1, 0, 2);
    if (Event == 0 || Event == 5) for (const auto& Damage : S.DamageSamples)
    {
        DrawDebugPoint(World, Damage.DamagedLocation, 12, FColor::Yellow, true);
        if (Damage.bHasInstigatorLocation) DrawDebugLine(World, Damage.InstigatorLocation, Damage.DamagedLocation, FColor::Yellow, true);
    }
    Summary->SetText(FText::FromString(FString::Printf(TEXT("Filtered: %d shots | %d contacts | %d damage | %d deaths. Player=cyan, AI=orange, hit=green, death=red."),
        S.ShotSamples.Num(), S.HitSamples.Num(), S.DamageSamples.Num(), S.DeathSamples.Num())));
#endif
}

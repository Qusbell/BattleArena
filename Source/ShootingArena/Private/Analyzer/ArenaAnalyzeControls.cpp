#include "Analyzer/ArenaAnalyzeControls.h"
#include "Analyzer/ArenaAnalyzeLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Analyzer/ArenaAnalyzeRenderLibrary.h"
#include "Brushes/SlateColorBrush.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
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
    auto Font = Label->GetFont(); Font.Size = 13; Label->SetFont(Font);
    Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Label->SetText(FText::FromString(TEXT("Arena Analyzer"))); Root->AddChild(Label);
    Files = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("SessionFiles")); Root->AddChild(Files);
    auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChild(Row);
    auto Button = [&](const TCHAR* Caption)
    {
        auto* B = WidgetTree->ConstructWidget<UButton>(); auto* T = WidgetTree->ConstructWidget<UTextBlock>();
        T->SetFont(Font); T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        T->SetText(FText::FromString(Caption)); B->AddChild(T); Row->AddChild(B); return B;
    };
    Button(TEXT("Refresh files"))->OnClicked.AddDynamic(this, &ThisClass::RefreshFiles);
    Button(TEXT("Load"))->OnClicked.AddDynamic(this, &ThisClass::LoadSession);
    Actors = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ActorFilter")); Row->AddChild(Actors);
    for (const TCHAR* Option : {TEXT("All actors"), TEXT("Players"), TEXT("AI")}) Actors->AddOption(Option);
    Actors->SetSelectedIndex(0);
    Weapons = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("WeaponFilter")); Root->AddChild(Weapons);
    Weapons->AddOption(TEXT("All weapons")); Weapons->SetSelectedIndex(0);
    Row = WidgetTree->ConstructWidget<UHorizontalBox>(); Root->AddChild(Row);
    Events = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("EventFilter")); Row->AddChild(Events);
    for (const TCHAR* Option : {TEXT("All events"), TEXT("Shots"), TEXT("Hits"), TEXT("Deaths"), TEXT("Movement"), TEXT("Damage")}) Events->AddOption(Option);
    Events->SetSelectedIndex(0);
    Button(TEXT("Draw"))->OnClicked.AddDynamic(this, &ThisClass::DrawRecords);
    Button(TEXT("Clear"))->OnClicked.AddDynamic(this, &ThisClass::ClearRecords);
    Summary = WidgetTree->ConstructWidget<UTextBlock>(); Summary->SetFont(Font);
    Summary->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Summary->SetAutoWrapText(true); Summary->SetWrapTextAt(300); Root->AddChild(Summary);
    for (auto* Combo : {Files.Get(), Actors.Get(), Weapons.Get(), Events.Get()})
    {
        ConfigureCombo(Combo);
        Combo->OnGenerateWidgetEvent.BindDynamic(this, &ThisClass::GenerateOption);
    }
    RefreshFiles();
    return Result;
}
FString UArenaAnalyzeControls::SelectedFile() const
{
    return Files ? Files->GetSelectedOption() : FString();
}
void UArenaAnalyzeControls::RefreshFiles()
{
    const FString Previous = SelectedFile();
    const TArray<FString> Names = UArenaAnalyzeLibrary::GetAnalyzeFiles();
    Files->ClearOptions();
    for (const auto& Name : Names) Files->AddOption(Name);
    if (Names.Contains(Previous)) Files->SetSelectedOption(Previous);
    else if (!Names.IsEmpty()) Files->SetSelectedIndex(0);
    Summary->SetText(FText::FromString(Names.IsEmpty() ? TEXT("No recordings found.") : TEXT("Select a recording, then Load or Draw.")));
}

void UArenaAnalyzeControls::LoadSession()
{
    const FString File = SelectedFile();
    FArenaAnalyzeSession Loaded;
    if (File.IsEmpty() || !UArenaAnalyzeLibrary::LoadAnalyzeFileByName(File, Loaded))
    { Session = FArenaAnalyzeSession(); LoadedFile.Reset(); Summary->SetText(FText::FromString(TEXT("Select a valid recording."))); return; }
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

void UArenaAnalyzeControls::ConfigureCombo(UComboBoxString* Combo)
{
    if (!Combo) return;
    const FSlateColorBrush Dark(FLinearColor(0.035f, 0.045f, 0.065f, 1));
    const FSlateColorBrush Hover(FLinearColor(0.08f, 0.20f, 0.34f, 1));
    const FSlateColorBrush Selected(FLinearColor(0.03f, 0.30f, 0.55f, 1));
    auto Style = Combo->GetWidgetStyle();
    Style.ComboButtonStyle.ButtonStyle.SetNormal(Dark).SetHovered(Hover).SetPressed(Selected);
    Style.ComboButtonStyle.DownArrowImage.TintColor = FSlateColor(FLinearColor::White);
    Style.ComboButtonStyle.MenuBorderBrush = Dark;
    Style.ComboButtonStyle.ContentPadding = FMargin(10,6);
    Combo->SetWidgetStyle(Style);
    auto Row = Combo->GetItemStyle();
    Row.TextColor = FSlateColor(FLinearColor::White); Row.SelectedTextColor = FSlateColor(FLinearColor::White);
    Row.EvenRowBackgroundBrush = Dark; Row.OddRowBackgroundBrush = Dark;
    Row.EvenRowBackgroundHoveredBrush = Hover; Row.OddRowBackgroundHoveredBrush = Hover;
    Row.ActiveBrush = Selected; Row.ActiveHoveredBrush = Selected;
    Row.InactiveBrush = Selected; Row.InactiveHoveredBrush = Hover;
    Combo->SetItemStyle(Row); Combo->SetContentPadding(FMargin(10,6)); Combo->SetMaxListHeight(240);
    // UComboBoxString exposes these construction-only settings without runtime setters.
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    Combo->ForegroundColor = FSlateColor(FLinearColor::White); Combo->Font.Size = 13;
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
}
UWidget* UArenaAnalyzeControls::GenerateOption(FString Option)
{
    auto* Text = WidgetTree->ConstructWidget<UTextBlock>();
    auto Font = Text->GetFont(); Font.Size = 13; Text->SetFont(Font);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::White)); Text->SetText(FText::FromString(Option));
    return Text;
}
AActor* UArenaAnalyzeControls::GetVisualizer() const
{
    UUserWidget* Owner = GetTypedOuter<UUserWidget>();
    UFunction* Function = Owner ? Owner->FindFunction(TEXT("Get Visualizer")) : nullptr;
    if (!Function) return nullptr;
    FStructOnScope Params(Function); Owner->ProcessEvent(Function, Params.GetStructMemory());
    for (TFieldIterator<FObjectPropertyBase> It(Function); It; ++It)
        if (It->HasAnyPropertyFlags(CPF_OutParm))
            if (auto* Actor = Cast<AActor>(It->GetObjectPropertyValue_InContainer(Params.GetStructMemory()))) return Actor;
    return nullptr;
}
void UArenaAnalyzeControls::ClearRecords()
{
    UUserWidget* Owner = GetTypedOuter<UUserWidget>();
    if (UFunction* Function = Owner ? Owner->FindFunction(TEXT("Clear")) : nullptr) Owner->ProcessEvent(Function, nullptr);
    Summary->SetText(FText::FromString(TEXT("Visualizer cleared.")));
}
void UArenaAnalyzeControls::DrawRecords()
{
    if (LoadedFile != SelectedFile() || LoadedFile.IsEmpty()) LoadSession();
    if (LoadedFile.IsEmpty() || LoadedFile != SelectedFile()) return;
    AActor* Visualizer = GetVisualizer();
    if (!Visualizer) { Summary->SetText(FText::FromString(TEXT("Visualizer unavailable."))); return; }
    const double Start = FPlatformTime::Seconds();
    const auto S = UArenaAnalyzeLibrary::FilterSession(Session, WeaponIds.IsValidIndex(Weapons->GetSelectedIndex() - 1)
        ? WeaponIds[Weapons->GetSelectedIndex() - 1] : FString(), Actors->GetSelectedIndex());
    const int32 Count = UArenaAnalyzeRenderLibrary::RenderSession(Visualizer, S, Events->GetSelectedIndex());
    Summary->SetText(FText::FromString(FString::Printf(TEXT("%d shots | %d contacts | %d damage | %d deaths\nISM: %d instances, %.1f ms. Previous draw replaced."),
        S.ShotSamples.Num(), S.HitSamples.Num(), S.DamageSamples.Num(), S.DeathSamples.Num(), Count, (FPlatformTime::Seconds() - Start) * 1000)));
}

#include "Localization/LanguageSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/EditableText.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Internationalization/Text.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformMisc.h"
#include "Misc/DataValidation.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "GameFramework/PlayerState.h"
#include "Internationalization/StringTableRegistry.h"
#include "Internationalization/StringTableCore.h"

DEFINE_LOG_CATEGORY_STATIC(LogArenaLanguage, Log, All);

#if WITH_EDITOR
EDataValidationResult FLanguageRow::IsDataValid(FDataValidationContext& Context) const
{
    bool Valid=true;
    if (CultureCode.IsEmpty() || !FInternationalization::Get().GetCulture(CultureCode))
    {Context.AddError(FText::FromString(TEXT("Language row has an invalid CultureCode.")));Valid=false;}
    if (IsEnabled && (!Font || DisplayName.IsEmpty()))
    {Context.AddError(FText::FromString(TEXT("Enabled language requires a Font and native DisplayName.")));Valid=false;}
    return Valid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

UDataTable* ULanguageSubsystem::GetLanguageTable()
{
    static TWeakObjectPtr<UDataTable> Cached;
    if (!Cached.IsValid()) Cached = LoadObject<UDataTable>(nullptr,
        TEXT("/Game/QuakeLike_1_0/Localization/DT_Language.DT_Language"));
    return Cached.Get();
}

bool ULanguageSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UGameInstance* GI = Cast<UGameInstance>(Outer);
    return Super::ShouldCreateSubsystem(Outer) && GI && !IsRunningDedicatedServer() && !GI->IsDedicatedServerInstance();
}

TArray<FLanguageRow> ULanguageSubsystem::GetEnabledLanguages() const
{
    TArray<FLanguageRow> Result;
    if (UDataTable* Table = GetLanguageTable())
    {
        for (const FName Name : Table->GetRowNames())
            if (const FLanguageRow* Row = Table->FindRow<FLanguageRow>(Name, TEXT("Language")); Row && Row->IsEnabled)
                Result.Add(*Row);
    }
    return Result;
}

FString ULanguageSubsystem::ChooseStartupLanguage(const TArray<FLanguageRow>& Languages, bool HasSaved,
    const FString& Saved, const FString& OS)
{
    FString Default;
    for (const FLanguageRow& Row : Languages)
        if (Row.IsEnabled && Row.IsDefault) { Default = Row.CultureCode; break; }
    if (Default.IsEmpty())
        for (const FLanguageRow& Row : Languages)
            if (Row.IsEnabled) { Default = Row.CultureCode; break; }
    const FString Candidate = HasSaved ? Saved : OS;
    for (const FLanguageRow& Row : Languages)
        if (Row.IsEnabled && Row.CultureCode.Equals(Candidate, ESearchCase::IgnoreCase)) return Row.CultureCode;
    // OS regional language may inherit an enabled parent; invalid saved selections always use the default.
    if (!HasSaved)
    {
        const FLanguageRow* BestParent = nullptr;
        for (const FLanguageRow& Row : Languages)
            if (Row.IsEnabled && Candidate.StartsWith(Row.CultureCode + TEXT("-"), ESearchCase::IgnoreCase)
                && (!BestParent || Row.CultureCode.Len() > BestParent->CultureCode.Len())) BestParent = &Row;
        if (BestParent) return BestParent->CultureCode;
    }
    return Default;
}

void ULanguageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveTable = GetLanguageTable();
    PreviousCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
#if WITH_EDITOR
    WasPreviewEnabled = FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();
#endif
    FString Saved;
    const bool HasSaved = GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Saved, GGameUserSettingsIni);
    const auto Languages = GetEnabledLanguages();
    if (Languages.FilterByPredicate([](const FLanguageRow& Row) { return Row.IsDefault; }).Num() != 1)
        UE_LOG(LogArenaLanguage, Error, TEXT("DT_Language requires exactly one enabled default language."));
    const FString Code = ChooseStartupLanguage(Languages, HasSaved, Saved, FPlatformMisc::GetDefaultLanguage());
    if (!Apply(Code, false)) UE_LOG(LogArenaLanguage, Error, TEXT("Could not initialize language '%s'. Check DT_Language and localization data."), *Code);
}

void ULanguageSubsystem::Deinitialize()
{
#if WITH_EDITOR
    if (GetWorld() && GetWorld()->WorldType == EWorldType::PIE)
    {
        UKismetInternationalizationLibrary::SetCurrentCulture(PreviousCulture, false);
        if (WasPreviewEnabled) FTextLocalizationManager::Get().EnableGameLocalizationPreview(PreviousCulture);
        else FTextLocalizationManager::Get().DisableGameLocalizationPreview();
    }
#endif
    Super::Deinitialize();
}

bool ULanguageSubsystem::Apply(const FString& Code, bool Save)
{
    const TArray<FLanguageRow> Languages = GetEnabledLanguages();
    if (!Languages.ContainsByPredicate([&](const FLanguageRow& Row) { return Row.CultureCode == Code; })) return false;
    if (!UKismetInternationalizationLibrary::SetCurrentCulture(Code, Save)) return false;
#if WITH_EDITOR
    FTextLocalizationManager::Get().EnableGameLocalizationPreview(Code);
#endif
    CurrentCode = Code;
    OnLanguageChanged.Broadcast();
    ULanguageLibrary::RefreshViewportFonts(GetWorld());
    UE_LOG(LogArenaLanguage, Log, TEXT("Applied local language %s (saved=%d) before screen display"), *Code, Save);
    return true;
}

bool ULanguageSubsystem::SetLanguage(const FString& Code) { return Apply(Code, true); }

const TCHAR* ULanguageLibrary::StringTablePath() { return TEXT("/Game/QuakeLike_1_0/Localization/ST_Game.ST_Game"); }
FText ULanguageLibrary::Text(FName Key) { return FText::FromStringTable(FName(StringTablePath()), Key.ToString()); }
FText ULanguageLibrary::FormatText(FName Key, const FText& A, const FText& B, const FText& C, const FText& D, const FText& E, const FText& F, const FText& G, const FText& H)
{
    FFormatNamedArguments Args;
    Args.Add(TEXT("A"), A); Args.Add(TEXT("B"), B); Args.Add(TEXT("C"), C); Args.Add(TEXT("D"), D);
    Args.Add(TEXT("E"), E); Args.Add(TEXT("F"), F); Args.Add(TEXT("G"), G); Args.Add(TEXT("H"), H);
    return FText::Format(Text(Key), Args);
}
FString ULanguageLibrary::DisplayString(FName Key, const FText& A, const FText& B, const FText& C, const FText& D, const FText& E, const FText& F, const FText& G, const FText& H)
{
    FString Serialized;
    FTextStringHelper::WriteToBuffer(Serialized, FormatText(Key, A, B, C, D, E, F, G, H));
    return FString(TEXT("\x1f" "ArenaText:")) + Serialized;
}
FText ULanguageLibrary::ReadDisplayString(const FString& Value)
{
    // Only our explicit text-history serialization is decoded. User nicknames remain invariant.
    const FString Prefix(TEXT("\x1f" "ArenaText:"));
    if (Value.StartsWith(Prefix) && Value.Len() <= 65536)
    {
        // The engine's text-history parser is recursive. Bound nesting before invoking it,
        // ignoring parentheses in quoted nicknames and respecting escaped quotes.
        int32 Depth = 0;
        bool Quoted = false;
        bool Escaped = false;
        for (int32 Index = Prefix.Len(); Index < Value.Len(); ++Index)
        {
            const TCHAR Character = Value[Index];
            if (Quoted)
            {
                if (Escaped) Escaped = false;
                else if (Character == TEXT('\\')) Escaped = true;
                else if (Character == TEXT('"')) Quoted = false;
            }
            else if (Character == TEXT('"')) Quoted = true;
            else if (Character == TEXT('('))
            {
                if (++Depth > 32) return FText::AsCultureInvariant(Value);
            }
            else if (Character == TEXT(')') && --Depth < 0) return FText::AsCultureInvariant(Value);
        }
        if (!Quoted && Depth == 0)
        {
            FText Result;
            const TCHAR* End = FTextStringHelper::ReadFromBuffer(*Value + Prefix.Len(), Result, nullptr, nullptr, true);
            if (End)
            {
                while (FChar::IsWhitespace(*End)) ++End;
                if (*End == 0) return Result;
            }
        }
    }
    return FText::AsCultureInvariant(Value);
}
FText ULanguageLibrary::DisplayName(FName Domain, FName Id)
{
    const FString Key = Domain.ToString() + TEXT("_") + Id.ToString();
    auto Table = FStringTableRegistry::Get().FindStringTable(FName(StringTablePath()));
    if (!Table)
    {
        LoadObject<UObject>(nullptr, StringTablePath());
        Table = FStringTableRegistry::Get().FindStringTable(FName(StringTablePath()));
    }
    FString Source;
    if (Table && Table->GetSourceString(Key, Source)) return Text(FName(*Key));
    static TSet<FString> Reported;
    if (!Reported.Contains(Key)) { Reported.Add(Key); UE_LOG(LogArenaLanguage, Error, TEXT("Missing display-name String Table key: %s"), *Key); }
    return FText::AsCultureInvariant(Id.ToString());
}
FString ULanguageLibrary::ParticipantDisplayString(APlayerState* State)
{
    if (!IsValid(State)) return FString();
    const FStrProperty* Campaign = FindFProperty<FStrProperty>(State->GetClass(), TEXT("CampaignName"));
    FString Id = Campaign ? Campaign->GetPropertyValue_InContainer(State) : FString();
    if (State->IsABot() && Id.IsEmpty())
        if (const FNameProperty* Selected = FindFProperty<FNameProperty>(State->GetClass(), TEXT("SelectedCharacterRow")))
        { const FName Row = Selected->GetPropertyValue_InContainer(State); if (!Row.IsNone()) Id = Row.ToString(); }
    // A human may also have a selected character. Never replace their nickname with that character's name.
    if (State->IsABot() && !Id.IsEmpty())
    {
        const FText Name = DisplayName(TEXT("Character"), FName(*Id));
        const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(FName(StringTablePath()));
        FString Source;
        if (Table && Table->GetSourceString(TEXT("Character_") + Id, Source))
        {
            FString Serialized; FTextStringHelper::WriteToBuffer(Serialized, Name); return FString(TEXT("\x1f" "ArenaText:")) + Serialized;
        }
    }
    FString Serialized; FTextStringHelper::WriteToBuffer(Serialized, FText::AsCultureInvariant(State->GetPlayerName()));
    return FString(TEXT("\x1f" "ArenaText:")) + Serialized;
}

FSlateFontInfo ULanguageLibrary::ResolveFont(FSlateFontInfo Existing)
{
    static uint64 CachedFrame = MAX_uint64;
    static FString CachedLanguage;
    static TWeakObjectPtr<UFont> CachedFont;
    const FString Language = FInternationalization::Get().GetCurrentLanguage()->GetName();
    if (CachedFrame != GFrameCounter || CachedLanguage != Language)
    {
        CachedFrame = GFrameCounter; CachedLanguage = Language; CachedFont.Reset();
        int32 BestMatch = -1;
        if (UDataTable* Table = ULanguageSubsystem::GetLanguageTable())
            for (const FName Key : Table->GetRowNames())
                if (const FLanguageRow* Row = Table->FindRow<FLanguageRow>(Key, TEXT("Font")); Row && Row->IsEnabled && Row->Font)
                    if (Row->CultureCode == Language || Language.StartsWith(Row->CultureCode + TEXT("-")))
                        if (Row->CultureCode.Len() > BestMatch) { BestMatch = Row->CultureCode.Len(); CachedFont = Row->Font; }
    }
    if (UFont* Font = CachedFont.Get())
    {
        Existing.FontObject = Font;
        const FCompositeFont* Composite = Font->GetCompositeFont();
        if (Composite && Composite->DefaultTypeface.Fonts.Num() &&
            !Composite->DefaultTypeface.Fonts.ContainsByPredicate([&](const FTypefaceEntry& Entry) { return Entry.Name == Existing.TypefaceFontName; }))
            Existing.TypefaceFontName = Composite->DefaultTypeface.Fonts[0].Name;
    }
    return Existing;
}

void ULanguageLibrary::RefreshViewportFonts(UWorld* World)
{
    if (!World || !World->IsGameWorld() || World->GetNetMode() == NM_DedicatedServer) return;
    for (TObjectIterator<UTextBlock> It; It; ++It)
        if (IsValid(*It) && !It->IsTemplate() && It->GetWorld() == World && It->GetCachedWidget().IsValid())
        {
            const FSlateFontInfo Font = ResolveFont(It->GetFont());
            if (Font != It->GetFont()) It->SetFont(Font);
        }
    for (TObjectIterator<UEditableText> It; It; ++It)
        if (IsValid(*It) && !It->IsTemplate() && It->GetWorld()==World && It->GetCachedWidget().IsValid())
        { const auto Font=ResolveFont(It->GetFont());if(Font!=It->GetFont())It->SetFont(Font); }
    for (TObjectIterator<UEditableTextBox> It; It; ++It)
        if (IsValid(*It) && !It->IsTemplate() && It->GetWorld()==World && It->GetCachedWidget().IsValid())
        {
            const auto Font=ResolveFont(It->WidgetStyle.TextStyle.Font);
            if(Font!=It->WidgetStyle.TextStyle.Font) {It->WidgetStyle.TextStyle.Font=Font;It->SynchronizeProperties();}
        }
}

TSharedRef<SWidget> ULanguageSelector::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    if (!WidgetTree->RootWidget)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        WidgetTree->RootWidget = Row;
        Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(ULanguageLibrary::Text(TEXT("Settings_Language")));
        FSlateFontInfo Font = Label->GetFont(); Font.Size = 20; Label->SetFont(ULanguageLibrary::ResolveFont(Font));
        UHorizontalBoxSlot* Left = Row->AddChildToHorizontalBox(Label);
        Left->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); Left->SetVerticalAlignment(VAlign_Center); Left->SetPadding(FMargin(14, 8));
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetWidthOverride(200); Size->SetMinDesiredHeight(40);
        Combo = WidgetTree->ConstructWidget<UComboBoxString>(); Size->AddChild(Combo);
        Row->AddChildToHorizontalBox(Size)->SetPadding(FMargin(8));
        Combo->OnGenerateWidgetEvent.BindDynamic(this, &ThisClass::GenerateOption);
        Combo->OnSelectionChanged.AddDynamic(this, &ThisClass::Selected);
    }
    return Super::RebuildWidget();
}
void ULanguageSelector::NativeConstruct()
{
    Super::NativeConstruct();
    if (ULanguageSubsystem* Previous = BoundLanguage.Get())
        Previous->OnLanguageChanged.RemoveDynamic(this, &ThisClass::Refresh);
    BoundLanguage.Reset();
    if (UGameInstance* GI = GetGameInstance()) if (ULanguageSubsystem* Language = GI->GetSubsystem<ULanguageSubsystem>())
    {
        BoundLanguage = Language;
        Language->OnLanguageChanged.AddUniqueDynamic(this, &ThisClass::Refresh);
    }
    Refresh();
}
void ULanguageSelector::NativeDestruct()
{
    if (ULanguageSubsystem* Language = BoundLanguage.Get())
        Language->OnLanguageChanged.RemoveDynamic(this, &ThisClass::Refresh);
    BoundLanguage.Reset();
    Super::NativeDestruct();
}
void ULanguageSelector::Refresh()
{
    if (!IsValid(Combo) || !IsValid(Label)) return;
    ULanguageSubsystem* Language = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULanguageSubsystem>() : nullptr;
    if (!Language) return;
    TGuardValue<bool> Guard(Refreshing, true);
    Label->SetText(ULanguageLibrary::Text(TEXT("Settings_Language")));
    Options = Language->GetEnabledLanguages(); Combo->ClearOptions();
    for (const FLanguageRow& Row : Options) Combo->AddOption(Row.CultureCode);
    Combo->SetSelectedOption(Language->GetCurrentLanguageCode());
    InvalidateLayoutAndVolatility();
}
void ULanguageSelector::Selected(FString Item, ESelectInfo::Type SelectionType)
{
    if (Refreshing || SelectionType == ESelectInfo::Direct) return;
    if (UGameInstance* GI = GetGameInstance()) if (ULanguageSubsystem* Language = GI->GetSubsystem<ULanguageSubsystem>())
        if (!Language->SetLanguage(Item)) Refresh();
}
UWidget* ULanguageSelector::GenerateOption(FString Item)
{
    if (!IsValid(WidgetTree)) return nullptr;
    UTextBlock* Result = WidgetTree->ConstructWidget<UTextBlock>();
    const FLanguageRow* Row = Options.FindByPredicate([&](const FLanguageRow& R) { return R.CultureCode == Item; });
    Result->SetText(Row ? Row->DisplayName : FText::AsCultureInvariant(Item));
    FSlateFontInfo Font = Result->GetFont(); Font.Size = 18;
    Result->SetFont(ULanguageLibrary::ResolveFont(Font));
    return Result;
}

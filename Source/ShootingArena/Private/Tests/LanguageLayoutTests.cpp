#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Localization/LanguageSubsystem.h"
#include "UI/ReusableStyleHost.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "Serialization/BufferArchive.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTranslatedButtonLayoutTest,
    "ShootingArena.Localization.ButtonContentSizing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTranslatedButtonLayoutTest::RunTest(const FString& Parameters)
{
    const FString Culture = FInternationalization::Get().GetCurrentCulture()->GetName();
    const bool Preview = FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();
    ON_SCOPE_EXIT
    {
        UKismetInternationalizationLibrary::SetCurrentCulture(Culture, false);
        if (Preview) FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
        else FTextLocalizationManager::Get().DisableGameLocalizationPreview();
    };
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    UClass* WidgetClass = LoadClass<UReusableStyleHost>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameFlow/UI/03_Widget/06_SettingsComponents/WBP_SettingsButton.WBP_SettingsButton_C"));
    UReusableStyleHost* Host = CreateWidget<UReusableStyleHost>(Context.GetTestWorld()->GetGameInstance(), WidgetClass);
    if (!TestNotNull(TEXT("Existing settings button wrapper"), Host)) return false;
    Host->ButtonStyle = LoadClass<UCommonButtonStyle>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameFlow/UI/02_Styles/02_ButtonStyles/03_BS_Setting/BS_Setting_Action.BS_Setting_Action_C"));
    TSharedRef<SWidget> Slate = Host->TakeWidget();
    UTextBlock* Label = Cast<UTextBlock>(Host->GetWidgetFromName(TEXT("Label")));
    USizeBox* Bounds = Cast<USizeBox>(Host->GetWidgetFromName(TEXT("StyleBounds")));
    if (!TestNotNull(TEXT("Button label"), Label) || !TestNotNull(TEXT("Style bounds"), Bounds)) return false;
    const int32 FontSize = Label->GetFont().Size;
    Label->SetText(FText::AsCultureInvariant(TEXT("기본값으로")));
    Host->ForceLayoutPrepass();
    const float KoreanWidth = Host->GetDesiredSize().X;
    Label->SetText(FText::AsCultureInvariant(TEXT("Reset to Default")));
    Host->ForceLayoutPrepass();
    TestTrue(TEXT("English text fits its button content box"), Host->GetDesiredSize().X >= Label->GetDesiredSize().X);
    TestTrue(TEXT("Longer text never shrinks the button"), Host->GetDesiredSize().X >= KoreanWidth);
    Label->SetText(FText::AsCultureInvariant(TEXT("Reset to Default (all settings)")));
    Host->ForceLayoutPrepass();
    TestTrue(TEXT("Content can grow beyond the Korean minimum"), Host->GetDesiredSize().X > KoreanWidth);
    TestEqual(TEXT("Text fit preserves the authored font size"), int32(Label->GetFont().Size), FontSize);
    TestFalse(TEXT("A minimum width is not a fixed width"), Bounds->IsWidthOverride());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanguageSelectorPlacementTest,
    "ShootingArena.Localization.SelectorPlacementAndImmediateApply",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLanguageSelectorPlacementTest::RunTest(const FString& Parameters)
{
    FString Saved;
    const bool HadSaved = GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Saved, GGameUserSettingsIni);
    const FString Culture = FInternationalization::Get().GetCurrentCulture()->GetName();
    const bool Preview = FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    UGameInstance* GI = Context.GetTestWorld()->GetGameInstance();
    ULanguageSubsystem* Language = GI->GetSubsystem<ULanguageSubsystem>();
    if (!TestNotNull(TEXT("Device-local language subsystem"), Language)) return false;
    const FString OriginalCode = Language->GetCurrentLanguageCode();
    ON_SCOPE_EXIT
    {
        Language->SetLanguage(OriginalCode);
        if (HadSaved) GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *Saved, GGameUserSettingsIni);
        else GConfig->RemoveKey(TEXT("Internationalization"), TEXT("Culture"), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
        UKismetInternationalizationLibrary::SetCurrentCulture(Culture, false);
        if (Preview) FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
        else FTextLocalizationManager::Get().DisableGameLocalizationPreview();
    };
    ULanguageSelector* Settings = CreateWidget<ULanguageSelector>(GI);
    ULanguageSelector* Main = CreateWidget<ULanguageSelector>(GI);
    Settings->bShowSectionHeading = true;
    Main->bInlineChoices = true;
    TSharedRef<SWidget> SettingsSlate = Settings->TakeWidget();
    TSharedRef<SWidget> MainSlate = Main->TakeWidget();
    if (!TestNotNull(TEXT("Gameplay language combo"), Settings->Combo.Get())
        || !TestNotNull(TEXT("Korean shortcut"), Main->KoreanButton.Get())
        || !TestNotNull(TEXT("English shortcut"), Main->EnglishButton.Get())) return false;

    for (int32 Tab : {1, 2, 3})
    {
        Settings->SetSettingsTab(Tab);
        TestEqual(TEXT("Other settings tabs collapse the entire language section"), Settings->GetVisibility(), ESlateVisibility::Collapsed);
    }
    Settings->SetSettingsTab(0);
    TestEqual(TEXT("Gameplay tab shows the language category"), Settings->GetVisibility(), ESlateVisibility::Visible);
    Main->SelectEnglish();
    TestEqual(TEXT("Main menu shortcut applies English immediately"), Language->GetCurrentLanguageCode(), FString(TEXT("en")));
    TestEqual(TEXT("Settings reflects the main menu change"), Settings->Combo->GetSelectedOption(), FString(TEXT("en")));
    Settings->Selected(TEXT("ko"), ESelectInfo::OnMouseClick);
    TestEqual(TEXT("Settings applies Korean immediately"), Language->GetCurrentLanguageCode(), FString(TEXT("ko")));
    TestTrue(TEXT("Both surfaces keep the same subscribed subsystem"), Main->BoundLanguage.Get() == Settings->BoundLanguage.Get());
    Settings->RemoveFromParent();
    Main->RemoveFromParent();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayDefaultsTest,
    "ShootingArena.Localization.GameplayDefaultsPreserveLanguage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameplayDefaultsTest::RunTest(const FString& Parameters)
{
    const FString Culture = FInternationalization::Get().GetCurrentCulture()->GetName();
    const bool Preview = FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();
    ON_SCOPE_EXIT
    {
        UKismetInternationalizationLibrary::SetCurrentCulture(Culture, false);
        if (Preview) FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
        else FTextLocalizationManager::Get().DisableGameLocalizationPreview();
    };
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Context.GetTestWorld();
    UGameInstance* OriginalGI = World->GetGameInstance();
    UClass* GIClass = LoadClass<UGameInstance>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameMode/BP_QuakeGameInstance.BP_QuakeGameInstance_C"));
    UClass* SettingsClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameProgressFlow/WBP_SettingsMenu.WBP_SettingsMenu_C"));
    if (!TestNotNull(TEXT("Actual game instance class"), GIClass)
        || !TestNotNull(TEXT("Actual settings class"), SettingsClass)) return false;

    // Use the actual BP defaults and handlers without Init/Construct/BeginPlay.
    // The temporary slot keeps the user's existing settings save untouched.
    UGameInstance* GI = NewObject<UGameInstance>(GEngine, GIClass);
    World->SetGameInstance(GI);
    const FString Slot = TEXT("Automation_GameplayDefaults_") + FGuid::NewGuid().ToString();
    ON_SCOPE_EXIT
    {
        UGameplayStatics::DeleteGameInSlot(Slot, 0);
        World->SetGameInstance(OriginalGI);
    };
    FStrProperty* SlotProperty = FindFProperty<FStrProperty>(GIClass, TEXT("SlotName"));
    FBoolProperty* Outline = FindFProperty<FBoolProperty>(GIClass, TEXT("bShowCharacterOutline"));
    FBoolProperty* AimName = FindFProperty<FBoolProperty>(GIClass, TEXT("bShowAimNickname"));
    if (!TestNotNull(TEXT("Save slot property"), SlotProperty)
        || !TestNotNull(TEXT("Outline setting"), Outline)
        || !TestNotNull(TEXT("Aim nickname setting"), AimName)) return false;
    SlotProperty->SetPropertyValue_InContainer(GI, Slot);
    // Outer the widget to the test world so it resolves the uninitialized BP GI.
    UUserWidget* Settings = NewObject<UUserWidget>(World, SettingsClass);
    if (!TestTrue(TEXT("Initialize actual settings designer tree"), Settings->Initialize())) return false;
    UButton* Reset = Cast<UButton>(Settings->GetWidgetFromName(TEXT("Btn_Default")));
    FIntProperty* Tab = FindFProperty<FIntProperty>(SettingsClass, TEXT("SelectedTabIndex"));
    if (!TestNotNull(TEXT("Actual default button"), Reset)
        || !TestNotNull(TEXT("Selected tab"), Tab)) return false;
    Tab->SetPropertyValue_InContainer(Settings, 0);
    const bool DefaultOutline = Outline->GetPropertyValue_InContainer(GIClass->GetDefaultObject());
    const bool DefaultAimName = AimName->GetPropertyValue_InContainer(GIClass->GetDefaultObject());
    FStructProperty* ActiveColor = FindFProperty<FStructProperty>(SettingsClass, TEXT("ActiveTabColor"));
    if (!TestNotNull(TEXT("Active toggle color"), ActiveColor)) return false;
    const FLinearColor ExpectedColor = *ActiveColor->ContainerPtrToValuePtr<FLinearColor>(Settings);
    for (const TCHAR* Code : {TEXT("en"), TEXT("ko")})
    {
        UKismetInternationalizationLibrary::SetCurrentCulture(Code, false);
        Outline->SetPropertyValue_InContainer(GI, !DefaultOutline);
        AimName->SetPropertyValue_InContainer(GI, !DefaultAimName);
        Reset->OnClicked.Broadcast();
        TestEqual(TEXT("Reset preserves selected language"), FInternationalization::Get().GetCurrentCulture()->GetName(), FString(Code));
        TestEqual(TEXT("Reset restores outline default"), Outline->GetPropertyValue_InContainer(GI), DefaultOutline);
        TestEqual(TEXT("Reset restores aim nickname default"), AimName->GetPropertyValue_InContainer(GI), DefaultAimName);
        for (const TCHAR* Name : {TEXT("Btn_ToggleOn_1"), TEXT("Btn_ToggleOn_2")})
        {
            UButton* OnButton = Cast<UButton>(Settings->GetWidgetFromName(Name));
            if (TestNotNull(TEXT("On button"), OnButton))
                TestEqual(TEXT("Reset refreshes active toggle highlight"), OnButton->GetBackgroundColor(), ExpectedColor);
        }
        USaveGame* Save = UGameplayStatics::LoadGameFromSlot(Slot, 0);
        if (!TestNotNull(TEXT("Reset writes settings save"), Save)) return false;
        for (const FName Name : {FName(TEXT("bShowCharacterOutline")), FName(TEXT("bShowAimNickname"))})
        {
            FBoolProperty* SavedProperty = FindFProperty<FBoolProperty>(Save->GetClass(), Name);
            FBoolProperty* SourceProperty = FindFProperty<FBoolProperty>(GIClass, Name);
            if (TestNotNull(TEXT("Saved gameplay toggle"), SavedProperty))
                TestEqual(TEXT("Saved toggle matches restored value"), SavedProperty->GetPropertyValue_InContainer(Save), SourceProperty->GetPropertyValue_InContainer(GI));
        }
        Reset->OnClicked.Broadcast();
        TestEqual(TEXT("Repeated reset preserves language"), FInternationalization::Get().GetCurrentCulture()->GetName(), FString(Code));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanguageScreenLayoutTest,
    "ShootingArena.Localization.ScreenLayoutPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLanguageScreenLayoutTest::RunTest(const FString& Parameters)
{
    FString Saved;
    const bool HadSaved = GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Saved, GGameUserSettingsIni);
    const FString Culture = FInternationalization::Get().GetCurrentCulture()->GetName();
    const bool Preview = FTextLocalizationManager::Get().IsGameLocalizationPreviewEnabled();
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    UGameInstance* GI = Context.GetTestWorld()->GetGameInstance();
    ULanguageSubsystem* Language = GI->GetSubsystem<ULanguageSubsystem>();
    if (!TestNotNull(TEXT("Preview language subsystem"), Language)) return false;
    const FString OriginalCode = Language->GetCurrentLanguageCode();
    ON_SCOPE_EXIT
    {
        Language->SetLanguage(OriginalCode);
        if (HadSaved) GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *Saved, GGameUserSettingsIni);
        else GConfig->RemoveKey(TEXT("Internationalization"), TEXT("Culture"), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
        UKismetInternationalizationLibrary::SetCurrentCulture(Culture, false);
        if (Preview) FTextLocalizationManager::Get().EnableGameLocalizationPreview(Culture);
        else FTextLocalizationManager::Get().DisableGameLocalizationPreview();
    };

    UClass* MainClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameFlow/UI/WBP_MainMenu_SDM.WBP_MainMenu_SDM_C"));
    UClass* SettingsClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameProgressFlow/WBP_SettingsMenu.WBP_SettingsMenu_C"));
    // Paint initialized designer trees, bypassing each screen's Construct.
    // No player, BeginPlay, travel, connection attempt, or media playback starts.
    auto PrepareTab = [&](UUserWidget* Settings, int32 Index)
    {
        UFunction* SelectTab = Settings->FindFunction(TEXT("SelectTab"));
        if (!TestNotNull(TEXT("Existing settings tab function"), SelectTab)) return false;
        FStructOnScope Params(SelectTab);
        FIntProperty* IndexProperty = FindFProperty<FIntProperty>(SelectTab, TEXT("Index"));
        if (!TestNotNull(TEXT("Settings tab index argument"), IndexProperty)) return false;
        IndexProperty->SetPropertyValue_InContainer(Params.GetStructMemory(), Index);
        Settings->ProcessEvent(SelectTab, Params.GetStructMemory());
        return true;
    };
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("Diagnostics/LanguageLayout");
    IFileManager::Get().MakeDirectory(*Directory, true);
    FWidgetRenderer Renderer(true);
    auto Render = [&](const TSharedRef<SWidget>& Widget, const FString& Name)
    {
        UTextureRenderTarget2D* Target = Renderer.DrawWidget(Widget, FVector2D(1920, 1080));
        FlushRenderingCommands();
        if (Target) Renderer.DrawWidget(Target, Widget, FVector2D(1920, 1080), 0.0f);
        FlushRenderingCommands();
        FBufferArchive PNG;
        TestTrue(*FString::Printf(TEXT("Preview saved: %s"), *Name), Target
            && FImageUtils::ExportRenderTarget2DAsPNG(Target, PNG)
            && FFileHelper::SaveArrayToFile(PNG, *(Directory / Name)));
    };
    for (const TCHAR* Code : {TEXT("ko"), TEXT("en")})
    {
        Language->SetLanguage(Code);
        // Fresh Slate trees avoid same-frame text/layout caches between captures.
        UUserWidget* Main = CreateWidget<UUserWidget>(GI, MainClass);
        UUserWidget* Settings = CreateWidget<UUserWidget>(GI, SettingsClass);
        if (!TestNotNull(TEXT("Actual main menu"), Main) || !TestNotNull(TEXT("Actual settings menu"), Settings)) return false;
        UWidget* MainLanguage = Main->GetWidgetFromName(TEXT("MainMenuLanguageSelector"));
        UWidget* SettingsLanguage = Settings->GetWidgetFromName(TEXT("LanguageSelector"));
        if (!TestNotNull(TEXT("Main menu shortcut asset placement"), MainLanguage)
            || !TestNotNull(TEXT("Settings language section"), SettingsLanguage)) return false;
        TestTrue(TEXT("Main menu shortcuts follow the existing vertical menu"), MainLanguage->GetParent() == Main->GetWidgetFromName(TEXT("VerticalBox_0")));
        TestTrue(TEXT("Language lives inside the settings list"), SettingsLanguage->GetParent() == Settings->GetWidgetFromName(TEXT("SB_SettingList")));
        UReusableStyleLibrary::ApplyWidgetStyleTree(Main);
        UReusableStyleLibrary::ApplyWidgetStyleTree(Settings);
        if (!PrepareTab(Settings, 0)) return false;
        Render(Main->WidgetTree->RootWidget->TakeWidget(), FString::Printf(TEXT("main-%s.png"), Code));
        Render(Settings->WidgetTree->RootWidget->TakeWidget(), FString::Printf(TEXT("settings-gameplay-%s.png"), Code));
        TestEqual(TEXT("Actual gameplay tab shows the section"), SettingsLanguage->GetVisibility(), ESlateVisibility::Visible);
        UTextBlock* Quit = Cast<UTextBlock>(Main->GetWidgetFromName(TEXT("TextBlock_3")));
        if (TestNotNull(TEXT("Main menu exit label"), Quit))
            TestEqual(TEXT("The actual menu text follows the chosen language"), Quit->GetText().ToString(), FString(Code) == TEXT("ko") ? FString(TEXT("나가기")) : FString(TEXT("Exit")));
    }
    for (int32 Index : {1, 2, 3})
    {
        UUserWidget* Settings = CreateWidget<UUserWidget>(GI, SettingsClass);
        UReusableStyleLibrary::ApplyWidgetStyleTree(Settings);
        if (!PrepareTab(Settings, Index)) return false;
        UWidget* SettingsLanguage = Settings->GetWidgetFromName(TEXT("LanguageSelector"));
        TestEqual(TEXT("Actual other tab hides the complete section"), SettingsLanguage->GetVisibility(), ESlateVisibility::Collapsed);
        if (Index == 1) Render(Settings->WidgetTree->RootWidget->TakeWidget(), TEXT("settings-control-en.png"));
    }
    return true;
}

#endif

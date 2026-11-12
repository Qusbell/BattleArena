#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Localization/LanguageSubsystem.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Kismet/KismetInternationalizationLibrary.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanguageSelectionTest,"ShootingArena.Localization.StartupSelection",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLanguageSelectionTest::RunTest(const FString&)
{
    FLanguageRow Korean;Korean.CultureCode=TEXT("ko");Korean.IsDefault=true;
    FLanguageRow English;English.CultureCode=TEXT("en");
    FLanguageRow Disabled;Disabled.CultureCode=TEXT("ja");Disabled.IsEnabled=false;
    const TArray<FLanguageRow> Rows={Korean,English,Disabled};
    auto Resolve=[&](bool Saved,const TCHAR* Value,const TCHAR* OS){return ULanguageSubsystem::ChooseStartupLanguage(Rows,Saved,Value,OS);};
    TestEqual(TEXT("Saved language wins over OS"),Resolve(true,TEXT("en"),TEXT("ko-KR")),FString(TEXT("en")));
    TestEqual(TEXT("Invalid saved language uses default, not OS"),Resolve(true,TEXT("fr"),TEXT("en-US")),FString(TEXT("ko")));
    TestEqual(TEXT("Disabled saved language uses default"),Resolve(true,TEXT("ja"),TEXT("en-US")),FString(TEXT("ko")));
    TestEqual(TEXT("Absent saved language matches OS parent"),Resolve(false,TEXT(""),TEXT("en-US")),FString(TEXT("en")));
    TestEqual(TEXT("Unsupported OS uses default"),Resolve(false,TEXT(""),TEXT("fr-FR")),FString(TEXT("ko")));
    FLanguageRow RegionalEnglish;RegionalEnglish.CultureCode=TEXT("en-GB");
    const TArray<FLanguageRow> RegionalRows={Korean,English,RegionalEnglish};
    TestEqual(TEXT("OS chooses the most specific supported parent independent of row order"),
        ULanguageSubsystem::ChooseStartupLanguage(RegionalRows,false,TEXT(""),TEXT("en-GB-x-test")),FString(TEXT("en-GB")));
    TestEqual(TEXT("Saved unsupported regional code still falls back to the default"),
        ULanguageSubsystem::ChooseStartupLanguage(RegionalRows,true,TEXT("en-GB-x-test"),TEXT("en")),FString(TEXT("ko")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLanguageHistoryTest,"ShootingArena.Localization.TextHistoryAndNickname",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLanguageHistoryTest::RunTest(const FString&)
{
    const FString Previous=FInternationalization::Get().GetCurrentCulture()->GetName();
    const FText Empty=FText::GetEmpty();
    UKismetInternationalizationLibrary::SetCurrentCulture(TEXT("ko"),false);
    const FText Text=ULanguageLibrary::FormatText(TEXT("HUD_RespawnCountdown"),FText::AsNumber(3),Empty,Empty,Empty,Empty,Empty,Empty,Empty);
    const FString Encoded=ULanguageLibrary::DisplayString(TEXT("HUD_RespawnCountdown"),FText::AsNumber(3),Empty,Empty,Empty,Empty,Empty,Empty,Empty);
    const FText Decoded=ULanguageLibrary::ReadDisplayString(Encoded);
    TestEqual(TEXT("Serialized format preserves its current display"),Decoded.ToString(),Text.ToString());
    const FString Nickname=TEXT("LOCTABLE(\"literal nickname\") 한글 Player");
    TestEqual(TEXT("Unencoded nickname is never interpreted as localization markup"),ULanguageLibrary::ReadDisplayString(Nickname).ToString(),Nickname);
    FSlateFontInfo Font;Font.Size=37;Font.OutlineSettings.OutlineSize=2;
    const FSlateFontInfo Resolved=ULanguageLibrary::ResolveFont(Font);
    TestNotNull(TEXT("Configured font is loaded"),Resolved.FontObject.Get());
    TestEqual(TEXT("Language font preserves size"),Resolved.Size,37.f);
    TestEqual(TEXT("Language font preserves outline"),Resolved.OutlineSettings.OutlineSize,2);
    UKismetInternationalizationLibrary::SetCurrentCulture(Previous,false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FParticipantDisplayNameTest,"ShootingArena.Localization.ParticipantDisplayName",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FParticipantDisplayNameTest::RunTest(const FString&)
{
    UClass* StateClass=LoadClass<APlayerState>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameMode/BP_QuakePlayerState.BP_QuakePlayerState_C"));
    if (!TestNotNull(TEXT("Actual PlayerState class"),StateClass)) return false;
    FNameProperty* Selected=FindFProperty<FNameProperty>(StateClass,TEXT("SelectedCharacterRow"));
    FStrProperty* Campaign=FindFProperty<FStrProperty>(StateClass,TEXT("CampaignName"));
    if (!TestNotNull(TEXT("Selected character property"),Selected) || !TestNotNull(TEXT("Campaign name property"),Campaign)) return false;
    const UWorld::InitializationValues Values=UWorld::InitializationValues().AllowAudioPlayback(false)
        .RequiresHitProxies(false).CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::EditorPreview,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("Transient test world without PIE or BeginPlay"),World)) return false;
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
        // This package was created only for this test; never leave a save prompt for its actors.
        World->GetOutermost()->SetDirtyFlag(false);
    };
    APlayerState* State=World->SpawnActor<APlayerState>(StateClass);
    if (!TestNotNull(TEXT("Transient participant"),State)) return false;
    Selected->SetPropertyValue_InContainer(State,TEXT("D01"));
    Campaign->SetPropertyValue_InContainer(State,TEXT("D02"));
    const FString Nickname=TEXT("한글 Player LOCTABLE(\"literal\")");
    State->SetPlayerName(Nickname);
    State->SetIsABot(false);
    const FText Human=ULanguageLibrary::ReadDisplayString(ULanguageLibrary::ParticipantDisplayString(State));
    TestEqual(TEXT("Human nickname survives selected character and campaign fields"),Human.ToString(),Nickname);
    TestTrue(TEXT("Human nickname remains culture invariant"),Human.IsCultureInvariant());
    State->SetIsABot(true);
    const FText CampaignBot=ULanguageLibrary::ReadDisplayString(ULanguageLibrary::ParticipantDisplayString(State));
    TestTrue(TEXT("Bot campaign name retains its String Table identity"),CampaignBot.IsFromStringTable());
    TestEqual(TEXT("Bot campaign ID takes priority over selected character"),CampaignBot.ToString(),
        ULanguageLibrary::DisplayName(TEXT("Character"),TEXT("D02")).ToString());
    Campaign->SetPropertyValue_InContainer(State,FString());
    const FText SelectedBot=ULanguageLibrary::ReadDisplayString(ULanguageLibrary::ParticipantDisplayString(State));
    TestTrue(TEXT("Bot selected character retains its String Table identity"),SelectedBot.IsFromStringTable());
    TestEqual(TEXT("Bot uses selected character when campaign ID is empty"),SelectedBot.ToString(),
        ULanguageLibrary::DisplayName(TEXT("Character"),TEXT("D01")).ToString());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDisplayStringRobustnessTest,"ShootingArena.Localization.MalformedDisplayString",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDisplayStringRobustnessTest::RunTest(const FString&)
{
    const FString Prefix(TEXT("\x1f" "ArenaText:"));
    const TArray<FString> Invalid={
        Prefix+TEXT("INVTEXT(\"broken\""),
        Prefix+TEXT("INVTEXT(\"ok\") trailing"),
        Prefix+TEXT("not serialized"),
        Prefix+FString::ChrN(80,TEXT('('))+TEXT("\"deep\"")+FString::ChrN(80,TEXT(')')),
        Prefix+FString::ChrN(66000,TEXT('x'))};
    for (const FString& Input : Invalid)
    {
        const FText Output=ULanguageLibrary::ReadDisplayString(Input);
        TestTrue(TEXT("Rejected input is culture invariant"),Output.IsCultureInvariant());
        TestEqual(TEXT("Malformed, excessive or trailing input is preserved literally"),Output.ToString(),Input);
    }
    TestEqual(TEXT("Valid invariant history is still decoded"),
        ULanguageLibrary::ReadDisplayString(Prefix+TEXT("INVTEXT(\"ok\")")).ToString(),FString(TEXT("ok")));
    const FText Empty=FText::GetEmpty();
    const FText Name=FText::AsCultureInvariant(TEXT("Player \\\"quoted\\\" ")+FString::ChrN(80,TEXT('(')));
    const FString Encoded=ULanguageLibrary::DisplayString(TEXT("Lobby_Joined"),Name,Empty,Empty,Empty,Empty,Empty,Empty,Empty);
    TestEqual(TEXT("Quoted nickname punctuation does not count toward parser nesting"),
        ULanguageLibrary::ReadDisplayString(Encoded).ToString(),
        ULanguageLibrary::FormatText(TEXT("Lobby_Joined"),Name,Empty,Empty,Empty,Empty,Empty,Empty,Empty).ToString());
    return true;
}
#endif

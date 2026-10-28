#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "UI/MapPreviewMedia.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "FileMediaSource.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MediaPlayer.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace MapPreviewTest
{
class FPlaybackSequence : public IAutomationLatentCommand
{
public:
    explicit FPlaybackSequence(FAutomationTestBase* InTest)
        : Test(InTest), Playback(NewObject<UMapPreviewPlayback>()),
          Card(NewObject<UMapPreviewBinding>()), Difficulty(NewObject<UMapPreviewBinding>()),
          CardImage(NewObject<UImage>()), DifficultyImage(NewObject<UImage>()),
          Button(NewObject<UButton>()), Still(NewObject<UTexture2D>())
    {
        UFileMediaSource* Source = NewObject<UFileMediaSource>();
        Source->SetFilePath(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("Movies/MainMenuPreview.mp4")));
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/QuakeLike_1_0/GameFlow/UI/MapPreviewMedia/M_MapPreviewVideo.M_MapPreviewVideo"));
        if (!Test->TestNotNull(TEXT("Preview UI material exists"), Material))
        {
            bFailed = true;
            return;
        }
        Playback->Initialize(Source, Material);
        CardImage->SetBrushFromTexture(Still.Get());
        DifficultyImage->SetBrushFromTexture(Still.Get());
        Card->Initialize(nullptr, CardImage.Get(), Button.Get(), Playback.Get(), false);
        Test->TestFalse(TEXT("Normal card does not start a video"), Card->GetMediaPlayer()->IsPreparing());
        Test->TestTrue(TEXT("Normal card uses its separate still image"), CardImage->GetBrush().GetResourceObject() == Still.Get());
        Button->OnHovered.Broadcast();
        Started = FPlatformTime::Seconds();
        StageStarted = Started;
    }

    virtual bool Update() override
    {
        if (bFailed)
        {
            return true;
        }
        const double Now = FPlatformTime::Seconds();
        UMediaPlayer* Player = Playback->GetPlayer();
        if (Now - Started > 20.0)
        {
            Test->AddError(FString::Printf(TEXT("Map preview sequence timed out at stage %d"), Stage));
            Finish();
            return true;
        }
        if (Stage == 0 && Player->IsPlaying() && Player->GetTime().GetTotalSeconds() > 0.3)
        {
            Test->TestTrue(TEXT("Hover displays video material"),
                CardImage->GetBrush().GetResourceObject() == Playback->GetVideoMaterial());
            Button->OnUnhovered.Broadcast();
            PausedTime = Player->GetTime();
            Test->TestTrue(TEXT("Mouse leave restores original image"), CardImage->GetBrush().GetResourceObject() == Still.Get());
            Stage = 1;
            StageStarted = Now;
        }
        else if (Stage == 1 && Now - StageStarted > 0.4)
        {
            Test->TestTrue(TEXT("Video position stays paused off the card"),
                FMath::Abs((Player->GetTime() - PausedTime).GetTotalSeconds()) < 0.08);
            Button->OnHovered.Broadcast();
            Test->TestTrue(TEXT("Repeated hover resumes the same player"), Card->GetMediaPlayer() == Player);
            Stage = 2;
        }
        else if (Stage == 2 && Player->IsPlaying() && Player->GetTime() > PausedTime + FTimespan::FromSeconds(0.25))
        {
            const FTimespan BeforeHandoff = Player->GetTime();
            Difficulty->Initialize(nullptr, DifficultyImage.Get(), nullptr, Playback.Get(), true);
            Card->Release();
            Test->TestTrue(TEXT("Difficulty inherits the same decoder and timeline"), Difficulty->GetMediaPlayer() == Player);
            Test->TestTrue(TEXT("Removing map selection does not pause difficulty"), Player->IsPlaying());
            Test->TestTrue(TEXT("Handoff does not reset playback position"), Player->GetTime() >= BeforeHandoff);
            PausedTime = BeforeHandoff;
            Stage = 3;
            StageStarted = Now;
        }
        else if (Stage == 3 && Now - StageStarted > 0.4)
        {
            Difficulty->SetPreviewActive(false);
            Test->TestTrue(TEXT("Difficulty ignores hover state and keeps playing"), Player->IsPlaying());
            Test->TestTrue(TEXT("Difficulty playback advances after handoff"), Player->GetTime() > PausedTime);
            Test->TestTrue(TEXT("Difficulty displays video"), DifficultyImage->GetBrush().GetResourceObject() == Playback->GetVideoMaterial());
            Difficulty->Release();
            Stage = 4;
            StageStarted = Now;
        }
        else if (Stage == 4 && Now - StageStarted > 0.4)
        {
            Test->TestFalse(TEXT("Last screen release pauses media after the asynchronous rate change"), Player->IsPlaying());
            Finish();
            return true;
        }
        return false;
    }

private:
    void Finish()
    {
        Card->Release();
        Difficulty->Release();
        Playback->Shutdown();
        Test->TestNull(TEXT("Shutdown releases the media player reference"), Playback->GetPlayer());
        Test->TestNull(TEXT("Shutdown releases the dynamic video material"), Playback->GetVideoMaterial());
        Test->TestNull(TEXT("Shutdown releases the media source reference"), Playback->GetSource());
    }
    FAutomationTestBase* Test;
    TStrongObjectPtr<UMapPreviewPlayback> Playback;
    TStrongObjectPtr<UMapPreviewBinding> Card;
    TStrongObjectPtr<UMapPreviewBinding> Difficulty;
    TStrongObjectPtr<UImage> CardImage;
    TStrongObjectPtr<UImage> DifficultyImage;
    TStrongObjectPtr<UButton> Button;
    TStrongObjectPtr<UTexture2D> Still;
    FTimespan PausedTime;
    double Started = 0;
    double StageStarted = 0;
    int32 Stage = 0;
    bool bFailed = false;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapPreviewMediaPlaybackTest, "ShootingArena.UI.MapPreview.HoverResumeAndHandoff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapPreviewMediaPlaybackTest::RunTest(const FString& Parameters)
{
    if (!FPaths::FileExists(FPaths::ProjectContentDir() / TEXT("Movies/MainMenuPreview.mp4")))
    {
        AddWarning(TEXT("Media smoke test requires Content/Movies/MainMenuPreview.mp4."));
        return true;
    }
    ADD_LATENT_AUTOMATION_COMMAND(MapPreviewTest::FPlaybackSequence(this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapPreviewTravelCleanupTest, "ShootingArena.UI.MapPreview.TravelCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMapPreviewTravelCleanupTest::RunTest(const FString& Parameters)
{
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
    TStrongObjectPtr<UGameInstance> OtherInstance(NewObject<UGameInstance>());
    TStrongObjectPtr<UMapPreviewSubsystem> Subsystem(NewObject<UMapPreviewSubsystem>(Instance.Get()));
    UClass* ScreenClass = LoadClass<UUserWidget>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameFlow/UI/WBP_DifficultySelect.WBP_DifficultySelect_C"));
    if (!TestNotNull(TEXT("Concrete menu widget class"), ScreenClass)) return false;
    TStrongObjectPtr<UUserWidget> Screen(NewObject<UUserWidget>(Instance.Get(), ScreenClass));
    TStrongObjectPtr<UImage> Image(NewObject<UImage>());
    TStrongObjectPtr<UButton> Button(NewObject<UButton>());
    TStrongObjectPtr<UDataTable> Table(NewObject<UDataTable>());
    Table->RowStruct = FMapPreviewMediaRow::StaticStruct();
    FMapPreviewMediaRow Row;
    Row.PreviewVideo = NewObject<UFileMediaSource>();
    Table->AddRow(TEXT("Map_03"), Row);
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/QuakeLike_1_0/GameFlow/UI/MapPreviewMedia/M_MapPreviewVideo.M_MapPreviewVideo"));
    if (!TestNotNull(TEXT("Video material"), Material)) return false;
    UMapPreviewBinding* Binding = Subsystem->Configure(Screen.Get(), Image.Get(), Button.Get(),
        TEXT("Map_03"), Table.Get(), Material, false);
    if (!TestNotNull(TEXT("Menu preview binding"), Binding)) return false;
    UMediaPlayer* OriginalPlayer = Binding->GetMediaPlayer();
    if (!TestNotNull(TEXT("Allocated preview player"), OriginalPlayer)) return false;
    UMapPreviewPlayback* Playback = Cast<UMapPreviewPlayback>(OriginalPlayer->GetOuter());
    TestTrue(TEXT("Hover events bound before travel"), Button->OnHovered.IsBound());

    FWorldContext OtherContext;
    OtherContext.WorldType = EWorldType::Game;
    OtherContext.OwningGameInstance = OtherInstance.Get();
    Subsystem->HandlePreLoadMap(OtherContext, TEXT("/Game/QuakeLike_Base/Lvl_PortalMap"));
    TestTrue(TEXT("Another PIE client's travel preserves this menu's playback"), Binding->GetMediaPlayer() == OriginalPlayer);

    FWorldContext OwnContext;
    OwnContext.WorldType = EWorldType::Game;
    OwnContext.OwningGameInstance = Instance.Get();
    Subsystem->HandlePreLoadMap(OwnContext, TEXT("/Game/QuakeLike_Base/Lvl_PortalMap"));
    TestNull(TEXT("Game entry detaches widget playback"), Binding->GetMediaPlayer());
    TestNull(TEXT("Game entry releases decoder ownership"), Playback->GetPlayer());
    TestNull(TEXT("Game entry releases material ownership"), Playback->GetVideoMaterial());
    TestNull(TEXT("Game entry releases source ownership"), Playback->GetSource());
    TestTrue(TEXT("Game entry clears the map playback cache"), Subsystem->Playbacks.IsEmpty());
    TestTrue(TEXT("Game entry clears widget bindings"), Subsystem->Bindings.IsEmpty());
    TestFalse(TEXT("Game entry unbinds hover callbacks"), Button->OnHovered.IsBound());
    Binding->SetPreviewActive(true);
    TestNull(TEXT("Stale hover cannot reopen a released player"), Binding->GetMediaPlayer());

    UMapPreviewBinding* NewBinding = Subsystem->Configure(Screen.Get(), Image.Get(), Button.Get(),
        TEXT("Map_03"), Table.Get(), Material, false);
    TestNotNull(TEXT("Returning to the menu recreates playback"), NewBinding);
    if (NewBinding)
    {
        TestTrue(TEXT("Recreated menu playback uses a fresh decoder"), NewBinding->GetMediaPlayer() != OriginalPlayer);
    }
    Subsystem->ReleaseAllPreviews();
    return true;
}

#endif

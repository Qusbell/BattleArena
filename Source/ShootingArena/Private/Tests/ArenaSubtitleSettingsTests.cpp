#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "UI/ArenaGameViewportClient.h"
#include "UI/BuffDurationRing.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/BufferArchive.h"
#include "SubtitleManager.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaSubtitleViewportConfigurationTest, "ShootingArena.UI.SubtitleViewportConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArenaSubtitleViewportConfigurationTest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Engine is available"), GEngine)) return false;
	UClass* ViewportClass = GEngine->GameViewportClientClass.Get();
	if (!TestNotNull(TEXT("Engine loaded the configured viewport class"), ViewportClass)) return false;
	if (!TestTrue(TEXT("Configured viewport applies the subtitle settings"),
		ViewportClass->IsChildOf(UArenaGameViewportClient::StaticClass()))) return false;
	const auto* Viewport = ViewportClass->GetDefaultObject<UArenaGameViewportClient>();
	TestTrue(TEXT("CommonUI input routing is retained"), ViewportClass->IsChildOf(UCommonGameViewportClient::StaticClass()));
	if (!TestNotNull(TEXT("Configured viewport loads the saved subtitle settings"), Viewport->SubtitleSettings.Get())) return false;
	TestEqual(TEXT("Viewport uses the existing HUD settings asset"), Viewport->SubtitleSettings->GetPathName(),
		FString(TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing")));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaSubtitleSettingsTest, "ShootingArena.UI.SubtitleSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FArenaSubtitleSettingsTest::RunTest(const FString& Parameters)
{
	FSubtitleManager* Manager = FSubtitleManager::GetSubtitleManager();
	if (!TestFalse(TEXT("No foreign subtitle renderer is replaced by this fixture"), Manager->OnSetSubtitleText().IsBound())) return false;
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	UGameInstance* GI = World->GetGameInstance();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
	auto* Viewport = NewObject<UArenaGameViewportClient>(GEngine);
	FWorldContext& WorldContext = GEngine->GetWorldContextFromWorldChecked(World);
	UGameViewportClient* PreviousViewport = WorldContext.GameViewport;
	WorldContext.GameViewport = Viewport;
	ON_SCOPE_EXIT
	{
		WorldContext.GameViewport = PreviousViewport;
		GI->RemoveLocalPlayer(Player);
	};
	if (!TestEqual(TEXT("Subtitle fixture has one local player"), GEngine->GetNumGamePlayers(Viewport), 1)) return false;
	if (!TestNotNull(TEXT("Viewport holds the saved HUD settings DA"), Viewport->SubtitleSettings.Get())) return false;
	Viewport->SubtitleSettings = DuplicateObject<UBuffDurationRingSettings>(Viewport->SubtitleSettings, GetTransientPackage());
	auto* Style = Viewport->SubtitleSettings.Get();
	Style->bShowSubtitles = true;
	const bool bEnabled = GEngine->bSubtitlesEnabled, bForcedOff = GEngine->bSubtitlesForcedOff;
	GEngine->bSubtitlesEnabled = true; GEngine->bSubtitlesForcedOff = false;
	const PTRINT SubtitleID = reinterpret_cast<PTRINT>(Viewport);
	FText Observed;
	const FDelegateHandle DisplayHandle = Manager->OnSetSubtitleText().AddUObject(Viewport, &UArenaGameViewportClient::DrawSubtitleText);
	const FDelegateHandle ObserveHandle = Manager->OnSetSubtitleText().AddLambda([&](const FText& Text) { Observed = Text; });
	ON_SCOPE_EXIT
	{
		Manager->OnSetSubtitleText().Remove(DisplayHandle);
		Manager->OnSetSubtitleText().Remove(ObserveHandle);
		Viewport->SubtitleCanvas = nullptr;
		GEngine->bSubtitlesEnabled = bEnabled; GEngine->bSubtitlesForcedOff = bForcedOff;
	};
	FSubtitleCue First; First.Text = FText::FromString(TEXT("BUFF 12")); First.Time = 0.0f;
	FSubtitleCue Second; Second.Text = FText::FromString(TEXT("READY")); Second.Time = 2.0f;
	Manager->QueueSubtitles(SubtitleID, 100.0f, true, true, 4.0f, {First, Second}, 0.0f, 0.0f);
	const FString Dir = FPaths::ProjectSavedDir() / TEXT("Automation/Subtitles");
	IFileManager::Get().MakeDirectory(*Dir, true);
	auto Capture = [&](const TCHAR* Name, float Time)
	{
		UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>();
		Target->RenderTargetFormat = RTF_RGBA8; Target->ClearColor = FLinearColor::Black;
		Target->InitAutoFormat(640, 480); Target->UpdateResourceImmediate(true);
		FTextureRenderTargetResource* Resource = Target->GameThread_GetRenderTargetResource();
		FCanvas Canvas(Resource, nullptr, static_cast<UWorld*>(nullptr), GMaxRHIFeatureLevel);
		Viewport->SubtitleCanvas = &Canvas;
		FIntRect Region(0, 0, 640, 480);
		Manager->DisplaySubtitles(&Canvas, Region, Time);
		Canvas.Flush_GameThread(); FlushRenderingCommands();
		Viewport->SubtitleCanvas = nullptr;
		TArray<FColor> Pixels;
		TestTrue(TEXT("Subtitle GPU pixels are readable"), Resource->ReadPixels(Pixels));
		FIntRect Bounds(640, 480, 0, 0);
		for (int32 I = 0; I < Pixels.Num(); ++I)
		{
			if (Pixels[I].R < 40 || Pixels[I].G < 40 || Pixels[I].B < 40) continue;
			Bounds.Min.X = FMath::Min(Bounds.Min.X, I % 640); Bounds.Max.X = FMath::Max(Bounds.Max.X, I % 640 + 1);
			Bounds.Min.Y = FMath::Min(Bounds.Min.Y, I / 640); Bounds.Max.Y = FMath::Max(Bounds.Max.Y, I / 640 + 1);
		}
		FBufferArchive PNG;
		if (TestTrue(TEXT("Caption preview can be encoded"), FImageUtils::ExportRenderTarget2DAsPNG(Target, PNG)))
			TestTrue(TEXT("Real Canvas caption preview saved"), FFileHelper::SaveArrayToFile(PNG, *(Dir / Name)));
		AddInfo(FString::Printf(TEXT("%s: glyph bounds (%d,%d)-(%d,%d)"), Name,
			Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y));
		return Bounds;
	};
	Style->SubtitlePosition = FVector2D(0.3, 0.35); Style->SubtitleTextScale = 1.0f;
	const FIntRect Left = Capture(TEXT("X030.png"), 1.0f);
	TestTrue(TEXT("Engine sound subtitle text reaches the renderer"), Observed.EqualTo(First.Text));
	TestTrue(TEXT("Caption draws visible glyphs"), Left.Width() > 20 && Left.Height() > 5);
	Style->SubtitlePosition.X = 0.7;
	const FIntRect Right = Capture(TEXT("X070.png"), 1.0f);
	TestTrue(TEXT("X alone moves glyphs by 40 percent of screen width"), FMath::Abs((Right.Min.X - Left.Min.X) - 256) <= 2);
	TestEqual(TEXT("X preserves vertical placement"), Right.Min.Y, Left.Min.Y);
	TestEqual(TEXT("X preserves glyph width"), Right.Width(), Left.Width());
	Style->SubtitlePosition = FVector2D(0.5, 0.35);
	const FIntRect Upper = Capture(TEXT("Y035.png"), 1.0f);
	Style->SubtitlePosition.Y = 0.75;
	const FIntRect Lower = Capture(TEXT("Y075.png"), 1.0f);
	TestTrue(TEXT("Y alone moves glyphs by 40 percent of screen height"), FMath::Abs((Lower.Min.Y - Upper.Min.Y) - 192) <= 2);
	TestEqual(TEXT("Y preserves horizontal placement"), Lower.Min.X, Upper.Min.X);
	TestEqual(TEXT("Y preserves glyph height"), Lower.Height(), Upper.Height());
	Style->SubtitleTextScale = 0.75f;
	const FIntRect Small = Capture(TEXT("Scale075.png"), 1.0f);
	Style->SubtitleTextScale = 1.5f;
	const FIntRect Requested = Capture(TEXT("X050_Y075_Scale150.png"), 1.0f);
	TestTrue(TEXT("Scale alone increases actual glyph height"), Requested.Height() > Small.Height() * 1.7f);
	TestTrue(TEXT("Scale alone increases actual glyph width"), Requested.Width() > Small.Width() * 1.7f);
	TestTrue(TEXT("Requested X centers the caption"), FMath::Abs((Requested.Min.X + Requested.Max.X) / 2 - 320) <= 4);
	TestTrue(TEXT("Requested Y places the caption near the 75 percent baseline"), Requested.Max.Y <= 360 && Requested.Max.Y >= 330);
	FVector2D MinPos(0.0, 0.0), MaxPos(1.0, 1.0);
	Viewport->GetSubtitleRegion(MinPos, MaxPos);
	TestTrue(TEXT("One-player region allows Y=0.75"), FMath::IsNearlyEqual(MaxPos.Y, 0.75));
	// Add the second player without invoking layout on this renderer-only viewport.
	WorldContext.GameViewport = PreviousViewport;
	ULocalPlayer* SecondPlayer = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(SecondPlayer, FPlatformMisc::GetPlatformUserForUserIndex(1));
	WorldContext.GameViewport = Viewport;
	Viewport->GetSubtitleRegion(MinPos, MaxPos);
	TestTrue(TEXT("Split-screen keeps the engine's upper-half subtitle region"), FMath::IsNearlyEqual(MaxPos.Y, 0.5));
	const FIntRect Split = Capture(TEXT("SplitScreen.png"), 1.0f);
	TestTrue(TEXT("Split-screen rendering respects the upper-half limit"), Split.Max.Y <= 240 && Split.Height() > 5);
	WorldContext.GameViewport = PreviousViewport;
	GI->RemoveLocalPlayer(SecondPlayer);
	WorldContext.GameViewport = Viewport;
	Style->bShowSubtitles = false;
	const FIntRect HiddenByCheckbox = Capture(TEXT("CheckboxHidden.png"), 1.0f);
	TestTrue(TEXT("DA checkbox hides caption glyphs only"), HiddenByCheckbox.Width() < 0);
	TestTrue(TEXT("Hidden caption still follows the engine's active sound cue"), Observed.EqualTo(First.Text));
	Style->bShowSubtitles = true;
	const FIntRect ShownByCheckbox = Capture(TEXT("CheckboxShown.png"), 1.0f);
	TestTrue(TEXT("Re-enabling the checkbox restores the active caption"), ShownByCheckbox == Requested);
	GEngine->bSubtitlesEnabled = false;
	const FIntRect Disabled = Capture(TEXT("Disabled.png"), 1.0f);
	TestTrue(TEXT("Disabled subtitles stay invisible"), Disabled.Width() < 0);
	GEngine->bSubtitlesEnabled = true; GEngine->bSubtitlesForcedOff = true;
	const FIntRect ForcedOff = Capture(TEXT("ForcedOff.png"), 1.0f);
	TestTrue(TEXT("Forced-off subtitles stay invisible"), ForcedOff.Width() < 0);
	GEngine->bSubtitlesForcedOff = false;
	Capture(TEXT("NextCue.png"), 2.5f);
	TestTrue(TEXT("Engine advances to the next original sound cue"), Observed.EqualTo(Second.Text));
	const FIntRect Expired = Capture(TEXT("Expired.png"), 5.0f);
	TestTrue(TEXT("Engine expiry clears text without a new timer"), Observed.IsEmpty() && Expired.Width() < 0);
	Capture(TEXT("ExpiredCleanup.png"), 5.1f);
	return !HasAnyErrors();
}

#endif

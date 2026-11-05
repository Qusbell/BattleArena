#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "UI/ArenaGameViewportClient.h"
#include "UI/BuffDurationRing.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/BufferArchive.h"
#include "SubtitleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaSubtitleSettingsTest, "ShootingArena.UI.SubtitleSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FArenaSubtitleSettingsTest::RunTest(const FString& Parameters)
{
	auto* Viewport = NewObject<UArenaGameViewportClient>(GEngine);
	if (!TestNotNull(TEXT("Viewport holds the saved HUD settings DA"), Viewport->SubtitleSettings.Get())) return false;
	Viewport->SubtitleSettings = DuplicateObject<UBuffDurationRingSettings>(Viewport->SubtitleSettings, GetTransientPackage());
	auto* Style = Viewport->SubtitleSettings.Get();
	const bool bEnabled = GEngine->bSubtitlesEnabled, bForcedOff = GEngine->bSubtitlesForcedOff;
	GEngine->bSubtitlesEnabled = true; GEngine->bSubtitlesForcedOff = false;
	FSubtitleManager* Manager = FSubtitleManager::GetSubtitleManager();
	if (!TestFalse(TEXT("No foreign subtitle renderer is replaced by this fixture"), Manager->OnSetSubtitleText().IsBound())) return false;
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
	Manager->QueueSubtitles(0x42554646, 100.0f, true, true, 4.0f, {First, Second}, 0.0f, 0.0f);
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
		if (FImageUtils::ExportRenderTarget2DAsPNG(Target, PNG))
			TestTrue(TEXT("Real Canvas caption preview saved"), FFileHelper::SaveArrayToFile(PNG, *(Dir / Name)));
		return Bounds;
	};
	Style->SubtitlePosition = FVector2D(0.5, 0.4); Style->SubtitleTextScale = 1.0f;
	const FIntRect Normal = Capture(TEXT("DefaultScale.png"), 1.0f);
	TestTrue(TEXT("Engine sound subtitle text reaches the renderer"), Observed.EqualTo(First.Text));
	TestTrue(TEXT("Caption draws visible glyphs"), Normal.Width() > 20 && Normal.Height() > 5);
	Style->SubtitlePosition = FVector2D(0.3, 0.2); Style->SubtitleTextScale = 1.5f;
	const FIntRect Edited = Capture(TEXT("DA_Left_Up_Scale150.png"), 1.0f);
	TestTrue(TEXT("DA X moves subtitle left"), Edited.Min.X < Normal.Min.X - 50);
	TestTrue(TEXT("DA Y moves subtitle up"), Edited.Min.Y < Normal.Min.Y - 50);
	TestTrue(TEXT("DA scale increases actual glyph height"), Edited.Height() > Normal.Height() * 1.3f);
	TestTrue(TEXT("DA scale increases actual glyph width"), Edited.Width() > Normal.Width() * 1.3f);
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

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/UIRecordingSettings.h"
#include "UI/ReusableStyleHost.h"
#include "UObject/UnrealType.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "RenderingThread.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRecordingSettingsTest, "ShootingArena.UI.RecordingSettings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRecordingSettingsTest::RunTest(const FString&)
{
	UUIRecordingSettings* Settings = NewObject<UUIRecordingSettings>();
	const UEnum* Sections = StaticEnum<ERecordingUISection>();
	for (int32 Index = 0; Index < Sections->NumEnums() - 1; ++Index)
	{
		const auto Section = static_cast<ERecordingUISection>(Sections->GetValueByIndex(Index));
		FBoolProperty* Flag = FindFProperty<FBoolProperty>(Settings->GetClass(), *Sections->GetNameStringByIndex(Index));
		if (!TestNotNull(TEXT("Every section has its own DA checkbox"), Flag)) return false;
		TestTrue(TEXT("Existing UI is visible by default"), Settings->IsShown(Section));
		Flag->SetPropertyValue_InContainer(Settings, false);
		for (int32 Other = 0; Other < Sections->NumEnums() - 1; ++Other)
			TestEqual(TEXT("Disabling one checkbox affects only that section"),
				Settings->IsShown(static_cast<ERecordingUISection>(Sections->GetValueByIndex(Other))), Other != Index);
		Flag->SetPropertyValue_InContainer(Settings, true);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRecordingPaintTest, "ShootingArena.UI.RecordingPaintMask",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRecordingPaintTest::RunTest(const FString&)
{
	UUIRecordingSettings* Settings = UUIRecordingSettings::Get();
	UClass* OutlineStyle = LoadClass<UBlueprintBrushStyle>(nullptr,
		TEXT("/Game/QuakeLike_1_0/GameFlow/UI/02_Styles/06_WidgetStyles/04_WS_Common/WS_Common_LocalOutline.WS_Common_LocalOutline_C"));
	if (!TestNotNull(TEXT("Recording profile is installed"), Settings)
		|| !TestNotNull(TEXT("Actual scoreboard local-row outline style"), OutlineStyle)) return false;
	TGuardValue<bool> Restore(Settings->Scoreboard, true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	URecordingUIMask* Mask = NewObject<URecordingUIMask>(World);
	Mask->Section = ERecordingUISection::Scoreboard;
	UBorder* Child = NewObject<UBorder>(World);
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Child->SetBrush(Brush);
	Child->SetRenderOpacity(0.35f);
	Mask->SetContent(Child);
	TSharedRef<SWidget> Slate = Mask->TakeWidget();
	FWidgetRenderer Renderer(false, true);
	auto Painted = [&]()
	{
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(32, 32));
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
		return Pixels.ContainsByPredicate([](const FColor& Pixel) { return Pixel.A > 0; });
	};
	TestTrue(TEXT("Enabled section draws its content"), Painted());
	Settings->Scoreboard = false;
	TestFalse(TEXT("Disabled section draws no pixels"), Painted());
	TestEqual(TEXT("Child visibility is preserved"), Child->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Child animation opacity is preserved"), Child->GetRenderOpacity(), 0.35f);
	Settings->Scoreboard = true;
	TestTrue(TEXT("Re-enabling the same widget restores its drawing"), Painted());

	UImage* Outline = NewObject<UImage>(World);
	Outline->SetBrush(OutlineStyle->GetDefaultObject<UBlueprintBrushStyle>()->Brush);
	Outline->SetRenderOpacity(0.0f);
	Mask->SetContent(Outline);
	TestTrue(TEXT("The scoreboard outline draws even when its widget opacity is zero"), Painted());
	Settings->Scoreboard = false;
	TestFalse(TEXT("Disabling the scoreboard also clips the independent outline"), Painted());
	Settings->Scoreboard = true;
	TestTrue(TEXT("The outline draws again when the scoreboard is enabled"), Painted());
	Mask->ReleaseSlateResources(true);
	World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUIRecordingRespawnProgressTest, "ShootingArena.UI.RecordingRespawnProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUIRecordingRespawnProgressTest::RunTest(const FString&)
{
	UUIRecordingSettings* Settings = UUIRecordingSettings::Get();
	UMaterialInterface* Fill = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/QuakeLike_1_0/UI/DeathUI/Materials/M_RespawnFillGradient.M_RespawnFillGradient"));
	UTexture2D* Background = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/CompShooterHUD/Textures/Healthbar/T_ProgressBase_336x24.T_ProgressBase_336x24"));
	if (!TestNotNull(TEXT("Recording profile"), Settings) || !TestNotNull(TEXT("Actual respawn fill material"), Fill)
		|| !TestNotNull(TEXT("Actual respawn background texture"), Background)) return false;
	TGuardValue<bool> Restore(Settings->DeathScreen, true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	URecordingUIMask* Mask = NewObject<URecordingUIMask>(World);
	Mask->Section = ERecordingUISection::DeathScreen;
	UProgressBar* Bar = NewObject<UProgressBar>(World);
	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.BackgroundImage.SetResourceObject(Background);
	Style.BackgroundImage.DrawAs = ESlateBrushDrawType::Image;
	Style.FillImage.SetResourceObject(Fill);
	Style.FillImage.DrawAs = ESlateBrushDrawType::Image;
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(FLinearColor::White);
	Mask->SetContent(Bar);
	TSharedRef<SWidget> Slate = Mask->TakeWidget();
	FWidgetRenderer Renderer(false, true);
	auto Painted = [&]()
	{
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(336, 24));
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
		return Pixels.ContainsByPredicate([](const FColor& Pixel) { return Pixel.A > 0; });
	};
	for (const float Percent : {0.0f, 0.5f, 1.0f})
	{
		Bar->SetPercent(Percent);
		Settings->DeathScreen = true;
		TestTrue(TEXT("Respawn bar is visible when enabled"), Painted());
		Settings->DeathScreen = false;
		TestFalse(TEXT("Neither background nor material fill remains when hidden"), Painted());
		TestEqual(TEXT("Recording toggle preserves countdown progress"), Bar->GetPercent(), Percent);
		Settings->DeathScreen = true;
		TestTrue(TEXT("Respawn bar can be shown again"), Painted());
	}
	Mask->ReleaseSlateResources(true);
	World->DestroyWorld(false);
	return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/UIRecordingSettings.h"
#include "UObject/UnrealType.h"
#include "Components/Border.h"
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
	if (!TestNotNull(TEXT("Recording profile is installed"), Settings)) return false;
	TGuardValue<bool> Restore(Settings->Menus, true);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	URecordingUIMask* Mask = NewObject<URecordingUIMask>(World);
	Mask->Section = ERecordingUISection::Menus;
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
	Settings->Menus = false;
	TestFalse(TEXT("Disabled section draws no pixels"), Painted());
	TestEqual(TEXT("Child visibility is preserved"), Child->GetVisibility(), ESlateVisibility::Visible);
	TestEqual(TEXT("Child animation opacity is preserved"), Child->GetRenderOpacity(), 0.35f);
	Settings->Menus = true;
	TestTrue(TEXT("Re-enabling the same widget restores its drawing"), Painted());
	Mask->ReleaseSlateResources(true);
	World->DestroyWorld(false);
	return true;
}
#endif

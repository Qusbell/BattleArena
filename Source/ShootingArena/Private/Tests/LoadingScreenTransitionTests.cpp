#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Loading/LoadingScreenSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SNullWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoadingScreenTransitionSafetyTest,
	"ShootingArena.Loading.TransitionSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLoadingScreenTransitionSafetyTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
	TStrongObjectPtr<UGameInstance> OtherInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<ULoadingScreenSubsystem> Loading(NewObject<ULoadingScreenSubsystem>(Instance.Get()));
	TStrongObjectPtr<UWorld> OwnWorld(NewObject<UWorld>());
	TStrongObjectPtr<UWorld> OtherWorld(NewObject<UWorld>());
	OwnWorld->SetGameInstance(Instance.Get());
	OtherWorld->SetGameInstance(OtherInstance.Get());

	Loading->bSkipStartupOverlay = true;
	Loading->HandlePostLoadMap(OtherWorld.Get());
	TestTrue(TEXT("Another client's load does not consume this instance's startup skip"), Loading->bSkipStartupOverlay);
	Loading->HandlePostLoadMap(nullptr);
	TestTrue(TEXT("A failed map callback does not consume the startup skip"), Loading->bSkipStartupOverlay);
	Loading->HandlePostLoadMap(OwnWorld.Get());
	TestFalse(TEXT("The owning world's load consumes the startup skip"), Loading->bSkipStartupOverlay);

	// Supply an existing overlay without creating a viewport, travelling, or
	// executing the widget's Construct/BeginPlay logic.
	Loading->LoadingOverlay = SNullWidget::NullWidget;
	Loading->bOverlayAddedToViewport = true;
	Loading->bTransitionActive = true;
	Loading->HideLoadingScreen();
	TestTrue(TEXT("Completing the old transition starts its fade-out"), Loading->bFadingOut);
	Loading->BeginExternalLoading(TEXT("New transition"));
	TestFalse(TEXT("A new transition cancels the old fade-out"), Loading->bFadingOut);
	Loading->Tick(0.5f);
	TestTrue(TEXT("The old fade cannot remove the new loading screen"), Loading->IsLoadingScreenActive());
	TestTrue(TEXT("The new external transition remains active"), Loading->bExternalLoadingRequested && Loading->bTransitionActive);
	Loading->RemoveLoadingScreen(false);
	TestFalse(TEXT("Explicit cleanup releases the overlay"), Loading->IsLoadingScreenActive());
	return true;
}

#endif

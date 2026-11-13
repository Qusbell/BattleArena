#if WITH_DEV_AUTOMATION_TESTS

#include "UI/RespawnCountdownLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnCountdownStartTest, "ShootingArena.UI.RespawnCountdownStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRespawnCountdownStartTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	UClass* Class = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/QuakeLike_1_0/UI/DeathUI/WBP_DeathScreen.WBP_DeathScreen_C"));
	if (!TestNotNull(TEXT("Existing death screen"), Class)) return false;
	UUserWidget* Widget = CreateWidget<UUserWidget>(World, Class);
	if (!TestNotNull(TEXT("Death screen instance"), Widget)) return false;
	auto* Current = FindFProperty<FNumericProperty>(Class, TEXT("CurrentRespawnTime"));
	auto* Timer = FindFProperty<FStructProperty>(Class, TEXT("RespawnTimerHandle"));
	auto* Bar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("PB_RespawnBar")));
	if (!TestNotNull(TEXT("Remaining time"), Current) || !TestNotNull(TEXT("Timer field"), Timer)
		|| !TestNotNull(TEXT("Respawn progress bar"), Bar)) return false;
	auto& Handle = *Timer->ContainerPtrToValuePtr<FTimerHandle>(Widget);
	auto Remaining = [&] { return Current->GetFloatingPointPropertyValue(Current->ContainerPtrToValuePtr<void>(Widget)); };

	TestTrue(TEXT("Countdown initializes"), URespawnCountdownLibrary::StartRespawnCountdown(Widget, 2.4));
	TestEqual(TEXT("Uses the supplied duration"), Remaining(), 2.4);
	TestEqual(TEXT("Initial display is empty"), Bar->GetPercent(), 0.f);
	TestTrue(TEXT("Timer is scheduled"), World->GetTimerManager().TimerExists(Handle));
	Current->SetFloatingPointPropertyValue(Current->ContainerPtrToValuePtr<void>(Widget), 1.2);
	Widget->ProcessEvent(Widget->FindFunctionChecked(TEXT("RefreshRespawnDisplay")), nullptr);
	TestEqual(TEXT("Half the wait fills half the bar"), Bar->GetPercent(), 0.5f);
	Current->SetFloatingPointPropertyValue(Current->ContainerPtrToValuePtr<void>(Widget), 0.0);
	Widget->ProcessEvent(Widget->FindFunctionChecked(TEXT("RefreshRespawnDisplay")), nullptr);
	TestEqual(TEXT("Finished wait fills the bar"), Bar->GetPercent(), 1.f);
	const FTimerHandle Previous = Handle;
	TestTrue(TEXT("An active countdown can restart"), URespawnCountdownLibrary::StartRespawnCountdown(Widget, 1.2));
	TestFalse(TEXT("Restart removes the previous timer"), World->GetTimerManager().TimerExists(Previous));
	TestEqual(TEXT("Restart resets remaining time"), Remaining(), 1.2);
	TestEqual(TEXT("Restart empties the bar"), Bar->GetPercent(), 0.f);

	TestTrue(TEXT("Zero duration is handled"), URespawnCountdownLibrary::StartRespawnCountdown(Widget, 0.0));
	TestEqual(TEXT("Zero duration empties the bar"), Bar->GetPercent(), 0.f);
	TestEqual(TEXT("Zero duration hides the screen"), Widget->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("Zero duration has no timer"), World->GetTimerManager().TimerExists(Handle));
	TestTrue(TEXT("Negative duration is handled"), URespawnCountdownLibrary::StartRespawnCountdown(Widget, -1.0));
	TestEqual(TEXT("Negative duration is clamped"), Remaining(), 0.0);
	TestTrue(TEXT("Nonfinite duration cannot start an endless countdown"),
		URespawnCountdownLibrary::StartRespawnCountdown(Widget, std::numeric_limits<double>::infinity()));
	TestEqual(TEXT("Nonfinite duration is cleared"), Remaining(), 0.0);
	TestFalse(TEXT("Missing widget is rejected"), URespawnCountdownLibrary::StartRespawnCountdown(nullptr, 1.0));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRespawnProgressGradientTest, "ShootingArena.UI.RespawnProgressGradient",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRespawnProgressGradientTest::RunTest(const FString&)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	auto* Class = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/QuakeLike_1_0/UI/DeathUI/WBP_DeathScreen.WBP_DeathScreen_C"));
	if (!TestNotNull(TEXT("Death screen class"), Class)) return false;
	auto* Widget = CreateWidget<UUserWidget>(Context.GetTestWorld(), Class);
	if (!TestNotNull(TEXT("Death screen instance"), Widget)) return false;
	auto* Nested = Cast<UUserWidget>(Widget->GetWidgetFromName(TEXT("WB_RespawnProgress")));
	auto* Bar = Nested ? Cast<UProgressBar>(Nested->GetWidgetFromName(TEXT("PB_Base"))) : nullptr;
	const auto* Style = FindFProperty<FObjectPropertyBase>(Class, TEXT("RespawnBarStyle"));
	UObject* OriginalSettings = Style ? Style->GetObjectPropertyValue_InContainer(Widget) : nullptr;
	if (!TestNotNull(TEXT("Visible progress bar"), Bar) || !TestNotNull(TEXT("Respawn style DA"), OriginalSettings)) return false;

	UObject* Settings = DuplicateObject(OriginalSettings, Widget);
	Style->SetObjectPropertyValue_InContainer(Widget, Settings);
	auto* Enabled = FindFProperty<FBoolProperty>(Settings->GetClass(), TEXT("UseProgressGradient"));
	auto* Center = FindFProperty<FStructProperty>(Settings->GetClass(), TEXT("ProgressGradientCenterColor"));
	auto* Edge = FindFProperty<FStructProperty>(Settings->GetClass(), TEXT("ProgressGradientEdgeColor"));
	auto* Start = FindFProperty<FNumericProperty>(Settings->GetClass(), TEXT("ProgressGradientStart"));
	auto* End = FindFProperty<FNumericProperty>(Settings->GetClass(), TEXT("ProgressGradientEnd"));
	if (!TestNotNull(TEXT("Gradient toggle"), Enabled) || !TestNotNull(TEXT("Gradient centre color"), Center)
		|| !TestNotNull(TEXT("Gradient edge color"), Edge) || !TestNotNull(TEXT("Gradient start"), Start)
		|| !TestNotNull(TEXT("Gradient end"), End)) return false;
	const UObject* SolidFill = Bar->GetWidgetStyle().FillImage.GetResourceObject();
	Enabled->SetPropertyValue_InContainer(Settings, true);
	*Center->ContainerPtrToValuePtr<FLinearColor>(Settings) = FLinearColor::Red;
	*Edge->ContainerPtrToValuePtr<FLinearColor>(Settings) = FLinearColor::Blue;
	Start->SetFloatingPointPropertyValue(Start->ContainerPtrToValuePtr<void>(Settings), 0.25);
	End->SetFloatingPointPropertyValue(End->ContainerPtrToValuePtr<void>(Settings), 0.75);
	TestTrue(TEXT("Starting the countdown applies the gradient"), URespawnCountdownLibrary::StartRespawnCountdown(Widget, 3.0));
	auto* Fill = Cast<UMaterialInstanceDynamic>(Bar->GetWidgetStyle().FillImage.GetResourceObject());
	if (!TestNotNull(TEXT("Visible bar uses a dynamic gradient"), Fill)) return false;
	TestEqual(TEXT("Center uses its DA color"), Fill->K2_GetVectorParameterValue(TEXT("CenterColor")), FLinearColor::Red);
	TestEqual(TEXT("Edges use their DA color"), Fill->K2_GetVectorParameterValue(TEXT("EdgeColor")), FLinearColor::Blue);
	TestEqual(TEXT("Gradient starts at the DA position"), Fill->K2_GetScalarParameterValue(TEXT("GradientStart")), 0.25f);
	TestEqual(TEXT("Gradient blends only between the DA positions"), Fill->K2_GetScalarParameterValue(TEXT("GradientWidth")), 0.5f);

	*Edge->ContainerPtrToValuePtr<FLinearColor>(Settings) = FLinearColor::Green;
	Start->SetFloatingPointPropertyValue(Start->ContainerPtrToValuePtr<void>(Settings), 1.5);
	End->SetFloatingPointPropertyValue(End->ContainerPtrToValuePtr<void>(Settings), 0.5);
	URespawnCountdownLibrary::StartRespawnCountdown(Widget, 3.0);
	TestTrue(TEXT("Restart reuses the instance"), Bar->GetWidgetStyle().FillImage.GetResourceObject() == Fill);
	TestEqual(TEXT("Restart refreshes the edge color"), Fill->K2_GetVectorParameterValue(TEXT("EdgeColor")), FLinearColor::Green);
	TestEqual(TEXT("Reversed endpoints are ordered"), Fill->K2_GetScalarParameterValue(TEXT("GradientStart")), 0.5f);
	TestEqual(TEXT("Out-of-range endpoints are clamped"), Fill->K2_GetScalarParameterValue(TEXT("GradientWidth")), 0.5f);

	End->SetFloatingPointPropertyValue(End->ContainerPtrToValuePtr<void>(Settings), 1.0);
	URespawnCountdownLibrary::StartRespawnCountdown(Widget, 3.0);
	TestEqual(TEXT("Coincident endpoints retain a positive width"), Fill->K2_GetScalarParameterValue(TEXT("GradientWidth")), 0.001f);

	Enabled->SetPropertyValue_InContainer(Settings, false);
	URespawnCountdownLibrary::StartRespawnCountdown(Widget, 3.0);
	TestTrue(TEXT("Disabling restores the original fill"), Bar->GetWidgetStyle().FillImage.GetResourceObject() == SolidFill);
	return !HasAnyErrors();
}

#endif

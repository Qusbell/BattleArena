#if WITH_DEV_AUTOMATION_TESTS

#include "UI/RespawnCountdownLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Engine/World.h"
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
	TestEqual(TEXT("Initial display is full"), Bar->GetPercent(), 1.f);
	TestTrue(TEXT("Timer is scheduled"), World->GetTimerManager().TimerExists(Handle));
	const FTimerHandle Previous = Handle;
	TestTrue(TEXT("An active countdown can restart"), URespawnCountdownLibrary::StartRespawnCountdown(Widget, 1.2));
	TestFalse(TEXT("Restart removes the previous timer"), World->GetTimerManager().TimerExists(Previous));
	TestEqual(TEXT("Restart resets remaining time"), Remaining(), 1.2);

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

#endif

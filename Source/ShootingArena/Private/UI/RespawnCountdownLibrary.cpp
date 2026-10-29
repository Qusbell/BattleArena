#include "UI/RespawnCountdownLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

bool URespawnCountdownLibrary::StartRespawnCountdown(UUserWidget* Widget, double RespawnDuration)
{
	UWorld* World = IsValid(Widget) ? Widget->GetWorld() : nullptr;
	if (!IsInGameThread() || !IsValid(World) || World->bIsTearingDown) return false;

	UClass* Class = Widget->GetClass();
	auto Number = [Class](const TCHAR* Name) -> FNumericProperty*
	{
		auto* Property = FindFProperty<FNumericProperty>(Class, Name);
		return Property && Property->IsFloatingPoint() ? Property : nullptr;
	};
	FNumericProperty* Maximum = Number(TEXT("MaxRespawnTime"));
	FNumericProperty* Current = Number(TEXT("CurrentRespawnTime"));
	FNumericProperty* Interval = Number(TEXT("RespawnUpdateInterval"));
	auto* TimerProperty = FindFProperty<FStructProperty>(Class, TEXT("RespawnTimerHandle"));
	UFunction* Refresh = Widget->FindFunction(TEXT("RefreshRespawnDisplay"));
	UFunction* Update = Widget->FindFunction(TEXT("UpdateRespawnTimer"));
	if (!Maximum || !Current || !Interval || !TimerProperty
		|| TimerProperty->Struct != TBaseStructure<FTimerHandle>::Get()
		|| !Refresh || Refresh->NumParms != 0 || !Update || Update->NumParms != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s is missing the death screen countdown fields or parameterless display events."), *GetNameSafe(Widget));
		return false;
	}

	const float Rate = float(Interval->GetFloatingPointPropertyValue(Interval->ContainerPtrToValuePtr<void>(Widget)));
	if (!FMath::IsFinite(Rate) || Rate <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s requires a finite, positive RespawnUpdateInterval."), *GetNameSafe(Widget));
		return false;
	}

	FTimerHandle& Handle = *TimerProperty->ContainerPtrToValuePtr<FTimerHandle>(Widget);
	FTimerManager& Timers = World->GetTimerManager();
	Timers.ClearTimer(Handle);
	const double Duration = FMath::IsFinite(RespawnDuration) ? FMath::Max(0.0, RespawnDuration) : 0.0;
	Maximum->SetFloatingPointPropertyValue(Maximum->ContainerPtrToValuePtr<void>(Widget), Duration);
	Current->SetFloatingPointPropertyValue(Current->ContainerPtrToValuePtr<void>(Widget), Duration);
	Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Widget->ProcessEvent(Refresh, nullptr);
	if (Duration <= 0.0)
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		return true;
	}

	Timers.SetTimer(Handle, FTimerDelegate::CreateUFunction(Widget, Update->GetFName()), Rate, true);
	return true;
}

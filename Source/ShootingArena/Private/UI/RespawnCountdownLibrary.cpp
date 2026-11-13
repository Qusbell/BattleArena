#include "UI/RespawnCountdownLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
void ApplyRespawnProgressStyle(UUserWidget& Widget)
{
	const auto* StyleProperty = FindFProperty<FObjectPropertyBase>(Widget.GetClass(), TEXT("RespawnBarStyle"));
	const UObject* Settings = StyleProperty ? StyleProperty->GetObjectPropertyValue_InContainer(&Widget) : nullptr;
	auto* Progress = Cast<UUserWidget>(Widget.GetWidgetFromName(TEXT("WB_RespawnProgress")));
	auto* Fill = Progress ? Cast<UProgressBar>(Progress->GetWidgetFromName(TEXT("PB_Base"))) : nullptr;
	if (!Settings || !Fill) return;

	const auto ReadNumber = [Settings](FName Name, double Default)
	{
		const auto* Property = FindFProperty<FNumericProperty>(Settings->GetClass(), Name);
		if (!Property || !Property->IsFloatingPoint()) return Default;
		const double Value = Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Settings));
		return FMath::IsFinite(Value) ? Value : Default;
	};
	const auto* ScreenClass = Cast<UWidgetBlueprintGeneratedClass>(Widget.GetClass());
	if (const UWidgetTree* Tree = ScreenClass ? ScreenClass->GetWidgetTreeArchetype() : nullptr)
	{
		const UWidget* Original = Tree->FindWidget(TEXT("WB_RespawnProgress"));
		const auto* Frame = Cast<USizeBox>(Progress->GetWidgetFromName(TEXT("SB_Base")));
		if (Original && Frame && Frame->GetWidthOverride() > 0.0f && Frame->GetHeightOverride() > 0.0f)
		{
			const FVector2D BaseSize(Frame->GetWidthOverride(), Frame->GetHeightOverride());
			const FVector2D DefaultSize = BaseSize * Original->GetRenderTransform().Scale;
			const double Width = ReadNumber(TEXT("RespawnProgressWidth"), DefaultSize.X);
			const double Height = ReadNumber(TEXT("RespawnProgressHeight"), DefaultSize.Y);
			Progress->SetRenderScale(FVector2D(Width > 0.0 ? Width : DefaultSize.X,
				Height > 0.0 ? Height : DefaultSize.Y) / BaseSize);
		}
		auto* BarGroup = Widget.GetWidgetFromName(TEXT("OV_RespawnProgress"));
		const UWidget* OriginalGroup = Tree->FindWidget(TEXT("OV_RespawnProgress"));
		if (BarGroup && OriginalGroup)
			BarGroup->SetRenderTranslation(OriginalGroup->GetRenderTransform().Translation
				+ FVector2D(0.0, ReadNumber(TEXT("RespawnProgressVerticalOffset"), 0.0)));
	}

	const auto* Enabled = FindFProperty<FBoolProperty>(Settings->GetClass(), TEXT("UseProgressGradient"));
	if (!Enabled || !Enabled->GetPropertyValue_InContainer(Settings))
	{
		const auto* Class = Cast<UWidgetBlueprintGeneratedClass>(Progress->GetClass());
		const UWidgetTree* Tree = Class ? Class->GetWidgetTreeArchetype() : nullptr;
		const auto* Original = Tree ? Tree->FindWidget<UProgressBar>(TEXT("PB_Base")) : nullptr;
		if (Original && Cast<UMaterialInstanceDynamic>(Fill->GetWidgetStyle().FillImage.GetResourceObject()))
			Fill->SetWidgetStyle(Original->GetWidgetStyle());
		return;
	}

	const auto* Center = FindFProperty<FStructProperty>(Settings->GetClass(), TEXT("ProgressGradientCenterColor"));
	const auto* Edge = FindFProperty<FStructProperty>(Settings->GetClass(), TEXT("ProgressGradientEdgeColor"));
	if (!Center || !Edge || Center->Struct != TBaseStructure<FLinearColor>::Get()
		|| Edge->Struct != TBaseStructure<FLinearColor>::Get()) return;

	FProgressBarStyle Style = Fill->GetWidgetStyle();
	auto* Material = Cast<UMaterialInstanceDynamic>(Style.FillImage.GetResourceObject());
	if (!Material)
	{
		const auto* Template = Cast<UProgressBar>(Widget.GetWidgetFromName(TEXT("PB_RespawnBar")));
		auto* Parent = Template ? Cast<UMaterialInterface>(Template->GetWidgetStyle().FillImage.GetResourceObject()) : nullptr;
		if (!Parent) return;
		Material = UMaterialInstanceDynamic::Create(Parent, Fill);
		Style.FillImage.SetResourceObject(Material);
		Fill->SetWidgetStyle(Style);
	}
	Material->SetVectorParameterValue(TEXT("CenterColor"), *Center->ContainerPtrToValuePtr<FLinearColor>(Settings));
	Material->SetVectorParameterValue(TEXT("EdgeColor"), *Edge->ContainerPtrToValuePtr<FLinearColor>(Settings));

	const float Start = FMath::Clamp(ReadNumber(TEXT("ProgressGradientStart"), 0.0), 0.0, 1.0);
	const float End = FMath::Clamp(ReadNumber(TEXT("ProgressGradientEnd"), 1.0), 0.0, 1.0);
	constexpr float MinimumWidth = 0.001f;
	const float Lower = FMath::Min(FMath::Min(Start, End), 1.0f - MinimumWidth);
	Material->SetScalarParameterValue(TEXT("GradientStart"), Lower);
	Material->SetScalarParameterValue(TEXT("GradientWidth"), FMath::Max(FMath::Max(Start, End) - Lower, MinimumWidth));
}
}

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
	ApplyRespawnProgressStyle(*Widget);
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

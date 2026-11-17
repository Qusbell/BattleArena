#include "UI/RespawnCountdownLibrary.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
// Two S_Shine-style glows pinned to the left and right edges of the centered sweep (SB_SliderBase).
// The sweep's width already follows progress every tick, so the glows track both fill tips without a tick of their own.
void ApplyCenterTipGlows(UUserWidget& Progress, const FLinearColor& Shine, const FLinearColor& SweepTint, bool bFromCenter)
{
	static const FName TipsName(TEXT("CV_RespawnTips"));
	UWidget* Tips = Progress.GetWidgetFromName(TipsName);
	auto* SweepBox = Cast<USizeBox>(Progress.GetWidgetFromName(TEXT("SB_SliderBase")));
	const auto* Thumb = Cast<USlider>(Progress.GetWidgetFromName(TEXT("S_Shine")));
	if (!Tips && bFromCenter && SweepBox && SweepBox->GetContent() && Thumb && Progress.WidgetTree)
	{
		auto* Stack = Progress.WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("OV_RespawnSweep"));
		auto* Canvas = Progress.WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TipsName);
		UWidget* Fx = SweepBox->GetContent();
		for (UWidget* Child : {Fx, static_cast<UWidget*>(Canvas)})
		{
			UOverlaySlot* Slot = Stack->AddChildToOverlay(Child);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}
		SweepBox->SetContent(Stack);
		const FSlateBrush& Glow = Thumb->GetWidgetStyle().NormalThumbImage;
		for (const float Side : {0.0f, 1.0f})
		{
			auto* Image = Progress.WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Image->SetBrush(Glow);
			UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Image);
			Slot->SetAnchors(FAnchors(Side, 0.5f));
			Slot->SetAlignment(FVector2D(0.5f, 0.5f));
			Slot->SetPosition(FVector2D::ZeroVector);
		}
		Tips = Canvas;
	}
	if (!Tips) return;
	Tips->SetVisibility(bFromCenter ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	// shortcut: the sweep border (b_Slider1) tints children with SweepTint, so divide it out to approximate ShineColor;
	// channels where ShineColor exceeds SweepTint clamp at 1. Move the glows out of b_Slider1 if exact color matters.
	const auto Ratio = [](float A, float B) { return FMath::Clamp(A / FMath::Max(B, 0.001f), 0.0f, 1.0f); };
	const FLinearColor TipColor(Ratio(Shine.R, SweepTint.R), Ratio(Shine.G, SweepTint.G),
		Ratio(Shine.B, SweepTint.B), Ratio(Shine.A, SweepTint.A));
	// The bar's render scale is non-uniform (e.g. 5.6 x 2.5); narrow the glow by Y/X so it stays round on screen.
	const FVector2D Scale = Progress.GetRenderTransform().Scale;
	for (UWidget* Child : Cast<UPanelWidget>(Tips)->GetAllChildren())
		if (auto* Image = Cast<UImage>(Child))
		{
			Image->SetColorAndOpacity(TipColor);
			const FVector2D Size(Image->GetBrush().ImageSize);
			Cast<UCanvasPanelSlot>(Image->Slot)->SetSize(Scale.X > 0.0 && Scale.Y > 0.0
				? FVector2D(Size.X * Scale.Y / Scale.X, Size.Y) : Size);
		}
}

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

	const auto ReadBool = [Settings](FName Name)
	{
		const auto* Property = FindFProperty<FBoolProperty>(Settings->GetClass(), Name);
		return Property && Property->GetPropertyValue_InContainer(Settings);
	};
	const auto* ProgressClass = Cast<UWidgetBlueprintGeneratedClass>(Progress->GetClass());
	const UWidgetTree* ProgressTree = ProgressClass ? ProgressClass->GetWidgetTreeArchetype() : nullptr;
	const auto* Original = ProgressTree ? ProgressTree->FindWidget<UProgressBar>(TEXT("PB_Base")) : nullptr;

	// Center fill applies only to this respawn instance; WB_AmmoProgress is shared with the ammo HUD.
	const bool bFromCenter = ReadBool(TEXT("ProgressFillFromCenter"));
	const EProgressBarFillType::Type DefaultFill = Original ? EProgressBarFillType::Type(Original->GetBarFillType())
		: EProgressBarFillType::LeftToRight;
	Fill->SetBarFillType(bFromCenter ? EProgressBarFillType::FillFromCenterHorizontal : DefaultFill);
	// The shine slider tracks a single right-hand tip, which does not match a symmetric fill.
	const UWidget* OriginalShine = ProgressTree ? ProgressTree->FindWidget(TEXT("SB_Shine")) : nullptr;
	if (UWidget* Shine = Progress->GetWidgetFromName(TEXT("SB_Shine")))
		Shine->SetVisibility(bFromCenter ? ESlateVisibility::Collapsed
			: OriginalShine ? OriginalShine->GetVisibility() : ESlateVisibility::SelfHitTestInvisible);
	// The FX overlay grows with progress from its slot edge; centering makes it widen symmetrically.
	const auto* OriginalSweep = ProgressTree ? ProgressTree->FindWidget(TEXT("SB_SliderBase")) : nullptr;
	const UWidget* Sweep = Progress->GetWidgetFromName(TEXT("SB_SliderBase"));
	if (auto* SweepSlot = Sweep ? Cast<UBorderSlot>(Sweep->Slot) : nullptr)
	{
		const auto* OriginalSlot = OriginalSweep ? Cast<UBorderSlot>(OriginalSweep->Slot) : nullptr;
		SweepSlot->SetHorizontalAlignment(bFromCenter ? HAlign_Center
			: OriginalSlot ? OriginalSlot->GetHorizontalAlignment() : HAlign_Left);
	}
	// T_ProgressFx only glows at its right end; the mirror material folds it so both ends glow.
	const auto* MirrorProperty = FindFProperty<FObjectPropertyBase>(Settings->GetClass(), TEXT("ProgressFxMirrorMaterial"));
	UObject* Mirror = MirrorProperty ? MirrorProperty->GetObjectPropertyValue_InContainer(Settings) : nullptr;
	const auto* OriginalFx = ProgressTree ? ProgressTree->FindWidget<UImage>(TEXT("Image_195")) : nullptr;
	if (auto* Fx = Cast<UImage>(Progress->GetWidgetFromName(TEXT("Image_195"))); Fx && OriginalFx)
	{
		FSlateBrush Brush = OriginalFx->GetBrush();
		if (bFromCenter && Mirror) Brush.SetResourceObject(Mirror);
		Fx->SetBrush(Brush);
	}
	const auto ReadColor = [Settings](FName Name, const FLinearColor& Default)
	{
		const auto* Property = FindFProperty<FStructProperty>(Settings->GetClass(), Name);
		return Property && Property->Struct == TBaseStructure<FLinearColor>::Get()
			? *Property->ContainerPtrToValuePtr<FLinearColor>(Settings) : Default;
	};
	const bool bGradient = ReadBool(TEXT("UseProgressGradient"));
	FLinearColor SweepTint = ReadColor(TEXT("ProgressColor"), FLinearColor::White);
	if (bFromCenter && bGradient)
	{
		// The ammo BP darkens the fill border to ~30% ProgressColor, which crushes the gradient; show it as authored,
		// and tint the edge-bright sweep with EdgeColor so the sides read lighter than the center.
		// shortcut: switching these DA toggles off mid-session keeps both tints until the BP recolors the bar.
		if (auto* FillBorder = Cast<UBorder>(Progress->GetWidgetFromName(TEXT("b_PBase"))))
			FillBorder->SetContentColorAndOpacity(FLinearColor::White);
		SweepTint = ReadColor(TEXT("ProgressGradientEdgeColor"), SweepTint);
		if (auto* SweepBorder = Cast<UBorder>(Progress->GetWidgetFromName(TEXT("b_Slider1"))))
			SweepBorder->SetContentColorAndOpacity(SweepTint);
	}
	ApplyCenterTipGlows(*Progress, ReadColor(TEXT("ShineColor"), FLinearColor::White), SweepTint, bFromCenter);

	if (!bGradient)
	{
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
	// 1: gradient position runs from the bar center (0) to both edges (1).
	Material->SetScalarParameterValue(TEXT("SymmetricGradient"), bFromCenter ? 1.0f : 0.0f);

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

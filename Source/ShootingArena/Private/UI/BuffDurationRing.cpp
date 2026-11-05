#include "UI/BuffDurationRing.h"

#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	float SafeValue(float Value, float Fallback, float Min, float Max)
	{
		return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Fallback;
	}

	void ApplyVisibility(UWidget* Widget, bool bVisible)
	{
		if (!IsValid(Widget)) return;
		const ESlateVisibility Desired = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (Widget->GetVisibility() != Desired) Widget->SetVisibility(Desired);
	}
}

const UBuffDurationRingSettings* UBuffDurationRingWidget::GetRingSettings() const
{
	return IsValid(Settings) ? Settings.Get() : GetDefault<UBuffDurationRingSettings>();
}

void UBuffDurationRingWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyRingStyle();
	if (IsDesignTime() && RingMID)
	{
		RingMID->SetScalarParameterValue(TEXT("Percent"), 0.65f);
		RingMID->SetVectorParameterValue(TEXT("FillColor"), GetRingSettings()->QuadColor);
	}
}

void UBuffDurationRingWidget::ApplyRingStyle()
{
	const UBuffDurationRingSettings* Style = GetRingSettings();
	const float Diameter = SafeValue(Style->Diameter, 212.0f, 16.0f, 1024.0f);
	if (RingSizeBox)
	{
		if (RingSizeBox->GetWidthOverride() != Diameter) RingSizeBox->SetWidthOverride(Diameter);
		if (RingSizeBox->GetHeightOverride() != Diameter) RingSizeBox->SetHeightOverride(Diameter);
	}
	if (!CircleImg) return;
	const float Angle = FMath::IsFinite(Style->StartAngle) ? Style->StartAngle : -90.0f;
	if (CircleImg->GetRenderTransform().Angle != Angle) CircleImg->SetRenderTransformAngle(Angle);
	UMaterialInterface* Material = Style->RingMaterial;
	if (!Material && !RingMID) Material = Cast<UMaterialInterface>(CircleImg->GetBrush().GetResourceObject());
	if (!Material) Material = AppliedRingMaterial;
	if (Material && (!RingMID || AppliedRingMaterial != Material))
	{
		RingMID = UMaterialInstanceDynamic::Create(Material, this);
		AppliedRingMaterial = Material;
		CircleImg->SetBrushFromMaterial(RingMID);
		AppliedProgress = AppliedSize = AppliedSteps = -1.0f;
		AppliedFillColor = FLinearColor::Transparent;
		AppliedEmptyColor = FLinearColor(-1.0f, -1.0f, -1.0f, -1.0f);
	}
	if (!RingMID) return;
	const float Size = SafeValue(Style->RingSize, 1.06f, 1.001f, 2.0f);
	const float Steps = SafeValue(Style->RingSteps, 800.0f, 1.0f, 1000.0f);
	if (AppliedSize != Size) { RingMID->SetScalarParameterValue(TEXT("Size"), Size); AppliedSize = Size; }
	if (AppliedSteps != Steps) { RingMID->SetScalarParameterValue(TEXT("Steps"), Steps); AppliedSteps = Steps; }
	if (AppliedEmptyColor != Style->EmptyColor)
	{
		RingMID->SetVectorParameterValue(TEXT("EmptyColor"), Style->EmptyColor);
		AppliedEmptyColor = Style->EmptyColor;
	}
}

void UBuffDurationRingWidget::ApplyIconStyle(UImage* Icon)
{
	if (!IsValid(Icon)) return;
	const UBuffDurationRingSettings* Style = GetRingSettings();
	const bool bNewIcon = StyledIcon.Get() != Icon;
	UTexture2D* Texture = Style->IconTexture;
	if (!Texture) Texture = Cast<UTexture2D>(Icon->GetBrush().GetResourceObject());
	if (!Texture && !bNewIcon) Texture = AppliedIconTexture;
	if (bNewIcon || AppliedIconMaterial != Style->IconMaterial)
	{
		// No scene/background blur: only this texture is sampled by the icon material.
		IconMID = Style->IconMaterial ? UMaterialInstanceDynamic::Create(Style->IconMaterial, this) : nullptr;
		AppliedIconMaterial = Style->IconMaterial;
		StyledIcon = Icon;
		AppliedIconTexture = nullptr;
		AppliedBlur = AppliedIconOpacity = -1.0f;
		AppliedIconSize = FVector2D::ZeroVector;
		AppliedIconTint = FLinearColor::Transparent;
		if (IconMID) Icon->SetBrushFromMaterial(IconMID);
		else if (Texture) Icon->SetBrushFromTexture(Texture);
	}
	if (Texture && AppliedIconTexture != Texture)
	{
		if (IconMID) IconMID->SetTextureParameterValue(TEXT("IconTexture"), Texture);
		else Icon->SetBrushFromTexture(Texture);
		AppliedIconTexture = Texture;
	}
	const FVector2D Size(SafeValue(Style->IconSize.X, 180.0f, 1.0f, 1024.0f),
		SafeValue(Style->IconSize.Y, 180.0f, 1.0f, 1024.0f));
	if (AppliedIconSize != Size)
	{
		Icon->SetDesiredSizeOverride(Size);
		if (IconMID) IconMID->SetVectorParameterValue(TEXT("IconUVStep"), FLinearColor(1.0f / Size.X, 1.0f / Size.Y, 0.0f, 0.0f));
		AppliedIconSize = Size;
	}
	const FVector2D Offset = Style->IconOffset.ContainsNaN() ? FVector2D::ZeroVector : Style->IconOffset;
	if (Icon->GetRenderTransform().Translation != Offset) Icon->SetRenderTranslation(Offset);
	const float Alpha = SafeValue(Style->IconOpacity, 0.22f, 0.0f, 1.0f)
		* SafeValue(Style->IconColor.A, 1.0f, 0.0f, 1.0f);
	const float Blur = SafeValue(Style->IconBlurStrength, 2.0f, 0.0f, 12.0f);
	if (IconMID)
	{
		if (AppliedBlur != Blur) { IconMID->SetScalarParameterValue(TEXT("BlurPixels"), Blur); AppliedBlur = Blur; }
		if (AppliedIconOpacity != Alpha) { IconMID->SetScalarParameterValue(TEXT("IconOpacity"), Alpha); AppliedIconOpacity = Alpha; }
	}
	const float RenderAlpha = IconMID ? 1.0f : Alpha;
	if (Icon->GetRenderOpacity() != RenderAlpha) Icon->SetRenderOpacity(RenderAlpha);
}

float UBuffDurationRingWidget::GetIconFlashMultiplier(double Remaining) const
{
	const UBuffDurationRingSettings* Style = GetRingSettings();
	if (!Style->bIconFlashEnabled || !FMath::IsFinite(Remaining) || Remaining <= 0.0) return 1.0f;
	const float Strength = SafeValue(Style->IconFlashStrength, 0.5f, 0.0f, 4.0f);
	if (Strength <= 0.0f) return 1.0f;
	const double Interval = SafeValue(Style->IconFlashInterval, 1.0f, 0.05f, 60.0f);
	const double Duration = FMath::Min<double>(SafeValue(Style->IconFlashDuration, 0.15f, 0.01f, 2.0f), Interval);
	// Remaining counts down. Each interval boundary starts a short pulse, then eases to normal.
	// With a 1s interval this is the same boundary used by ceil(Remaining) for the centre number.
	const double Remainder = FMath::Fmod(Remaining, Interval);
	const double Age = Remainder <= 0.000001 ? 0.0 : Interval - Remainder;
	const float Envelope = static_cast<float>(FMath::Clamp(1.0 - Age / Duration, 0.0, 1.0));
	return 1.0f + Strength * Envelope * Envelope;
}

FLinearColor UBuffDurationRingWidget::GetTimeDisplayColor(double Remaining) const
{
	const UBuffDurationRingSettings* Style = GetRingSettings();
	const double Start = SafeValue(Style->TimeWarningStartSeconds, 5.0f, 0.1f, 60.0f);
	if (!Style->bTimeWarningFlashEnabled || !FMath::IsFinite(Remaining) || Remaining <= 0.0 || Remaining > Start)
		return Style->TimeColor;
	const double Period = SafeValue(Style->TimeWarningFlashInterval, 0.5f, 0.05f, 10.0f);
	// Derive phase from the existing remaining time; never schedule or accumulate another timer.
	if (FMath::Fmod(Start - Remaining, Period) >= Period * 0.5) return Style->TimeColor;
	const FLinearColor Warning = Style->TimeWarningColor;
	return FMath::IsFinite(Warning.R) && FMath::IsFinite(Warning.G)
		&& FMath::IsFinite(Warning.B) && FMath::IsFinite(Warning.A) ? Warning : FLinearColor::Red;
}

void UBuffDurationRingWidget::ApplyStatus(double Remaining, float Progress, bool bQuad, UTextBlock* TimeText, UImage* Icon)
{
	ApplyRingStyle();
	ApplyIconStyle(Icon);
	const UBuffDurationRingSettings* Style = GetRingSettings();
	const bool bActive = FMath::IsFinite(Remaining) && Remaining > 0.0;
	const FLinearColor Color = bQuad ? Style->QuadColor : Style->ProtectionColor;
	const float Percent = bActive && FMath::IsFinite(Progress) ? FMath::Clamp(Progress, 0.0f, 1.0f) : 0.0f;
	if (RingMID)
	{
		if (AppliedProgress != Percent) { RingMID->SetScalarParameterValue(TEXT("Percent"), Percent); AppliedProgress = Percent; }
		if (bActive && AppliedFillColor != Color)
		{
			RingMID->SetVectorParameterValue(TEXT("FillColor"), Color);
			AppliedFillColor = Color;
		}
	}
	ApplyVisibility(this, bActive && Style->bShowRing && RingMID && Percent > 0.0f);
	ApplyVisibility(Icon, bActive && Style->bShowIcon);
	ApplyVisibility(TimeText, bActive && Style->bShowTime);
	if (!bActive) return;
	const bool bFiniteIconRGB = FMath::IsFinite(Style->IconColor.R)
		&& FMath::IsFinite(Style->IconColor.G) && FMath::IsFinite(Style->IconColor.B);
	const FLinearColor BaseIconColor = bFiniteIconRGB ? Style->IconColor : FLinearColor::White;
	FLinearColor IconTint = BaseIconColor * GetIconFlashMultiplier(Remaining);
	// Alpha is applied once through IconOpacity; keep the material/image tint fully opaque.
	IconTint.A = 1.0f;
	if (IconMID && AppliedIconTint != IconTint)
	{
		IconMID->SetVectorParameterValue(TEXT("IconTint"), IconTint);
		AppliedIconTint = IconTint;
	}
	const FLinearColor ImageTint = IconMID ? FLinearColor::White : IconTint;
	if (IsValid(Icon) && Icon->GetColorAndOpacity() != ImageTint) Icon->SetColorAndOpacity(ImageTint);
	if (!IsValid(TimeText)) return;
	const int32 Seconds = FMath::CeilToInt(FMath::Min(Remaining, static_cast<double>(MAX_int32 - 1)));
	const FText Number = FText::AsNumber(Seconds);
	const FText Text = Style->bShowSecondsSuffix
		? FText::Format(NSLOCTEXT("Buff", "SecondsRemaining", "{0}s"), Number) : Number;
	if (!TimeText->GetText().EqualTo(Text)) TimeText->SetText(Text);
	const FSlateColor TimeColor(GetTimeDisplayColor(Remaining));
	if (TimeText->GetColorAndOpacity() != TimeColor) TimeText->SetColorAndOpacity(TimeColor);
	FSlateFontInfo Font = TimeText->GetFont();
	const int32 Size = FMath::Clamp(Style->TimeFontSize, 8, 128);
	const int32 Outline = FMath::Clamp(Style->TimeOutlineSize, 0, 8);
	if (Font.Size != Size || Font.OutlineSettings.OutlineSize != Outline)
	{
		Font.Size = Size;
		Font.OutlineSettings.OutlineSize = Outline;
		Font.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
		TimeText->SetFont(Font);
	}
	const FVector2D Offset = Style->TimeOffset.ContainsNaN() ? FVector2D::ZeroVector : Style->TimeOffset;
	if (TimeText->GetRenderTransform().Translation != Offset) TimeText->SetRenderTranslation(Offset);
}

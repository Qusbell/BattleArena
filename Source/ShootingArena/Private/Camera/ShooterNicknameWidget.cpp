#include "Camera/ShooterNicknameWidget.h"
#include "Camera/ShooterNicknameComponent.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "GameFramework/PlayerController.h"

void UShooterNicknameWidget::RefreshDisplay()
{
	if (const TSharedPtr<SWidget> Cached = GetCachedWidget()) Cached->Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UShooterNicknameWidget::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
	const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 LayerId,
	const FWidgetStyle& WidgetStyle, bool bParentEnabled) const
{
	const int32 BaseLayer = Super::NativePaint(Args, Geometry, CullingRect, Elements, LayerId, WidgetStyle, bParentEnabled);
	const UShooterNicknameComponent* Component = DisplayComponent.Get();
	if (!Component || Component->GetDisplayedNickname().IsEmpty() || !FSlateApplication::IsInitialized()) return BaseLayer;
	const FShooterNicknameStyle Style = Component->GetNicknameStyle();
	if (!Style.bEnabled) return BaseLayer;
	FVector2D Position;
	USlateBlueprintLibrary::ScreenToWidgetLocal(GetOwningPlayer(), Geometry, Component->GetCrosshairViewportPosition(), Position, true);
	const FString Name = Component->GetDisplayedNickname().ToString();
	const FVector2D Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Name, Style.Font);
	Position += Style.Offset - FVector2D(Size.X * 0.5, 0.0);
	FSlateDrawElement::MakeText(Elements, BaseLayer + 1,
		Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), Name,
		Style.Font, ESlateDrawEffect::None, Style.Color * WidgetStyle.GetColorAndOpacityTint());
	return BaseLayer + 1;
}

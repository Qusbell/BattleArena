#include "Camera/ShooterNicknameWidget.h"
#include "Camera/ShooterNicknameComponent.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UShooterNicknameWidget::SetDisplayComponent(UShooterNicknameComponent* Component)
{
	DisplayComponent = Component;
}

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
	const APlayerController* PC = GetOwningPlayer();
	if (!IsValid(Component) || Component->IsBeingDestroyed() || !IsValid(PC)
		|| PC->IsActorBeingDestroyed() || !PC->GetLocalPlayer()
		|| Component->GetDisplayedNickname().IsEmpty() || !FSlateApplication::IsInitialized()) return BaseLayer;
	FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
	if (!Renderer) return BaseLayer;
	const FShooterNicknameStyle Style = Component->GetNicknameStyle();
	if (!Style.bEnabled) return BaseLayer;
	const UWorld* World = PC->GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!World || World->bIsTearingDown || !IsValid(Viewport)) return BaseLayer;
	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	if (ViewportSize.ContainsNaN() || ViewportSize.X <= 0.0 || ViewportSize.Y <= 0.0) return BaseLayer;
	// AbsoluteToLocal inverts this matrix and asserts for a singular transform.
	// A collapsed scale or an invalid viewport must never enter that conversion.
	const float Determinant = Geometry.GetAccumulatedRenderTransform().GetMatrix().Determinant();
	if (!FMath::IsFinite(Determinant) || FMath::IsNearlyZero(Determinant)
		|| Geometry.GetAccumulatedRenderTransform().GetTranslation().ContainsNaN()) return BaseLayer;
	const FVector2D CrosshairPosition = Component->GetCrosshairViewportPosition();
	if (CrosshairPosition.ContainsNaN()) return BaseLayer;
	FVector2D Position;
	USlateBlueprintLibrary::ScreenToWidgetLocal(GetOwningPlayer(), Geometry, CrosshairPosition, Position, true);
	const FString Name = Component->GetDisplayedNickname().ToString();
	const FVector2D Size = Renderer->GetFontMeasureService()->Measure(Name, Style.Font);
	if (Position.ContainsNaN() || Size.ContainsNaN()) return BaseLayer;
	Position += FVector2D(Style.Offset.X, Style.bShowAboveCrosshair ? -FMath::Abs(Style.Offset.Y) - Size.Y : FMath::Abs(Style.Offset.Y))
		- FVector2D(Size.X * 0.5, 0.0);
	// Slate stores transforms as floats; a finite double offset can still overflow there.
	if (Position.ContainsNaN() || !FMath::IsFinite(static_cast<float>(Position.X))
		|| !FMath::IsFinite(static_cast<float>(Position.Y))) return BaseLayer;
	FSlateDrawElement::MakeText(Elements, BaseLayer + 1,
		Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), Name,
		Style.Font, ESlateDrawEffect::None, Component->GetDisplayedNicknameColor() * WidgetStyle.GetColorAndOpacityTint());
	return BaseLayer + 1;
}

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterNicknameWidget.generated.h"

class UShooterNicknameComponent;

/** Independent native overlay; existing crosshair and HUD Blueprint assets are untouched. */
UCLASS()
class SHOOTINGARENA_API UShooterNicknameWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetDisplayComponent(UShooterNicknameComponent* Component);
	void RefreshDisplay();
protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Elements, int32 LayerId, const FWidgetStyle& WidgetStyle, bool bParentEnabled) const override;
private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UShooterNicknameComponent> DisplayComponent;
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Styling/SlateTypes.h"
#include "ReusableStyleHost.generated.h"

class UCommonButtonStyle;
class UCommonTextStyle;
class UButton;

/** Appearance lives in Blueprint class defaults; these classes contain no game settings. */
UCLASS(Abstract, Blueprintable, BlueprintType)
class SHOOTINGARENA_API UBlueprintSliderStyle : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FSliderStyle Style;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FLinearColor BarColor = FLinearColor::White;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FLinearColor HandleColor = FLinearColor::White;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Layout") FVector2D DesiredSize = FVector2D::ZeroVector;
};

UCLASS(Abstract, Blueprintable, BlueprintType)
class SHOOTINGARENA_API UBlueprintScrollStyle : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FScrollBoxStyle Style;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FScrollBarStyle BarStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FVector2D Thickness = FVector2D::ZeroVector;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FMargin Padding;
};

UCLASS(Abstract, Blueprintable, BlueprintType)
class SHOOTINGARENA_API UBlueprintComboStyle : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FComboBoxStyle Style;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FTableRowStyle ItemStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FScrollBarStyle ScrollbarStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FSlateFontInfo Font;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") TSubclassOf<UCommonTextStyle> TextStyle;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FSlateColor Foreground;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Layout") FMargin Padding;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Layout") FVector2D DesiredSize = FVector2D::ZeroVector;
};

UCLASS(Abstract, Blueprintable, BlueprintType)
class SHOOTINGARENA_API UBlueprintBrushStyle : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FSlateBrush Brush;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FLinearColor Tint = FLinearColor::White;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") FMargin Padding;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Style") float BlurStrength = 0.0f;
};

/** The reusable combo applies font defaults as well as its public Slate styles. */
UCLASS()
class SHOOTINGARENA_API UStyleableComboBoxString : public UComboBoxString
{
    GENERATED_BODY()
public:
    void ApplyBlueprintStyle(const UBlueprintComboStyle& Style);
};

/** Reusable WBP components contain StyleBounds + a NamedSlot called Content.
 * Existing native widgets stay in that slot, retaining names and event bindings.
 */
UCLASS(Abstract, Blueprintable)
class SHOOTINGARENA_API UReusableStyleHost : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance|Blueprint Styles") TSubclassOf<UCommonButtonStyle> ButtonStyle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance|Blueprint Styles") TSubclassOf<UBlueprintSliderStyle> SliderStyle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance|Blueprint Styles") TSubclassOf<UBlueprintScrollStyle> ScrollStyle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance|Blueprint Styles") TSubclassOf<UBlueprintComboStyle> ComboStyle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance|Blueprint Styles") TSubclassOf<UBlueprintBrushStyle> BrushStyle;

    UFUNCTION(BlueprintCallable, Category="Appearance") void ApplyStyle();
    void SetSelected(bool bInSelected);
protected:
    virtual void NativePreConstruct() override;
    UPROPERTY(Transient) bool bSelected = false;
};

UCLASS()
class SHOOTINGARENA_API UReusableStyleLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="UI|Blueprint Styles")
    static FSlateBrush GetBrushStyle(TSubclassOf<UBlueprintBrushStyle> StyleClass);

    UFUNCTION(BlueprintCallable, Category="UI|Blueprint Styles")
    static void ApplyWidgetStyleTree(UUserWidget* Screen);

    UFUNCTION(BlueprintCallable, Category="UI|Blueprint Styles")
    static void SetStyledButtonSelected(UButton* Button, bool bSelected);
};

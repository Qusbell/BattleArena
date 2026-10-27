#include "UI/ReusableStyleHost.h"
#include "UI/UIPalette.h"

#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/NamedSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
bool ApplyFirstTextStyle(UWidget* Widget, UCommonTextStyle* TextStyle)
{
    if (!Widget || !TextStyle) return false;
    if (UCommonTextBlock* Text = Cast<UCommonTextBlock>(Widget))
    {
        UUIThemePaletteLibrary::ApplyObject(TextStyle);
        Text->SetStyle(TextStyle->GetClass());
        Text->SetColorAndOpacity(FSlateColor::UseForeground());
        return true;
    }
    if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        for (UWidget* Child : Panel->GetAllChildren())
            if (ApplyFirstTextStyle(Child, TextStyle)) return true;
    return false;
}

void VisitStyleWidgets(UWidget* Widget, TSet<UWidget*>& Seen)
{
    if (!Widget || Seen.Contains(Widget)) return;
    Seen.Add(Widget);
    if (UReusableStyleHost* Host = Cast<UReusableStyleHost>(Widget)) Host->ApplyStyle();
    if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
    {
        TArray<FName> Slots;
        UserWidget->GetSlotNames(Slots);
        for (FName Name : Slots) VisitStyleWidgets(UserWidget->GetContentForSlot(Name), Seen);
        if (UserWidget->WidgetTree) VisitStyleWidgets(UserWidget->WidgetTree->RootWidget, Seen);
    }
    if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
        for (UWidget* Child : Panel->GetAllChildren()) VisitStyleWidgets(Child, Seen);
}
}

void UStyleableComboBoxString::ApplyBlueprintStyle(const UBlueprintComboStyle& InStyle)
{
    UUIThemePaletteLibrary::PreparePalette();
    UCommonTextStyle* Typography = InStyle.TextStyle ? InStyle.TextStyle->GetDefaultObject<UCommonTextStyle>() : nullptr;
    UUIThemePaletteLibrary::ApplyObject(Typography);
    const FSlateFontInfo& ResolvedFont = Typography ? Typography->Font : InStyle.Font;
    const FSlateColor ResolvedColor = Typography ? FSlateColor(Typography->Color) : InStyle.Foreground;
    SetWidgetStyle(InStyle.Style);
    SetItemStyle(InStyle.ItemStyle);
    SetContentPadding(InStyle.Padding);
    // These construction properties also back the future popup item generator.
    // Update an already constructed default label without rebuilding the combo
    // or disturbing its options, selected value, and bound delegates.
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    ScrollBarStyle = InStyle.ScrollbarStyle;
    Font = ResolvedFont;
    ForegroundColor = ResolvedColor;
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    if (TSharedPtr<STextBlock> Text = DefaultComboBoxContent.Pin())
    {
        Text->SetFont(ResolvedFont);
        Text->SetColorAndOpacity(ResolvedColor);
    }
    InvalidateLayoutAndVolatility();
}

void UReusableStyleHost::NativePreConstruct()
{
    UUIThemePaletteLibrary::PreparePalette();
    Super::NativePreConstruct();
    ApplyStyle();
}

void UReusableStyleHost::SetSelected(bool bInSelected)
{
    bSelected = bInSelected;
    ApplyStyle();
}

void UReusableStyleHost::ApplyStyle()
{
    UUIThemePaletteLibrary::PreparePalette();
    UWidget* Content = GetContentForSlot(TEXT("Content"));
    if (!Content)
        if (UNamedSlot* ContentSlotWidget = Cast<UNamedSlot>(GetWidgetFromName(TEXT("Content"))))
            Content = ContentSlotWidget->GetContent();
    if (!Content) return;
    // The existing controller still owns visibility of its native controls.
    // Mirror collapsed/hidden state so a wrapper never leaves an empty row.
    const ESlateVisibility ContentVisibility = Content->GetVisibility();
    SetVisibility(ContentVisibility == ESlateVisibility::Collapsed || ContentVisibility == ESlateVisibility::Hidden
        ? ContentVisibility : ESlateVisibility::SelfHitTestInvisible);
    USizeBox* Bounds = Cast<USizeBox>(GetWidgetFromName(TEXT("StyleBounds")));
    if (UButton* Button = Cast<UButton>(Content); Button && ButtonStyle)
    {
        UCommonButtonStyle* Style = ButtonStyle->GetDefaultObject<UCommonButtonStyle>();
        UUIThemePaletteLibrary::ApplyObject(Style);
        FButtonStyle SlateStyle;
        if (bSelected)
        {
            Style->GetSelectedBaseBrush(SlateStyle.Normal);
            Style->GetSelectedHoveredBrush(SlateStyle.Hovered);
            Style->GetSelectedPressedBrush(SlateStyle.Pressed);
        }
        else
        {
            Style->GetNormalBaseBrush(SlateStyle.Normal);
            Style->GetNormalHoveredBrush(SlateStyle.Hovered);
            Style->GetNormalPressedBrush(SlateStyle.Pressed);
        }
        Style->GetDisabledBrush(SlateStyle.Disabled);
        UCommonTextStyle* NormalText = bSelected ? Style->GetSelectedTextStyle() : Style->GetNormalTextStyle();
        if (!NormalText) NormalText = Style->GetNormalTextStyle();
        UCommonTextStyle* HoverText = bSelected ? Style->GetSelectedHoveredTextStyle() : Style->GetNormalHoveredTextStyle();
        if (!HoverText) HoverText = NormalText;
        UCommonTextStyle* DisabledText = Style->GetDisabledTextStyle();
        UUIThemePaletteLibrary::ApplyObject(NormalText);
        UUIThemePaletteLibrary::ApplyObject(HoverText);
        UUIThemePaletteLibrary::ApplyObject(DisabledText);
        if (NormalText) SlateStyle.SetNormalForeground(FSlateColor(NormalText->Color));
        if (HoverText)
        {
            SlateStyle.SetHoveredForeground(FSlateColor(HoverText->Color));
            SlateStyle.SetPressedForeground(FSlateColor(HoverText->Color));
        }
        if (DisabledText) SlateStyle.SetDisabledForeground(FSlateColor(DisabledText->Color));
        FMargin ButtonPadding;
        Style->GetButtonPadding(ButtonPadding);
        // SButton adds style padding to its content slot padding. Apply the
        // Blueprint spacing once so fixed-width buttons can center their text.
        SlateStyle.SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
        Button->SetStyle(SlateStyle);
        if (UWidget* Child = Button->GetContent())
            if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Child->Slot)) ContentSlot->SetPadding(ButtonPadding);
        UCommonTextStyle* TextStyle = bSelected ? Style->GetSelectedTextStyle() : Style->GetNormalTextStyle();
        if (!TextStyle) TextStyle = Style->GetNormalTextStyle();
        ApplyFirstTextStyle(Button->GetContent(), TextStyle);
        if (Bounds)
        {
            Bounds->SetMinDesiredWidth(Style->MinWidth);
            Bounds->SetMinDesiredHeight(Style->MinHeight);
            if (Style->MinWidth > 0) Bounds->SetWidthOverride(Style->MinWidth);
            else Bounds->ClearWidthOverride();
        }
    }
    if (USlider* Slider = Cast<USlider>(Content); Slider && SliderStyle)
    {
        UBlueprintSliderStyle* Style = SliderStyle->GetDefaultObject<UBlueprintSliderStyle>();
        UUIThemePaletteLibrary::ApplyObject(Style);
        Slider->SetWidgetStyle(Style->Style);
        Slider->SetSliderBarColor(Style->BarColor);
        Slider->SetSliderHandleColor(Style->HandleColor);
        if (Bounds)
        {
            Bounds->SetWidthOverride(Style->DesiredSize.X);
            Bounds->SetHeightOverride(Style->DesiredSize.Y);
        }
    }
    if (UScrollBox* Scroll = Cast<UScrollBox>(Content); Scroll && ScrollStyle)
    {
        UBlueprintScrollStyle* Style = ScrollStyle->GetDefaultObject<UBlueprintScrollStyle>();
        UUIThemePaletteLibrary::ApplyObject(Style);
        Scroll->SetWidgetStyle(Style->Style);
        Scroll->SetWidgetBarStyle(Style->BarStyle);
        Scroll->SetScrollbarThickness(Style->Thickness);
        Scroll->SetScrollbarPadding(Style->Padding);
    }
    if (UStyleableComboBoxString* Combo = Cast<UStyleableComboBoxString>(Content); Combo && ComboStyle)
    {
        UBlueprintComboStyle* Style = ComboStyle->GetDefaultObject<UBlueprintComboStyle>();
        UUIThemePaletteLibrary::ApplyObject(Style);
        Combo->ApplyBlueprintStyle(*Style);
        if (Bounds)
        {
            Bounds->SetMinDesiredWidth(Style->DesiredSize.X);
            Bounds->SetMinDesiredHeight(Style->DesiredSize.Y);
        }
    }
    if (BrushStyle)
    {
        UBlueprintBrushStyle* Style = BrushStyle->GetDefaultObject<UBlueprintBrushStyle>();
        UUIThemePaletteLibrary::ApplyObject(Style);
        if (UBorder* Border = Cast<UBorder>(Content))
        {
            Border->SetBrush(Style->Brush);
            Border->SetBrushColor(Style->Tint);
            Border->SetPadding(Style->Padding);
        }
        if (UImage* Image = Cast<UImage>(Content))
        {
            Image->SetBrush(Style->Brush);
            Image->SetColorAndOpacity(Style->Tint);
        }
        if (UBackgroundBlur* Blur = Cast<UBackgroundBlur>(Content)) Blur->SetBlurStrength(Style->BlurStrength);
    }
}

FSlateBrush UReusableStyleLibrary::GetBrushStyle(TSubclassOf<UBlueprintBrushStyle> StyleClass)
{
    UUIThemePaletteLibrary::PreparePalette();
    if (StyleClass)
    {
        UBlueprintBrushStyle* Style = StyleClass->GetDefaultObject<UBlueprintBrushStyle>();
        UUIThemePaletteLibrary::ApplyObject(Style);
        return Style->Brush;
    }

    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
    return Brush;
}

void UReusableStyleLibrary::ApplyWidgetStyleTree(UUserWidget* Screen)
{
    UUIThemePaletteLibrary::PreparePalette();
    TSet<UWidget*> Seen;
    VisitStyleWidgets(Screen, Seen);
    UUIThemePaletteLibrary::ApplyWidgetTree(Screen);
}

void UReusableStyleLibrary::SetStyledButtonSelected(UButton* Button, bool bSelected)
{
    if (!Button) return;
    for (UWidget* Parent = Button->GetParent(); Parent; Parent = Parent->GetParent())
        if (UReusableStyleHost* Host = Parent->GetTypedOuter<UReusableStyleHost>())
        {
            Host->SetSelected(bSelected);
            return;
        }
}

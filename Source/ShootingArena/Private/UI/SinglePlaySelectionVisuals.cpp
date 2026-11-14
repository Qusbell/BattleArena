#include "UI/ReusableStyleHost.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/UIPalette.h"

namespace
{
    template <class T>
    T* FindWidget(UUserWidget* Screen, const FName Name)
    {
        return Cast<T>(Screen->GetWidgetFromName(Name));
    }

    void UpdateOutline(UUserWidget* Screen, FName Name, bool bVisible, FName Role, bool bNeutral = false)
    {
        if (UImage* Image = FindWidget<UImage>(Screen, Name))
        {
            Image->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
            if (bVisible)
            {
                FLinearColor Color = UUIThemePaletteLibrary::GetColor(Role);
                if (bNeutral)
                {
                    const float Gray = Color.GetLuminance();
                    Color = FLinearColor(Gray, Gray, Gray, Color.A);
                }
                Image->SetColorAndOpacity(Color);
            }
        }
    }

    bool HasButtonFocus(UUserWidget* Screen, UButton* Button)
    {
        APlayerController* Player = Screen->GetOwningPlayer();
        return Player && (Button->HasUserFocus(Player) || Button->HasKeyboardFocus());
    }

    void UpdateButtonMotion(UButton* Button, float DeltaTime)
    {
        const float TargetScale = Button->GetIsEnabled() && Button->IsPressed() ? 0.98f : 1.0f;
        const float Scale = FMath::FInterpConstantTo(Button->GetRenderTransform().Scale.X, TargetScale, DeltaTime, 1.0f);
        Button->SetRenderScale(FVector2D(Scale, Scale));
        Button->SetRenderOpacity(Button->GetIsEnabled() ? 1.0f : 0.4f);
    }
}

void UReusableStyleLibrary::UpdateSinglePlaySelectionVisuals(UUserWidget* Screen, int32 SelectedMap,
    int32 SelectedDifficulty, float DeltaTime)
{
    if (!Screen || !Screen->GetOwningPlayer())
        return;

    static const FName MapButtons[] = { TEXT("LevelFrame"), TEXT("LevelFrame_2") };
    static const FName MapRetainers[] = { TEXT("C2Preview1"), TEXT("C2Preview2") };
    static const FName MapSelection[] = { TEXT("C2MapSelect1"), TEXT("C2MapSelect2") };
    static const FName MapOutlines[] = { TEXT("C2MapFocus1"), TEXT("C2MapFocus2") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(MapButtons); ++Index)
    {
        UButton* Button = FindWidget<UButton>(Screen, MapButtons[Index]);
        URetainerBox* Preview = FindWidget<URetainerBox>(Screen, MapRetainers[Index]);
        if (!Button || !Preview)
            continue;

        const bool bFocused = HasButtonFocus(Screen, Button);
        const bool bHovered = Button->IsHovered();
        if (UMaterialInstanceDynamic* Material = Preview->GetEffectMaterial())
        {
            const bool bFullColor = Index == SelectedMap || bHovered || bFocused;
            const float Saturation = Material->K2_GetScalarParameterValue(TEXT("Saturation"));
            const float Brightness = Material->K2_GetScalarParameterValue(TEXT("Brightness"));
            Material->SetScalarParameterValue(TEXT("Saturation"), FMath::FInterpConstantTo(
                Saturation, bFullColor ? 1.0f : 0.15f, DeltaTime, 4.25f));
            Material->SetScalarParameterValue(TEXT("Brightness"), FMath::FInterpConstantTo(
                Brightness, bHovered ? 1.10f : bFullColor ? 1.0f : 0.78f, DeltaTime, 1.6f));
        }
        const bool bSelected = Index == SelectedMap;
        UpdateOutline(Screen, MapSelection[Index], bSelected, TEXT("Text1"));
        UpdateOutline(Screen, MapOutlines[Index], true,
            bSelected ? FName(TEXT("Accent")) : FName(TEXT("LineStrong")), !bSelected);
        UpdateButtonMotion(Button, DeltaTime);
    }

    static const FName Buttons[] = { TEXT("Button_Easy"), TEXT("Button_Normal"), TEXT("Button_Hard"),
        TEXT("Btn_Start"), TEXT("Btn_Back") };
    static const FName Selection[] = { TEXT("C2EasySelect"), TEXT("C2NormalSelect"), TEXT("C2HardSelect") };
    static const FName Labels[] = { TEXT("TextBlock_1"), TEXT("TextBlock_2"), TEXT("TextBlock_3") };
    static const FName Roles[] = { TEXT("DiffEasy"), TEXT("DiffNormal"), TEXT("DiffHard") };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Buttons); ++Index)
    {
        UButton* Button = FindWidget<UButton>(Screen, Buttons[Index]);
        if (!Button)
            continue;

        if (Index < UE_ARRAY_COUNT(Selection))
        {
            const bool bSelected = Index == SelectedDifficulty;
            UpdateOutline(Screen, Selection[Index], bSelected, Roles[Index], Index == 0);
            if (UTextBlock* Text = FindWidget<UTextBlock>(Screen, Labels[Index]))
                Text->SetColorAndOpacity(FSlateColor(UUIThemePaletteLibrary::GetColor(bSelected ? Roles[Index] : FName(TEXT("Text2")))));
            const FLinearColor Base = UUIThemePaletteLibrary::GetColor(TEXT("Bg800"));
            const FLinearColor Selected = UUIThemePaletteLibrary::GetColor(TEXT("Bg700"));
            Button->SetBackgroundColor(bSelected ? FLinearColor(Selected.R / Base.R, Selected.G / Base.G,
                Selected.B / Base.B, 1.0f) : FLinearColor::White);
        }
        UpdateButtonMotion(Button, DeltaTime);
    }

    if (UImage* Previous = FindWidget<UImage>(Screen, TEXT("C2PreviousBackground")))
    {
        const float Opacity = FMath::Max(0.0f, Previous->GetRenderOpacity() - DeltaTime / 0.24f);
        Previous->SetRenderOpacity(Opacity);
        if (Opacity == 0.0f)
            Previous->SetVisibility(ESlateVisibility::Hidden);
    }
}

void UReusableStyleLibrary::SetSinglePlayBackground(UUserWidget* Screen, UImage* Background, UTexture2D* Texture)
{
    if (!Screen || !Background || !Texture || Background->GetBrush().GetResourceObject() == Texture)
        return;

    if (UImage* Previous = FindWidget<UImage>(Screen, TEXT("C2PreviousBackground")))
    {
        if (Background->GetBrush().GetResourceObject())
        {
            Previous->SetBrush(Background->GetBrush());
            Previous->SetRenderOpacity(1.0f);
            Previous->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
    Background->SetBrushFromTexture(Texture, false);
}


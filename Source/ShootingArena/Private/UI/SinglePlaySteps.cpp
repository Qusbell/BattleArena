#include "UI/SinglePlaySteps.h"

#include "Blueprint/GameViewportSubsystem.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Localization/LanguageSubsystem.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/UIPalette.h"
#include "Widgets/SWidget.h"

namespace
{
    constexpr float TransitionDuration = 0.32f;
    constexpr float BackdropFadeDuration = 0.24f;
    constexpr float StartOutlineDuration = 0.18f;
    constexpr float StartOutlineIdleInset = 7.0f;
    constexpr float StartOutlineActiveInset = 4.0f;
    constexpr float DifficultySweepDuration = 0.56f;
    constexpr float DifficultyRevealDuration = DifficultySweepDuration * 0.45f;
    const FName DifficultyFocusImages[] = { TEXT("C2EasyFocus"), TEXT("C2NormalFocus"), TEXT("C2HardFocus") };

    template <typename T = UWidget>
    T* Find(const UUserWidget& Screen, const FName Name)
    {
        return Cast<T>(Screen.GetWidgetFromName(Name));
    }

    FLinearColor Color(const FName Name, float Alpha = 1.0f)
    {
        FLinearColor Result = UUIThemePaletteLibrary::GetColor(Name);
        Result.A *= Alpha;
        return Result;
    }

    bool NavigationFocus(const UButton* Button, bool bFocusSetByNavigation = false)
    {
        const TSharedPtr<SWidget> Widget = Button ? Button->GetCachedWidget() : nullptr;
        const TOptional<EFocusCause> Cause = Widget ? Widget->HasAnyUserFocus() : TOptional<EFocusCause>();
        return Cause == EFocusCause::Navigation || (bFocusSetByNavigation && Cause == EFocusCause::SetDirectly);
    }

    FText TableText(const TCHAR* Key)
    {
        return FText::FromStringTable(FName(ULanguageLibrary::StringTablePath()), Key);
    }

    float EaseOut(float Elapsed, float Duration)
    {
        return FMath::InterpEaseOut(0.0f, 1.0f, FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f), 3.0f);
    }

    void LayoutMapCards(const UUserWidget& Screen, const FVector2D& PanelSize)
    {
        UWidget* Cards = Find(Screen, TEXT("CanvasPanel_1"));
        const UWidget* Title = Find(Screen, TEXT("MapStepTitle"));
        const UCanvasPanelSlot* CardsSlot = Cards ? Cast<UCanvasPanelSlot>(Cards->Slot) : nullptr;
        const UCanvasPanelSlot* TitleSlot = Title ? Cast<UCanvasPanelSlot>(Title->Slot) : nullptr;
        if (!CardsSlot || !TitleSlot)
            return;

        const FVector2D DesignSize = CardsSlot->GetSize();
        const double TitleInset = TitleSlot->GetPosition().X;
        const double AvailableWidth = PanelSize.X - 2.0 * TitleInset;
        if (AvailableWidth <= 0.0 || PanelSize.Y <= 0.0 || DesignSize.X <= 0.0 || DesignSize.Y <= 0.0)
            return;

        constexpr double BottomInset = 128.0;
        const double CardsTop = (PanelSize.Y - DesignSize.Y) * 0.5;
        const double AvailableHeight = PanelSize.Y - BottomInset - CardsTop;
        if (AvailableHeight <= 0.0)
            return;

        const double Scale = FMath::Min(AvailableWidth / DesignSize.X, AvailableHeight / DesignSize.Y);
        const double CenteredLeft = (PanelSize.X - DesignSize.X * Scale) * 0.5;
        Cards->SetRenderTransformPivot(FVector2D(0.5, 0.0));
        Cards->SetRenderScale(FVector2D(Scale, Scale));
        Cards->SetRenderTranslation(FVector2D(TitleInset - CenteredLeft, 0.0));
    }
}

void USinglePlaySteps::NativeConstruct()
{
    Super::NativeConstruct();
    if (MapScreen.IsValid())
    {
        if (UButton* Easy = DifficultyButton(0))
            Easy->OnClicked.AddUniqueDynamic(this, &ThisClass::ChooseEasy);
        if (UButton* Normal = DifficultyButton(1))
            Normal->OnClicked.AddUniqueDynamic(this, &ThisClass::ChooseNormal);
        if (UButton* Hard = DifficultyButton(2))
            Hard->OnClicked.AddUniqueDynamic(this, &ThisClass::ChooseHard);
        if (UButton* Start = Find<UButton>(*this, TEXT("Btn_Start")))
            Start->OnClicked.AddUniqueDynamic(this, &ThisClass::StartMatch);
        if (UButton* Back = Find<UButton>(*this, TEXT("Btn_Back")))
            Back->OnClicked.AddUniqueDynamic(this, &ThisClass::ReturnToMap);
    }

    const UImage* StartOutline = Find<UImage>(*this, TEXT("C2StartFocus"));
    if (!StartOutline)
        return;

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(DifficultyFocusImages); ++Index)
    {
        const UButton* Button = DifficultyButton(Index);
        UImage* Outline = Find<UImage>(*this, DifficultyFocusImages[Index]);
        const UCanvasPanelSlot* ButtonSlot = Button ? Cast<UCanvasPanelSlot>(Button->Slot) : nullptr;
        UCanvasPanelSlot* OutlineSlot = Outline ? Cast<UCanvasPanelSlot>(Outline->Slot) : nullptr;
        if (!ButtonSlot || !OutlineSlot)
            continue;

        constexpr float VerticalInset = 4.0f;
        OutlineSlot->SetLayout(ButtonSlot->GetLayout());
        OutlineSlot->SetPosition(ButtonSlot->GetPosition() + FVector2D(0.0f, VerticalInset));
        OutlineSlot->SetSize(ButtonSlot->GetSize() - FVector2D(0.0f, 2.0f * VerticalInset));
        OutlineSlot->SetZOrder(ButtonSlot->GetZOrder() + 1);
        Outline->SetBrush(StartOutline->GetBrush());
    }
}

void USinglePlaySteps::NativeDestruct()
{
    if (USinglePlaySteps* Screen = DifficultyScreen)
    {
        DifficultyScreen = nullptr;
        Screen->RemoveFromParent();
    }
    Super::NativeDestruct();
}

void USinglePlaySteps::ReturnToMap()
{
    if (USinglePlaySteps* Map = MapScreen.Get())
        Map->OpenMapStep();
}

UTextBlock* USinglePlaySteps::GetLaunchErrorText() const
{
    return Find<UTextBlock>(*this, TEXT("SinglePlayLaunchError"));
}

UButton* USinglePlaySteps::MapButton(int32 Index) const
{
    return Find<UButton>(*this, Index == 1 ? TEXT("LevelFrame_2") : TEXT("LevelFrame"));
}

UButton* USinglePlaySteps::DifficultyButton(int32 Index) const
{
    static const FName Names[] = { TEXT("Button_Easy"), TEXT("Button_Normal"), TEXT("Button_Hard") };
    return Find<UButton>(*this, Names[FMath::Clamp(Index, 0, 2)]);
}

void USinglePlaySteps::OpenMapStep()
{
    ChangeStep(false, bDifficultyStep);
}

void USinglePlaySteps::OpenDifficultyStep(int32 SelectedMap)
{
    if (bDifficultyStep || !DifficultyScreenClass || !GetOwningPlayer())
        return;

    if (!DifficultyScreen)
    {
        DifficultyScreen = CreateWidget<USinglePlaySteps>(GetOwningPlayer(), DifficultyScreenClass);
        if (!DifficultyScreen)
            return;
        DifficultyScreen->MapScreen = this;
    }

    if (SelectedMap >= 0)
        MapIndex = FMath::Clamp(SelectedMap, 0, 1);
    // Commit the highlighted row even when the player uses Enter on the default card.
    ChooseMap(MapIndex);
    if (const UImage* Preview = Find<UImage>(*this, MapIndex == 1 ? TEXT("Lvl_Image_2") : TEXT("Lvl_Image")))
        PreviewBackdrop = Preview->GetBrush();

    DifficultyScreen->DifficultyIndex = DifficultyIndex;
    DifficultyScreen->PreviousDifficultyIndex = DifficultyIndex;
    DifficultyScreen->DifficultySelectionElapsed = DifficultySweepDuration;
    if (UTextBlock* Breadcrumb = Find<UTextBlock>(*DifficultyScreen, TEXT("DifficultyBreadcrumb")))
        if (const UTextBlock* MapName = Find<UTextBlock>(*this, MapIndex == 1 ? TEXT("MapName_2") : TEXT("MapName_1")))
            Breadcrumb->SetText(FText::Format(TableText(TEXT("SinglePlay_MapBreadcrumb")), TableText(TEXT("UI_2395ba340f11")), MapName->GetText()));

    if (!DifficultyScreen->IsInViewport())
    {
        DifficultyScreen->SetRenderOpacity(0.0f);
        DifficultyScreen->SetRenderTranslation(FVector2D(48.0f, 0.0f));
        const UGameViewportSubsystem* Viewport = UGameViewportSubsystem::Get(GetWorld());
        const int32 MapZOrder = Viewport ? Viewport->GetWidgetSlot(this).ZOrder : 0;
        DifficultyScreen->AddToViewport(MapZOrder + 1);
    }
    bHasOpenedDifficulty = true;
    ChangeStep(true, true);
}

void USinglePlaySteps::HoverMap(int32 Index)
{
    if (bDifficultyStep || Index == MapIndex)
        return;
    MapIndex = FMath::Clamp(Index, 0, 1);
    ChooseMap(Index);
    if (UButton* Button = MapButton(Index))
        Button->SetFocus();
}

void USinglePlaySteps::ChangeStep(bool bNext, bool bAnimate)
{
    bDifficultyStep = bNext;
    UpdateBackdrop();
    TransitionElapsed = bAnimate ? 0.0f : TransitionDuration;
    if (UWidget* Map = Find(*this, TEXT("Panel_MapStep")))
        Map->SetVisibility(bNext ? (bAnimate ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden)
            : ESlateVisibility::SelfHitTestInvisible);
    SetVisibility(bNext ? ESlateVisibility::HitTestInvisible : ESlateVisibility::SelfHitTestInvisible);
    if (UWidget* Back = Find(*this, TEXT("BackButtonBounds")))
        Back->SetVisibility(bNext ? ESlateVisibility::Hidden : ESlateVisibility::SelfHitTestInvisible);
    if (DifficultyScreen)
        DifficultyScreen->SetVisibility(bNext ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::HitTestInvisible);
    if (UWidget* Target = bNext ? DifficultyScreen->DifficultyButton(DifficultyIndex) : MapButton(MapIndex))
        Target->SetFocus();
}

FReply USinglePlaySteps::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    return HandleStepKey(Event.GetKey(), Event.IsRepeat()) ? FReply::Handled()
        : Super::NativeOnPreviewKeyDown(Geometry, Event);
}

bool USinglePlaySteps::IsStepKey(const FKey& Key)
{
    return Key == EKeys::Enter || Key == EKeys::Escape || Key == EKeys::Tab
        || Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Up || Key == EKeys::Down
        || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Gamepad_FaceButton_Right
        || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right
        || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down
        || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right
        || Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down;
}

bool USinglePlaySteps::HandleStepKey(const FKey& Key, bool bRepeat)
{
    const USinglePlaySteps* Map = MapScreen.Get();
    if (Map ? !Map->bDifficultyStep : bDifficultyStep)
        return false;

    const bool bConfirm = Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom;
    const bool bBack = Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right;
    if (bBack)
    {
        if (!bRepeat)
        {
            if (Map)
                ReturnToMap();
            else if (UButton* Back = Find<UButton>(*this, TEXT("Btn_Back")))
                Back->OnClicked.Broadcast();
        }
        return true;
    }

    if (!Map)
    {
        if (bConfirm)
        {
            if (!bRepeat)
                OpenDifficultyStep();
            return true;
        }
        if (Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Left
            || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Left || Key == EKeys::Gamepad_LeftStick_Right)
        {
            const int32 Next = Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left ? 0 : 1;
            if (Next != MapIndex)
            {
                MapIndex = Next;
                ChooseMap(Next);
            }
            if (UButton* Button = MapButton(Next))
                Button->SetFocus();
            return true;
        }
    }
    else
    {
        if (Key == EKeys::Enter)
            return true;
        if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right || Key == EKeys::Tab)
        {
            if (UButton* Start = Find<UButton>(*this, TEXT("Btn_Start")))
            {
                Start->SetFocus();
                bStartFocused = true;
            }
            return true;
        }
        if (Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Up
            || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Up || Key == EKeys::Gamepad_LeftStick_Down)
        {
            const bool bUp = Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up;
            const int32 Next = FMath::Clamp(DifficultyIndex + (bUp ? -1 : 1), 0, 2);
            if (UButton* Difficulty = DifficultyButton(Next))
            {
                if (Next != DifficultyIndex)
                    Difficulty->OnClicked.Broadcast();
                Difficulty->SetFocus();
                DifficultyNavigationTarget = Difficulty;
            }
            return true;
        }
    }
    return false;
}

void USinglePlaySteps::SetBackdropBrush(const FSlateBrush& Brush)
{
    if (PreviewBackdrop.GetResourceObject() != Brush.GetResourceObject())
    {
        PreviewBackdrop = Brush;
        UpdateBackdrop();
    }
}

void USinglePlaySteps::SetMapBackdrop(UTexture2D* Texture)
{
    if (Texture && MapBackdrop.GetResourceObject() != Texture)
    {
        MapBackdrop.SetResourceObject(Texture);
        UpdateBackdrop();
    }
}

void USinglePlaySteps::UpdateBackdrop()
{
    const FSlateBrush& Brush = bDifficultyStep && PreviewBackdrop.GetResourceObject() ? PreviewBackdrop : MapBackdrop;
    UImage* Background = Find<UImage>(*this, TEXT("C2SelectedBackground"));
    if (!Background || !Brush.GetResourceObject() || Background->GetBrush().GetResourceObject() == Brush.GetResourceObject())
        return;
    if (UImage* Previous = Find<UImage>(*this, TEXT("C2PreviousBackground")); Previous && Background->GetBrush().GetResourceObject())
    {
        Previous->SetBrush(Background->GetBrush());
        Previous->SetVisibility(ESlateVisibility::HitTestInvisible);
        Previous->SetRenderOpacity(1.0f);
        BackdropFadeRemaining = BackdropFadeDuration;
    }
    Background->SetBrush(Brush);
}

void USinglePlaySteps::UpdateStepVisuals(int32 Map, int32 Difficulty, float DeltaTime)
{
    if (USinglePlaySteps* Owner = MapScreen.Get())
    {
        if (Owner->bDifficultyStep)
        {
            UpdateDifficultyVisuals(Difficulty, DeltaTime);
            Owner->DifficultyIndex = DifficultyIndex;
        }
        return;
    }

    MapIndex = FMath::Clamp(Map, 0, 1);
    if (!bHasOpenedDifficulty)
        DifficultyIndex = FMath::Clamp(Difficulty, 0, 2);
    if (bInitialMapFocus)
    {
        bInitialMapFocus = false;
        if (!bDifficultyStep)
            if (UButton* Button = MapButton(MapIndex))
                Button->SetFocus();
    }
    TransitionElapsed = FMath::Min(TransitionElapsed + DeltaTime, TransitionDuration);
    const float Progress = FMath::InterpEaseOut(0.0f, 1.0f, TransitionElapsed / TransitionDuration, 3.0f);
    if (UWidget* MapPanel = Find(*this, TEXT("Panel_MapStep")))
    {
        LayoutMapCards(*this, MapPanel->GetCachedGeometry().GetLocalSize());
        MapPanel->SetRenderTranslation(FVector2D(-48.0f * (bDifficultyStep ? Progress : 1.0f - Progress), 0));
        MapPanel->SetRenderOpacity(bDifficultyStep ? 1.0f - Progress : Progress);
        if (bDifficultyStep && Progress >= 1.0f)
            MapPanel->SetVisibility(ESlateVisibility::Hidden);
    }
    if (DifficultyScreen)
    {
        DifficultyScreen->SetRenderTranslation(FVector2D(48.0f * (bDifficultyStep ? 1.0f - Progress : Progress), 0));
        DifficultyScreen->SetRenderOpacity(bDifficultyStep ? Progress : 1.0f - Progress);
        if (!bDifficultyStep && Progress >= 1.0f)
        {
            USinglePlaySteps* ClosingScreen = DifficultyScreen;
            DifficultyScreen = nullptr;
            ClosingScreen->RemoveFromParent();
        }
    }

    const float DifficultyBlend = bDifficultyStep ? Progress : 1.0f - Progress;
    if (UBackgroundBlur* Blur = Find<UBackgroundBlur>(*this, TEXT("SinglePlayBackgroundBlur")))
        Blur->SetBlurStrength(FMath::Lerp(10.0f, 0.0f, DifficultyBlend));
    if (UImage* Scrim = Find<UImage>(*this, TEXT("C2BackgroundScrim")))
        Scrim->SetRenderOpacity(1.0f - DifficultyBlend);
    if (UImage* Gradient = Find<UImage>(*this, TEXT("DifficultyBackdropGradient")))
        Gradient->SetRenderOpacity(DifficultyBlend);
    if (BackdropFadeRemaining > 0.0f)
    {
        BackdropFadeRemaining = FMath::Max(0.0f, BackdropFadeRemaining - DeltaTime);
        if (UImage* Previous = Find<UImage>(*this, TEXT("C2PreviousBackground")))
        {
            Previous->SetRenderOpacity(BackdropFadeRemaining / BackdropFadeDuration);
            if (BackdropFadeRemaining <= 0.0f)
                Previous->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    static const FName Previews[] = { TEXT("C2Preview1"), TEXT("C2Preview2") };
    static const FName Borders[] = { TEXT("C2MapFocus1"), TEXT("C2MapFocus2") };
    static const FName Bars[] = { TEXT("C2MapSelect1"), TEXT("C2MapSelect2") };
    static const FName Names[] = { TEXT("MapName_1"), TEXT("MapName_2") };
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const bool bSelected = Index == MapIndex;
        const UButton* Button = MapButton(Index);
        const bool bHovered = Button && Button->IsHovered();
        if (URetainerBox* Retainer = Find<URetainerBox>(*this, Previews[Index]))
        {
            if (UMaterialInstanceDynamic* Effect = Retainer->GetEffectMaterial())
            {
                const float Saturation = bSelected ? 1.0f : bHovered ? 0.8f : 0.4f;
                const float Brightness = bSelected ? 1.0f : bHovered ? 0.8f : 0.55f;
                Effect->SetScalarParameterValue(TEXT("Saturation"), FMath::FInterpConstantTo(
                    Effect->K2_GetScalarParameterValue(TEXT("Saturation")), Saturation, DeltaTime, 3.0f));
                Effect->SetScalarParameterValue(TEXT("Brightness"), FMath::FInterpConstantTo(
                    Effect->K2_GetScalarParameterValue(TEXT("Brightness")), Brightness, DeltaTime, 2.25f));
            }
        }
        if (UImage* Border = Find<UImage>(*this, Borders[Index]))
            Border->SetColorAndOpacity(bSelected ? Color(TEXT("White")) : Color(TEXT("Text1"), 0.10f));
        if (UImage* Bar = Find<UImage>(*this, Bars[Index]))
            Bar->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
        if (UTextBlock* Name = Find<UTextBlock>(*this, Names[Index]))
            Name->SetColorAndOpacity(FSlateColor(Color(TEXT("Text1"), bSelected ? 1.0f : 0.55f)));
    }

}

void USinglePlaySteps::UpdateDifficultyVisuals(int32 Difficulty, float DeltaTime)
{
    const int32 NextDifficulty = FMath::Clamp(Difficulty, 0, 2);
    if (NextDifficulty != DifficultyIndex)
    {
        PreviousDifficultyIndex = DifficultyIndex;
        DifficultySelectionElapsed = 0.0f;
        if (const UButton* Button = DifficultyButton(NextDifficulty))
        {
            const UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Button->Slot);
            if (UImage* Sweep = Find<UImage>(*this, TEXT("DifficultySelectionSweep")); ButtonSlot && Sweep)
                if (UCanvasPanelSlot* SweepSlot = Cast<UCanvasPanelSlot>(Sweep->Slot))
                    SweepSlot->SetPosition(ButtonSlot->GetPosition());
        }
    }
    else
    {
        DifficultySelectionElapsed = FMath::Min(DifficultySelectionElapsed + DeltaTime, DifficultySweepDuration);
    }
    DifficultyIndex = NextDifficulty;

    static const FName Roles[] = { TEXT("DiffEasy"), TEXT("DiffNormalGlowBlue"), TEXT("DiffHard") };
    static const FName Labels[] = { TEXT("TextBlock_1"), TEXT("TextBlock_2"), TEXT("TextBlock_3") };
    static const FName Markers[] = { TEXT("C2EasySelect"), TEXT("C2NormalSelect"), TEXT("C2HardSelect") };
    static const TCHAR* Descriptions[] = { TEXT("SinglePlay_DescriptionEasy"), TEXT("SinglePlay_DescriptionNormal"),
        TEXT("SinglePlay_DescriptionHard") };
    const float SelectedProgress = EaseOut(DifficultySelectionElapsed, 0.26f);
    const float PreviousProgress = EaseOut(DifficultySelectionElapsed, 0.20f);
    const bool bSweepPlaying = DifficultySelectionElapsed < DifficultySweepDuration;
    if (UImage* Sweep = Find<UImage>(*this, TEXT("DifficultySelectionSweep")))
    {
        Sweep->SetVisibility(bSweepPlaying ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
        if (bSweepPlaying)
        {
            const float Fade = EaseOut(DifficultySelectionElapsed - DifficultyRevealDuration,
                DifficultySweepDuration - DifficultyRevealDuration);
            Sweep->SetColorAndOpacity(Color(Roles[DifficultyIndex], 0.22f * (1.0f - Fade)));
            if (UMaterialInstanceDynamic* Material = Sweep->GetDynamicMaterial())
                Material->SetScalarParameterValue(TEXT("Reveal"), EaseOut(DifficultySelectionElapsed, DifficultyRevealDuration));
        }
    }
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const bool bSelected = Index == DifficultyIndex;
        const UButton* Button = DifficultyButton(Index);
        const bool bPressed = Button && Button->IsPressed();
        const bool bHovered = Button && Button->IsHovered();
        const bool bFocused = NavigationFocus(Button, DifficultyNavigationTarget.Get() == Button);
        const float PressOpacity = bPressed ? 0.92f : 1.0f;
        const float RoleWeight = bSelected ? SelectedProgress : Index == PreviousDifficultyIndex ? 1.0f - PreviousProgress : 0.0f;
        const FLinearColor SelectedColor = Color(Roles[Index], PressOpacity);
        if (UImage* Marker = Find<UImage>(*this, Markers[Index]))
        {
            const FLinearColor Neutral = Color(TEXT("Text1"), (bHovered ? 0.28f : 0.14f) * PressOpacity);
            Marker->SetColorAndOpacity(FMath::Lerp(Neutral, SelectedColor, RoleWeight));
            Marker->SetRenderScale(FVector2D(1.0f, bSelected ? FMath::Lerp(0.25f, 1.0f, SelectedProgress) : 1.0f));
        }
        if (UTextBlock* Label = Find<UTextBlock>(*this, Labels[Index]))
        {
            const FLinearColor Neutral = Color(TEXT("Text1"), (bHovered ? 0.70f : 0.42f) * PressOpacity);
            Label->SetColorAndOpacity(FSlateColor(FMath::Lerp(Neutral, SelectedColor, RoleWeight)));
            Label->SetRenderTranslation(FVector2D(bSelected ? -10.0f * (1.0f - SelectedProgress) : 0.0f, 0.0f));
        }
        if (UImage* Outline = Find<UImage>(*this, DifficultyFocusImages[Index]))
        {
            Outline->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
            Outline->SetColorAndOpacity(Color(TEXT("Accent")));
        }
    }
    if (UTextBlock* Description = Find<UTextBlock>(*this, TEXT("DifficultyDescription")))
    {
        const float DescriptionProgress = EaseOut(DifficultySelectionElapsed, 0.30f);
        Description->SetText(TableText(Descriptions[DifficultyIndex]));
        Description->SetRenderOpacity(DescriptionProgress);
        Description->SetRenderTranslation(FVector2D(0.0f, 8.0f * (1.0f - DescriptionProgress)));
    }
    if (UButton* Start = Find<UButton>(*this, TEXT("Btn_Start")))
    {
        bStartFocused = bStartFocused && Start->HasAnyUserFocus();
        const bool bEmphasized = Start->GetIsEnabled() && (Start->IsHovered() || NavigationFocus(Start, bStartFocused));
        const float Brightness = Start->IsPressed() ? 0.92f : 1.0f;
        Start->SetBackgroundColor(FLinearColor(Brightness, Brightness, Brightness, 1.0f));
        Start->SetRenderOpacity(Start->GetIsEnabled() ? 1.0f : 0.4f);
        StartOutlineInset = FMath::FInterpConstantTo(StartOutlineInset,
            bEmphasized ? StartOutlineActiveInset : StartOutlineIdleInset,
            DeltaTime, (StartOutlineIdleInset - StartOutlineActiveInset) / StartOutlineDuration);
        if (UImage* Outline = Find<UImage>(*this, TEXT("C2StartFocus")))
        {
            Outline->SetRenderScale(FVector2D((320.0f + 2.0f * StartOutlineInset) / 334.0f,
                (80.0f + 2.0f * StartOutlineInset) / 94.0f));
            const float Emphasis = (StartOutlineIdleInset - StartOutlineInset) / (StartOutlineIdleInset - StartOutlineActiveInset);
            Outline->SetColorAndOpacity(Color(TEXT("Accent"), FMath::Lerp(0.45f, 1.0f, Emphasis)));
        }
    }
}

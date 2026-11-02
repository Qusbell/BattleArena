#include "UI/CompetitiveRankingHUD.h"
#include "UI/CompetitiveRankingOrder.h"
#include "UI/KillMedalUISettings.h"
#include "Audio/KillStreakVoiceComponent.h"
#include "UI/UIPalette.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/FontCache.h"

namespace
{
	constexpr float PanelWidth = 286.0f;
	constexpr float PanelHeight = 56.0f;
	constexpr float RankingScale = 0.975f;
	constexpr float TimerGap = 5.0f;

	int32 ReadInt(const UObject* Object, FName Name)
	{
		const FIntProperty* Property = IsValid(Object) ? FindFProperty<FIntProperty>(Object->GetClass(), Name) : nullptr;
		return Property ? Property->GetPropertyValue_InContainer(Object) : 0;
	}
}

UCompetitiveRankingHUD::UCompetitiveRankingHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FClassFinder<UUserWidget> Highlight(
		TEXT("/Game/CompShooterHUD/UI/Content/Medals/WB_Medal_Highlight"));
	StreakHighlightClass = Highlight.Class;
}

void UCompetitiveRankingHUD::HideStreakMedals()
{
	if (StreakPanel) StreakPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UCompetitiveRankingHUD::RefreshStreakMedals(int32 Count)
{
	Count = FMath::Max(0, Count);
	if (DisplayedStreak == Count) return;
	if (!ActiveStreakSettings) ActiveStreakSettings = KillMedalUISettings.LoadSynchronous();
	const UKillMedalUISettings* Settings = ActiveStreakSettings ? ActiveStreakSettings.Get() : GetDefault<UKillMedalUISettings>();
	const bool bLargeMedal = Count >= FMath::Max(1, Settings->LargeMedalMinKills);
	if (!StreakPanel && Count > 0)
	{
		UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetRootWidget());
		if (!Canvas || !WidgetTree) return;
		StreakPanel = WidgetTree->ConstructWidget<UVerticalBox>();
		StreakPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* StreakSlot = Canvas->AddChildToCanvas(StreakPanel);
		StreakSlot->SetAnchors(FAnchors(Settings->MedalAnchor.X, Settings->MedalAnchor.Y));
		StreakSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		StreakSlot->SetPosition(Settings->MedalPosition);
		StreakSlot->SetAutoSize(true);
		StreakSlot->SetZOrder(11);
		StreakIcons = WidgetTree->ConstructWidget<UHorizontalBox>();
		UHorizontalBox* CountRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		CountRow->SetRenderTranslation(Settings->CountPosition);
		StreakCountLabel = WidgetTree->ConstructWidget<UTextBlock>();
		StreakCountLabel->SetFont(Settings->CountFont);
		StreakCountLabel->SetColorAndOpacity(FSlateColor(Settings->CountColor));
		StreakCountLabel->SetShadowColorAndOpacity(FLinearColor::Black);
		StreakCountLabel->SetShadowOffset(FVector2D(1, 1));
		UTexture2D* AccentTexture = LoadObject<UTexture2D>(nullptr,
			TEXT("/Game/CompShooterHUD/Textures/Highlights/T_Highlight2_1920x500.T_Highlight2_1920x500"));
		const float FontSize = FMath::Max(1.0f, Settings->CountFont.Size);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			UImage* Accent = WidgetTree->ConstructWidget<UImage>();
			FSlateBrush Brush;
			Brush.SetResourceObject(AccentTexture);
			Brush.ImageSize = FVector2D(FontSize * 2.25f, FontSize * 0.5f);
			Brush.SetUVRegion(FBox2f(FVector2f(0, 0), FVector2f(0.5f, 1)));
			Brush.Mirroring = Side == 0 ? ESlateBrushMirrorType::NoMirror : ESlateBrushMirrorType::Horizontal;
			Accent->SetBrush(Brush);
			Accent->SetColorAndOpacity(Settings->CountColor);
			UHorizontalBoxSlot* AccentSlot = CountRow->AddChildToHorizontalBox(Accent);
			AccentSlot->SetVerticalAlignment(VAlign_Center);
			AccentSlot->SetPadding(Side == 0 ? FMargin(0, 0, FontSize * 0.25f, 0) : FMargin(FontSize * 0.25f, 0, 0, 0));
			if (Side == 0) CountRow->AddChildToHorizontalBox(StreakCountLabel)->SetVerticalAlignment(VAlign_Center);
		}
		UVerticalBoxSlot* CountSlot = StreakPanel->AddChildToVerticalBox(CountRow);
		CountSlot->SetHorizontalAlignment(HAlign_Center);
		CountSlot->SetPadding(FMargin(0, 0, 0, 8));
		StreakPanel->AddChildToVerticalBox(StreakIcons)->SetHorizontalAlignment(HAlign_Center);
	}
	DisplayedStreak = Count;
	if (!StreakPanel) return;
	if (StreakHighlight) StreakHighlight->StopAnimationsAndLatentActions();
	StreakHighlight = nullptr;
	StreakPanel->SetVisibility(Count > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	StreakIcons->ClearChildren();
	const int32 IconCount = bLargeMedal ? 1 : Count;
	UTexture2D* Texture = bLargeMedal && Settings->LargeMedalTexture ? Settings->LargeMedalTexture.Get() : Settings->MedalTexture.Get();
	for (int32 Index = 0; Index < IconCount; ++Index)
	{
		const FVector2D Size = bLargeMedal ? Settings->LargeMedalSize : Index == 0 ? Settings->FirstMedalSize : Settings->AdditionalMedalSize;
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>();
		Box->SetWidthOverride(FMath::Max(1.0, Size.X));
		Box->SetHeightOverride(FMath::Max(1.0, Size.Y));
		UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
		Scale->SetStretch(EStretch::ScaleToFit);
		Box->SetContent(Scale);
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrushFromTexture(Texture, true);
		Scale->SetContent(Image);
		if (Index == IconCount - 1 && StreakHighlightClass)
		{
			StreakHighlight = CreateWidget<UUserWidget>(this, StreakHighlightClass);
			if (StreakHighlight)
			{
				Scale->SetContent(nullptr);
				UOverlay* Award = WidgetTree->ConstructWidget<UOverlay>();
				Scale->SetContent(Award);
				Award->AddChildToOverlay(Image);
				UOverlaySlot* HighlightSlot = Award->AddChildToOverlay(StreakHighlight);
				HighlightSlot->SetHorizontalAlignment(HAlign_Center);
				HighlightSlot->SetVerticalAlignment(VAlign_Center);
				StreakHighlight->SetVisibility(ESlateVisibility::HitTestInvisible);
				// CompShooter's 1024-wide highlight surrounds a 128-wide medal.
				StreakHighlight->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
				StreakHighlight->SetRenderScale(FVector2D(8.0f, 8.0f));
				StreakHighlight->SetRenderOpacity(FMath::Clamp(Settings->HighlightOpacity, 0.0f, 1.0f));
				if (UFunction* Play = StreakHighlight->FindFunction(TEXT("PlayHighlight")))
					StreakHighlight->ProcessEvent(Play, nullptr);
			}
		}
		UHorizontalBoxSlot* IconSlot = StreakIcons->AddChildToHorizontalBox(Box);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(FMath::Max(0.0f, Settings->MedalGap) * 0.5f, 0.0f));
	}
	StreakCountLabel->SetVisibility(bLargeMedal ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	StreakCountLabel->GetParent()->SetVisibility(StreakCountLabel->GetVisibility());
	StreakCountLabel->SetText(bLargeMedal
		? FText::AsNumber(Count)
		: FText::GetEmpty());
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StreakDisplayTimer);
		if (Count > 0)
		{
			const float Duration = FMath::IsFinite(Settings->DisplayDuration) ? FMath::Max(0.01f, Settings->DisplayDuration) : 1.2f;
			World->GetTimerManager().SetTimer(StreakDisplayTimer, this, &ThisClass::HideStreakMedals, Duration, false);
		}
	}
}

void UCompetitiveRankingHUD::NativeConstruct()
{
	UUIThemePaletteLibrary::PreparePalette();
	Super::NativeConstruct();
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(GetRootWidget());
	if (!Canvas || !WidgetTree || RankingPanel) return;
	RankingPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CompetitiveRanking"));
	RankingPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	RankingSlot = Canvas->AddChildToCanvas(RankingPanel);
	RankingSlot->SetSize(FVector2D(PanelWidth, PanelHeight));
	RankingSlot->SetZOrder(10);
	LeaderProgress = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("RankingProgress"));
	FProgressBarStyle ProgressStyle = LeaderProgress->GetWidgetStyle();
	ProgressStyle.BackgroundImage.TintColor = FSlateColor(UUIThemePaletteLibrary::GetColor(TEXT("Transparent")));
	ProgressStyle.FillImage = FSlateBrush();
	ProgressStyle.FillImage.DrawAs = ESlateBrushDrawType::Image;
	ProgressStyle.FillImage.TintColor = FSlateColor(UUIThemePaletteLibrary::GetColor(TEXT("PureWhite")));
	LeaderProgress->SetWidgetStyle(ProgressStyle);
	LeaderProgress->SetFillColorAndOpacity(UUIThemePaletteLibrary::GetColor(TEXT("RankingOpponent")));
	USizeBox* ProgressHeight = WidgetTree->ConstructWidget<USizeBox>();
	ProgressHeight->SetHeightOverride(4.0f);
	ProgressHeight->SetContent(LeaderProgress);
	RankingPanel->AddChildToVerticalBox(ProgressHeight);

	UUserWidget* Timer = Cast<UUserWidget>(GetWidgetFromName(TEXT("WB_Timer")));
	UTextBlock* TimerText = Timer ? Cast<UTextBlock>(Timer->GetWidgetFromName(TEXT("Text_Minutes"))) : nullptr;
	FSlateFontInfo Font = TimerText ? TimerText->GetFont() : FCoreStyle::GetDefaultFontStyle("Regular", 16);
	if (USizeBox* TimerBox = Timer ? Cast<USizeBox>(Timer->GetWidgetFromName(TEXT("SB_Timer"))) : nullptr)
	{
		const auto Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		const auto FontCache = FSlateApplication::Get().GetRenderer()->GetFontCache();
		const FCharacterEntry& DigitGlyph = FontCache->GetCharacterList(Font, 1.0f).GetCharacter(TEXT('0'), EFontFallback::FF_Max);
		if (DigitGlyph.Valid && DigitGlyph.VSize > 0)
			Font.Size = FMath::RoundToInt(Font.Size * PanelHeight * RankingScale / DigitGlyph.VSize);
		if (UTextBlock* Colon = Cast<UTextBlock>(Timer->GetWidgetFromName(TEXT("Text_Points"))))
			Colon->SetFont(Font);
		float DigitWidth = 0;
		for (TCHAR Digit = TEXT('0'); Digit <= TEXT('9'); ++Digit)
			DigitWidth = FMath::Max(DigitWidth, float(Measure->Measure(FString::Chr(Digit), Font).X));
		FSlateFontInfo DigitFont = Font;
		DigitFont.bForceMonospaced = true;
		DigitFont.MonospacedWidth = DigitWidth / Font.Size;
		for (const TCHAR* Name : {TEXT("Text_Minutes"), TEXT("Text_Seconds")})
			if (UTextBlock* Text = Cast<UTextBlock>(Timer->GetWidgetFromName(Name)))
			{
				Text->SetFont(DigitFont);
				Text->SetMinDesiredWidth(2.0f * DigitWidth);
				Text->SetJustification(ETextJustify::Right);
			}
		// Reserve the widest digits so time changes never alter the frame width.
		const float TimerWidth = FMath::CeilToFloat(4.0f * DigitWidth + Measure->Measure(TEXT(":"), Font).X + 4.0f);
		ensure(TimerWidth > 4.0f * DigitWidth);
		TimerBox->SetWidthOverride(TimerWidth);
	}
	Font = FCoreStyle::GetDefaultFontStyle("Regular", 14);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UBorder* Outline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("RankingRow%d"), Index));
		Outline->SetPadding(FMargin(1.0f));
		RankingPanel->AddChildToVerticalBox(Outline);
		RowBorders.Add(Outline);
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetBrushColor(UUIThemePaletteLibrary::GetColor(TEXT("RankingBackground")));
		Background->SetPadding(FMargin(4.0f, 0.0f));
		Outline->SetContent(Background);
		USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
		Height->SetHeightOverride(24.0f);
		Background->SetContent(Height);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Height->SetContent(Row);
		for (int32 Column = 0; Column < 3; ++Column)
		{
			const TCHAR* Prefix = Column == 0 ? TEXT("Rank") : Column == 1 ? TEXT("Name") : TEXT("Score");
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
				*FString::Printf(TEXT("Ranking%s%d"), Prefix, Index));
			Text->SetFont(Font);
			Text->SetColorAndOpacity(FSlateColor(UUIThemePaletteLibrary::GetColor(TEXT("PureWhite"))));
			Text->SetShadowColorAndOpacity(UUIThemePaletteLibrary::GetColor(TEXT("RankingShadow")));
			Text->SetShadowOffset(FVector2D(1, 1));
			if (Column == 1)
			{
				Text->SetShadowOffset(FVector2D::ZeroVector);
				Text->SetPixelSnapping(EWidgetPixelSnapping::SnapToPixel);
				Text->SetClipping(EWidgetClipping::ClipToBounds);
				Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
				UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(Text);
				NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				NameSlot->SetVerticalAlignment(VAlign_Center);
				NameLabels.Add(Text);
			}
			else
			{
				USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
				Width->SetWidthOverride(Column == 0 ? 28.0f : 44.0f);
				Width->SetContent(Text);
				CastChecked<USizeBoxSlot>(Text->Slot)->SetVerticalAlignment(VAlign_Center);
				Row->AddChildToHorizontalBox(Width);
				if (Column == 0) RankLabels.Add(Text);
				else { Text->SetJustification(ETextJustify::Right); ScoreLabels.Add(Text); }
			}
		}
	}
	RefreshRanking();
}

void UCompetitiveRankingHUD::BindScoreChanges(AGameStateBase* State)
{
	if (BoundGameState.Get() == State) return;
	FScriptDelegate Delegate;
	Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(UCompetitiveRankingHUD, RefreshRanking));
	if (AGameStateBase* Previous = BoundGameState.Get())
		if (const auto* Property = FindFProperty<FMulticastDelegateProperty>(Previous->GetClass(), TEXT("OnAnyPlayerScoreChanged")))
			Property->RemoveDelegate(Delegate, Previous);
	BoundGameState = State;
	if (State)
		if (const auto* Property = FindFProperty<FMulticastDelegateProperty>(State->GetClass(), TEXT("OnAnyPlayerScoreChanged")))
			Property->AddDelegate(Delegate, State);
}

void UCompetitiveRankingHUD::RefreshRanking()
{
	if (!RankingPanel) return;
	AGameStateBase* State = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	BindScoreChanges(State);
	APlayerController* PC = GetOwningPlayer();
	APlayerState* Local = PC ? PC->PlayerState : nullptr;
	TArray<FCompetitiveRankEntry> Players;
	if (State)
		for (APlayerState* Player : State->PlayerArray)
			if (IsValid(Player) && !Player->IsActorBeingDestroyed() && !Player->IsInactive())
				if (const auto* Kills = FindFProperty<FIntProperty>(Player->GetClass(), TEXT("KillCount")))
					Players.Add({Player, Kills->GetPropertyValue_InContainer(Player)});
	const TArray<FCompetitiveRankEntry> Entries = BuildCompactRanking(MoveTemp(Players), Local);
	for (int32 Index = 0; Index < RowBorders.Num(); ++Index)
	{
		RowBorders[Index]->SetVisibility(Entries.IsValidIndex(Index) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (!Entries.IsValidIndex(Index)) continue;
		const auto& Entry = Entries[Index];
		const bool IsLocal = Entry.Player == Local;
		const bool IsLeader = Entry.Rank == 1;
		const FName OutlineRole = IsLocal
			? (IsLeader ? TEXT("RankingLocal") : TEXT("RankingLocalOutlineMuted"))
			: (IsLeader ? TEXT("RankingOpponent") : TEXT("RankingOpponentOutlineMuted"));
		const FName BackgroundRole = IsLocal
			? (IsLeader ? TEXT("RankingLocalBackgroundLeader") : TEXT("RankingLocalBackground"))
			: (IsLeader ? TEXT("RankingOpponentBackgroundLeader") : TEXT("RankingOpponentBackground"));
		RowBorders[Index]->SetBrushColor(UUIThemePaletteLibrary::GetColor(OutlineRole));
		CastChecked<UBorder>(RowBorders[Index]->GetContent())->SetBrushColor(UUIThemePaletteLibrary::GetColor(BackgroundRole));
		const FSlateColor TextColor(UUIThemePaletteLibrary::GetColor(IsLeader ? TEXT("RankingLeaderText")
			: IsLocal ? TEXT("RankingLocalText") : TEXT("RankingOpponentText")));
		RankLabels[Index]->SetColorAndOpacity(TextColor);
		NameLabels[Index]->SetColorAndOpacity(TextColor);
		ScoreLabels[Index]->SetColorAndOpacity(TextColor);
		RankLabels[Index]->SetText(FText::AsNumber(Entry.Rank));
		NameLabels[Index]->SetText(UShooterDisplayLibrary::GetPlayerStateDisplayName(Entry.Player));
		ScoreLabels[Index]->SetText(FText::AsNumber(Entry.Kills));
	}
	int32 Limit = ReadInt(State, TEXT("EndKillPoint"));
	if (Limit <= 0 && State)
		if (const auto* Config = FindFProperty<FObjectPropertyBase>(State->GetClass(), TEXT("GameConfig")))
			Limit = ReadInt(Config->GetObjectPropertyValue_InContainer(State), TEXT("MaxScore"));
	LeaderProgress->SetPercent(Limit > 0 && !Entries.IsEmpty() ? FMath::Clamp(float(Entries[0].Kills) / Limit, 0.0f, 1.0f) : 0.0f);
	LeaderProgress->SetFillColorAndOpacity(UUIThemePaletteLibrary::GetColor(
		!Entries.IsEmpty() && Entries[0].Player == Local ? TEXT("RankingLocal") : TEXT("RankingOpponent")));
	RankingPanel->SetRenderOpacity(Entries.IsEmpty() ? 0.0f : 1.0f);
}

void UCompetitiveRankingHUD::PositionBesideTimer(const FGeometry& Geometry)
{
	UUserWidget* Timer = Cast<UUserWidget>(GetWidgetFromName(TEXT("WB_Timer")));
	UWidget* TimerFrame = Timer ? Timer->GetWidgetFromName(TEXT("b_Timer")) : nullptr;
	UWidget* Seconds = Timer ? Timer->GetWidgetFromName(TEXT("Text_Seconds")) : nullptr;
	UWidget* TimerContainer = GetWidgetFromName(TEXT("SB_Timer"));
	if (!Seconds || !TimerFrame || !TimerFrame->IsVisible() || !Timer->IsVisible()
		|| !TimerContainer || !TimerContainer->IsVisible())
	{
		RankingPanel->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	const FGeometry& Digits = Seconds->GetCachedGeometry();
	const FGeometry& Frame = TimerFrame->GetCachedGeometry();
	const float Determinant = Geometry.GetAccumulatedRenderTransform().GetMatrix().Determinant();
	if (!FMath::IsFinite(Determinant) || FMath::IsNearlyZero(Determinant) || Digits.GetLocalSize().Y <= 0 || Frame.GetLocalSize().X <= 0) return;
	const FSlateFontInfo TimerFont = CastChecked<UTextBlock>(Seconds)->GetFont();
	const auto Renderer = FSlateApplication::Get().GetRenderer();
	const auto Measure = Renderer->GetFontMeasureService();
	const FCharacterEntry& Digit = Renderer->GetFontCache()->GetCharacterList(TimerFont, 1.0f).GetCharacter(TEXT('0'), EFontFallback::FF_Max);
	// Align against the painted digits rather than the font's empty ascent/descent space.
	const float InkTop = Digit.Valid ? Measure->GetMaxCharacterHeight(TimerFont) + Measure->GetBaseline(TimerFont) - Digit.VerticalOffset : 0.0f;
	const float InkHeight = Digit.Valid ? float(Digit.VSize) : float(Digits.GetLocalSize().Y);
	const FVector2D Top = Geometry.AbsoluteToLocal(Digits.LocalToAbsolute(FVector2D(Digits.GetLocalSize().X, InkTop)));
	const FVector2D Bottom = Geometry.AbsoluteToLocal(Digits.LocalToAbsolute(FVector2D(Digits.GetLocalSize().X, InkTop + InkHeight)));
	// Layout coordinates exclude our render offset, avoiding feedback from last frame's position.
	const auto ToRootLayout = Concatenate(Frame.GetAccumulatedLayoutTransform(), Inverse(Geometry.GetAccumulatedLayoutTransform()));
	const FVector2D Left = TransformPoint(ToRootLayout, FVector2D::ZeroVector);
	const FVector2D Right = TransformPoint(ToRootLayout, FVector2D(Frame.GetLocalSize().X, 0));
	if (Top.ContainsNaN() || Bottom.ContainsNaN() || Left.ContainsNaN() || Right.ContainsNaN()) return;
	const float Scale = RankingScale;
	if (!FMath::IsNearlyEqual(float(RankingSlot->GetSize().X), PanelWidth * Scale))
	{
		// Resize layout and glyphs instead of stretching the rendered text.
		RankingSlot->SetSize(FVector2D(PanelWidth, PanelHeight) * Scale);
		CastChecked<USizeBox>(LeaderProgress->GetParent())->SetHeightOverride(4.0f * Scale);
		for (int32 Index = 0; Index < RowBorders.Num(); ++Index)
		{
			UBorder* Outline = RowBorders[Index];
			Outline->SetPadding(FMargin(Scale));
			UBorder* Background = CastChecked<UBorder>(Outline->GetContent());
			Background->SetPadding(FMargin(4.0f * Scale, 0));
			CastChecked<USizeBox>(Background->GetContent())->SetHeightOverride(24.0f * Scale);
			CastChecked<USizeBox>(RankLabels[Index]->GetParent())->SetWidthOverride(28.0f * Scale);
			CastChecked<USizeBox>(ScoreLabels[Index]->GetParent())->SetWidthOverride(44.0f * Scale);
			for (UTextBlock* Label : {RankLabels[Index].Get(), NameLabels[Index].Get(), ScoreLabels[Index].Get()})
			{
				FSlateFontInfo Font = Label->GetFont();
				Font.Size = FMath::RoundToInt(14.0f * Scale);
				Label->SetFont(Font);
				Label->SetShadowOffset(FVector2D::ZeroVector);
				Label->SetPixelSnapping(EWidgetPixelSnapping::SnapToPixel);
			}
		}
	}
	const double Shift = (Geometry.GetLocalSize().X - (Left.X + Right.X + TimerGap + PanelWidth * Scale)) * 0.5;
	TimerContainer->SetRenderTranslation(FVector2D(Shift, TimerContainer->GetRenderTransform().Translation.Y));
	RankingPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	RankingPanel->SetRenderTransformPivot(FVector2D::ZeroVector);
	RankingPanel->SetRenderScale(FVector2D(1.0f));
	const FVector2D Position(FMath::Clamp(Right.X + Shift + TimerGap, 0.0,
		FMath::Max(0.0, Geometry.GetLocalSize().X - PanelWidth * Scale - 8.0)),
		FMath::Max(0.0, (Top.Y + Bottom.Y - PanelHeight * Scale) * 0.5));
	const FVector2D Absolute = Geometry.LocalToAbsolute(Position);
	RankingSlot->SetPosition(Geometry.AbsoluteToLocal(FVector2D(FMath::RoundToDouble(Absolute.X), FMath::RoundToDouble(Absolute.Y))));
}

void UCompetitiveRankingHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const APlayerController* Owner = GetOwningPlayer();
	const UKillStreakVoiceComponent* Streak = Owner && Owner->PlayerState
		? Owner->PlayerState->FindComponentByClass<UKillStreakVoiceComponent>() : nullptr;
	RefreshStreakMedals(Streak ? Streak->GetCurrentStreakCount() : 0);
	if (!RankingPanel) return;
	PositionBesideTimer(MyGeometry);
	if (UWidget* MessageArea = GetWidgetFromName(TEXT("SB_Killmessage")))
	{
		APlayerController* PC = GetOwningPlayer();
		const auto* DeathProperty = PC ? FindFProperty<FObjectPropertyBase>(PC->GetClass(), TEXT("deathScreen")) : nullptr;
		UUserWidget* DeathScreen = DeathProperty ? Cast<UUserWidget>(DeathProperty->GetObjectPropertyValue_InContainer(PC)) : nullptr;
		UWidget* RespawnBar = DeathScreen ? DeathScreen->GetWidgetFromName(TEXT("PB_RespawnBar")) : nullptr;
		if (DeathScreen && DeathScreen->IsInViewport() && DeathScreen->IsVisible() && RespawnBar)
		{
			UUserWidget* Messages = Cast<UUserWidget>(GetWidgetFromName(TEXT("WB_KillMessage")));
			UPanelWidget* Content = Messages ? Cast<UPanelWidget>(Messages->GetWidgetFromName(TEXT("OV_Content"))) : nullptr;
			UUserWidget* Message = Content && Content->GetChildrenCount() ? Cast<UUserWidget>(Content->GetChildAt(0)) : nullptr;
			UWidget* Text = Message ? Message->GetWidgetFromName(TEXT("VerticalBox_0")) : nullptr;
			if (Text && RespawnBar->GetCachedGeometry().GetLocalSize().X > 0 && Text->GetCachedGeometry().GetLocalSize().Y > 0
				&& !FMath::IsNearlyZero(MyGeometry.GetAccumulatedLayoutTransform().GetScale()))
			{
				// Use layout transforms so the previous render offset never accumulates.
				const auto ToRoot = Inverse(MyGeometry.GetAccumulatedLayoutTransform());
				const auto BarLayout = Concatenate(RespawnBar->GetCachedGeometry().GetAccumulatedRenderTransform(), ToRoot);
				const auto TextLayout = Concatenate(Text->GetCachedGeometry().GetAccumulatedLayoutTransform(), ToRoot);
				const FVector2D BarTop = TransformPoint(BarLayout, FVector2D(RespawnBar->GetCachedGeometry().GetLocalSize().X * 0.5, 0));
				const FVector2D TextBottom = TransformPoint(TextLayout, FVector2D(Text->GetCachedGeometry().GetLocalSize().X * 0.5, Text->GetCachedGeometry().GetLocalSize().Y));
				const FVector2D Offset = BarTop - TextBottom + FVector2D(0, 16.0f);
				if (!Offset.ContainsNaN()) MessageArea->SetRenderTranslation(Offset);
			}
		}
		else if (StreakPanel && DisplayedStreak > 0 && StreakIcons)
		{
			UUserWidget* Messages = Cast<UUserWidget>(GetWidgetFromName(TEXT("WB_KillMessage")));
			UPanelWidget* Content = Messages ? Cast<UPanelWidget>(Messages->GetWidgetFromName(TEXT("OV_Content"))) : nullptr;
			UUserWidget* Message = Content && Content->GetChildrenCount() ? Cast<UUserWidget>(Content->GetChildAt(0)) : nullptr;
			UWidget* Text = Message ? Message->GetWidgetFromName(TEXT("VerticalBox_0")) : nullptr;
			if (Text && StreakIcons->GetCachedGeometry().GetLocalSize().Y > 0 && Text->GetCachedGeometry().GetLocalSize().Y > 0
				&& !FMath::IsNearlyZero(MyGeometry.GetAccumulatedLayoutTransform().GetScale()))
			{
				const auto ToRoot = Inverse(MyGeometry.GetAccumulatedLayoutTransform());
				const auto IconsLayout = Concatenate(StreakIcons->GetCachedGeometry().GetAccumulatedLayoutTransform(), ToRoot);
				const auto TextLayout = Concatenate(Text->GetCachedGeometry().GetAccumulatedLayoutTransform(), ToRoot);
				const FVector2D IconsBottom = TransformPoint(IconsLayout, FVector2D(StreakIcons->GetCachedGeometry().GetLocalSize().X * 0.5, StreakIcons->GetCachedGeometry().GetLocalSize().Y));
				const FVector2D TextTop = TransformPoint(TextLayout, FVector2D(Text->GetCachedGeometry().GetLocalSize().X * 0.5, 0));
				const FVector2D Offset = IconsBottom - TextTop + FVector2D(0, 8);
				if (!Offset.ContainsNaN()) MessageArea->SetRenderTranslation(Offset);
			}
		}
		else MessageArea->SetRenderTranslation(FVector2D::ZeroVector);
	}
	RefreshElapsed += InDeltaTime;
	// Player joins/leaves and late replication can arrive without a score-change broadcast.
	if (RefreshElapsed >= 0.5f) { RefreshElapsed = 0.0f; RefreshRanking(); }
}

void UCompetitiveRankingHUD::NativeDestruct()
{
	BindScoreChanges(nullptr);
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(StreakDisplayTimer);
	if (StreakHighlight) StreakHighlight->StopAnimationsAndLatentActions();
	StreakHighlight = nullptr;
	if (StreakPanel) StreakPanel->RemoveFromParent();
	StreakPanel = nullptr;
	StreakIcons = nullptr;
	StreakCountLabel = nullptr;
	ActiveStreakSettings = nullptr;
	DisplayedStreak = INDEX_NONE;
	if (RankingPanel) RankingPanel->RemoveFromParent();
	RankingPanel = nullptr;
	RankingSlot = nullptr;
	LeaderProgress = nullptr;
	RowBorders.Reset();
	RankLabels.Reset();
	NameLabels.Reset();
	ScoreLabels.Reset();
	Super::NativeDestruct();
}

#include "UI/CompetitiveRankingHUD.h"
#include "UI/CompetitiveRankingOrder.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

namespace
{
	constexpr float PanelWidth = 286.0f;
	constexpr float PanelHeight = 48.0f;
	constexpr float TimerGap = 5.0f;
	const FLinearColor OpponentColor(0.95f, 0.32f, 0.055f, 0.85f);
	const FLinearColor LocalColor(0.025f, 0.68f, 0.90f, 0.85f);

	int32 ReadInt(const UObject* Object, FName Name)
	{
		const FIntProperty* Property = IsValid(Object) ? FindFProperty<FIntProperty>(Object->GetClass(), Name) : nullptr;
		return Property ? Property->GetPropertyValue_InContainer(Object) : 0;
	}
}

void UCompetitiveRankingHUD::NativeConstruct()
{
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
	ProgressStyle.BackgroundImage.TintColor = FSlateColor(FLinearColor::Transparent);
	ProgressStyle.FillImage = FSlateBrush();
	ProgressStyle.FillImage.DrawAs = ESlateBrushDrawType::Image;
	ProgressStyle.FillImage.TintColor = FSlateColor(FLinearColor::White);
	LeaderProgress->SetWidgetStyle(ProgressStyle);
	LeaderProgress->SetFillColorAndOpacity(OpponentColor);
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
		float DigitWidth = 0;
		for (TCHAR Digit = TEXT('0'); Digit <= TEXT('9'); ++Digit)
			DigitWidth = FMath::Max(DigitWidth, float(Measure->Measure(FString::Chr(Digit), Font).X));
		// Reserve the widest digits so time changes never alter the frame width.
		const float TimerWidth = FMath::CeilToFloat(4.0f * DigitWidth + Measure->Measure(TEXT(":"), Font).X + 4.0f);
		ensure(TimerWidth > 4.0f * DigitWidth);
		TimerBox->SetWidthOverride(TimerWidth);
	}
	Font.Size = 16;
	for (int32 Index = 0; Index < 2; ++Index)
	{
		UBorder* Outline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("RankingRow%d"), Index));
		Outline->SetPadding(FMargin(1.0f));
		RankingPanel->AddChildToVerticalBox(Outline);
		RowBorders.Add(Outline);
		UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
		Background->SetBrushColor(FLinearColor(0.008f, 0.012f, 0.018f, 0.78f));
		Background->SetPadding(FMargin(4.0f, 0.0f));
		Outline->SetContent(Background);
		USizeBox* Height = WidgetTree->ConstructWidget<USizeBox>();
		Height->SetHeightOverride(20.0f);
		Background->SetContent(Height);
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		Height->SetContent(Row);
		for (int32 Column = 0; Column < 3; ++Column)
		{
			const TCHAR* Prefix = Column == 0 ? TEXT("Rank") : Column == 1 ? TEXT("Name") : TEXT("Score");
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
				*FString::Printf(TEXT("Ranking%s%d"), Prefix, Index));
			Text->SetFont(Font);
			Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Text->SetShadowColorAndOpacity(FLinearColor(0, 0, 0, 0.7f));
			Text->SetShadowOffset(FVector2D(1, 1));
			if (Column == 1)
			{
				FSlateFontInfo NameFont = Font;
				NameFont.TypefaceFontName = TEXT("Barlow_Semibold");
				Text->SetFont(NameFont);
				Text->SetShadowOffset(FVector2D::ZeroVector);
				Text->SetPixelSnapping(EWidgetPixelSnapping::SnapToPixel);
				Text->SetClipping(EWidgetClipping::ClipToBounds);
				Text->SetTextOverflowPolicy(ETextOverflowPolicy::Ellipsis);
				Row->AddChildToHorizontalBox(Text)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				NameLabels.Add(Text);
			}
			else
			{
				USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>();
				Width->SetWidthOverride(Column == 0 ? 28.0f : 44.0f);
				Width->SetContent(Text);
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
		RowBorders[Index]->SetBrushColor(Entry.Player == Local ? LocalColor : OpponentColor);
		RankLabels[Index]->SetText(FText::AsNumber(Entry.Rank));
		NameLabels[Index]->SetText(UShooterDisplayLibrary::GetPlayerStateDisplayName(Entry.Player));
		ScoreLabels[Index]->SetText(FText::AsNumber(Entry.Kills));
	}
	int32 Limit = ReadInt(State, TEXT("EndKillPoint"));
	if (Limit <= 0 && State)
		if (const auto* Config = FindFProperty<FObjectPropertyBase>(State->GetClass(), TEXT("GameConfig")))
			Limit = ReadInt(Config->GetObjectPropertyValue_InContainer(State), TEXT("MaxScore"));
	LeaderProgress->SetPercent(Limit > 0 && !Entries.IsEmpty() ? FMath::Clamp(float(Entries[0].Kills) / Limit, 0.0f, 1.0f) : 0.0f);
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
	const FVector2D Top = Geometry.AbsoluteToLocal(Digits.LocalToAbsolute(FVector2D(Digits.GetLocalSize().X, 0)));
	const FVector2D Bottom = Geometry.AbsoluteToLocal(Digits.LocalToAbsolute(Digits.GetLocalSize()));
	// Layout coordinates exclude our render offset, avoiding feedback from last frame's position.
	const auto ToRootLayout = Concatenate(Frame.GetAccumulatedLayoutTransform(), Inverse(Geometry.GetAccumulatedLayoutTransform()));
	const FVector2D Left = TransformPoint(ToRootLayout, FVector2D::ZeroVector);
	const FVector2D Right = TransformPoint(ToRootLayout, FVector2D(Frame.GetLocalSize().X, 0));
	if (Top.ContainsNaN() || Bottom.ContainsNaN() || Left.ContainsNaN() || Right.ContainsNaN()) return;
	const float Scale = FMath::Clamp(float((Bottom.Y - Top.Y) / PanelHeight), 0.975f, 2.25f);
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
			CastChecked<USizeBox>(Background->GetContent())->SetHeightOverride(20.0f * Scale);
			CastChecked<USizeBox>(RankLabels[Index]->GetParent())->SetWidthOverride(28.0f * Scale);
			CastChecked<USizeBox>(ScoreLabels[Index]->GetParent())->SetWidthOverride(44.0f * Scale);
			for (UTextBlock* Label : {RankLabels[Index].Get(), NameLabels[Index].Get(), ScoreLabels[Index].Get()})
			{
				FSlateFontInfo Font = Label->GetFont();
				Font.Size = FMath::RoundToInt(16.0f * Scale);
				Font.TypefaceFontName = TEXT("Barlow_Medium");
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
	RankingSlot->SetPosition(FVector2D(FMath::Clamp(Right.X + Shift + TimerGap, 0.0,
		FMath::Max(0.0, Geometry.GetLocalSize().X - PanelWidth * Scale - 8.0)),
		FMath::Max(0.0, (Top.Y + Bottom.Y - PanelHeight * Scale) * 0.5)));
}

void UCompetitiveRankingHUD::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!RankingPanel) return;
	PositionBesideTimer(MyGeometry);
	RefreshElapsed += InDeltaTime;
	// Player joins/leaves and late replication can arrive without a score-change broadcast.
	if (RefreshElapsed >= 0.5f) { RefreshElapsed = 0.0f; RefreshRanking(); }
}

void UCompetitiveRankingHUD::NativeDestruct()
{
	BindScoreChanges(nullptr);
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

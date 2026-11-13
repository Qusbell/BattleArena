#include "UI/ArenaGameViewportClient.h"

#include "UI/BuffDurationRing.h"
#include "Localization/LanguageSubsystem.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Misc/ScopeExit.h"
#include "SubtitleManager.h"
#include "UObject/ConstructorHelpers.h"

UArenaGameViewportClient::UArenaGameViewportClient()
{
	static ConstructorHelpers::FObjectFinder<UBuffDurationRingSettings> SettingsAsset(
		TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing"));
	SubtitleSettings = SettingsAsset.Object;
}

void UArenaGameViewportClient::Draw(FViewport* InViewport, FCanvas* SceneCanvas)
{
	ULanguageLibrary::RefreshViewportFonts(GetWorld());
	FSubtitleManager* Manager = FSubtitleManager::GetSubtitleManager();
	// Bind only during this viewport's draw. Leave any other subtitle display in charge.
	if (!SceneCanvas || !SubtitleSettings || Manager->OnSetSubtitleText().IsBound())
	{
		Super::Draw(InViewport, SceneCanvas);
		return;
	}
	SubtitleCanvas = SceneCanvas;
	const FDelegateHandle Handle = Manager->OnSetSubtitleText().AddUObject(this, &ThisClass::DrawSubtitleText);
	ON_SCOPE_EXIT
	{
		Manager->OnSetSubtitleText().Remove(Handle);
		SubtitleCanvas = nullptr;
	};
	// The engine still selects the active sound cue, priority and expiry time.
	Super::Draw(InViewport, SceneCanvas);
}

void UArenaGameViewportClient::DrawSubtitleText(const FText& Text)
{
	// The manager's display-delegate path doesn't check the global subtitle preferences.
	if (!SubtitleCanvas || !SubtitleSettings || !SubtitleSettings->bShowSubtitles || Text.IsEmpty() || !GEngine
		|| GEngine->bSubtitlesForcedOff || !GEngine->bSubtitlesEnabled) return;
	UFont* Font = GEngine->GetSubtitleFont();
	Font = const_cast<UFont*>(Cast<UFont>(ULanguageLibrary::ResolveFont(FSlateFontInfo(Font, 24)).FontObject));
	if (!Font) return;
	const float Scale = FMath::IsFinite(SubtitleSettings->SubtitleTextScale)
		? FMath::Clamp(SubtitleSettings->SubtitleTextScale, 0.25f, 4.0f) : 1.0f;
	FVector2D Position = SubtitleSettings->SubtitlePosition;
	Position.X = FMath::IsFinite(Position.X) ? FMath::Clamp(Position.X, 0.0, 1.0) : 0.5;
	Position.Y = FMath::IsFinite(Position.Y) ? FMath::Clamp(Position.Y, 0.0, 1.0) : 0.7;
	FVector2D MinPos(0.0, 0.0), MaxPos(1.0, 1.0);
	Super::GetSubtitleRegion(MinPos, MaxPos);
	Position.Y = FMath::Min(Position.Y, MaxPos.Y); // Keep the split-screen upper-half limit.
	const FIntPoint Size = SubtitleCanvas->GetRenderTarget()->GetSizeXY();
	if (!CachedSubtitleText.EqualTo(Text) || CachedSubtitleFont != Font || CachedSubtitleScale != Scale || CachedSubtitleWidth != Size.X)
	{
		CachedSubtitleText = Text;
		CachedSubtitleFont = Font;
		CachedSubtitleScale = Scale;
		CachedSubtitleWidth = Size.X;
		FTextSizingParameters Parameters(0.0f, 0.0f, Size.X * 0.8f / Scale, 0.0f, Font);
		FCanvasWordWrapper Wrapper;
		TArray<FWrappedStringElement> Wrapped;
		UCanvas::WrapString(Wrapper, Parameters, 0.0f, Text.ToString(), Wrapped);
		WrappedSubtitleLines.Reset();
		for (const FWrappedStringElement& Line : Wrapped) WrappedSubtitleLines.Add(Line.Value);
	}
	const float LineHeight = Font->GetMaxCharHeight() * 1.1f * Scale;
	float Y = FMath::Max(0.0f, static_cast<float>(Size.Y * Position.Y) - LineHeight * WrappedSubtitleLines.Num());
	for (const FString& Line : WrappedSubtitleLines)
	{
		FCanvasTextItem Item(FVector2D(Size.X * Position.X, Y), FText::FromString(Line), Font, FLinearColor::White);
		Item.Scale = FVector2D(Scale, Scale);
		Item.bCentreX = true;
		Item.bOutlined = true;
		Item.OutlineColor = FLinearColor::Black;
		SubtitleCanvas->DrawItem(Item);
		Y += LineHeight;
	}
}

void UArenaGameViewportClient::GetSubtitleRegion(FVector2D& MinPos, FVector2D& MaxPos)
{
	Super::GetSubtitleRegion(MinPos, MaxPos);
	const double Setting = SubtitleSettings ? SubtitleSettings->SubtitlePosition.Y : SubtitleBottom;
	const double Bottom = FMath::IsFinite(Setting) ? FMath::Clamp(Setting, 0.0, 1.0) : 0.7;
	// Keep the engine's smaller split-screen region and CommonUI input routing intact.
	MaxPos.Y = FMath::Min(MaxPos.Y, Bottom);
}

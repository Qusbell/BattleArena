#include "UI/UIRecordingSettings.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SNullWidget.h"
#include "Engine/World.h"

bool UUIRecordingSettings::IsShown(ERecordingUISection Section) const
{
	switch (Section)
	{
	case ERecordingUISection::Combat: return Combat;
	case ERecordingUISection::Timer: return Timer;
	case ERecordingUISection::Ranking: return Ranking;
	case ERecordingUISection::Scoreboard: return Scoreboard;
	case ERecordingUISection::Crosshair: return Crosshair;
	case ERecordingUISection::KillFeed: return KillFeed;
	case ERecordingUISection::Medals: return Medals;
	case ERecordingUISection::Buffs: return Buffs;
	case ERecordingUISection::DamageEffects: return DamageEffects;
	case ERecordingUISection::DeathScreen: return DeathScreen;
	case ERecordingUISection::Menus: return Menus;
	case ERecordingUISection::Results: return Results;
	case ERecordingUISection::Nicknames: return Nicknames;
	case ERecordingUISection::Cursor: return Cursor;
	case ERecordingUISection::Loading: return Loading;
	default: return true;
	}
}

UUIRecordingSettings* UUIRecordingSettings::Get()
{
	static TWeakObjectPtr<UUIRecordingSettings> Cached;
	if (!Cached.IsValid()) Cached = LoadObject<UUIRecordingSettings>(nullptr,
		TEXT("/Game/QuakeLike_1_0/UI/Recording/DA_UIRecordingSettings.DA_UIRecordingSettings"));
	return Cached.Get();
}

class SRecordingUIMask : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRecordingUIMask) {} SLATE_END_ARGS()
	void Construct(const FArguments&, URecordingUIMask* InOwner) { Owner = InOwner; }
	void SetContent(TSharedRef<SWidget> Content) { ChildSlot[Content]; }
	virtual void Tick(const FGeometry& Geometry, double Time, float DeltaTime) override
	{
		SCompoundWidget::Tick(Geometry, Time, DeltaTime);
		const bool Shown = !Owner.IsValid() || Owner->IsContentShown();
		if (Shown != bLastShown) { bLastShown = Shown; Invalidate(EInvalidateWidgetReason::Paint); }
	}
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Cull,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool Enabled) const override
	{
		if (!Owner.IsValid() || Owner->IsContentShown())
			return SCompoundWidget::OnPaint(Args, Geometry, Cull, Elements, Layer, Style, Enabled);

		// Brush outlines can ignore inherited opacity. Clip the draws while Slate still runs child ticks.
		FWidgetStyle MaskedStyle(Style);
		MaskedStyle.BlendOpacity(0.0f);
		FSlateClippingZone EmptyClip(FSlateRect(0, 0, 0, 0));
		EmptyClip.SetAlwaysClip(true);
		Elements.PushClip(EmptyClip);
		const int32 LastLayer = SCompoundWidget::OnPaint(Args, Geometry, Cull, Elements, Layer, MaskedStyle, Enabled);
		Elements.PopClip();
		return LastLayer;
	}
private:
	TWeakObjectPtr<URecordingUIMask> Owner;
	bool bLastShown = true;
};

bool URecordingUIMask::IsContentShown() const
{
	return !ActiveSettings || !GetWorld() || !GetWorld()->IsGameWorld() || ActiveSettings->IsShown(Section);
}

TSharedRef<SWidget> URecordingUIMask::RebuildWidget()
{
	ActiveSettings = UUIRecordingSettings::Get();
	Mask = SNew(SRecordingUIMask, this);
	Mask->SetContent(GetContent() ? GetContent()->TakeWidget() : SNullWidget::NullWidget);
	return Mask.ToSharedRef();
}

void URecordingUIMask::OnSlotAdded(UPanelSlot*)
{
	if (Mask) Mask->SetContent(GetContent() ? GetContent()->TakeWidget() : SNullWidget::NullWidget);
}

void URecordingUIMask::OnSlotRemoved(UPanelSlot*)
{
	if (Mask) Mask->SetContent(SNullWidget::NullWidget);
}

void URecordingUIMask::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Mask.Reset();
}

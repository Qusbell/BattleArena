#include "UI/CrosshairRecoilLibrary.h"

#include "Animation/MovieScene2DTransformSection.h"
#include "Animation/MovieSceneMarginSection.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Engine/World.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
float Evaluate(const FMovieSceneFloatChannel& Channel, FFrameTime Time)
{
	float Value = 0.f;
	Channel.Evaluate(Time, Value);
	return Value;
}

// The series' authored spacing curves are offsets from zero. Sample their original motion profile,
// including between keys, so diagonal crosshairs also use distance rather than distance per axis.
float PeakDistance(TConstArrayView<const FMovieSceneFloatChannel*> Channels,
	TFunctionRef<FVector2D(FFrameTime)> Position)
{
	TArray<FFrameNumber> Times;
	for (const FMovieSceneFloatChannel* Channel : Channels) Times.Append(Channel->GetData().GetTimes());
	if (Times.IsEmpty()) return 0.f;
	Times.Sort();
	float Peak = 0.f;
	for (FFrameNumber Time : Times) Peak = FMath::Max(Peak, float(Position(Time).Size()));
	const double Start = Times[0].Value;
	const double Range = double(Times.Last().Value) - Start;
	for (int32 I = 0; I <= 128; ++I)
		Peak = FMath::Max(Peak, float(Position(FFrameTime::FromDecimal(Start + Range * I / 128.0)).Size()));
	return Peak;
}

bool ScaleChannel(const FMovieSceneFloatChannel& Source, FMovieSceneFloatChannel& Dest, double Factor)
{
	if (!FMath::IsFinite(Factor)) return false;
	auto AssignFinite = [](float& DestValue, double Value)
	{
		if (!FMath::IsFinite(Value) || FMath::Abs(Value) > MAX_flt) return false;
		DestValue = float(Value); return true;
	};
	Dest = Source; // Always rebuild from source: repeated shots never compound scaling.
	for (FMovieSceneFloatValue& Key : Dest.GetData().GetValues())
	{
		const float ArriveSlope = Key.Tangent.ArriveTangent;
		const float LeaveSlope = Key.Tangent.LeaveTangent;
		if (!AssignFinite(Key.Value, double(Key.Value) * Factor)
			|| !AssignFinite(Key.Tangent.ArriveTangent, double(ArriveSlope) * Factor)
			|| !AssignFinite(Key.Tangent.LeaveTangent, double(LeaveSlope) * Factor)) return false;
		// Preserve weighted Bezier handles after scaling their vertical component.
		auto ScaleWeight = [Factor](float Weight, double Slope)
		{
			return double(Weight) * FMath::Sqrt((1.0 + FMath::Square(Slope * Factor)) / (1.0 + FMath::Square(Slope)));
		};
		if (!AssignFinite(Key.Tangent.ArriveTangentWeight, ScaleWeight(Key.Tangent.ArriveTangentWeight, ArriveSlope))
			|| !AssignFinite(Key.Tangent.LeaveTangentWeight, ScaleWeight(Key.Tangent.LeaveTangentWeight, LeaveSlope))) return false;
	}
	if (Source.GetDefault().IsSet())
	{
		float Default = 0.f;
		if (!AssignFinite(Default, double(Source.GetDefault().GetValue()) * Factor)) return false;
		Dest.SetDefault(Default);
	}
	return true;
}

FVector2D MarginPosition(const UOverlaySlot* Slot, float Left, float Top, float Right, float Bottom)
{
	if (!IsValid(Slot)) Slot = nullptr;
	const EHorizontalAlignment H = Slot ? Slot->GetHorizontalAlignment() : HAlign_Center;
	const EVerticalAlignment V = Slot ? Slot->GetVerticalAlignment() : VAlign_Center;
	const float X = H == HAlign_Left ? Left : H == HAlign_Right ? -Right : (Left - Right) * .5f;
	const float Y = V == VAlign_Top ? Top : V == VAlign_Bottom ? -Bottom : (Top - Bottom) * .5f;
	return FVector2D(X, Y);
}

bool AdjustSection(const UMovieSceneSection* Source, UMovieSceneSection* Dest, const UOverlaySlot* Slot, float Amount)
{
	if (!IsValid(Source) || !IsValid(Dest) || Source == Dest || Source->GetClass() != Dest->GetClass()) return false;
	if (const UMovieSceneMarginSection* S = Cast<UMovieSceneMarginSection>(Source))
	{
		UMovieSceneMarginSection* D = Cast<UMovieSceneMarginSection>(Dest);
		if (!D) return false;
		const FMovieSceneFloatChannel* Channels[] = {&S->LeftCurve, &S->TopCurve, &S->RightCurve, &S->BottomCurve};
		const float Peak = PeakDistance(Channels, [&](FFrameTime T)
		{
			return MarginPosition(Slot, Evaluate(S->LeftCurve, T), Evaluate(S->TopCurve, T),
				Evaluate(S->RightCurve, T), Evaluate(S->BottomCurve, T));
		});
		const double Factor = Peak > UE_SMALL_NUMBER ? double(Amount) / Peak : 1.0;
		return ScaleChannel(S->LeftCurve, D->LeftCurve, Factor)
			&& ScaleChannel(S->TopCurve, D->TopCurve, Factor)
			&& ScaleChannel(S->RightCurve, D->RightCurve, Factor)
			&& ScaleChannel(S->BottomCurve, D->BottomCurve, Factor);
	}
	else if (const UMovieScene2DTransformSection* Transform = Cast<UMovieScene2DTransformSection>(Source))
	{
		UMovieScene2DTransformSection* D = Cast<UMovieScene2DTransformSection>(Dest);
		if (!D) return false;
		const FMovieSceneFloatChannel* Channels[] = {&Transform->Translation[0], &Transform->Translation[1]};
		const float Peak = PeakDistance(Channels, [&](FFrameTime T)
		{
			return FVector2D(Evaluate(Transform->Translation[0], T), Evaluate(Transform->Translation[1], T));
		});
		const double Factor = Peak > UE_SMALL_NUMBER ? double(Amount) / Peak : 1.0;
		return ScaleChannel(Transform->Translation[0], D->Translation[0], Factor)
			&& ScaleChannel(Transform->Translation[1], D->Translation[1], Factor);
	}
	return true;
}

bool SameStructure(const UMovieScene* Source, const UMovieScene* Dest)
{
	if (!IsValid(Source) || !IsValid(Dest) || Source == Dest) return false;
	const auto& A = Source->GetBindings(); const auto& B = Dest->GetBindings();
	if (A.Num() != B.Num()) return false;
	for (int32 I = 0; I < A.Num(); ++I)
	{
		if (A[I].GetObjectGuid() != B[I].GetObjectGuid()) return false;
		const auto& AT = A[I].GetTracks(); const auto& BT = B[I].GetTracks();
		if (AT.Num() != BT.Num()) return false;
		for (int32 T = 0; T < AT.Num(); ++T)
		{
			if (!IsValid(AT[T]) || !IsValid(BT[T]) || AT[T] == BT[T] || AT[T]->GetClass() != BT[T]->GetClass()) return false;
			const auto& AS = AT[T]->GetAllSections(); const auto& BS = BT[T]->GetAllSections();
			if (AS.Num() != BS.Num()) return false;
			for (int32 S = 0; S < AS.Num(); ++S)
				if (!IsValid(AS[S]) || !IsValid(BS[S]) || AS[S] == BS[S] || AS[S]->GetClass() != BS[S]->GetClass()) return false;
		}
	}
	return true;
}
}

UWidgetAnimation* UCrosshairRecoilLibrary::PrepareRecoilWithSpread(UUserWidget* Widget, UWidgetAnimation* Animation, float SpreadAmount)
{
	if (!IsInGameThread() || !IsValid(Widget) || !IsValid(Animation) || !IsValid(Animation->GetMovieScene())) return nullptr;
	if (UWorld* World = Widget->GetWorld(); World && (!IsValid(World) || World->bIsTearingDown)) return nullptr;
	TStrongObjectPtr<UUserWidget> KeepWidget(Widget);
	TStrongObjectPtr<UWidgetAnimation> KeepSource(Animation);
	UCrosshairRecoilExtension* State = Widget->GetExtension<UCrosshairRecoilExtension>();
	if (!State) State = Widget->AddExtension<UCrosshairRecoilExtension>();
	if (!IsValid(State) || State->bPreparingRecoil || State->bStoppingRecoil) return nullptr;
	TStrongObjectPtr<UCrosshairRecoilExtension> KeepState(State);
	TGuardValue<bool> Preparing(State->bPreparingRecoil, true);
	StopRecoilWithSpread(Widget);
	if (!IsValid(Widget) || !IsValid(State) || !IsValid(Animation)) return nullptr;
	Widget->StopAnimation(Animation);
	if (!IsValid(Widget) || !IsValid(State) || !IsValid(Animation)) return nullptr;
	if (UWorld* World = Widget->GetWorld(); IsValid(World) && !World->bIsTearingDown) Widget->FlushAnimations();
	// Stop/flush can execute animation delegates and tear down or replace the widget.
	if (!IsValid(Widget) || !IsValid(State) || !IsValid(Animation) || !IsValid(Animation->GetMovieScene())) return nullptr;
	if (UWorld* World = Widget->GetWorld(); World && (!IsValid(World) || World->bIsTearingDown)) return nullptr;
	if (State->SourceAnimation != Animation || !IsValid(State->RuntimeAnimation)
		|| State->SourceSceneSignature != Animation->GetMovieScene()->GetSignature()
		|| !SameStructure(Animation->GetMovieScene(), State->RuntimeAnimation->GetMovieScene()))
	{
		State->SourceAnimation = Animation;
		State->RuntimeAnimation = DuplicateObject<UWidgetAnimation>(Animation, State,
			MakeUniqueObjectName(State, UWidgetAnimation::StaticClass(), Animation->GetFName()));
		if (!IsValid(State->RuntimeAnimation)) return nullptr;
		// MovieScene is not an instanced property. Explicitly duplicate it to avoid shared curve data.
		if (State->RuntimeAnimation->GetMovieScene() == Animation->GetMovieScene())
			State->RuntimeAnimation->MovieScene = DuplicateObject<UMovieScene>(Animation->GetMovieScene(), State->RuntimeAnimation);
		if (!SameStructure(Animation->GetMovieScene(), State->RuntimeAnimation->GetMovieScene()))
		{
			State->RuntimeAnimation = nullptr;
			return nullptr;
		}
		State->SourceSceneSignature = Animation->GetMovieScene()->GetSignature();
		State->RuntimeAnimation->SetFlags(RF_Transient);
		State->RuntimeAnimation->ClearFlags(RF_Public | RF_Standalone);
		State->RuntimeAnimation->SetSequenceFlags(State->RuntimeAnimation->GetFlags() | EMovieSceneSequenceFlags::Volatile);
	}
	TStrongObjectPtr<UWidgetAnimation> KeepRuntime(State->RuntimeAnimation);
	// Keep even valid-but-extreme inputs away from Slate transform/rectangle overflow.
	const float Amount = FMath::IsFinite(SpreadAmount) ? FMath::Clamp(SpreadAmount, 0.f, 100000.f) : 0.f;
	const TArray<FMovieSceneBinding>& SourceBindings = Animation->GetMovieScene()->GetBindings();
	const TArray<FMovieSceneBinding>& DestBindings = State->RuntimeAnimation->GetMovieScene()->GetBindings();
	for (int32 B = 0; B < SourceBindings.Num(); ++B)
	{
		const UOverlaySlot* Slot = nullptr;
		for (const FWidgetAnimationBinding& Binding : Animation->GetBindings())
		{
			if (Binding.AnimationGuid == SourceBindings[B].GetObjectGuid() && IsValid(Widget->WidgetTree))
			{
				if (const UWidget* Bound = Widget->WidgetTree->FindWidget(Binding.WidgetName); IsValid(Bound)) Slot = Cast<UOverlaySlot>(Bound->Slot);
			}
		}
		const auto& SourceTracks = SourceBindings[B].GetTracks();
		const auto& DestTracks = DestBindings[B].GetTracks();
		for (int32 T = 0; T < SourceTracks.Num(); ++T)
		{
			const auto& SourceSections = SourceTracks[T]->GetAllSections();
			const auto& DestSections = DestTracks[T]->GetAllSections();
			for (int32 S = 0; S < SourceSections.Num(); ++S)
				if (!AdjustSection(SourceSections[S], DestSections[S], Slot, Amount))
				{
					State->RuntimeAnimation = nullptr;
					return nullptr;
				}
		}
	}
	State->RuntimeAnimation->GetMovieScene()->MarkAsChanged();
	State->RuntimeAnimation->MarkAsChanged();
	return IsValid(Widget) && IsValid(State->RuntimeAnimation) ? State->RuntimeAnimation.Get() : nullptr;
}

void UCrosshairRecoilLibrary::StopRecoilWithSpread(UUserWidget* Widget)
{
	if (!IsInGameThread() || !IsValid(Widget)) return;
	TStrongObjectPtr<UUserWidget> KeepWidget(Widget);
	if (UCrosshairRecoilExtension* State = Widget->GetExtension<UCrosshairRecoilExtension>(); IsValid(State) && !State->bStoppingRecoil)
		if (IsValid(State->RuntimeAnimation))
		{
			TStrongObjectPtr<UCrosshairRecoilExtension> KeepState(State);
			TStrongObjectPtr<UWidgetAnimation> KeepAnimation(State->RuntimeAnimation);
			TGuardValue<bool> Stopping(State->bStoppingRecoil, true);
			Widget->StopAnimation(State->RuntimeAnimation);
			// Drain pending evaluation/stop actions before reusing and editing the cached sequence.
			if (UWorld* World = Widget->GetWorld(); IsValid(Widget) && IsValid(World) && !World->bIsTearingDown) Widget->FlushAnimations();
		}
}

UWidget* UCrosshairRecoilLibrary::GetCrosshairChildSafely(UUserWidget* Widget)
{
	if (!IsInGameThread() || !IsValid(Widget) || !IsValid(Widget->WidgetTree)) return nullptr;
	UPanelWidget* Panel = Cast<UPanelWidget>(Widget->WidgetTree->FindWidget(TEXT("OV_Crosshair")));
	if (!IsValid(Panel) || Panel->GetChildrenCount() == 0) return nullptr;
	UWidget* Child = Panel->GetChildAt(0);
	return IsValid(Child) ? Child : nullptr;
}

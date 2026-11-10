#pragma once
#include "CoreMinimal.h"

namespace AimAssistMath
{
inline FRotator Interpolate(FRotator Current, FRotator Target, float DeltaTime, float Time)
{
	if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f || Current.ContainsNaN() || Target.ContainsNaN()) return Current;
	Time = FMath::IsFinite(Time) ? FMath::Clamp(Time, 0.01f, 5.f) : 0.3f;
	const double Alpha = 1.0 - FMath::Exp(-FMath::Loge(10.0) * DeltaTime / Time);
	return (Current + (Target - Current).GetNormalized() * Alpha).GetNormalized();
}

inline bool ScreenCandidate(FVector2D Point, FIntRect Rect)
{
	return !Point.ContainsNaN() && Rect.Width() > 0 && Rect.Height() > 0
		&& Point.X >= Rect.Min.X && Point.Y >= Rect.Min.Y && Point.X < Rect.Max.X && Point.Y < Rect.Max.Y;
}

}

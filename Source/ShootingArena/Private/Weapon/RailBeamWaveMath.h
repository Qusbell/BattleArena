#pragma once

#include "CoreMinimal.h"

namespace RailBeamWaves
{
struct FSegment
{
	FVector Start = FVector::ZeroVector;
	FVector End = FVector::ZeroVector;
	double Length = 0.0;
	bool bValid = false;
};

/** Trim cosmetic waves only. The original beam/trace endpoints are never modified. */
inline FSegment MakeSegment(FVector Eye, FVector Muzzle, FVector Hit,
	double CameraClearance, double MuzzleClearance, double HitClearance)
{
	FSegment Result;
	if (Eye.ContainsNaN() || Muzzle.ContainsNaN() || Hit.ContainsNaN()
		|| !FMath::IsFinite(CameraClearance) || !FMath::IsFinite(MuzzleClearance) || !FMath::IsFinite(HitClearance)
		|| CameraClearance < 0 || MuzzleClearance < 0 || HitClearance < 0) return Result;
	const FVector Delta = Hit - Muzzle;
	const double Length = Delta.Size();
	if (!FMath::IsFinite(Length) || Length <= UE_SMALL_NUMBER || Length > 1.e12) return Result;
	const FVector Direction = Delta / Length;
	const FVector Relative = Muzzle - Eye;
	const double Projection = FVector::DotProduct(Relative, Direction);
	const double Discriminant = Projection * Projection - Relative.SizeSquared() + CameraClearance * CameraClearance;
	if (!FMath::IsFinite(Projection) || !FMath::IsFinite(Discriminant)) return Result;
	// Use the final sphere exit, including segments that initially head toward the
	// camera. Every remaining point must lie outside the protected camera region.
	double Begin = MuzzleClearance;
	if (Discriminant >= 0) Begin = FMath::Max(Begin, -Projection + FMath::Sqrt(Discriminant));
	const double Finish = Length - HitClearance;
	if (!FMath::IsFinite(Begin) || Finish - Begin <= UE_SMALL_NUMBER) return Result;
	Result.Start = Muzzle + Direction * Begin;
	Result.End = Muzzle + Direction * Finish;
	Result.Length = Finish - Begin;
	Result.bValid = !Result.Start.ContainsNaN() && !Result.End.ContainsNaN();
	return Result;
}

inline int32 Count(const FSegment& Segment, double Spacing, int32 AuthoredCount)
{
	if (!Segment.bValid || !FMath::IsFinite(Segment.Length) || Segment.Length <= 0
		|| !FMath::IsFinite(Spacing) || Spacing < 1 || AuthoredCount < 2 || Segment.Length < Spacing) return 0;
	const int32 Maximum = FMath::Min(AuthoredCount, 512);
	return static_cast<int32>(FMath::Min(FMath::FloorToDouble(Segment.Length / Spacing) + 1.0, static_cast<double>(Maximum)));
}

/** Proportional margins preserve a short wave on the same physical beam. FPR is
 * still applied once by the material; the rendered length only controls size. */
inline FSegment FitSegment(FVector Muzzle, FVector Hit, double MuzzleClearance, double HitClearance)
{
	FSegment Result;
	if (Muzzle.ContainsNaN() || Hit.ContainsNaN() || !FMath::IsFinite(MuzzleClearance)
		|| !FMath::IsFinite(HitClearance) || MuzzleClearance < 0 || HitClearance < 0) return Result;
	const double Length = FVector::Distance(Muzzle, Hit);
	if (!FMath::IsFinite(Length) || Length <= UE_SMALL_NUMBER || Length > 1.e12) return Result;
	const FVector Direction = (Hit - Muzzle) / Length;
	const double Begin = FMath::Min(MuzzleClearance, Length * 0.1);
	const double Finish = Length - FMath::Min(HitClearance, Length * 0.1);
	Result.Start = Muzzle + Direction * Begin;
	Result.End = Muzzle + Direction * Finish;
	Result.Length = Finish - Begin;
	Result.bValid = !Result.Start.ContainsNaN() && !Result.End.ContainsNaN() && Result.Length > UE_SMALL_NUMBER;
	return Result;
}

inline float SizeScale(double PhysicalLength, double RenderedLength, double FullSizeDistance)
{
	if (!FMath::IsFinite(PhysicalLength) || !FMath::IsFinite(RenderedLength) || !FMath::IsFinite(FullSizeDistance)
		|| PhysicalLength <= 0 || RenderedLength <= 0 || FullSizeDistance < 1) return 0.f;
	return static_cast<float>(FMath::Clamp(FMath::Min(PhysicalLength, RenderedLength) / FullSizeDistance, 0.0, 1.0));
}

inline int32 FittedCount(const FSegment& Segment, double Spacing, int32 AuthoredCount)
{
	if (!Segment.bValid || !FMath::IsFinite(Segment.Length) || Segment.Length <= 0
		|| !FMath::IsFinite(Spacing) || Spacing < 1 || AuthoredCount < 2) return 0;
	// Keep enough samples to retain the authored twisting ribbon on a short path.
	return FMath::Clamp(Count(Segment, Spacing, AuthoredCount), FMath::Min(16, AuthoredCount), FMath::Min(AuthoredCount, 512));
}
}

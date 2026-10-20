#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraTypes.h"

namespace FirstPersonRendering
{
inline float SafeScale(float Scale, float Fallback = 0.25f)
{
	return FMath::IsFinite(Scale) ? FMath::Clamp(Scale, 0.001f, 1.f) : Fallback;
}

inline bool IsValidFOV(float FOV)
{
	return FMath::IsFinite(FOV) && FOV >= 0.001f && FOV < 180.f;
}

/** Bake only an effect origin into world space; leave its distant endpoint alone. */
inline bool TryTransformEffectPosition(const FMinimalViewInfo& View, const FVector& Position, FVector& OutPosition)
{
	OutPosition = Position;
	if (Position.ContainsNaN() || View.Location.ContainsNaN() || View.Rotation.ContainsNaN()
		|| View.ProjectionMode != ECameraProjectionMode::Perspective || !IsValidFOV(View.FOV)
		|| !IsValidFOV(View.FirstPersonFOV) || !FMath::IsFinite(View.FirstPersonScale)
		|| View.FirstPersonScale <= 0.f || View.FirstPersonScale > 1.f) return false;
	const FVector Rendered = View.TransformWorldToFirstPerson(Position, false);
	if (Rendered.ContainsNaN()) return false;
	OutPosition = Rendered;
	return true;
}

inline float SafeFirstPersonFOV(float Requested, float MainFOV)
{
	return IsValidFOV(Requested) ? Requested : (IsValidFOV(MainFOV) ? MainFOV : 90.f);
}

/** CPU reference for the generated FirstPersonOutput material's axial weight. */
inline double BeamGeometryWeight(const FVector& Position, const FVector& Start, const FVector& End)
{
	if (Position.ContainsNaN() || Start.ContainsNaN() || End.ContainsNaN()) return 0.0;
	const FVector Axis = End - Start;
	const double LengthSq = Axis.SizeSquared();
	if (!FMath::IsFinite(LengthSq) || LengthSq < 0.000001) return 0.0;
	const double Along = FVector::DotProduct(Position - Start, Axis) / LengthSq;
	if (!FMath::IsFinite(Along)) return 0.0;
	const double T = FMath::Clamp(Along, 0.0, 1.0);
	return 1.0 - T * T * (3.0 - 2.0 * T);
}

inline float SafeNearClip(float Requested, float Existing, float Global)
{
	const float Near = FMath::IsFinite(Requested) ? FMath::Clamp(Requested, 0.01f, 5.f) : 0.5f;
	const float Base = FMath::IsFinite(Existing) && Existing >= UE_SMALL_NUMBER ? Existing
		: (FMath::IsFinite(Global) && Global >= UE_SMALL_NUMBER ? Global : 5.f);
	return FMath::Min(Base, Near);
}

/** Fit a camera-centred bound inside the pawn's capsule; no extra collision queries. */
inline float FitScaleToCapsule(float RequestedScale, const FVector& Eye, double BoundDistance,
	const FVector& Centre, const FVector& Axis, double Radius, double HalfHeight,
	double Margin, double FOVExpansion = 1.0)
{
	const float Maximum = SafeScale(RequestedScale);
	if (Eye.ContainsNaN() || Centre.ContainsNaN() || Axis.ContainsNaN()
		|| !FMath::IsFinite(BoundDistance) || BoundDistance <= UE_SMALL_NUMBER
		|| !FMath::IsFinite(Radius) || Radius <= 0.0 || !FMath::IsFinite(HalfHeight) || HalfHeight < Radius
		|| !FMath::IsFinite(FOVExpansion)) return Maximum;
	const double AxisSizeSquared = Axis.SizeSquared();
	if (!FMath::IsFinite(AxisSizeSquared) || AxisSizeSquared <= UE_SMALL_NUMBER) return Maximum;
	const FVector Up = Axis.GetSafeNormal();
	if (Up.IsNearlyZero()) return Maximum;
	const double SegmentHalf = FMath::Max(HalfHeight - Radius, 0.0);
	const double Along = FMath::Clamp(FVector::DotProduct(Eye - Centre, Up), -SegmentHalf, SegmentHalf);
	const double Safety = FMath::Clamp(FMath::IsFinite(Margin) ? Margin : 2.0, 0.0, Radius * 0.5);
	const double Clearance = Radius - FVector::Distance(Eye, Centre + Up * Along) - Safety;
	// A camera outside its collision capsule has no safe interior sphere. Keep the
	// configured mode instead of collapsing the gun to an invisible near-zero size.
	if (!FMath::IsFinite(Clearance) || Clearance <= 0.0) return Maximum;
	const double Extent = BoundDistance * FMath::Max(FOVExpansion, 1.0);
	if (!FMath::IsFinite(Extent) || Extent <= UE_SMALL_NUMBER) return Maximum;
	return FMath::Min(Maximum, static_cast<float>(FMath::Clamp(Clearance / Extent, 0.001, 1.0)));
}
}

#include "SeamlessPortalMath.h"

namespace SeamlessPortalMath
{
	static FVector Turn(const FVector& v) { return FVector(-v.X, -v.Y, v.Z); }

	bool IsValidFrame(const FTransform& frame)
	{
		return !frame.ContainsNaN() && frame.GetRotation().IsNormalized();
	}

	FVector MapPoint(const FTransform& entry, const FTransform& exit, const FVector& point)
	{
		return exit.GetLocation() + exit.GetRotation().RotateVector(
			Turn(entry.GetRotation().UnrotateVector(point - entry.GetLocation())));
	}

	FVector MapVector(const FTransform& entry, const FTransform& exit, const FVector& vector)
	{
		return exit.GetRotation().RotateVector(Turn(entry.GetRotation().UnrotateVector(vector)));
	}

	FQuat MapRotation(const FTransform& entry, const FTransform& exit, const FQuat& rotation)
	{
		return (exit.GetRotation() * FQuat(FVector::UpVector, PI)
			* entry.GetRotation().Inverse() * rotation).GetNormalized();
	}

	bool BuildWindowProjection(const FVector& eye, const FVector2D& halfSize,
		double clipOffset, FMatrix& outProjection)
	{
		outProjection = FMatrix::Identity;
		const double distance = -eye.X;
		if (eye.ContainsNaN() || halfSize.ContainsNaN() || !FMath::IsFinite(clipOffset)
			|| distance <= 0.001 || halfSize.X <= 0.001 || halfSize.Y <= 0.001) return false;
		// Reversed-Z infinite perspective, expressed after UE's view-axis permutation.
		// At the exit plane, Y=-halfWidth always maps to U=0, regardless of eye position.
		outProjection = FMatrix(
			FPlane(distance / halfSize.X, 0, 0, 0),
			FPlane(0, distance / halfSize.Y, 0, 0),
			FPlane(eye.Y / halfSize.X, eye.Z / halfSize.Y, 0, 1),
			FPlane(0, 0, distance + FMath::Max(0.0, clipOffset), 0));
		return !outProjection.ContainsNaN();
	}

	bool FindFrontCrossing(const FVector& previous, const FVector& current,
		FVector& crossing, double& fraction)
	{
		crossing = FVector::ZeroVector;
		fraction = 0;
		if (previous.ContainsNaN() || current.ContainsNaN() || previous.X <= 0 || current.X > 0) return false;
		const double denominator = previous.X - current.X;
		if (!FMath::IsFinite(denominator) || denominator <= UE_DOUBLE_SMALL_NUMBER) return false;
		fraction = previous.X / denominator;
		crossing = FMath::Lerp(previous, current, fraction);
		return !crossing.ContainsNaN();
	}

	bool ScreenCoverage(TConstArrayView<FVector4> corners, float& coverage)
	{
		coverage = 0;
		TArray<FVector4, TInlineAllocator<16>> polygon;
		for (const FVector4& corner : corners)
		{
			if (corner.ContainsNaN()) return false;
			polygon.Add(corner);
		}
		auto Distance = [](const FVector4& p, int32 plane)
		{
			switch (plane)
			{
			case 0: return p.W - 0.00001;
			case 1: return p.W + p.X;
			case 2: return p.W - p.X;
			case 3: return p.W + p.Y;
			default: return p.W - p.Y;
			}
		};
		// Clip in homogeneous space before dividing by W. This also handles a camera partly inside the doorway.
		for (int32 plane = 0; plane < 5; ++plane)
		{
			if (polygon.IsEmpty()) return false;
			TArray<FVector4, TInlineAllocator<16>> clipped;
			FVector4 previous = polygon.Last();
			double previousDistance = Distance(previous, plane);
			for (const FVector4& point : polygon)
			{
				const double distance = Distance(point, plane);
				if ((distance >= 0) != (previousDistance >= 0))
					clipped.Add(FMath::Lerp(previous, point, previousDistance / (previousDistance - distance)));
				if (distance >= 0) clipped.Add(point);
				previous = point; previousDistance = distance;
			}
			polygon = MoveTemp(clipped);
		}
		if (polygon.IsEmpty()) return false;
		FVector2D minimum(1,1), maximum(-1,-1);
		for (const FVector4& point : polygon)
		{
			if (point.W <= 0) continue;
			minimum.X = FMath::Min(minimum.X, point.X / point.W); minimum.Y = FMath::Min(minimum.Y, point.Y / point.W);
			maximum.X = FMath::Max(maximum.X, point.X / point.W); maximum.Y = FMath::Max(maximum.Y, point.Y / point.W);
		}
		coverage = FMath::Clamp(FMath::Max(0.0, maximum.X - minimum.X) * FMath::Max(0.0, maximum.Y - minimum.Y) * 0.25, 0.0, 1.0);
		return coverage > 0;
	}

	bool CapsuleFits(const FTransform& frame, const FVector& centre, const FVector& up,
		double radius, double halfHeight, const FVector2D& halfSize, bool bEllipse)
	{
		if (!IsValidFrame(frame) || centre.ContainsNaN() || up.ContainsNaN() || up.IsNearlyZero()
			|| halfSize.ContainsNaN() || !FMath::IsFinite(radius) || !FMath::IsFinite(halfHeight)
			|| radius <= 0 || halfHeight < radius || halfSize.X <= 0 || halfSize.Y <= 0) return false;
		const FVector local = frame.InverseTransformPositionNoScale(centre);
		const FVector localUp = frame.GetRotation().UnrotateVector(up.GetSafeNormal());
		const double horizontal = FMath::Abs(local.Y) + radius + (halfHeight - radius) * FMath::Abs(localUp.Y);
		const double vertical = FMath::Abs(local.Z) + radius + (halfHeight - radius) * FMath::Abs(localUp.Z);
		if (!FMath::IsFinite(horizontal) || !FMath::IsFinite(vertical)) return false;
		// Conservative projected capsule rectangle: never admit a capsule through an oval corner.
		return bEllipse ? FMath::Square(horizontal / halfSize.X) + FMath::Square(vertical / halfSize.Y) <= 1.0
			: horizontal <= halfSize.X && vertical <= halfSize.Y;
	}
}

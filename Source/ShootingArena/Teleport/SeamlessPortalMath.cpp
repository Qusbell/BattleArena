#include "SeamlessPortalMath.h"

namespace SeamlessPortalMath
{
	static FVector Turn(const FVector& v) { return FVector(-v.X, -v.Y, v.Z); }

	// Exact projected sphere-cap containment, rather than fitting a bounding rectangle into an oval.
	// The latter incorrectly rejects upright players walking through the lower centre of a tall portal.
	static bool CircleFitsEllipse(const FVector2D& centre, double radius, const FVector2D& halfSize)
	{
		const double dy = 1.0/FMath::Square(halfSize.X), dz = 1.0/FMath::Square(halfSize.Y);
		const double a = radius*radius*dy, b = radius*radius*dz;
		const double gy = radius*dy*centre.X, gz = radius*dz*centre.Y;
		const double constant = centre.X*centre.X*dy+centre.Y*centre.Y*dz;
		const double maximum = FMath::Max(a,b);
		const double magnitude = FMath::Sqrt(gy*gy+gz*gz);
		if (magnitude <= 1.e-14) return constant+maximum <= 1.0;
		const double dominant = a >= b ? gy : gz;
		const double other = a >= b ? gz : gy;
		const double gap = FMath::Abs(a-b);
		if (FMath::Abs(dominant) <= 1.e-14 && gap > 1.e-14 && FMath::Abs(other) <= gap)
			return constant+maximum+other*other/gap <= 1.0;
		double low = maximum, high = maximum+magnitude;
		for (int32 i = 0; i < 48; ++i)
		{
			const double lambda = (low+high)*0.5;
			const double y = gy/FMath::Max(lambda-a,1.e-18);
			const double z = gz/FMath::Max(lambda-b,1.e-18);
			if (y*y+z*z > 1) low = lambda; else high = lambda;
		}
		// The dual value is an upper bound, so numerical rounding cannot admit an intersecting cap.
		return constant+high+gy*gy/FMath::Max(high-a,1.e-18)+gz*gz/FMath::Max(high-b,1.e-18) <= 1.0;
	}

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
		if (!bEllipse) return horizontal <= halfSize.X && vertical <= halfSize.Y;
		if (horizontal > halfSize.X || vertical > halfSize.Y) return false;
		const FVector2D centre2D(local.Y,local.Z);
		const FVector2D segment = FVector2D(localUp.Y,localUp.Z)*(halfHeight-radius);
		// A projected capsule is the convex hull of its two endpoint disks. An ellipse is convex too.
		return CircleFitsEllipse(centre2D+segment,radius,halfSize) && CircleFitsEllipse(centre2D-segment,radius,halfSize);
	}

	static double CapsuleOpeningDistanceSquared(const FVector& local, const FVector& segment,
		const FVector2D& halfSize, bool bEllipse)
	{
		// Distance from a capsule's axis segment to the filled opening, not to its bounding box.
		// For an outside point, closest ellipse coordinates are a^2*p/(a^2+lambda).
		auto DistanceSquared = [&](double along)
		{
			const FVector p = local+segment*along;
			if (!bEllipse)
				return p.X*p.X+FMath::Square(FMath::Max(0.0,FMath::Abs(p.Y)-halfSize.X))
					+FMath::Square(FMath::Max(0.0,FMath::Abs(p.Z)-halfSize.Y));
			if (FMath::Square(p.Y/halfSize.X)+FMath::Square(p.Z/halfSize.Y) <= 1) return p.X*p.X;
			const double a = halfSize.X*halfSize.X, b = halfSize.Y*halfSize.Y;
			double low = 0, high = FMath::Sqrt(FMath::Square(halfSize.X*p.Y)+FMath::Square(halfSize.Y*p.Z));
			for (int32 i = 0; i < 32; ++i)
			{
				const double lambda = (low+high)*0.5;
				if (FMath::Square(halfSize.X*p.Y/(a+lambda))+FMath::Square(halfSize.Y*p.Z/(b+lambda)) > 1)
					low = lambda;
				else high = lambda;
			}
			return p.X*p.X+FMath::Square(p.Y-a*p.Y/(a+high))+FMath::Square(p.Z-b*p.Z/(b+high));
		};
		const double centreDistance = DistanceSquared(0);
		if (centreDistance <= 1.e-12) return 0;
		if (FMath::Abs(segment.X) < 1.e-8 && centreDistance <= local.X*local.X+1.e-12)
			return local.X*local.X;
		double closest;
		// Upright wall portals (including arbitrary yaw) use one distance query, with no nested search.
		if (segment.IsNearlyZero()) closest = DistanceSquared(0);
		else if (FMath::Abs(segment.X) < 1.e-8 && FMath::Abs(segment.Y) < 1.e-8)
			closest = DistanceSquared(FMath::Clamp(-local.Z/segment.Z,-1.0,1.0));
		else if (FMath::Abs(segment.X) < 1.e-8 && FMath::Abs(segment.Z) < 1.e-8)
			closest = DistanceSquared(FMath::Clamp(-local.Y/segment.Y,-1.0,1.0));
		else if (FMath::Abs(segment.Y) < 1.e-8 && FMath::Abs(segment.Z) < 1.e-8)
			closest = DistanceSquared(FMath::Clamp(-local.X/segment.X,-1.0,1.0));
		else
		{
			// Squared distance to a convex set is convex along a segment, also for tilted portals.
			double low = -1, high = 1;
			for (int32 i = 0; i < 40; ++i)
			{
				const double first = (2*low+high)/3, second = (low+2*high)/3;
				if (DistanceSquared(first) <= DistanceSquared(second)) high = second; else low = first;
			}
			closest = FMath::Min3(DistanceSquared(-1),DistanceSquared(1),DistanceSquared((low+high)*0.5));
		}
		return closest;
	}

	bool CapsuleTouchesOpening(const FTransform& frame, const FVector& centre, const FVector& up,
		double radius, double halfHeight, const FVector2D& halfSize, bool bEllipse)
	{
		if (!IsValidFrame(frame) || centre.ContainsNaN() || up.ContainsNaN() || up.IsNearlyZero()
			|| halfSize.ContainsNaN() || !FMath::IsFinite(radius) || !FMath::IsFinite(halfHeight)
			|| radius <= 0 || halfHeight < radius || halfSize.X <= 0 || halfSize.Y <= 0) return false;
		FVector local = frame.InverseTransformPositionNoScale(centre);
		local.X = 0; // Aperture eligibility also prepares passage-wall exclusions before contact.
		const FVector segment = frame.GetRotation().UnrotateVector(up.GetSafeNormal())*(halfHeight-radius);
		if (FMath::Abs(local.Y)-FMath::Abs(segment.Y) > halfSize.X+radius
			|| FMath::Abs(local.Z)-FMath::Abs(segment.Z) > halfSize.Y+radius) return false;
		const double closest = CapsuleOpeningDistanceSquared(local,segment,halfSize,bEllipse);
		return FMath::IsFinite(closest) && closest <= radius*radius+1.e-6;
	}

	bool FindFrontCapsuleContact(const FTransform& frame, const FVector& previous, const FVector& current,
		const FVector& up, double radius, double halfHeight, const FVector2D& halfSize,
		bool bEllipse, double& fraction)
	{
		fraction = 0;
		if (!IsValidFrame(frame) || previous.ContainsNaN() || current.ContainsNaN()
			|| up.ContainsNaN() || up.IsNearlyZero() || halfSize.ContainsNaN()
			|| !FMath::IsFinite(radius) || !FMath::IsFinite(halfHeight)
			|| radius <= 0 || halfHeight < radius || halfSize.X <= 0 || halfSize.Y <= 0) return false;
		const FVector start = frame.InverseTransformPositionNoScale(previous);
		const FVector end = frame.InverseTransformPositionNoScale(current);
		const FVector delta = end-start;
		// Front-side inward or lateral contact, not standing still, receding, or arriving from behind.
		if (start.X < 0 || delta.X > 1.e-6 || delta.IsNearlyZero(1.e-6)) return false;
		const FVector segment = frame.GetRotation().UnrotateVector(up.GetSafeNormal())*(halfHeight-radius);
		const FVector margin = segment.GetAbs()+FVector(radius,halfSize.X+radius,halfSize.Y+radius);
		double low = 0, high = 1;
		// Clip the movement to a cheap swept AABB before evaluating curved contact distances.
		for (int32 axis = 0; axis < 3; ++axis)
		{
			if (FMath::Abs(delta[axis]) < 1.e-12)
			{
				if (FMath::Abs(start[axis]) > margin[axis]) return false;
				continue;
			}
			const double first = (-margin[axis]-start[axis])/delta[axis];
			const double second = (margin[axis]-start[axis])/delta[axis];
			low = FMath::Max(low,FMath::Min(first,second));
			high = FMath::Min(high,FMath::Max(first,second));
			if (low > high) return false;
		}
		const double threshold = radius*radius+1.e-6;
		auto DistanceSquared = [&](double t)
		{ return CapsuleOpeningDistanceSquared(start+delta*t,segment,halfSize,bEllipse); };
		if (DistanceSquared(low) <= threshold) { fraction = low; return true; }
		double inside = high;
		if (DistanceSquared(inside) > threshold)
		{
			// Convex distance over time finds a grazing contact even when both frame endpoints miss.
			double left = low, right = high;
			for (int32 i = 0; i < 40; ++i)
			{
				const double first = (2*left+right)/3, second = (left+2*right)/3;
				if (DistanceSquared(first) <= DistanceSquared(second)) right = second; else left = first;
			}
			inside = (left+right)*0.5;
			if (DistanceSquared(inside) > threshold) return false;
		}
		for (int32 i = 0; i < 32; ++i)
		{
			const double mid = (low+inside)*0.5;
			if (DistanceSquared(mid) <= threshold) inside = mid; else low = mid;
		}
		fraction = inside;
		return FMath::IsFinite(fraction);
	}
}

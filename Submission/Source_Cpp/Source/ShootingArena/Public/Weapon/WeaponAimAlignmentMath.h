#pragma once

#include "CoreMinimal.h"

/** Pure pose geometry shared by the worker node and editor coordinate verification. */
namespace WeaponAimAlignmentMath
{
	inline FTransform RotateAround(const FTransform& Transform, const FVector& Pivot, const FQuat& Rotation)
	{
		FTransform Result = Transform;
		Result.SetLocation(Pivot + Rotation.RotateVector(Transform.GetLocation() - Pivot));
		Result.SetRotation((Rotation * Transform.GetRotation()).GetNormalized());
		return Result;
	}

	inline float FreeHandWeight(const FVector& ViewForward, float BlendAngle)
	{
		if (ViewForward.ContainsNaN() || ViewForward.IsNearlyZero()) return 0.f;
		const double Pitch = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(ViewForward.GetSafeNormal().Z, -1.0, 1.0)));
		const double Range = FMath::IsFinite(BlendAngle) ? FMath::Clamp(BlendAngle, 1.f, 89.f) : 20.f;
		const double Weight = FMath::Clamp(-Pitch / Range, 0.0, 1.0);
		return Weight * Weight * (3.0 - 2.0 * Weight);
	}

	inline FVector StableJointTarget(const FVector& Shoulder, const FVector& Elbow,
		const FVector& Hand, const FVector& DesiredHand, const FVector& Bias)
	{
		FVector OldDirection = (Hand - Shoulder).GetSafeNormal();
		if (OldDirection.IsNearlyZero()) OldDirection = FVector::ForwardVector;
		FVector NewDirection = (DesiredHand - Shoulder).GetSafeNormal();
		if (NewDirection.IsNearlyZero()) NewDirection = OldDirection;
		FVector Bend = Elbow - Shoulder;
		Bend -= OldDirection * FVector::DotProduct(Bend, OldDirection);
		if (Bend.IsNearlyZero())
		{
			FVector Other;
			OldDirection.FindBestAxisVectors(Bend, Other);
		}
		Bend = FQuat::FindBetweenNormals(OldDirection, NewDirection).RotateVector(Bend).GetSafeNormal();
		// An axial roll of the weapon does not enter this calculation. The elbow stays
		// on the authored side of the arm, including near-vertical aiming directions.
		return Shoulder + NewDirection * (Elbow - Shoulder).Size()
			+ Bend * FMath::Max(1.0, (Elbow - Shoulder).Size()) + Bias;
	}

	inline FQuat ShareForearmTwist(const FTransform& OriginalLower, const FTransform& OriginalHand,
		const FTransform& SolvedLower, const FTransform& SolvedHand, const FQuat& DesiredHandRotation, float Share)
	{
		const FVector Axis = (SolvedHand.GetLocation() - SolvedLower.GetLocation()).GetSafeNormal();
		if (Axis.IsNearlyZero()) return SolvedLower.GetRotation();
		const FQuat WristLocal = OriginalLower.GetRotation().Inverse() * OriginalHand.GetRotation();
		FQuat Delta = (DesiredHandRotation * WristLocal.Inverse()) * SolvedLower.GetRotation().Inverse();
		Delta.Normalize();
		if (Delta.W < 0.0) Delta = Delta * -1.0;
		const FVector Projected = Axis * FVector::DotProduct(FVector(Delta.X, Delta.Y, Delta.Z), Axis);
		FQuat Twist(Projected.X, Projected.Y, Projected.Z, Delta.W);
		if (Twist.SizeSquared() < UE_SMALL_NUMBER) return SolvedLower.GetRotation();
		Twist.Normalize();
		// Bound the added forearm roll, leaving the remainder at the wrist.
		const double Angle = Twist.GetAngle();
		const double Limit = FMath::DegreesToRadians(60.0);
		if (Angle > Limit) Twist = FQuat::Slerp(FQuat::Identity, Twist, Limit / Angle).GetNormalized();
		return (FQuat::Slerp(FQuat::Identity, Twist, FMath::Clamp(Share, 0.f, 1.f))
			* SolvedLower.GetRotation()).GetNormalized();
	}
}

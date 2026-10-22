#pragma once

#include "CoreMinimal.h"

/** Portal frames use +X as the front, +Y as horizontal, and +Z as up. Scale never changes speed. */
namespace SeamlessPortalMath
{
	bool IsValidFrame(const FTransform& frame);
	FVector MapPoint(const FTransform& entry, const FTransform& exit, const FVector& point);
	FVector MapVector(const FTransform& entry, const FTransform& exit, const FVector& vector);
	FQuat MapRotation(const FTransform& entry, const FTransform& exit, const FQuat& rotation);
	bool BuildWindowProjection(const FVector& exitEyeLocal, const FVector2D& halfSize,
		double clipOffset, FMatrix& outProjection);
	bool ScreenCoverage(TConstArrayView<FVector4> corners, float& outCoverage);
	bool FindFrontCrossing(const FVector& previousLocal, const FVector& currentLocal,
		FVector& outCrossing, double& outFraction);
	bool CapsuleFits(const FTransform& frame, const FVector& centre, const FVector& capsuleUp,
		double radius, double halfHeight, const FVector2D& halfSize, bool bEllipse);
	/** Aperture-only eligibility for passage-wall clearance and destination checks. Ignores normal
	 * distance; actual entry timing/direction is authorized by FindFrontCapsuleContact below. */
	bool CapsuleTouchesOpening(const FTransform& frame, const FVector& centre, const FVector& capsuleUp,
		double radius, double halfHeight, const FVector2D& halfSize, bool bEllipse);
	/** First swept capsule contact with the actual plane/aperture from its front side.
	 * Includes lateral grazing and fast steps; never requires the centre to cross the plane. */
	bool FindFrontCapsuleContact(const FTransform& frame, const FVector& previous, const FVector& current,
		const FVector& capsuleUp, double radius, double halfHeight, const FVector2D& halfSize,
		bool bEllipse, double& outFraction);
}

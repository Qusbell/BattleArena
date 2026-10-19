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
}

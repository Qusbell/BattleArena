#pragma once

#include "CoreMinimal.h"

// The existing Blueprint probability/duration formula, independent of BT node lifetime.
struct FQuakeAIFireCycleState
{
	bool bFiring = false;
	double EndTime = 0.;

	bool IsExpired(double Now) const { return Now > EndTime; }
	void BeginNext(double Now, double Throttle, double Random)
	{
		bFiring = Throttle > Random;
		EndTime = Now + (bFiring ? 1. - Throttle : Throttle);
	}
};

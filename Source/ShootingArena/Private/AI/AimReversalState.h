#pragma once

#include "CoreMinimal.h"

// State belongs to one observer and is reset whenever its target changes.
struct FAimReversalState
{
    FVector PreviousVelocity = FVector::ZeroVector;
    double LastValidTime = -DBL_MAX;
    double PenaltyUntil = -DBL_MAX;

    bool Update(const FVector& Velocity, double Now, double MemorySeconds, double PenaltySeconds)
    {
        const FVector Horizontal(Velocity.X, Velocity.Y, 0.0);
        if (Horizontal.SizeSquared() >= 1.0)
        {
            if (Now - LastValidTime <= MemorySeconds && FVector::DotProduct(PreviousVelocity, Horizontal) < 0.0)
                PenaltyUntil = Now + PenaltySeconds;
            PreviousVelocity = Horizontal;
            LastValidTime = Now;
        }
        return Now < PenaltyUntil;
    }
};

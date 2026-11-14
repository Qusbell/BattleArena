#pragma once

#include "CoreMinimal.h"
#include "AI/PathLink/PathLinkTypes.h"

class AActor;

/** Optimistic reachability bound for an ordinary Jump traversal. */
struct FPathLinkJumpCapability
{
    static FPathLinkJumpCapability FromAgent(const AActor* Agent);

    bool Allows(EPathLinkType Type, const FVector& Entry, const FVector& Exit) const;

    FVector Up = FVector::UpVector;
    double Gravity = 0;
    double JumpSpeed = 0;
    double HorizontalSpeed = 0;
};

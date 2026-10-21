#pragma once

#include "CoreMinimal.h"

/** Exact static costs plus unresolved dynamic costs. Estimates order work; only bounds prune it. */
struct FPathLinkLazyEdge
{
    int32 From = INDEX_NONE;
    int32 To = INDEX_NONE;
    double Cost = 0;
    double LowerBound = 0;
    double Estimate = 0;
    bool bExact = true;
    bool bEnabled = true;
};

namespace PathLinkLazySearch
{
    /** Resolve returns the exact cost, or false for an unreachable edge. Each edge is resolved at most once. */
    bool FindRoute(int32 NodeCount, int32 Start, int32 Target, TArray<FPathLinkLazyEdge>& Edges,
        TFunctionRef<bool(int32, double&)> Resolve, TArray<int32>& OutRoute,
        int32& OutSolveCount, int32& OutResolvedCount);
}

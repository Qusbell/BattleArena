#include "AI/PathLink/Internal/PathLinkLazySearch.h"

#include "Algo/Reverse.h"

bool PathLinkLazySearch::FindRoute(int32 NodeCount, int32 Start, int32 Target,
    TArray<FPathLinkLazyEdge>& Edges, TFunctionRef<bool(int32, double&)> Resolve,
    TArray<int32>& OutRoute, int32& OutSolveCount, int32& OutResolvedCount)
{
    OutRoute.Reset(); OutSolveCount = 0; OutResolvedCount = 0;
    if (NodeCount <= 0 || Start < 0 || Target < 0 || Start >= NodeCount || Target >= NodeCount) return false;
    TArray<TArray<int32>> Adjacency;
    Adjacency.SetNum(NodeCount);
    int32 Unresolved = 0;
    for (int32 I = 0; I < Edges.Num(); ++I)
    {
        const auto& Edge = Edges[I];
        if (Edge.From < 0 || Edge.To < 0 || Edge.From >= NodeCount || Edge.To >= NodeCount) return false;
        Adjacency[Edge.From].Add(I);
        Unresolved += Edge.bEnabled && !Edge.bExact;
    }
    const double Infinity = TNumericLimits<double>::Max();
    TArray<double> Distances;
    TArray<int32> Previous;
    TArray<bool> Visited;
    auto Solve = [&](bool bBounds, TArray<int32>& Route, double& Cost)
    {
        ++OutSolveCount;
        Route.Reset(); Distances.Init(Infinity, NodeCount);
        Previous.Init(INDEX_NONE, NodeCount); Visited.Init(false, NodeCount);
        Distances[Start] = 0;
        for (int32 Iteration = 0; Iteration < NodeCount; ++Iteration)
        {
            int32 Node = INDEX_NONE;
            double Best = Infinity;
            for (int32 I = 0; I < NodeCount; ++I)
                if (!Visited[I] && Distances[I] < Best) { Node = I; Best = Distances[I]; }
            if (Node == INDEX_NONE || Node == Target) break;
            Visited[Node] = true;
            for (int32 Index : Adjacency[Node])
            {
                const auto& Edge = Edges[Index];
                if (!Edge.bEnabled) continue;
                const double Weight = Edge.bExact ? Edge.Cost : bBounds ? Edge.LowerBound : Edge.Estimate;
                const double Next = Best + Weight;
                if (Next < Distances[Edge.To]) { Distances[Edge.To] = Next; Previous[Edge.To] = Index; }
            }
        }
        Cost = Distances[Target];
        if (Previous[Target] == INDEX_NONE) return false;
        for (int32 Node = Target; Node != Start; Node = Edges[Previous[Node]].From)
            Route.Add(Previous[Node]);
        Algo::Reverse(Route);
        return true;
    };

    double Incumbent = Infinity;
    TArray<int32> Route;
    // One estimate-prioritized route supplies an incumbent. Every later solve uses certified bounds.
    // Each nonterminal pass resolves at least one edge, so no arbitrary candidate/pass cap is needed.
    for (int32 Pass = 0; Pass <= Unresolved + 1; ++Pass)
    {
        double Bound = Infinity;
        const bool bBounds = Pass > 0;
        if (!Solve(bBounds, Route, Bound)) return !OutRoute.IsEmpty();
        if (bBounds && Incumbent < Infinity && Bound >= Incumbent) return true;
        bool bReachable = true;
        double ExactCost = 0;
        for (int32 Index : Route)
        {
            auto& Edge = Edges[Index];
            if (!Edge.bExact)
            {
                ++OutResolvedCount;
                double Cost = 0;
                Edge.bEnabled = Resolve(Index, Cost) && FMath::IsFinite(Cost) && Cost >= 0;
                Edge.bExact = true;
                Edge.Cost = Cost;
            }
            if (!Edge.bEnabled) { bReachable = false; break; }
            ExactCost += Edge.Cost;
        }
        if (bReachable && ExactCost < Incumbent) { Incumbent = ExactCost; OutRoute = Route; }
    }
    return false; // Defensive: a pass must either resolve an edge, find an exact shortest path, or finish.
}

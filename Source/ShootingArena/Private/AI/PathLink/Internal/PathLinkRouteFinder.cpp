#include "AI/PathLink/Internal/PathLinkRouteFinder.h"

#include "AI/PathLink/PathLink.h"
#include "AI/PathLink/Internal/PathLinkLazySearch.h"
#include "AI/PathLink/Internal/PathLinkNavigation.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Misc/ScopeExit.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

DEFINE_LOG_CATEGORY_STATIC(LogPathLinkRouteFinder, Log, All);

namespace
{
    struct FRequestPath
    {
        FVector From;
        FVector To;
        FNavPathSharedPtr Path;
    };

    using FRequestPaths = TArray<FRequestPath>;

    FNavPathSharedPtr QueryPath(UWorld* World, const FVector& From, const FVector& To,
        AActor* Context, int32* Count, FRequestPaths* Cache)
    {
        if (Cache)
            for (const FRequestPath& Entry : *Cache)
                if (Entry.From == From && Entry.To == To) return Entry.Path;
        if (Count) ++*Count;
        FNavPathSharedPtr Path = PathLinkNavigation::FindPath(World, From, To, Context);
        if (Cache) Cache->Add({From, To, Path});
        return Path;
    }
    const FVector NavProjectionExtent(150.0, 150.0, 400.0);

    enum class EDynamicRouteEdgeType : uint8
    {
        DirectToTarget,
        EnterTraversal,
        TraversalToTarget
    };

    struct FDynamicRouteEdge
    {
        int32 From = INDEX_NONE;
        int32 To = INDEX_NONE;

        /** Dijkstra 비교용 전체 비용입니다. EnterTraversal이면 Normal + LinkCost입니다. */
        double Cost = 0.0;

        /** 이 Edge 안에서 실제 NavMesh 이동에 해당하는 거리입니다. */
        double NavDistance = 0.0;

        EDynamicRouteEdgeType Type = EDynamicRouteEdgeType::DirectToTarget;

        /** EnterTraversal일 때 새로 진입하는 Traversal Index입니다. */
        int32 TraversalIndex = INDEX_NONE;
        bool bEnabled = true;
    };

    bool ProjectToNavigation(
        UWorld* World,
        const FVector& SourceLocation,
        FVector& OutNavLocation)
    {
        OutNavLocation = SourceLocation;

        UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
        if (!IsValid(NavSystem))
        {
            return false;
        }

        FNavLocation ProjectedLocation;
        if (!NavSystem->ProjectPointToNavigation(
            SourceLocation,
            ProjectedLocation,
            NavProjectionExtent,
            static_cast<const FNavAgentProperties*>(nullptr),
            FSharedConstNavQueryFilter()))
        {
            return false;
        }

        OutNavLocation = ProjectedLocation.Location;
        return true;
    }

    /**
     * 후보 비교용 거리만 계산합니다. PathPoints를 복사하지 않아 Graph 구축/Route 비교 시 메모리 비용을 줄입니다.
     */
    bool BuildNavigationDistance(
        UWorld* World,
        const FVector& FromNavLocation,
        const FVector& ToNavLocation,
        AActor* PathfindingContext,
        double& OutDistance,
        int32* InOutNavQueryCount = nullptr,
        FRequestPaths* Cache = nullptr)
    {
        OutDistance = 0.0;

        if (FromNavLocation.Equals(ToNavLocation, 1.0f))
        {
            return true;
        }

        const FNavPathSharedPtr Path = QueryPath(World, FromNavLocation, ToNavLocation,
            PathfindingContext, InOutNavQueryCount, Cache);
        if (!Path)
        {
            return false;
        }

        OutDistance = Path->GetLength();
        return true;
    }

    /** 최종 선택된 Normal Segment에 대해서만 실제 PathPoints를 만듭니다. */
    bool BuildNavigationPath(
        UWorld* World,
        const FVector& FromNavLocation,
        const FVector& ToNavLocation,
        AActor* PathfindingContext,
        double& OutDistance,
        TArray<FVector>& OutPathPoints,
        int32* InOutNavQueryCount = nullptr,
        FRequestPaths* Cache = nullptr)
    {
        OutDistance = 0.0;
        OutPathPoints.Reset();

        if (FromNavLocation.Equals(ToNavLocation, 1.0f))
        {
            OutPathPoints.Add(FromNavLocation);
            OutPathPoints.Add(ToNavLocation);
            return true;
        }

        const FNavPathSharedPtr Path = QueryPath(World, FromNavLocation, ToNavLocation,
            PathfindingContext, InOutNavQueryCount, Cache);
        if (!Path)
        {
            return false;
        }

        OutDistance = Path->GetLength();
        OutPathPoints.Reserve(Path->GetPathPoints().Num());
        for (const FNavPathPoint& Point : Path->GetPathPoints()) OutPathPoints.Add(Point.Location);
        return true;
    }

    double GetLinkTraversalCost(
        const APathLink* Link,
        const FVector& EntryLocation,
        const FVector& ExitLocation)
    {
        if (!IsValid(Link))
        {
            return 0.0;
        }

        // Teleport는 순간이동 자체의 물리적 이동거리 Cost를 0으로 취급합니다.
        if (Link->GetLinkType() == EPathLinkType::Teleport)
        {
            return 0.0;
        }

        return FVector::Distance(EntryLocation, ExitLocation);
    }
}

bool FPathLinkRouteFinder::BuildStaticGraph(
    const TArray<APathLink*>& Links,
    AActor* PathfindingContext,
    FPathLinkStaticGraph& OutGraph,
    bool bComputeTransitions) const
{
    OutGraph.Reset();

    if (!IsValid(World))
    {
        return false;
    }

    UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(World);
    if (!IsValid(NavSystem))
    {
        return false;
    }

    const double BuildStartSeconds = FPlatformTime::Seconds();

    OutGraph.Traversals.Reserve(Links.Num() * 2);

    for (APathLink* Link : Links)
    {
        // Subsystem이 구조 Validation/Enabled 필터를 이미 끝낸 Link만 전달합니다.
        // 여기서는 Hot Path의 중복 Validation을 다시 수행하지 않습니다.
        if (!IsValid(Link) || !Link->IsEnabled())
        {
            continue;
        }

        const FVector ForwardEntry = Link->GetEntryLocation();
        const FVector ForwardExit = Link->GetExitLocation();

        if (ForwardEntry.ContainsNaN()
            || ForwardExit.ContainsNaN()
            || ForwardEntry.Equals(ForwardExit, 1.0f))
        {
            continue;
        }

        FVector ForwardEntryNav;
        FVector ForwardExitNav;
        if (!ProjectToNavigation(World, ForwardEntry, ForwardEntryNav)
            || !ProjectToNavigation(World, ForwardExit, ForwardExitNav))
        {
            continue;
        }

        auto AddTraversal = [&OutGraph, Link](
            const bool Reverse,
            const FVector& EntryLocation,
            const FVector& ExitLocation,
            const FVector& EntryNavLocation,
            const FVector& ExitNavLocation)
        {
            FPathLinkTraversalNode& Traversal = OutGraph.Traversals.AddDefaulted_GetRef();
            Traversal.Link = Link;
            Traversal.Reverse = Reverse;
            Traversal.LinkType = Link->GetLinkType();
            Traversal.EntryLocation = EntryLocation;
            Traversal.ExitLocation = ExitLocation;
            Traversal.EntryNavLocation = EntryNavLocation;
            Traversal.ExitNavLocation = ExitNavLocation;
            Traversal.LinkCost = GetLinkTraversalCost(Link, EntryLocation, ExitLocation);
        };

        AddTraversal(
            false,
            ForwardEntry,
            ForwardExit,
            ForwardEntryNav,
            ForwardExitNav);

        if (Link->IsTwoWay())
        {
            // TwoWay는 같은 두 Endpoint를 반대로 사용하는 별도 Traversal로 캐시합니다.
            AddTraversal(
                true,
                ForwardExit,
                ForwardEntry,
                ForwardExitNav,
                ForwardEntryNav);
        }
    }

    const int32 TraversalCount = OutGraph.Traversals.Num();
    OutGraph.Transitions.SetNum(TraversalCount * TraversalCount);
    if (!bComputeTransitions) return true;

    int32 NavQueryCount = 0;

    // 핵심 최적화 지점입니다.
    // Link Exit -> 다른 Link Entry는 Start/Target과 무관한 정적 값이므로 Graph 구축 시 한 번만 계산합니다.
    for (int32 FromIndex = 0; FromIndex < TraversalCount; ++FromIndex)
    {
        const FPathLinkTraversalNode& FromTraversal = OutGraph.Traversals[FromIndex];

        for (int32 ToIndex = 0; ToIndex < TraversalCount; ++ToIndex)
        {
            if (FromIndex == ToIndex)
            {
                continue;
            }

            const FPathLinkTraversalNode& ToTraversal = OutGraph.Traversals[ToIndex];

            // 같은 Link를 즉시 반대 방향으로 다시 타는 순환은 비음수 Cost 최단경로에서 이득이 없으므로 제외합니다.
            if (FromTraversal.Link == ToTraversal.Link)
            {
                continue;
            }

            double NavDistance = 0.0;
            if (!BuildNavigationDistance(
                World,
                FromTraversal.ExitNavLocation,
                ToTraversal.EntryNavLocation,
                PathfindingContext,
                NavDistance,
                &NavQueryCount))
            {
                continue;
            }

            const int32 TransitionIndex = FromIndex * TraversalCount + ToIndex;
            FPathLinkCachedTransition& Transition = OutGraph.Transitions[TransitionIndex];
            Transition.Reachable = true;
            Transition.NavDistance = NavDistance;
        }
    }

    OutGraph.PrecomputedNavQueryCount = NavQueryCount;

    const double BuildMilliseconds = (FPlatformTime::Seconds() - BuildStartSeconds) * 1000.0;
    UE_LOG(
        LogPathLinkRouteFinder,
        Log,
        TEXT("[PathLink][RouteCache] Build 완료 | Links=%d | Traversals=%d | LinkToLinkNavQueries=%d | Time=%.3fms | Context=%s"),
        Links.Num(),
        TraversalCount,
        NavQueryCount,
        BuildMilliseconds,
        *GetNameSafe(PathfindingContext));

    return true;
}

bool FPathLinkRouteFinder::FindShortestRoute(
    const FVector& StartLocation,
    const FVector& TargetLocation,
    const FPathLinkStaticGraph& StaticGraph,
    AActor* PathfindingContext,
    FPathLinkRouteResult& OutResult,
    bool bBuildSegments,
    FPathLinkQueryStats* OutStats,
    const FPathLinkRouteEstimates* Estimates,
    bool bEuclideanBoundsSafe) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(PathLink_DetailedRoute);
    FPathLinkQueryStats Stats;
    const double RequestStart = FPlatformTime::Seconds();
    ON_SCOPE_EXIT { Stats.TotalMilliseconds = (FPlatformTime::Seconds() - RequestStart) * 1000.0;
        if (OutStats) *OutStats = Stats; };
    FRequestPaths RequestPaths;
    OutResult.Reset();

    if (!IsValid(World))
    {
        return false;
    }

    if (StartLocation.Equals(TargetLocation, 1.0f))
    {
        OutResult.Success = true;
        OutResult.TotalDistance = 0.0;
        return true;
    }

    const double RouteStartSeconds = FPlatformTime::Seconds();

    FVector StartNavLocation;
    FVector TargetNavLocation;
    if (!ProjectToNavigation(World, StartLocation, StartNavLocation)
        || !ProjectToNavigation(World, TargetLocation, TargetNavLocation))
    {
        return false;
    }

    const int32 TraversalCount = StaticGraph.Traversals.Num();
    const int32 StartNode = 0;
    const int32 FirstTraversalNode = 1;
    const int32 TargetNode = FirstTraversalNode + TraversalCount;
    const int32 NodeCount = TargetNode + 1;
    int32& DynamicNavQueryCount = Stats.SynchronousNavQueries;
    TArray<FDynamicRouteEdge> SelectedEdges;
    TArray<FPathLinkLazyEdge> SearchEdges;
    TArray<int32> SelectedRouteEdgeIndices;
    const double CandidateStart = FPlatformTime::Seconds();

    auto AddDynamicEdge = [&](int32 From, int32 To, const FVector& A, const FVector& B,
        EDynamicRouteEdgeType Type, int32 TraversalIndex, double LinkCost, double Estimate, bool bDisconnected)
    {
        FDynamicRouteEdge Edge;
        Edge.From = From; Edge.To = To; Edge.Type = Type; Edge.TraversalIndex = TraversalIndex;
        SelectedEdges.Add(Edge);
        FPathLinkLazyEdge Search;
        Search.From = From; Search.To = To; Search.bExact = false;
        // Preserve the existing <=1cm zero-distance shortcut even at a polygon boundary.
        Search.bEnabled = !bDisconnected || A.Equals(B, 1.0f);
        // Native off-mesh shortcuts may be cheaper than Euclidean distance. Use zero unless ruled out.
        const double NormalBound = bEuclideanBoundsSafe && !A.Equals(B, 1.0f) ? FVector::Distance(A, B) : 0.0;
        Search.LowerBound = NormalBound + LinkCost;
        Search.Estimate = (FMath::IsFinite(Estimate) && Estimate >= 0 ? FMath::Max(NormalBound, Estimate) : NormalBound) + LinkCost;
        SearchEdges.Add(Search);
    };
    AddDynamicEdge(StartNode, TargetNode, StartNavLocation, TargetNavLocation,
        EDynamicRouteEdgeType::DirectToTarget, INDEX_NONE, 0, Estimates ? Estimates->DirectDistance : -1,
        Estimates && Estimates->bDirectDisconnected);
    for (int32 I = 0; I < TraversalCount; ++I)
    {
        const auto& T = StaticGraph.Traversals[I];
        if (!T.Link.IsValid()) continue;
        AddDynamicEdge(StartNode, FirstTraversalNode + I, StartNavLocation, T.EntryNavLocation,
            EDynamicRouteEdgeType::EnterTraversal, I, T.LinkCost,
            Estimates && Estimates->StartDistances.IsValidIndex(I) ? Estimates->StartDistances[I] : -1,
            Estimates && Estimates->StartDisconnected.IsValidIndex(I) && Estimates->StartDisconnected[I]);
        AddDynamicEdge(FirstTraversalNode + I, TargetNode, T.ExitNavLocation, TargetNavLocation,
            EDynamicRouteEdgeType::TraversalToTarget, I, 0,
            Estimates && Estimates->EndDistances.IsValidIndex(I) ? Estimates->EndDistances[I] : -1,
            Estimates && Estimates->EndDisconnected.IsValidIndex(I) && Estimates->EndDisconnected[I]);
    }
    for (int32 From = 0; From < TraversalCount; ++From)
        for (int32 To = 0; To < TraversalCount; ++To)
        {
            const auto* Transition = StaticGraph.GetTransition(From, To);
            if (!Transition || !Transition->Reachable || !StaticGraph.Traversals[From].Link.IsValid()
                || !StaticGraph.Traversals[To].Link.IsValid()) continue;
            FDynamicRouteEdge Edge;
            Edge.From = FirstTraversalNode + From; Edge.To = FirstTraversalNode + To;
            Edge.Type = EDynamicRouteEdgeType::EnterTraversal; Edge.TraversalIndex = To;
            Edge.NavDistance = Transition->NavDistance;
            Edge.Cost = Edge.NavDistance + StaticGraph.Traversals[To].LinkCost;
            SelectedEdges.Add(Edge);
            FPathLinkLazyEdge Search;
            Search.From = Edge.From; Search.To = Edge.To;
            Search.Cost = Search.LowerBound = Search.Estimate = Edge.Cost;
            SearchEdges.Add(Search);
        }
    Stats.CandidateMilliseconds += (FPlatformTime::Seconds() - CandidateStart) * 1000.0;
    auto Resolve = [&](int32 Index, double& Cost)
    {
        auto& Edge = SelectedEdges[Index];
        FVector A = StartNavLocation, B = TargetNavLocation;
        double LinkCost = 0;
        if (Edge.Type == EDynamicRouteEdgeType::EnterTraversal)
        {
            const auto& T = StaticGraph.Traversals[Edge.TraversalIndex];
            B = T.EntryNavLocation; LinkCost = T.LinkCost;
        }
        else if (Edge.Type == EDynamicRouteEdgeType::TraversalToTarget)
            A = StaticGraph.Traversals[Edge.TraversalIndex].ExitNavLocation;
        const bool bFound = BuildNavigationDistance(World, A, B, PathfindingContext,
            Edge.NavDistance, &DynamicNavQueryCount, &RequestPaths);
        Edge.Cost = Cost = Edge.NavDistance + LinkCost;
        return bFound;
    };
    const double SolveStart = FPlatformTime::Seconds();
    double NavQuerySeconds = 0;
    int32 ResolvedCount = 0;
    const bool bFound = PathLinkLazySearch::FindRoute(NodeCount, StartNode, TargetNode, SearchEdges,
        [&](int32 Index, double& Cost)
        {
            const double Begin = FPlatformTime::Seconds();
            const bool bReachable = Resolve(Index, Cost);
            NavQuerySeconds += FPlatformTime::Seconds() - Begin;
            return bReachable;
        }, SelectedRouteEdgeIndices, Stats.SolveCount, ResolvedCount);
    Stats.DijkstraMilliseconds += FMath::Max(0.0, FPlatformTime::Seconds() - SolveStart - NavQuerySeconds) * 1000.0;
    if (!bFound) return false;

    const double PointsStart = FPlatformTime::Seconds();
    ON_SCOPE_EXIT { if (bBuildSegments) Stats.PathPointsMilliseconds = (FPlatformTime::Seconds() - PointsStart) * 1000.0; };
    auto AddNormalSegment = [this, PathfindingContext, bBuildSegments, &OutResult, &DynamicNavQueryCount, &RequestPaths](
        const FVector& RawStart,
        const FVector& NavStart,
        const FVector& RawEnd,
        const FVector& NavEnd) -> bool
    {
        double ActualDistance = 0.0;
        TArray<FVector> PathPoints;

        if (!bBuildSegments)
        {
            if (!BuildNavigationDistance(World, NavStart, NavEnd, PathfindingContext,
                ActualDistance, &DynamicNavQueryCount, &RequestPaths)) return false;
            OutResult.TotalDistance += ActualDistance;
            return true;
        }

        if (!BuildNavigationPath(
            World,
            NavStart,
            NavEnd,
            PathfindingContext,
            ActualDistance,
            PathPoints,
            &DynamicNavQueryCount, &RequestPaths))
        {
            return false;
        }

        FPathLinkRouteSegment& Segment = OutResult.Segments.AddDefaulted_GetRef();
        Segment.SegmentType = EPathLinkSegmentType::Normal;
        Segment.StartLocation = RawStart;
        Segment.EndLocation = RawEnd;
        Segment.Distance = ActualDistance;
        Segment.PathPoints = MoveTemp(PathPoints);

        OutResult.TotalDistance += ActualDistance;
        return true;
    };

    auto AddLinkSegment = [&OutResult, bBuildSegments](const FPathLinkTraversalNode& Traversal)
    {
        if (!bBuildSegments) { OutResult.TotalDistance += Traversal.LinkCost; return; }
        FPathLinkRouteSegment& Segment = OutResult.Segments.AddDefaulted_GetRef();
        Segment.SegmentType = EPathLinkSegmentType::Link;
        Segment.StartLocation = Traversal.EntryLocation;
        Segment.EndLocation = Traversal.ExitLocation;
        Segment.Distance = Traversal.LinkCost;
        Segment.Link = Traversal.Link.Get();
        Segment.Reverse = Traversal.Reverse;
        Segment.LinkType = Traversal.LinkType;

        OutResult.TotalDistance += Traversal.LinkCost;

        if (APathLink* UsedLink = Traversal.Link.Get(); IsValid(UsedLink))
        {
            OutResult.UsedLinks.Add(UsedLink);
        }
    };

    OutResult.TotalDistance = 0.0;

    for (const int32 EdgeIndex : SelectedRouteEdgeIndices)
    {
        if (!SelectedEdges.IsValidIndex(EdgeIndex))
        {
            OutResult.Reset();
            return false;
        }

        const FDynamicRouteEdge& Edge = SelectedEdges[EdgeIndex];

        if (Edge.Type == EDynamicRouteEdgeType::DirectToTarget)
        {
            if (!AddNormalSegment(
                StartLocation,
                StartNavLocation,
                TargetLocation,
                TargetNavLocation))
            {
                OutResult.Reset();
                return false;
            }

            continue;
        }

        if (Edge.Type == EDynamicRouteEdgeType::TraversalToTarget)
        {
            const int32 SourceTraversalIndex = Edge.From - FirstTraversalNode;

            if (!StaticGraph.Traversals.IsValidIndex(SourceTraversalIndex))
            {
                OutResult.Reset();
                return false;
            }

            const FPathLinkTraversalNode& SourceTraversal = StaticGraph.Traversals[SourceTraversalIndex];

            if (!AddNormalSegment(
                SourceTraversal.ExitLocation,
                SourceTraversal.ExitNavLocation,
                TargetLocation,
                TargetNavLocation))
            {
                OutResult.Reset();
                return false;
            }

            continue;
        }

        if (!StaticGraph.Traversals.IsValidIndex(Edge.TraversalIndex))
        {
            OutResult.Reset();
            return false;
        }

        const FPathLinkTraversalNode& TargetTraversal = StaticGraph.Traversals[Edge.TraversalIndex];

        FVector RawNormalStart = StartLocation;
        FVector NavNormalStart = StartNavLocation;

        if (Edge.From != StartNode)
        {
            const int32 SourceTraversalIndex = Edge.From - FirstTraversalNode;

            if (!StaticGraph.Traversals.IsValidIndex(SourceTraversalIndex))
            {
                OutResult.Reset();
                return false;
            }

            const FPathLinkTraversalNode& SourceTraversal = StaticGraph.Traversals[SourceTraversalIndex];
            RawNormalStart = SourceTraversal.ExitLocation;
            NavNormalStart = SourceTraversal.ExitNavLocation;
        }

        if (!AddNormalSegment(
            RawNormalStart,
            NavNormalStart,
            TargetTraversal.EntryLocation,
            TargetTraversal.EntryNavLocation))
        {
            OutResult.Reset();
            return false;
        }

        AddLinkSegment(TargetTraversal);
    }

    OutResult.Success = true;

    UE_LOG(LogPathLinkRouteFinder, Verbose,
        TEXT("[PathLink][Route] Success | Traversals=%d | BakedPriority=%d | BoundsSafe=%d | ResolvedEdges=%d | Solves=%d | UsedLinks=%d | DynamicNavQueries=%d | Distance=%.2f | Time=%.3fms | Context=%s"),
        TraversalCount, Estimates != nullptr, bEuclideanBoundsSafe, ResolvedCount, Stats.SolveCount,
        OutResult.UsedLinks.Num(), DynamicNavQueryCount, OutResult.TotalDistance,
        (FPlatformTime::Seconds() - RouteStartSeconds) * 1000.0, *GetNameSafe(PathfindingContext));
    return true;
}

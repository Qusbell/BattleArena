#if WITH_DEV_AUTOMATION_TESTS
#include "AI/PathLink/Internal/PathLinkLazySearch.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathLinkLazySearchTest, "ShootingArena.PathLink.LazySearch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathLinkLazySearchTest::RunTest(const FString& Parameters)
{
    // Model two routes: an incumbent at 110 and a much better route whose estimate is 112.
    // The second route also has a zero-cost teleport and must not be pruned using its inflated estimate.
    TArray<FPathLinkLazyEdge> Edges;
    auto Add = [&](int32 From, int32 To, double Bound, double Estimate, bool bExact, double Cost)
    { FPathLinkLazyEdge E; E.From=From; E.To=To; E.LowerBound=Bound; E.Estimate=Estimate; E.bExact=bExact; E.Cost=Cost; Edges.Add(E); };
    Add(0,3,90,110,false,0);
    Add(0,1,35,54,false,0);
    Add(1,2,0,0,true,0);
    Add(2,3,28,58,false,0);
    const TArray<double> Actual={110,40,0,29};
    TArray<int32> Calls; Calls.Init(0,Edges.Num());
    TArray<int32> Route; int32 Solves=0,Resolved=0;
    const bool bFound=PathLinkLazySearch::FindRoute(4,0,3,Edges,[&](int32 I,double& Cost)
        { ++Calls[I]; Cost=Actual[I]; return true; },Route,Solves,Resolved);
    TestTrue(TEXT("Overestimated alternate route is checked"),bFound);
    TestTrue(TEXT("All alternate edges chosen"),Route==TArray<int32>({1,2,3}));
    for (int32 Count:Calls) TestTrue(TEXT("Each dynamic edge queried at most once"),Count<=1);

    // Missing bake: zero bounds remain correct; reject a misleading disconnected route.
    Edges.Reset();
    Add(0,3,0,100,false,0); Add(0,1,0,1,false,0); Add(1,3,0,1,false,0);
    Calls.Init(0,Edges.Num());
    TestTrue(TEXT("Unreachable candidate falls back to direct"),PathLinkLazySearch::FindRoute(4,0,3,Edges,
        [&](int32 I,double& Cost){ ++Calls[I]; Cost=100; return I!=1; },Route,Solves,Resolved));
    TestTrue(TEXT("Direct route retained"),Route==TArray<int32>({0}));
    TestEqual(TEXT("No query past rejected start edge"),Calls[2],0);

    // A far-away endpoint cannot improve an exact incumbent; its expensive query must be avoided.
    Edges.Reset();
    Add(0,3,10,10,true,10); Add(0,1,20,30,false,0); Add(1,3,20,30,false,0);
    TestTrue(TEXT("Bound-certified direct route"),PathLinkLazySearch::FindRoute(4,0,3,Edges,
        [](int32,double&){ return false; },Route,Solves,Resolved));
    TestEqual(TEXT("Pruned candidates require zero queries"),Resolved,0);

    // No path and one-way topology must not acquire an implicit reverse edge.
    Edges.Reset(); Add(1,0,5,5,true,5);
    TestFalse(TEXT("Directed unreachable target"),PathLinkLazySearch::FindRoute(2,0,1,Edges,
        [](int32,double&){ return false; },Route,Solves,Resolved));

    // Independent all-exact Floyd-Warshall reference, with deliberately unreliable estimates.
    FRandomStream Random(20261026);
    for (int32 Trial=0; Trial<200; ++Trial)
    {
        constexpr int32 N=6;
        const double Infinity=TNumericLimits<double>::Max()/4;
        double Reference[N][N];
        for (int32 A=0; A<N; ++A)
            for (int32 B=0; B<N; ++B) Reference[A][B]=A==B ? 0 : Infinity;
        Edges.Reset(); TArray<double> Costs; TArray<bool> Reachable;
        for (int32 A=0; A<N; ++A)
            for (int32 B=0; B<N; ++B)
            {
                if (A==B || Random.FRand()<.35f) continue;
                const double Cost=Random.RandRange(0,100);
                const bool bReachable=Random.FRand()>.2f;
                const bool bExact=bReachable && Random.FRand()<.25f;
                Add(A,B,Cost*Random.FRand(),Cost*(.1+5*Random.FRand()),bExact,Cost);
                Costs.Add(Cost); Reachable.Add(bReachable);
                if (bReachable) Reference[A][B]=Cost;
            }
        for (int32 K=0; K<N; ++K)
            for (int32 A=0; A<N; ++A)
                for (int32 B=0; B<N; ++B)
                    Reference[A][B]=FMath::Min(Reference[A][B],Reference[A][K]+Reference[K][B]);
        Calls.Init(0,Edges.Num());
        const bool bSuccess=PathLinkLazySearch::FindRoute(N,0,N-1,Edges,
            [&](int32 I,double& Cost){ ++Calls[I]; Cost=Costs[I]; return Reachable[I]; },Route,Solves,Resolved);
        TestEqual(TEXT("Random graph reachability matches exhaustive reference"),bSuccess,Reference[0][N-1]<Infinity);
        if (bSuccess)
        {
            double Total=0;
            for (int32 I:Route) Total+=Costs[I];
            TestEqual(TEXT("Random graph cost matches exhaustive reference"),Total,Reference[0][N-1]);
        }
        for (int32 Count:Calls) TestTrue(TEXT("Random graph edge queried once"),Count<=1);
    }
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "AI/PathLink/PathLinkRoutingCache.h"
#include "AI/PathLink/PathLink.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ScopeExit.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/ObjectReader.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPathLinkRoutingCacheTest, "ShootingArena.PathLink.BakedRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPathLinkRoutingCacheTest::RunTest(const FString& Parameters)
{
    IConsoleVariable* Strength = IConsoleManager::Get().FindConsoleVariable(TEXT("PathLink.EstimateBlendStrength"));
    IConsoleVariable* Range = IConsoleManager::Get().FindConsoleVariable(TEXT("PathLink.EstimateBlendRangeCm"));
    const float OldStrength = Strength->GetFloat(), OldRange = Range->GetFloat();
    ON_SCOPE_EXIT { Strength->SetWithCurrentPriority(OldStrength); Range->SetWithCurrentPriority(OldRange); };
    Strength->SetWithCurrentPriority(0.0f);
    const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("World"), World)) return false;
    UPathLinkRoutingCacheData* Cache = NewObject<UPathLinkRoutingCacheData>(World);
    auto SetPolys = [&](const TArray<FVector>& Centers, const TArray<int32>& IslandIDs)
    {
        Cache->Polygons.Reset(); Cache->Islands.Reset();
        for (int32 I = 0; I < Centers.Num(); ++I)
        {
            if (IslandIDs[I] >= Cache->Islands.Num()) Cache->Islands.SetNum(IslandIDs[I] + 1);
            FPathLinkBakedPoly& P = Cache->Polygons.AddDefaulted_GetRef();
            P.Center = Centers[I]; P.Island = IslandIDs[I]; P.LocalIndex = Cache->Islands[P.Island].PolygonCount++;
        }
        for (auto& Island : Cache->Islands) Island.Distances.Init(-1, Island.PolygonCount * Island.PolygonCount);
        for (auto& Island : Cache->Islands)
            for (int32 I = 0; I < Island.PolygonCount; ++I) Island.Distances[I*Island.PolygonCount+I] = 0;
    };
    auto Link = [&](int32 A, int32 B, double Cost, bool bReverse = false)
    {
        FPathLinkBakedTraversal& T = Cache->Traversals.AddDefaulted_GetRef();
        T.Link = World->SpawnActor<APathLink>(); T.EntryPoly = A; T.ExitPoly = B;
        T.Entry = T.EntryNav = Cache->Polygons[A].Center; T.Exit = T.ExitNav = Cache->Polygons[B].Center;
        T.LinkCost = Cost; T.Reverse = bReverse;
        return Cache->Traversals.Num() - 1;
    };
    double Distance = -1;
    SetPolys({FVector(0,0,0), FVector(100,0,0)}, {0,0});
    Cache->Islands[0].Distances = {0, 120, 160, 0};
    TestTrue(TEXT("Same island"), Cache->EstimateRoute(0, FVector::ZeroVector, 1, FVector(100,0,0), Distance));
    TestEqual(TEXT("Directed forward distance"), Distance, 120.0);
    Cache->EstimateRoute(1, FVector(100,0,0), 0, FVector::ZeroVector, Distance);
    TestEqual(TEXT("Asymmetric reverse distance"), Distance, 160.0);
    Cache->EstimateRoute(0, FVector(10,0,0), 0, FVector(37,0,0), Distance);
    TestEqual(TEXT("Arbitrary dynamic target in same polygon"), Distance, 27.0);
    TestEqual(TEXT("Different polygons include representative offsets"),
        Cache->EstimateNormalDistance(0, FVector(10,0,0), 1, FVector(130,0,0)), 160.0);
    Cache->Islands[0].Distances[1] = -1;
    TestFalse(TEXT("Unreachable entry in same weak component"), Cache->EstimateRoute(0, FVector::ZeroVector, 1, FVector(100,0,0), Distance));

    SetPolys({FVector(0,0,0), FVector(100,0,0), FVector(1000,0,0)}, {0,0,1});
    Cache->Islands[0].Distances = {0,100,100,0};
    Link(0,2,0); Link(1,2,0);
    Cache->TransitionDistances.Init(-1, 4);
    Cache->Traversals[0].ExitNav = FVector(1100,0,0);
    TestTrue(TEXT("Parallel links depend on current endpoints"), Cache->EstimateRoute(1, FVector(100,0,0), 2, FVector(1000,0,0), Distance));
    TestEqual(TEXT("Choose second link with better entry and exit"), Distance, 0.0);
    TestFalse(TEXT("One-way link cannot be reversed"), Cache->EstimateRoute(2, FVector(1000,0,0), 0, FVector::ZeroVector, Distance));
    Link(2,0,5,true); Cache->TransitionDistances.Init(-1, 9);
    Cache->EstimateRoute(2, FVector(1000,0,0), 0, FVector::ZeroVector, Distance);
    TestEqual(TEXT("Explicit reverse traversal"), Distance, 5.0);

    Cache->Traversals.Reset();
    SetPolys({FVector(0,0,0), FVector(1000,0,0), FVector(2000,0,0)}, {0,1,2});
    Link(0,1,0); Link(1,2,15); Cache->TransitionDistances.Init(-1, 4);
    Cache->TransitionDistances[1] = 0;
    Cache->EstimateRoute(0, FVector(0,0,0), 2, FVector(2037,0,0), Distance);
    TestEqual(TEXT("Multiple links, teleport zero cost, dynamic dropped item"), Distance, 52.0);
    TestFalse(TEXT("Disconnected islands cannot walk directly"), Cache->EstimateRoute(2, FVector(2000,0,0), 0, FVector::ZeroVector, Distance));

    Cache->Traversals.Reset();
    SetPolys({FVector(0,0,0), FVector(1000,0,0)}, {0,0});
    Cache->Islands[0].Distances = {0,1000,1000,0};
    Link(0,1,0); Cache->TransitionDistances = {-1};
    Cache->EstimateRoute(0, FVector::ZeroVector, 1, FVector(1000,0,0), Distance);
    TestEqual(TEXT("Same island can prefer a teleport"), Distance, 0.0);
    Cache->DataVersion = 1; Cache->NavigationSignature = TEXT("TestSignature");
    TestTrue(TEXT("Complete directed cache"), Cache->IsBaked());
    FPathLinkStaticGraph Graph; Cache->ExportStaticGraph(Graph);
    TestEqual(TEXT("Traversal cost survives graph export"), Graph.Traversals[0].LinkCost, 0.0);
    TestFalse(TEXT("Negative transition sentinel exports as unreachable"), Graph.Transitions[0].Reachable);
    TArray<uint8> Bytes;
    FObjectWriter Writer(Cache, Bytes);
    UPathLinkRoutingCacheData* Restored = NewObject<UPathLinkRoutingCacheData>(World);
    FObjectReader Reader(Restored, Bytes);
    TestTrue(TEXT("Cache properties survive serialization"), Restored->IsBaked());
    TestEqual(TEXT("Float32 matrix survives serialization"), Restored->Islands[0].Distances[1], 1000.0f);
    auto* Legacy = World->SpawnActor<APathLinkRoutingCache>();
    Legacy->DataVersion = 1; Legacy->NavigationSignature = TEXT("LegacySignature");
    Legacy->Polygons = Cache->Polygons; Legacy->Islands = Cache->Islands;
    Legacy->Traversals = Cache->Traversals; Legacy->TransitionDistances = Cache->TransitionDistances;
    auto* Migrated = NewObject<UPathLinkRoutingCacheData>(World);
    Legacy->CopyToData(Migrated);
    TestTrue(TEXT("Legacy actor migrates a complete bake"), Migrated->IsBaked());
    TestEqual(TEXT("Migration preserves float32 distances"), Migrated->Islands[0].Distances[1], 1000.0f);
    auto* Storage = NewObject<UPathLinkCacheStorageComponent>(World);
    TestNotNull(TEXT("Managed storage creates its payload without a placed actor"), Storage->Cache.Get());
    TestEqual(TEXT("Payload resolves world through its outer"), Cache->GetWorld(), World);
    Restored->DataVersion = 0;
    TestFalse(TEXT("Invalid bake rejected"), Restored->IsBaked());
    Cache->Traversals.Reset();
    SetPolys({FVector(0,0,0), FVector(100,0,0)}, {0,0});
    Cache->Islands[0].Distances = {0, 200, -1, 0};
    Strength->SetWithCurrentPriority(1.0f); Range->SetWithCurrentPriority(400.0f);
    TestEqual(TEXT("Blend uses baked polygon distance: weight=0.5"),
        Cache->EstimateNormalDistance(0, FVector::ZeroVector, 1, FVector(100,0,0)), 150.0);
    TestEqual(TEXT("Blend preserves directed unreachable sentinel"),
        Cache->EstimateNormalDistance(1, FVector(100,0,0), 0, FVector::ZeroVector), -1.0);
    Range->SetWithCurrentPriority(100.0f);
    TestEqual(TEXT("Beyond blend range is unchanged"),
        Cache->EstimateNormalDistance(0, FVector::ZeroVector, 1, FVector(100,0,0)), 200.0);
    Range->SetWithCurrentPriority(0.0f);
    TestEqual(TEXT("Zero range disables blend without division by zero"),
        Cache->EstimateNormalDistance(0, FVector::ZeroVector, 1, FVector(100,0,0)), 200.0);
    World->DestroyWorld(false);
    return true;
}
#endif

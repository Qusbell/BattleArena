#pragma once

#include "CoreMinimal.h"
#include "AI/PathLink/Internal/PathLinkNavSnapshot.h"
#include "AI/PathLink/PathLinkTypes.h"
#include "AI/PathLink/PathLinkRoutingCache.h"

class ARecastNavMesh;
class UPathLinkRoutingCacheData;
class UWorld;

/** A server-world job. A single navigation path query is the smallest time slice. */
class FPathLinkRuntimeCacheJob
{
public:
    bool Begin(UWorld* World, ARecastNavMesh* Nav, UPathLinkRoutingCacheData* Data,
        FPathLinkNavSnapshot&& Snapshot, FString& Error);
    bool Tick(double BudgetMilliseconds, FString& Error);
    bool IsComplete() const { return bComplete; }
    int32 GetSliceCount() const { return SliceCount; }
    double GetMaxSliceMilliseconds() const { return MaxSliceMilliseconds; }

private:
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<ARecastNavMesh> Nav;
    TWeakObjectPtr<UPathLinkRoutingCacheData> Data;
    FPathLinkNavSnapshot Snapshot;
    FString InitialLinkSignature;
    int32 Island = 0;
    int32 From = 0;
    int32 To = 0;
    bool bTransitions = false;
    bool bComplete = false;
    int32 SliceCount = 0;
    double MaxSliceMilliseconds = 0;
};

namespace PathLinkRuntimeCache
{
    struct FCapturedTraversal
    {
        FString LinkKey;
        bool Reverse = false;
        uint8 LinkType = 0;
        FVector Entry, Exit, EntryNav, ExitNav;
        double LinkCost = 0;
        int32 EntryPoly = INDEX_NONE, ExitPoly = INDEX_NONE;
    };
    /** Plain owned data: no UObjects are accessed by the background writer. */
    struct FCapturedCache
    {
        int32 DataVersion = 0;
        FString NavigationSignature, LinkSignature;
        bool bHasNativeOffMeshLinks = false;
        TArray<FPathLinkBakedPoly> Polygons;
        TArray<FPathLinkIslandInfo> Islands;
        TArray<FCapturedTraversal> Traversals;
        TArray<double> TransitionDistances;
    };
    FString FilePath(UWorld* World);
    bool CaptureSave(const UPathLinkRoutingCacheData& Data, FCapturedCache& Out, FString& Error);
    bool WriteCaptured(const FString& Path, FCapturedCache& Data, FString& Error);
    bool PrepareSave(UWorld* World, const UPathLinkRoutingCacheData& Data, TArray<uint8>& Bytes, FString& Error);
    bool WritePrepared(const FString& Path, const TArray<uint8>& Bytes, FString& Error);
    bool Save(UWorld* World, const UPathLinkRoutingCacheData& Data, FString& Error);
    bool Load(UWorld* World, ARecastNavMesh* Nav, const FPathLinkNavSnapshot& Snapshot,
        UPathLinkRoutingCacheData& Data, FString& Error);
}

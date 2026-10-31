#pragma once

#include "CoreMinimal.h"
#include "AI/PathLink/Internal/PathLinkNavSnapshot.h"
#include "AI/PathLink/PathLinkTypes.h"

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
    FString FilePath(UWorld* World);
    bool PrepareSave(UWorld* World, const UPathLinkRoutingCacheData& Data, TArray<uint8>& Bytes, FString& Error);
    bool WritePrepared(const FString& Path, const TArray<uint8>& Bytes, FString& Error);
    bool Save(UWorld* World, const UPathLinkRoutingCacheData& Data, FString& Error);
    bool Load(UWorld* World, ARecastNavMesh* Nav, const FPathLinkNavSnapshot& Snapshot,
        UPathLinkRoutingCacheData& Data, FString& Error);
}

#pragma once
#include "CoreMinimal.h"
#include "AI/Navigation/NavigationTypes.h"

class ARecastNavMesh;

struct FPathLinkNavSnapshot
{
    TArray<NavNodeRef> Refs;
    // Indexed by Detour tile slot; includes the runtime salt, unlike the geometry signature.
    TArray<NavNodeRef> TileRefs;
    TArray<FVector> Centers;
    TArray<TArray<int32>> Neighbors;
    TArray<TArray<int32>> Components;
    TArray<int32> Island;
    TArray<int32> LocalIndex;
    FString Signature;
    FSharedConstNavQueryFilter GroundFilter;
    bool bHasNativeOffMeshLinks = false;
};

namespace PathLinkBake
{
    constexpr int32 Version = 2;
    bool ReadNavigation(ARecastNavMesh* Nav, FPathLinkNavSnapshot& Out, FString& Error);
    bool HaveTileReferencesChanged(ARecastNavMesh* Nav, const TArray<NavNodeRef>& TileRefs);
}

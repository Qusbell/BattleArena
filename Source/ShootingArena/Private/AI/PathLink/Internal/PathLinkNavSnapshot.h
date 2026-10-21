#pragma once
#include "CoreMinimal.h"
#include "AI/Navigation/NavigationTypes.h"

class ARecastNavMesh;

struct FPathLinkNavSnapshot
{
    TArray<NavNodeRef> Refs;
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
    constexpr int32 Version = 1;
    bool ReadNavigation(ARecastNavMesh* Nav, FPathLinkNavSnapshot& Out, FString& Error);
}

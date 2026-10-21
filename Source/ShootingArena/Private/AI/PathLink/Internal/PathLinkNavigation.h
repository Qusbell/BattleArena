#pragma once

#include "CoreMinimal.h"
#include "AI/Navigation/NavigationTypes.h"

class ANavigationData;
class UWorld;

namespace PathLinkNavigation
{
    // Mirrors FindPathToLocationSynchronously's Context/NavData selection, without a UNavigationPath UObject.
    const ANavigationData* ResolveNavData(UWorld* World, AActor* Context, const FVector& Start);
    FNavPathSharedPtr FindPath(UWorld* World, const FVector& From, const FVector& To,
        AActor* Context, FSharedConstNavQueryFilter Filter = nullptr, bool bAllowPartial = true);
    bool CanShareDefaultGraph(UWorld* World, AActor* Context, bool bCheckOffMeshLinks = true);
}

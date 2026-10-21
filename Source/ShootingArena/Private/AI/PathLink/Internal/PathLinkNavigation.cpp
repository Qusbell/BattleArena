#include "AI/PathLink/Internal/PathLinkNavigation.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavMesh/RecastNavMesh.h"

const ANavigationData* PathLinkNavigation::ResolveNavData(UWorld* World, AActor* Context, const FVector& Start)
{
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World);
    if (!Nav) return nullptr;
    if (INavAgentInterface* Agent = Cast<INavAgentInterface>(Context))
        return Nav->GetNavDataForProps(Agent->GetNavAgentPropertiesRef(), Start);
    if (ANavigationData* Data = Cast<ANavigationData>(Context)) return Data;
    return Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
}

FNavPathSharedPtr PathLinkNavigation::FindPath(UWorld* World, const FVector& From, const FVector& To,
    AActor* Context, FSharedConstNavQueryFilter Filter, bool bAllowPartial)
{
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World);
    const ANavigationData* Data = ResolveNavData(World, Context, From);
    if (!Nav || !Data) return nullptr;
    if (!Filter) Filter = UNavigationQueryFilter::GetQueryFilter(*Data, Context, nullptr);
    FPathFindingQuery Query(Context, *Data, From, To, Filter);
    Query.SetAllowPartialPaths(bAllowPartial);
    const FPathFindingResult Result = Nav->FindPathSync(Query, EPathFindingMode::Regular);
    return Result.IsSuccessful() && Result.Path && Result.Path->IsValid() && !Result.Path->IsPartial()
        ? Result.Path : nullptr;
}

bool PathLinkNavigation::CanShareDefaultGraph(UWorld* World, AActor* Context, bool bCheckOffMeshLinks)
{
    UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World);
    // Multiple NavData may select different data depending on the query origin.
    if (!Nav || Nav->NavDataSet.Num() != 1) return false;
    const ANavigationData* Default = Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
    if (!Default || ResolveNavData(World, Context, FVector::ZeroVector) != Default) return false;
    FSharedConstNavQueryFilter Filter = UNavigationQueryFilter::GetQueryFilter(*Default, Context, nullptr);
    FSharedConstNavQueryFilter Base = UNavigationQueryFilter::GetQueryFilter(*Default, nullptr, nullptr);
    // A null FilterClass returns null; FPathFindingQuery then substitutes NavData's default filter.
    if (!Filter) Filter = Default->GetDefaultQueryFilter();
    if (!Base) Base = Default->GetDefaultQueryFilter();
    if (!Filter || !Base || !(*Filter == *Base) || Filter->GetMaxSearchNodes() != Base->GetMaxSearchNodes()) return false;
    if (!bCheckOffMeshLinks) return true;
    // Native off-mesh links can have per-owner permission callbacks even with equal filters.
    const ARecastNavMesh* Recast = Cast<ARecastNavMesh>(Default);
    if (!Recast) return false;
    TArray<FNavPoly> Links;
    for (int32 Tile = 0; Tile < Recast->GetNavMeshTilesCount(); ++Tile)
    {
        Links.Reset();
        Recast->GetNavLinksInTile(Tile, Links, false);
        if (!Links.IsEmpty()) return false;
    }
    return true;
}

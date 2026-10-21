#include "AI/PathLink/Internal/PathLinkNavSnapshot.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavMesh/RecastQueryFilter.h"
#include "Detour/DetourNavMesh.h"
#include "Misc/SecureHash.h"
#include "Misc/EngineVersion.h"

bool PathLinkBake::ReadNavigation(ARecastNavMesh* Nav, FPathLinkNavSnapshot& Out, FString& Error)
{
    Out = {};
#if WITH_RECAST
    const dtNavMesh* Mesh = Nav ? Nav->GetRecastMesh() : nullptr;
    if (!Mesh) { Error = TEXT("No built Recast NavMesh. Build navigation and load the full level first."); return false; }
    Out.GroundFilter = Nav->GetDefaultQueryFilter()->GetCopy();
    const_cast<FNavigationQueryFilter*>(Out.GroundFilter.Get())->SetExcludeFlags(
        Out.GroundFilter->GetExcludeFlags() | ARecastNavMesh::GetNavLinkFlag());
    const auto* Filter = static_cast<const FRecastQueryFilter*>(Out.GroundFilter->GetImplementation());

    TArray<const dtMeshTile*> Tiles;
    for (int32 Index = 0; Index < Mesh->getMaxTiles(); ++Index)
        if (const dtMeshTile* Tile = Mesh->getTile(Index); Tile && Tile->header) Tiles.Add(Tile);
    Tiles.Sort([](const dtMeshTile& A, const dtMeshTile& B)
    {
        if (A.header->x != B.header->x) return A.header->x < B.header->x;
        if (A.header->y != B.header->y) return A.header->y < B.header->y;
        return A.header->layer < B.header->layer;
    });

    FSHA1 Hash;
    auto AddBytes = [&Hash](const void* Data, uint32 Bytes) { if (Bytes) Hash.Update(static_cast<const uint8*>(Data), Bytes); };
    auto Add = [&AddBytes](const auto& Value) { AddBytes(&Value, sizeof(Value)); };
    Add(Version);
    const FString EngineVersion = FEngineVersion::Current().ToString();
    AddBytes(*EngineVersion, EngineVersion.Len() * sizeof(TCHAR));
    const FNavDataConfig& Config = Nav->GetConfig();
    Add(Config.AgentRadius); Add(Config.AgentHeight); Add(Config.AgentStepHeight);
    Add(Out.GroundFilter->GetIncludeFlags()); Add(Out.GroundFilter->GetExcludeFlags());
    Add(Out.GroundFilter->GetMaxSearchNodes()); Add(Out.GroundFilter->GetHeuristicScale());
    float Costs[RECAST_MAX_AREAS], Fixed[RECAST_MAX_AREAS];
    Out.GroundFilter->GetAllAreaCosts(Costs, Fixed, RECAST_MAX_AREAS);
    AddBytes(Costs, sizeof(Costs)); AddBytes(Fixed, sizeof(Fixed));

    TMap<NavNodeRef, int32> IndexByRef;
    for (const dtMeshTile* Tile : Tiles)
    {
        const dtMeshHeader& H = *Tile->header;
        Add(H.x); Add(H.y); Add(H.layer); Add(H.polyCount); Add(H.vertCount);
        AddBytes(Tile->verts, sizeof(dtReal) * H.vertCount * 3);
        Add(H.detailVertCount); Add(H.detailTriCount);
        Add(H.detailMeshCount);
        for (int32 D = 0; D < H.detailMeshCount; ++D)
        {
            Add(Tile->detailMeshes[D].vertBase); Add(Tile->detailMeshes[D].triBase);
            Add(Tile->detailMeshes[D].vertCount); Add(Tile->detailMeshes[D].triCount);
        }
        AddBytes(Tile->detailVerts, sizeof(dtReal) * H.detailVertCount * 3);
        AddBytes(Tile->detailTris, H.detailTriCount * 4);
        const dtPolyRef Base = Mesh->getPolyRefBase(Tile);
        for (int32 P = 0; P < H.polyCount; ++P)
        {
            const dtPoly& Poly = Tile->polys[P];
            Add(Poly.flags); Add(Poly.areaAndtype); Add(Poly.vertCount);
            AddBytes(Poly.verts, sizeof(Poly.verts[0]) * Poly.vertCount);
            if (Poly.getType() != DT_POLYTYPE_GROUND) { Out.bHasNativeOffMeshLinks = true; continue; }
            if (!Filter->passFilter(Base | P, Tile, &Poly)) continue;
            FVector Center;
            if (!Nav->GetPolyCenter(Base | P, Center)) continue;
            IndexByRef.Add(Base | P, Out.Refs.Num());
            Out.Refs.Add(Base | P);
            Out.Centers.Add(Center);
        }
    }
    if (Out.Refs.IsEmpty()) { Error = TEXT("NavMesh has no traversable ground polygons."); return false; }

    Out.Neighbors.SetNum(Out.Refs.Num());
    TArray<TArray<int32>> Undirected;
    Undirected.SetNum(Out.Refs.Num());
    for (int32 Index = 0; Index < Out.Refs.Num(); ++Index)
    {
        TArray<NavNodeRef> Refs;
        Nav->GetPolyNeighbors(Out.Refs[Index], Refs);
        for (NavNodeRef Ref : Refs)
            if (const int32* Other = IndexByRef.Find(Ref))
            {
                Out.Neighbors[Index].AddUnique(*Other);
                Undirected[Index].AddUnique(*Other);
                Undirected[*Other].AddUnique(Index);
            }
        Out.Neighbors[Index].Sort();
        Add(Out.Neighbors[Index].Num());
        for (int32 Other : Out.Neighbors[Index]) Add(Other);
    }
    // Weak connected components group ordinary polygons only. Directed reachability remains in each table.
    Out.Island.Init(INDEX_NONE, Out.Refs.Num());
    Out.LocalIndex.Init(INDEX_NONE, Out.Refs.Num());
    for (int32 Seed = 0; Seed < Out.Refs.Num(); ++Seed)
    {
        if (Out.Island[Seed] != INDEX_NONE) continue;
        const int32 Component = Out.Components.Num();
        TArray<int32>& Queue = Out.Components.AddDefaulted_GetRef();
        Queue.Add(Seed); Out.Island[Seed] = Component;
        for (int32 Head = 0; Head < Queue.Num(); ++Head)
            for (int32 Other : Undirected[Queue[Head]])
                if (Out.Island[Other] == INDEX_NONE) { Out.Island[Other] = Component; Queue.Add(Other); }
        Queue.Sort();
        for (int32 Local = 0; Local < Queue.Num(); ++Local) Out.LocalIndex[Queue[Local]] = Local;
    }
    Hash.Final();
    uint8 Digest[FSHA1::DigestSize];
    Hash.GetHash(Digest);
    Out.Signature = BytesToHex(Digest, sizeof(Digest));
    return true;
#else
    Error = TEXT("This target has no Recast support.");
    return false;
#endif
}

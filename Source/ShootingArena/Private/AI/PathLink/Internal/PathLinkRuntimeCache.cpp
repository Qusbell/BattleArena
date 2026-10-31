#include "AI/PathLink/Internal/PathLinkRuntimeCache.h"

#include "AI/PathLink/Internal/PathLinkNavigation.h"
#include "AI/PathLink/Internal/PathLinkRouteFinder.h"
#include "AI/PathLink/PathLink.h"
#include "AI/PathLink/PathLinkRoutingCache.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Misc/ScopeExit.h"
#include "NavigationPath.h"
#include "NavMesh/RecastNavMesh.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace
{
    FString LinkKey(const APathLink* Link)
    {
        const FVector A = Link->GetEntryLocation(), B = Link->GetExitLocation();
        return FString::Printf(TEXT("%.17g,%.17g,%.17g:%.17g,%.17g,%.17g:%d:%d:%d:%d"),
            A.X, A.Y, A.Z, B.X, B.Y, B.Z, int32(Link->GetLinkType()), Link->IsTwoWay(),
            Link->IsEnabled(), IsValid(Link->GetExitActor()));
    }

    FPathLinkBakedDepartureDirection DepartureDirection(const FNavigationPath& Path)
    {
        FVector Direction = FVector::ZeroVector;
        const auto& Points = Path.GetPathPoints();
        for (int32 I = 1; I < Points.Num(); ++I)
        {
            Direction = (Points[I].Location - Points[0].Location).GetSafeNormal();
            if (!Direction.IsNearlyZero()) break;
        }
        FPathLinkBakedDepartureDirection Result; Result.Set(Direction); return Result;
    }

    // Validate counts before allocating: a damaged cache must not request arbitrary memory.
    bool Count(FArchive& Ar, int32& N, int64 MinimumBytesPerItem)
    {
        Ar << N;
        if (Ar.IsLoading() && (N < 0 || int64(N) > (Ar.TotalSize() - Ar.Tell()) / MinimumBytesPerItem))
        { Ar.SetError(); return false; }
        return !Ar.IsError();
    }

    template<typename T> bool PlainArray(FArchive& Ar, TArray<T>& Values)
    {
        int32 N = Values.Num();
        if (!Count(Ar, N, sizeof(T))) return false;
        if (Ar.IsLoading()) Values.SetNumUninitialized(N);
        if (N) Ar.Serialize(Values.GetData(), int64(N) * sizeof(T));
        return !Ar.IsError();
    }

    bool Transfer(FArchive& Ar, UWorld* World, UPathLinkRoutingCacheData& Data)
    {
        int32 Format = 1;
        Ar << Format;
        if (Format != 1) return false;
        Ar << Data.DataVersion << Data.NavigationSignature << Data.LinkSignature << Data.bHasNativeOffMeshLinks;
        int32 N = Data.Polygons.Num();
        if (!Count(Ar, N, sizeof(double) * 3 + sizeof(int32) * 2)) return false;
        if (Ar.IsLoading()) Data.Polygons.SetNum(N);
        for (auto& P : Data.Polygons) Ar << P.Center << P.Island << P.LocalIndex;
        N = Data.Islands.Num();
        if (!Count(Ar, N, sizeof(int32) * 3)) return false;
        if (Ar.IsLoading()) Data.Islands.SetNum(N);
        for (auto& Island : Data.Islands)
        {
            Ar << Island.PolygonCount << Island.Float32TableMiB;
            if (!PlainArray(Ar, Island.Distances)) return false;
            int32 DirectionCount = Island.DepartureDirections.Num();
            if (!Count(Ar, DirectionCount, sizeof(uint32))) return false;
            if (Ar.IsLoading()) Island.DepartureDirections.SetNum(DirectionCount);
            for (auto& D : Island.DepartureDirections) Ar << D.Packed;
        }
        TMap<FString, APathLink*> Links;
        if (Ar.IsLoading())
            for (TActorIterator<APathLink> It(World); It; ++It)
            {
                const FString Key = LinkKey(*It);
                if (Links.Contains(Key)) return false;
                Links.Add(Key, *It);
            }
        N = Data.Traversals.Num();
        if (!Count(Ar, N, 1)) return false;
        if (Ar.IsLoading()) Data.Traversals.SetNum(N);
        for (auto& T : Data.Traversals)
        {
            FString Key = Ar.IsSaving() && IsValid(T.Link) ? LinkKey(T.Link) : FString();
            Ar << Key;
            if (Ar.IsLoading())
            {
                APathLink** Found = Links.Find(Key);
                if (!Found) return false;
                T.Link = *Found;
            }
            uint8 Type = uint8(T.LinkType);
            Ar << T.Reverse << Type << T.Entry << T.Exit << T.EntryNav << T.ExitNav
                << T.LinkCost << T.EntryPoly << T.ExitPoly;
            T.LinkType = EPathLinkType(Type);
        }
        return PlainArray(Ar, Data.TransitionDistances) && !Ar.IsError();
    }
}

bool FPathLinkRuntimeCacheJob::Begin(UWorld* InWorld, ARecastNavMesh* InNav,
    UPathLinkRoutingCacheData* InData, FPathLinkNavSnapshot&& InSnapshot, FString& Error)
{
    World = InWorld; Nav = InNav; Data = InData; Snapshot = MoveTemp(InSnapshot);
    if (!World.IsValid() || !Nav.IsValid() || !Data.IsValid()) { Error = TEXT("Missing world, navigation or cache."); return false; }
    InitialLinkSignature = UPathLinkRoutingCacheData::ComputeLinkSignature(World.Get());
    UPathLinkRoutingCacheData& Cache = *Data.Get();
    Cache.NavigationData = Nav.Get(); Cache.DataVersion = 0;
    Cache.Polygons.SetNum(Snapshot.Refs.Num()); Cache.Islands.SetNum(Snapshot.Components.Num());
    TMap<NavNodeRef, int32> IndexByRef;
    for (int32 I = 0; I < Snapshot.Refs.Num(); ++I)
    {
        IndexByRef.Add(Snapshot.Refs[I], I);
        Cache.Polygons[I].Center = Snapshot.Centers[I]; Cache.Polygons[I].Island = Snapshot.Island[I];
        Cache.Polygons[I].LocalIndex = Snapshot.LocalIndex[I];
    }
    for (int32 I = 0; I < Snapshot.Components.Num(); ++I)
    {
        const int64 Count = Snapshot.Components[I].Num();
        if (Count * Count >= MAX_int32) { Error = TEXT("An island exceeds the array index limit."); return false; }
        auto& Part = Cache.Islands[I]; Part.PolygonCount = int32(Count);
        Part.Float32TableMiB = double(Count * Count * sizeof(float)) / (1024.0 * 1024.0);
        Part.Distances.Init(-1.0f, int32(Count * Count));
        Part.DepartureDirections.SetNumZeroed(int32(Count * Count));
    }
    TArray<APathLink*> Links;
    for (TActorIterator<APathLink> It(World.Get()); It; ++It)
    {
        FString Duplicate;
        if (It->HasDuplicatePlacementError(Duplicate)) { Error = TEXT("Duplicate PathLink: ") + Duplicate; return false; }
        if (It->IsEnabled() && It->IsValidLinkForRouteCache()) Links.Add(*It);
    }
    FPathLinkStaticGraph Graph;
    if (!FPathLinkRouteFinder(World.Get()).BuildStaticGraph(Links, Nav.Get(), Graph, false))
    { Error = TEXT("Failed to collect PathLink traversals."); return false; }
    Cache.Traversals.Reset();
    for (const FPathLinkTraversalNode& Node : Graph.Traversals)
    {
        auto& T = Cache.Traversals.AddDefaulted_GetRef();
        T.Link = Node.Link.Get(); T.Reverse = Node.Reverse; T.LinkType = Node.LinkType;
        T.Entry = Node.EntryLocation; T.Exit = Node.ExitLocation;
        T.EntryNav = Node.EntryNavLocation; T.ExitNav = Node.ExitNavLocation; T.LinkCost = Node.LinkCost;
        FNavLocation Entry, Exit;
        if (!Nav->ProjectPoint(T.EntryNav, Entry, FVector(150,150,400), Snapshot.GroundFilter)
            || !Nav->ProjectPoint(T.ExitNav, Exit, FVector(150,150,400), Snapshot.GroundFilter)
            || !IndexByRef.Contains(Entry.NodeRef) || !IndexByRef.Contains(Exit.NodeRef))
        { Error = TEXT("A PathLink endpoint is outside the traversable ground polygons."); return false; }
        T.EntryPoly = IndexByRef[Entry.NodeRef]; T.ExitPoly = IndexByRef[Exit.NodeRef];
    }
    const int64 TraversalCount = Cache.Traversals.Num();
    if (TraversalCount * TraversalCount >= MAX_int32) { Error = TEXT("Traversal table exceeds the array index limit."); return false; }
    Cache.TransitionDistances.Init(-1, int32(TraversalCount * TraversalCount));
    Cache.bHasNativeOffMeshLinks = Snapshot.bHasNativeOffMeshLinks;
    return true;
}

bool FPathLinkRuntimeCacheJob::Tick(double BudgetMilliseconds, FString& Error)
{
    if (!World.IsValid() || !Nav.IsValid() || !Data.IsValid()) { Error = TEXT("World or navigation was destroyed during cache generation."); return false; }
    const double SliceStart = FPlatformTime::Seconds();
    ++SliceCount;
    ON_SCOPE_EXIT { MaxSliceMilliseconds = FMath::Max(MaxSliceMilliseconds, (FPlatformTime::Seconds() - SliceStart) * 1000.0); };
    UPathLinkRoutingCacheData& Cache = *Data.Get();
    const double Deadline = FPlatformTime::Seconds() + FMath::Max(0.01, BudgetMilliseconds) / 1000.0;
    do
    {
        if (!bTransitions)
        {
            if (!Snapshot.Components.IsValidIndex(Island)) { bTransitions = true; From = To = 0; continue; }
            const auto& Part = Snapshot.Components[Island]; const int32 N = Part.Num();
            if (From >= N) { ++Island; From = To = 0; continue; }
            auto& Table = Cache.Islands[Island]; const int32 Index = From * N + To;
            if (From == To) Table.Distances[Index] = 0;
            else if (const FNavPathSharedPtr Path = PathLinkNavigation::FindPath(World.Get(), Snapshot.Centers[Part[From]],
                Snapshot.Centers[Part[To]], Nav.Get(), Snapshot.GroundFilter, false))
            { Table.Distances[Index] = float(Path->GetLength()); Table.DepartureDirections[Index] = DepartureDirection(*Path); }
            if (++To == N) { To = 0; ++From; }
        }
        else
        {
            const int32 N = Cache.Traversals.Num();
            if (From >= N)
            {
                FPathLinkNavSnapshot Verify; FString VerifyError;
                if (!PathLinkBake::ReadNavigation(Nav.Get(), Verify, VerifyError)
                    || Verify.Signature != Snapshot.Signature
                    || UPathLinkRoutingCacheData::ComputeLinkSignature(World.Get()) != InitialLinkSignature)
                { Error = TEXT("Navigation or PathLink placement changed during runtime cache generation."); return false; }
                Cache.NavigationSignature = Snapshot.Signature;
                Cache.LinkSignature = InitialLinkSignature;
                Cache.DataVersion = PathLinkBake::Version;
                if (!Cache.IsBaked()) { Error = TEXT("Generated cache failed structural validation."); return false; }
                bComplete = true; return true;
            }
            const auto& A = Cache.Traversals[From]; const auto& B = Cache.Traversals[To];
            if (From != To && A.Link != B.Link)
            {
                if (A.ExitNav.Equals(B.EntryNav, 1.0)) Cache.TransitionDistances[From*N+To] = 0;
                else if (const FNavPathSharedPtr Path = PathLinkNavigation::FindPath(World.Get(), A.ExitNav, B.EntryNav,
                    Nav.Get(), Snapshot.GroundFilter, false)) Cache.TransitionDistances[From*N+To] = Path->GetLength();
            }
            if (++To == N) { To = 0; ++From; }
        }
    } while (FPlatformTime::Seconds() < Deadline);
    return true;
}

FString PathLinkRuntimeCache::FilePath(UWorld* World)
{
    const FString Map = World ? UWorld::RemovePIEPrefix(World->GetOutermost()->GetName()) : FString();
    FTCHARToUTF8 Utf8(*Map); uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Digest);
    const TCHAR* Kind = World && World->WorldType == EWorldType::PIE ? TEXT("PIE") : TEXT("Game");
    return FPaths::ProjectSavedDir() / TEXT("PathLinkCache") / Kind / (BytesToHex(Digest, sizeof(Digest)) + TEXT(".bin"));
}

bool PathLinkRuntimeCache::Save(UWorld* World, const UPathLinkRoutingCacheData& Data, FString& Error)
{
    TArray<uint8> Bytes; FMemoryWriter Writer(Bytes, true);
    if (!Transfer(Writer, World, const_cast<UPathLinkRoutingCacheData&>(Data))) { Error = TEXT("Could not serialize runtime cache."); return false; }
    Writer.Close(); uint8 Digest[FSHA1::DigestSize]; FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest);
    Bytes.Append(Digest, sizeof(Digest));
    const FString Path = FilePath(World), Temp = Path + TEXT(".tmp");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
    if (!FFileHelper::SaveArrayToFile(Bytes, *Temp) || !IFileManager::Get().Move(*Path, *Temp, true, true))
    { Error = FString::Printf(TEXT("Could not save runtime cache: %s"), *Path); return false; }
    return true;
}

bool PathLinkRuntimeCache::Load(UWorld* World, ARecastNavMesh* Nav, const FPathLinkNavSnapshot& Snapshot,
    UPathLinkRoutingCacheData& Data, FString& Error)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *FilePath(World))) { Error = TEXT("No saved runtime cache."); return false; }
    if (Bytes.Num() <= FSHA1::DigestSize) { Error = TEXT("Runtime cache file is incomplete."); return false; }
    const int32 PayloadSize = Bytes.Num() - FSHA1::DigestSize; uint8 Digest[FSHA1::DigestSize];
    FSHA1::HashBuffer(Bytes.GetData(), PayloadSize, Digest);
    if (FMemory::Memcmp(Digest, Bytes.GetData() + PayloadSize, sizeof(Digest)) != 0)
    { Error = TEXT("Runtime cache checksum mismatch."); return false; }
    Bytes.SetNum(PayloadSize); FMemoryReader Reader(Bytes, true);
    if (!Transfer(Reader, World, Data) || Reader.Tell() != Reader.TotalSize() || !Data.IsBaked()
        || Data.NavigationSignature != Snapshot.Signature || Data.Polygons.Num() != Snapshot.Refs.Num()
        || Data.LinkSignature != UPathLinkRoutingCacheData::ComputeLinkSignature(World))
    { Error = TEXT("Runtime cache is invalid or does not match the current world."); return false; }
    Data.NavigationData = Nav; return true;
}

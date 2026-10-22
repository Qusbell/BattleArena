#include "AI/PathLink/PathLinkRoutingCache.h"
#include "AI/PathLink/PathLink.h"
#include "AI/PathLink/PathLinkSubsystem.h"
#include "AI/PathLink/Internal/PathLinkNavSnapshot.h"
#include "AI/PathLink/Internal/PathLinkNavigation.h"
#include "AI/PathLink/Internal/PathLinkRouteFinder.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
#include "NavMesh/RecastNavMesh.h"
#include "NavigationSystem.h"
#include "Misc/Crc.h"
#include "HAL/IConsoleManager.h"
#include "Misc/SecureHash.h"
#include "Misc/ScopeExit.h"
#if WITH_EDITOR
#include "Misc/ScopedSlowTask.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogPathLinkBake, Log, All);

static TAutoConsoleVariable<float> CVarPathLinkDirectionStrength(TEXT("PathLink.EstimateDirectionStrength"), 1.0f,
    TEXT("[0,1] center-offset correction using baked departure tangents. 0 restores the legacy representative estimate."));
static TAutoConsoleVariable<float> CVarPathLinkBlendStrength(TEXT("PathLink.EstimateBlendStrength"), 0.0f,
    TEXT("Straight-distance blend strength [0,1], for approximate normal movement only. 0 restores center estimates."));
static TAutoConsoleVariable<float> CVarPathLinkBlendRange(TEXT("PathLink.EstimateBlendRangeCm"), 4000.0f,
    TEXT("Center-to-center baked path distance at which straight-distance blending reaches zero, in cm."));


namespace
{
    // Three signed components quantized to 8 bits plus an explicit valid bit; 4 bytes per 3D tangent.
    uint32 PackDirection(const FVector& Value)
    {
        if (Value.ContainsNaN()) return 0;
        const FVector Unit=Value.GetSafeNormal();
        if (Unit.IsNearlyZero()) return 0;
        auto Pack=[](double Axis) { return uint32(FMath::Clamp(FMath::RoundToInt(Axis*127.0),-127,127)+127); };
        return 0x80000000u | Pack(Unit.X) | (Pack(Unit.Y)<<8) | (Pack(Unit.Z)<<16);
    }
    FVector UnpackDirection(uint32 Value)
    {
        if (!(Value&0x80000000u)) return FVector::ZeroVector;
        return FVector(int32(Value&255)-127,int32((Value>>8)&255)-127,int32((Value>>16)&255)-127).GetSafeNormal();
    }
    FPathLinkBakedDepartureDirection PathDirection(const FNavigationPath& Path)
    {
        const auto& Points=Path.GetPathPoints();
        FVector Departure=FVector::ZeroVector;
        for (int32 I=1; I<Points.Num(); ++I)
        {
            Departure=(Points[I].Location-Points[0].Location).GetSafeNormal();
            if (!Departure.IsNearlyZero()) break;
        }
        FPathLinkBakedDepartureDirection Result; Result.Set(Departure); return Result;
    }
}

void FPathLinkBakedDepartureDirection::Set(const FVector& Direction)
{ Packed=PackDirection(Direction); }
bool FPathLinkBakedDepartureDirection::Get(FVector& OutDirection) const
{
    OutDirection=UnpackDirection(Packed);
    return !OutDirection.IsNearlyZero();
}
static_assert(sizeof(FPathLinkBakedDepartureDirection)==4,"Baked departure must occupy 4 bytes.");

bool UPathLinkRoutingCacheData::GetNormalDepartureDirection(int32 FromPoly,int32 ToPoly,FVector& OutDirection) const
{
    OutDirection=FVector::ZeroVector;
    if (!Polygons.IsValidIndex(FromPoly) || !Polygons.IsValidIndex(ToPoly) || FromPoly==ToPoly) return false;
    const auto& A=Polygons[FromPoly]; const auto& B=Polygons[ToPoly];
    if (A.Island!=B.Island || !Islands.IsValidIndex(A.Island)) return false;
    const auto& Island=Islands[A.Island];
    const int64 Index=int64(A.LocalIndex)*Island.PolygonCount+B.LocalIndex;
    if (!Island.Distances.IsValidIndex(Index) || Island.Distances[Index]<0 || !Island.DepartureDirections.IsValidIndex(Index)) return false;
    return Island.DepartureDirections[Index].Get(OutDirection);
}

FString UPathLinkRoutingCacheData::ComputeLinkSignature(UWorld* World)
{
    // Object names are intentionally omitted: PIE changes package prefixes, and cook remaps actor references.
    TArray<FString> Rows;
    for (TActorIterator<APathLink> It(World); It; ++It)
    {
        const APathLink* Link = *It;
        const FVector A = Link->GetEntryLocation(), B = Link->GetExitLocation();
        Rows.Add(FString::Printf(TEXT("%.17g,%.17g,%.17g:%.17g,%.17g,%.17g:%d:%d:%d:%d"),
            A.X, A.Y, A.Z, B.X, B.Y, B.Z, int32(Link->GetLinkType()), Link->IsTwoWay(),
            Link->IsEnabled(), IsValid(Link->GetExitActor())));
    }
    Rows.Sort();
    const FString Joined = FString::Join(Rows, TEXT("\n"));
    FTCHARToUTF8 Bytes(*Joined);
    FSHA1 Hash; Hash.Update(reinterpret_cast<const uint8*>(Bytes.Get()), Bytes.Length()); Hash.Final();
    uint8 Digest[FSHA1::DigestSize]; Hash.GetHash(Digest);
    return BytesToHex(Digest, sizeof(Digest));
}

bool UPathLinkRoutingCacheData::IsBaked() const
{
    if (DataVersion != PathLinkBake::Version || Polygons.IsEmpty() || Islands.IsEmpty() || NavigationSignature.IsEmpty()) return false;
    for (const FPathLinkIslandInfo& Island : Islands)
        if (int64(Island.PolygonCount) * Island.PolygonCount != Island.Distances.Num()
            || Island.DepartureDirections.Num()!=Island.Distances.Num()) return false;
    for (const FPathLinkBakedPoly& Poly : Polygons)
        if (!Islands.IsValidIndex(Poly.Island) || Poly.LocalIndex < 0
            || Poly.LocalIndex >= Islands[Poly.Island].PolygonCount || Poly.Center.ContainsNaN()) return false;
    return int64(Traversals.Num()) * Traversals.Num() == TransitionDistances.Num();
}

double UPathLinkRoutingCacheData::EstimateNormalDistance(int32 FromPoly, const FVector& From,
    int32 ToPoly, const FVector& To) const
{
    if (!Polygons.IsValidIndex(FromPoly) || !Polygons.IsValidIndex(ToPoly)) return -1;
    if (FromPoly == ToPoly) return FVector::Distance(From, To);
    const FPathLinkBakedPoly& A = Polygons[FromPoly];
    const FPathLinkBakedPoly& B = Polygons[ToPoly];
    if (A.Island != B.Island || !Islands.IsValidIndex(A.Island)) return -1;
    const FPathLinkIslandInfo& Island = Islands[A.Island];
    const int64 Index = int64(A.LocalIndex) * Island.PolygonCount + B.LocalIndex;
    if (!Island.Distances.IsValidIndex(Index) || Island.Distances[Index] < 0) return -1;
    // Representative-center detour, in cm. This is deliberately an estimate, not TotalDistance's exact path.
    const FVector StartOffset=From-A.Center, TargetOffset=To-B.Center;
    const double Legacy=StartOffset.Size()+Island.Distances[Index]+TargetOffset.Size();
    double Estimate=Legacy;
    const double DirectionStrength=CVarPathLinkDirectionStrength.GetValueOnGameThread();
    if (FMath::IsFinite(DirectionStrength) && DirectionStrength>0)
    {
        FVector Departure;
        double StartCost=StartOffset.Size(), TargetCost=TargetOffset.Size();
        if (Island.DepartureDirections.IsValidIndex(Index) && Island.DepartureDirections[Index].Get(Departure))
            StartCost=-FVector::DotProduct(StartOffset,Departure);
        const int64 ReverseIndex=int64(B.LocalIndex)*Island.PolygonCount+A.LocalIndex;
        // Reverse departure is only an approximate arrival tangent; reverse corridors can differ.
        if (Island.Distances.IsValidIndex(ReverseIndex) && Island.Distances[ReverseIndex]>=0
            && Island.DepartureDirections.IsValidIndex(ReverseIndex) && Island.DepartureDirections[ReverseIndex].Get(Departure))
            TargetCost=-FVector::DotProduct(TargetOffset,Departure);
        const double Projected=FMath::Max(FVector::Distance(From,To),Island.Distances[Index]+StartCost+TargetCost);
        Estimate=FMath::Lerp(Legacy,Projected,FMath::Clamp(DirectionStrength,0.0,1.0));
    }
    const double Strength = CVarPathLinkBlendStrength.GetValueOnGameThread();
    const double Range = CVarPathLinkBlendRange.GetValueOnGameThread();
    if (!FMath::IsFinite(Strength) || !FMath::IsFinite(Range) || Strength <= 0 || Range <= 0) return Estimate;
    // Gate by the baked P1->P2 distance, NOT the dynamic A->B straight distance.
    const double Weight = FMath::Clamp(Strength, 0.0, 1.0) * FMath::Clamp(1.0 - Island.Distances[Index] / Range, 0.0, 1.0);
    return FMath::Lerp(Estimate, FVector::Distance(From, To), Weight);
}

bool UPathLinkRoutingCacheData::EstimateRoute(int32 StartPoly, const FVector& Start,
    int32 TargetPoly, const FVector& Target, double& OutDistance) const
{
    OutDistance = 0;
    const int32 Count = Traversals.Num();
    const double Infinity = TNumericLimits<double>::Max();
    TArray<double, TInlineAllocator<64>> Distances;
    TArray<bool, TInlineAllocator<64>> Visited;
    Distances.Init(Infinity, Count); Visited.Init(false, Count);
    double Best = EstimateNormalDistance(StartPoly, Start, TargetPoly, Target);
    if (Best < 0) Best = Infinity;
    for (int32 I = 0; I < Count; ++I)
    {
        const FPathLinkBakedTraversal& T = Traversals[I];
        const double Distance = EstimateNormalDistance(StartPoly, Start, T.EntryPoly, T.EntryNav);
        if (IsValid(T.Link) && Distance >= 0) Distances[I] = Distance + T.LinkCost;
    }
    for (int32 Pass = 0; Pass < Count; ++Pass)
    {
        int32 Current = INDEX_NONE;
        double Minimum = Best;
        for (int32 I = 0; I < Count; ++I)
            if (!Visited[I] && Distances[I] < Minimum) { Minimum = Distances[I]; Current = I; }
        if (Current == INDEX_NONE) break;
        Visited[Current] = true;
        const FPathLinkBakedTraversal& From = Traversals[Current];
        const double EndDistance = EstimateNormalDistance(From.ExitPoly, From.ExitNav, TargetPoly, Target);
        if (EndDistance >= 0) Best = FMath::Min(Best, Minimum + EndDistance);
        for (int32 To = 0; To < Count; ++To)
        {
            const double Transition = TransitionDistances[Current * Count + To];
            if (!Visited[To] && Transition >= 0 && IsValid(Traversals[To].Link))
                Distances[To] = FMath::Min(Distances[To], Minimum + Transition + Traversals[To].LinkCost);
        }
    }
    if (Best == Infinity) return false;
    OutDistance = Best;
    return true;
}

void UPathLinkRoutingCacheData::ExportStaticGraph(FPathLinkStaticGraph& Out) const
{
    Out.Reset(); Out.Traversals.Reserve(Traversals.Num());
    for (const FPathLinkBakedTraversal& T : Traversals)
    {
        FPathLinkTraversalNode& N = Out.Traversals.AddDefaulted_GetRef();
        N.Link = T.Link; N.Reverse = T.Reverse; N.LinkType = T.LinkType;
        N.EntryLocation = T.Entry; N.ExitLocation = T.Exit;
        N.EntryNavLocation = T.EntryNav; N.ExitNavLocation = T.ExitNav; N.LinkCost = T.LinkCost;
    }
    Out.Transitions.SetNum(TransitionDistances.Num());
    for (int32 I = 0; I < TransitionDistances.Num(); ++I)
    {
        Out.Transitions[I].Reachable = TransitionDistances[I] >= 0;
        Out.Transitions[I].NavDistance = FMath::Max(0.0, TransitionDistances[I]);
    }
}

SIZE_T UPathLinkRoutingCacheData::GetCacheAllocatedSize() const
{
    SIZE_T Bytes = Polygons.GetAllocatedSize() + Traversals.GetAllocatedSize() + TransitionDistances.GetAllocatedSize()
        + Islands.GetAllocatedSize() + NavigationSignature.GetAllocatedSize() + LinkSignature.GetAllocatedSize();
    for (const FPathLinkIslandInfo& Island : Islands) Bytes += Island.Distances.GetAllocatedSize()+Island.DepartureDirections.GetAllocatedSize();
    return Bytes;
}

void UPathLinkRoutingCacheData::AnalyzeNavigation()
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    if (!World || World->IsGameWorld()) return;
    Modify();
    if (UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
    { Diagnostic = TEXT("Navigation is building or locked. Wait for completion before analysis/bake."); return; }
    if (!NavigationData)
        if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent(World))
            NavigationData = Cast<ARecastNavMesh>(Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate));
    FPathLinkNavSnapshot Snapshot;
    if (!PathLinkBake::ReadNavigation(NavigationData, Snapshot, Diagnostic)) return;
    const bool bKeepTables = IsBaked() && NavigationSignature == Snapshot.Signature
        && LinkSignature == ComputeLinkSignature(World);
    if (!bKeepTables)
    {
        if (auto* Subsystem = World->GetSubsystem<UPathLinkSubsystem>()) Subsystem->NotifyRoutingCacheChanged();
        DataVersion = 0; Islands.Reset(); Polygons.Reset(); Traversals.Reset(); TransitionDistances.Reset();
    }
    TotalPolygons = Snapshot.Refs.Num(); LargestIslandPolygons = 0; ExpectedTableMiB = 0;
    int64 QueryCount = 0;
    if (!bKeepTables) Islands.SetNum(Snapshot.Components.Num());
    for (int32 I = 0; I < Snapshot.Components.Num(); ++I)
    {
        const int64 N = Snapshot.Components[I].Num();
        Islands[I].PolygonCount = N;
        Islands[I].Float32TableMiB = double(N * N * sizeof(float)) / (1024.0 * 1024.0);
        LargestIslandPolygons = FMath::Max(LargestIslandPolygons, int32(N));
        ExpectedTableMiB += Islands[I].Float32TableMiB;
        QueryCount += N * (N - 1);
    }
    UE_LOG(LogPathLinkBake, Display, TEXT("All-pairs tables: distances=%.3fMiB departure-directions=%.3fMiB combined=%.3fMiB (4+4 bytes/pair)."),
        ExpectedTableMiB, ExpectedTableMiB, ExpectedTableMiB*2);
    const double SampleStart = FPlatformTime::Seconds();
    int32 Samples = 0;
    for (const TArray<int32>& Component : Snapshot.Components)
    {
        if (Component.Num() < 2) continue;
        for (int32 I = 0; I < 32 && Samples < 32; ++I)
        {
            const int32 A = Component[(int64(I) * Component.Num()) / 32];
            const int32 B = Component[((int64(I) * Component.Num()) / 32 + Component.Num() / 2) % Component.Num()];
            PathLinkNavigation::FindPath(World, Snapshot.Centers[A], Snapshot.Centers[B], NavigationData, Snapshot.GroundFilter, false);
            ++Samples;
        }
    }
    EstimatedBakeSeconds = Samples ? (FPlatformTime::Seconds() - SampleStart) / Samples * QueryCount : 0;
    Diagnostic = FString::Printf(TEXT("Polygons=%d Islands=%d Largest=%d Float32=%.3fMiB Estimated=%.2fs (%d samples). Load all navigation tiles/links before baking. Island IDs are ordinary ground-poly components, not tiles or AI Areas."),
        TotalPolygons, Islands.Num(), LargestIslandPolygons, ExpectedTableMiB, EstimatedBakeSeconds, Samples);
    UE_LOG(LogPathLinkBake, Display, TEXT("%s"), *Diagnostic);
    for (int32 I = 0; I < Islands.Num(); ++I)
        UE_LOG(LogPathLinkBake, Display, TEXT("Island %d: Polygons=%d DirectedFloat32=%.4fMiB"), I, Islands[I].PolygonCount, Islands[I].Float32TableMiB);
    MarkPackageDirty();
#endif
}

void UPathLinkRoutingCacheData::ClearRoutingCache()
{
#if WITH_EDITOR
    if (!GetWorld() || GetWorld()->IsGameWorld()) return;
    Modify(); DataVersion = 0; Polygons.Reset(); Islands.Reset(); Traversals.Reset(); TransitionDistances.Reset();
    if (auto* Subsystem = GetWorld()->GetSubsystem<UPathLinkSubsystem>()) Subsystem->NotifyRoutingCacheChanged();
    NavigationSignature.Reset(); LinkSignature.Reset(); TotalPolygons = LargestIslandPolygons = 0;
    Diagnostic = TEXT("Cache cleared."); MarkPackageDirty();
#endif
}

void UPathLinkRoutingCacheData::BakeRoutingCache()
{
#if WITH_EDITOR
    UWorld* World = GetWorld();
    if (!World || World->IsGameWorld()) return;
    AnalyzeNavigation();
    if (!NavigationData || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World)) return;
    FPathLinkNavSnapshot Snapshot;
    if (!PathLinkBake::ReadNavigation(NavigationData, Snapshot, Diagnostic)) return;
    DataVersion = 0; // Failed/cancelled bakes are never usable.
    if (auto* Subsystem = World->GetSubsystem<UPathLinkSubsystem>()) Subsystem->NotifyRoutingCacheChanged();
    const double Begin = FPlatformTime::Seconds();
    ON_SCOPE_EXIT { ActualBakeSeconds = FPlatformTime::Seconds() - Begin;
        MarkPackageDirty(); UE_LOG(LogPathLinkBake, Display, TEXT("%s Actual=%.3fs"), *Diagnostic, ActualBakeSeconds); };

    TArray<APathLink*> Links;
    for (TActorIterator<APathLink> It(World); It; ++It)
    {
        FString Duplicate;
        if (It->HasDuplicatePlacementError(Duplicate)) { Diagnostic = TEXT("Duplicate PathLink: ") + Duplicate; return; }
        if (It->IsEnabled() && It->IsValidLinkForRouteCache()) Links.Add(*It);
    }
    const int64 MaxTraversals = int64(Links.Num()) * 2;
    const double EstimatedMiB = ExpectedTableMiB * (1.0+double(sizeof(FPathLinkBakedDepartureDirection))/sizeof(float)) + double(MaxTraversals * MaxTraversals * sizeof(double)
        + int64(Snapshot.Refs.Num()) * sizeof(FPathLinkBakedPoly)) / (1024.0 * 1024.0);
    if (EstimatedMiB > MemoryBudgetMiB || EstimatedBakeSeconds > TimeBudgetSeconds)
    { Diagnostic = FString::Printf(TEXT("Bake refused: estimated %.3fMiB / %.1fs exceeds budget %.3fMiB / %.1fs. All-Pairs was not allocated."),
        EstimatedMiB, EstimatedBakeSeconds, MemoryBudgetMiB, TimeBudgetSeconds); return; }
    for (const TArray<int32>& Component : Snapshot.Components)
        if (int64(Component.Num()) * Component.Num() >= MAX_int32)
        { Diagnostic = TEXT("Bake refused: an island table exceeds TArray's int32 limit."); return; }

    FPathLinkStaticGraph Graph;
    if (!FPathLinkRouteFinder(World).BuildStaticGraph(Links, NavigationData, Graph, false))
    { Diagnostic = TEXT("Failed to collect PathLink traversals."); return; }
    const int32 T = Graph.Traversals.Num();
    int64 Work = int64(T) * T;
    for (const auto& C : Snapshot.Components) Work += int64(C.Num()) * C.Num();
    FScopedSlowTask Progress(double(Work), NSLOCTEXT("PathLink", "Bake", "Baking directed PathLink navigation distances"));
    if (!IsRunningCommandlet()) Progress.MakeDialog(true);
    auto Stop = [&]()
    {
        if (Progress.ShouldCancel()) { Diagnostic = TEXT("Bake cancelled; cache is invalid."); return true; }
        if (FPlatformTime::Seconds() - Begin > TimeBudgetSeconds)
        { Diagnostic = TEXT("Bake exceeded time budget; cache is invalid."); return true; }
        return false;
    };
    TMap<NavNodeRef, int32> IndexByRef;
    Polygons.SetNum(Snapshot.Refs.Num());
    for (int32 I = 0; I < Polygons.Num(); ++I)
    {
        IndexByRef.Add(Snapshot.Refs[I], I);
        Polygons[I].Center = Snapshot.Centers[I]; Polygons[I].Island = Snapshot.Island[I];
        Polygons[I].LocalIndex = Snapshot.LocalIndex[I];
    }
    for (int32 Island = 0; Island < Snapshot.Components.Num(); ++Island)
    {
        const auto& Component = Snapshot.Components[Island];
        const int32 N = Component.Num();
        TArray<float>& Table = Islands[Island].Distances;
        Table.Init(-1.0f, N * N);
        Islands[Island].DepartureDirections.SetNumZeroed(N*N);
        for (int32 A = 0; A < N; ++A)
            for (int32 B = 0; B < N; ++B)
            {
                if (Stop()) return;
                Progress.EnterProgressFrame(1);
                if (A == B) { Table[A * N + B] = 0; continue; }
                const FNavPathSharedPtr Path = PathLinkNavigation::FindPath(World, Snapshot.Centers[Component[A]],
                    Snapshot.Centers[Component[B]], NavigationData, Snapshot.GroundFilter, false);
                if (Path)
                {
                    Table[A * N + B] = float(Path->GetLength());
                    Islands[Island].DepartureDirections[A*N+B]=PathDirection(*Path);
                }
            }
    }
    Traversals.Reset();
    for (const FPathLinkTraversalNode& Node : Graph.Traversals)
    {
        FPathLinkBakedTraversal& B = Traversals.AddDefaulted_GetRef();
        B.Link = Node.Link.Get(); B.Reverse = Node.Reverse; B.LinkType = Node.LinkType;
        B.Entry = Node.EntryLocation; B.Exit = Node.ExitLocation;
        B.EntryNav = Node.EntryNavLocation; B.ExitNav = Node.ExitNavLocation; B.LinkCost = Node.LinkCost;
        FNavLocation Entry, Exit;
        if (!NavigationData->ProjectPoint(B.EntryNav, Entry, FVector(150,150,400), Snapshot.GroundFilter)
            || !NavigationData->ProjectPoint(B.ExitNav, Exit, FVector(150,150,400), Snapshot.GroundFilter)
            || !IndexByRef.Contains(Entry.NodeRef) || !IndexByRef.Contains(Exit.NodeRef))
        { Diagnostic = TEXT("A PathLink endpoint does not map to a traversable ground polygon."); return; }
        B.EntryPoly = IndexByRef[Entry.NodeRef]; B.ExitPoly = IndexByRef[Exit.NodeRef];
    }
    TransitionDistances.Init(-1, T * T);
    for (int32 A = 0; A < T; ++A)
        for (int32 B = 0; B < T; ++B)
        {
            if (Stop()) return;
            Progress.EnterProgressFrame(1);
            if (A == B || Traversals[A].Link == Traversals[B].Link) continue;
            if (Traversals[A].ExitNav.Equals(Traversals[B].EntryNav, 1.0))
            { TransitionDistances[A*T+B] = 0; continue; }
            const FNavPathSharedPtr Path = PathLinkNavigation::FindPath(World, Traversals[A].ExitNav,
                Traversals[B].EntryNav, NavigationData, Snapshot.GroundFilter, false);
            if (Path) TransitionDistances[A*T+B] = Path->GetLength();
        }
    FPathLinkNavSnapshot Verify;
    FString Error;
    if (!PathLinkBake::ReadNavigation(NavigationData, Verify, Error) || Verify.Signature != Snapshot.Signature)
    { Diagnostic = TEXT("Navigation changed during bake; cache is invalid."); return; }
    NavigationSignature = Snapshot.Signature; LinkSignature = ComputeLinkSignature(World);
    bHasNativeOffMeshLinks = Snapshot.bHasNativeOffMeshLinks;
    DataVersion = PathLinkBake::Version;
    Diagnostic = FString::Printf(TEXT("Bake complete. Polygons=%d Islands=%d Traversals=%d Persistent=%.3fMiB. Save the map to cook the cache."),
        Polygons.Num(), Islands.Num(), Traversals.Num(), double(GetCacheAllocatedSize()) / (1024*1024));
#endif
}

APathLinkRoutingCache::APathLinkRoutingCache()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;
#if WITH_EDITOR
    bIsSpatiallyLoaded = false;
#endif
}


UWorld* UPathLinkRoutingCacheData::GetWorld() const
{
    return GetOuter() ? GetOuter()->GetWorld() : nullptr;
}

UPathLinkCacheStorageComponent::UPathLinkCacheStorageComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    Cache = CreateDefaultSubobject<UPathLinkRoutingCacheData>(TEXT("RoutingData"));
}

// Retain the old actor's reflected properties so existing maps migrate without losing their bake.
static void CopyRoutingProperties(const UObject* Source, UObject* Target)
{
    for (TFieldIterator<FProperty> It(UPathLinkRoutingCacheData::StaticClass()); It; ++It)
    {
        FProperty* Destination = Target->GetClass()->FindPropertyByName(It->GetFName());
        FProperty* Origin = Source->GetClass()->FindPropertyByName(It->GetFName());
        if (Destination && Origin && Destination->SameType(Origin))
            Destination->CopyCompleteValue(Destination->ContainerPtrToValuePtr<void>(Target),
                Origin->ContainerPtrToValuePtr<void>(Source));
    }
}
void APathLinkRoutingCache::CopyToData(UPathLinkRoutingCacheData* Data) const { CopyRoutingProperties(this, Data); }
static UPathLinkRoutingCacheData* LegacyData(const APathLinkRoutingCache* Actor)
{
    auto* Data = NewObject<UPathLinkRoutingCacheData>(const_cast<APathLinkRoutingCache*>(Actor));
    Actor->CopyToData(Data);
    return Data;
}
void APathLinkRoutingCache::AnalyzeNavigation() { auto* Data = LegacyData(this); Data->AnalyzeNavigation(); CopyRoutingProperties(Data, this); }
void APathLinkRoutingCache::BakeRoutingCache() { auto* Data = LegacyData(this); Data->BakeRoutingCache(); CopyRoutingProperties(Data, this); }
void APathLinkRoutingCache::ClearRoutingCache() { auto* Data = LegacyData(this); Data->ClearRoutingCache(); CopyRoutingProperties(Data, this); }
bool APathLinkRoutingCache::IsBaked() const { return LegacyData(this)->IsBaked(); }
double APathLinkRoutingCache::EstimateNormalDistance(int32 A, const FVector& From, int32 B, const FVector& To) const
{ return LegacyData(this)->EstimateNormalDistance(A, From, B, To); }
bool APathLinkRoutingCache::EstimateRoute(int32 A, const FVector& From, int32 B, const FVector& To, double& Out) const
{ return LegacyData(this)->EstimateRoute(A, From, B, To, Out); }
void APathLinkRoutingCache::ExportStaticGraph(FPathLinkStaticGraph& Out) const { LegacyData(this)->ExportStaticGraph(Out); }
SIZE_T APathLinkRoutingCache::GetCacheAllocatedSize() const { return LegacyData(this)->GetCacheAllocatedSize(); }
FString APathLinkRoutingCache::ComputeLinkSignature(UWorld* World) { return UPathLinkRoutingCacheData::ComputeLinkSignature(World); }

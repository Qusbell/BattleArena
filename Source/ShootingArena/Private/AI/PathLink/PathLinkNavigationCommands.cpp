#include "AI/PathLink/PathLinkSubsystem.h"
#if WITH_EDITOR

#include "AI/PathLink/Internal/PathLinkNavSnapshot.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/OutputDevice.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "AI/PathLink/PathLinkRoutingCache.h"
#include "AI/PathLink/Internal/PathLinkNavigation.h"
#include "Components/LineBatchComponent.h"
#include "Containers/Ticker.h"

namespace PathLinkNavigationCommands
{
    // Editor-world line-batcher lifetimes depend on world/component ticking, not wall-clock time.
    // Own a batch so manual clear does not flush other systems' debug geometry.
    struct FPolygonOverlay
    {
        static constexpr uint32 BatchID = 0x504C4E50;
        TWeakObjectPtr<ULineBatchComponent> Batcher;
        FTSTicker::FDelegateHandle ExpiryTicker;
        double Deadline = 0;
        TArray<FBatchedLine> Lines;
        TArray<FBatchedPoint> Points;

        ~FPolygonOverlay()
        {
            if (ExpiryTicker.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(ExpiryTicker);
        }

        void Clear()
        {
            if (ExpiryTicker.IsValid()) FTSTicker::GetCoreTicker().RemoveTicker(ExpiryTicker);
            ExpiryTicker.Reset();
            if (ULineBatchComponent* Component = Batcher.Get()) Component->ClearBatch(BatchID);
            Batcher.Reset(); Lines.Reset(); Points.Reset();
            if (GEditor) GEditor->RedrawAllViewports();
        }

        void Begin(UWorld* World)
        {
            Clear();
            Batcher = World->GetLineBatcher(UWorld::ELineBatcherType::WorldPersistent);
        }

        void Add(const TArray<FVector>& Vertices, const FVector& Center, FColor Color,
            float Thickness = 2, float PointSize = 8)
        {
            const FVector Offset(0,0,12);
            for (int32 V = 0; V < Vertices.Num(); ++V)
                Lines.Emplace(Vertices[V]+Offset, Vertices[(V+1)%Vertices.Num()]+Offset,
                    FLinearColor(Color), -1, Thickness, 0, BatchID);
            Points.Emplace(Center+Offset, FLinearColor(Color), PointSize, -1, 0, BatchID);
        }

        void Commit(double Seconds)
        {
            if (ULineBatchComponent* Component = Batcher.Get())
            {
                Component->BatchedPoints.Append(Points);
                Component->DrawLines(Lines);
            }
            Lines.Reset(); Points.Reset();
            if (Seconds > 0)
            {
                Deadline = FPlatformTime::Seconds() + Seconds;
                ExpiryTicker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FPolygonOverlay::Tick), 0.1f);
            }
            if (GEditor) GEditor->RedrawAllViewports();
        }

        bool Tick(float)
        {
            if (Batcher.IsValid() && FPlatformTime::Seconds() < Deadline) return true;
            ExpiryTicker.Reset(); // This delegate returns false, so the ticker removes it.
            Clear();
            UE_LOG(LogTemp, Display, TEXT("[PathLink][NavPoly] Timed overlay expired (wall-clock time)."));
            return false;
        }
    };

    static FPolygonOverlay PolygonOverlay;

    static void DrawAllPolygons(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        double Seconds = 0;
        if (Args.Num() > 1 || (Args.Num() && !LexTryParseString(Seconds, *Args[0]))
            || !FMath::IsFinite(Seconds) || Seconds < 0)
        { Output.Logf(TEXT("Usage: PathLink.NavDrawPolys [seconds=0]; 0 means until cleared.")); return; }
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
        { Output.Logf(TEXT("[PathLink][NavPoly] Editor navigation unavailable or building.")); return; }
        PolygonOverlay.Begin(World);
        int32 Count = 0;
        for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
        {
            FPathLinkNavSnapshot Snapshot; FString Error;
            if (!PathLinkBake::ReadNavigation(*It, Snapshot, Error)) { Output.Logf(TEXT("%s"), *Error); continue; }
            TArray<FVector> Vertices;
            for (int32 I = 0; I < Snapshot.Refs.Num(); ++I)
            {
                Vertices.Reset();
                if (!It->GetPolyVerts(Snapshot.Refs[I], Vertices)) continue;
                PolygonOverlay.Add(Vertices, Snapshot.Centers[I], I % 2 ? FColor::Cyan : FColor::Yellow);
                ++Count;
            }
        }
        const int32 LineCount = PolygonOverlay.Lines.Num();
        PolygonOverlay.Commit(Seconds);
        Output.Logf(TEXT("[PathLink][NavPoly] Drawn polygons=%d edges=%d centers=%d. Duration=%.3fs (0=until clear). Use PathLink.NavClearPolys."),
            Count, LineCount, Count, Seconds);
    }

    static void ClearPolygonOverlay(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        if (!Args.IsEmpty()) { Output.Logf(TEXT("Usage: PathLink.NavClearPolys")); return; }
        PolygonOverlay.Clear();
        Output.Logf(TEXT("[PathLink][NavPoly] NavPoly overlay cleared (outlines and centers)."));
    }

    static FAutoConsoleCommandWithWorldArgsAndOutputDevice DrawAllPolygonsCommand(TEXT("PathLink.NavDrawPolys"),
        TEXT("Draw all loaded ground-poly outlines and centers. Optional seconds; default 0=until clear. Replaces the previous NavPoly overlay."),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&DrawAllPolygons));
    static FAutoConsoleCommandWithWorldArgsAndOutputDevice ClearPolygonsCommand(TEXT("PathLink.NavClearPolys"),
        TEXT("Clear the NavPoly diagnostic outlines and centers."),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&ClearPolygonOverlay));
    struct FPolygonSize
    {
        int32 Index = INDEX_NONE;
        double Diameter = 0;
        double Radius = 0;
        double Area = 0;
        TArray<FVector> Vertices;
        FBox Bounds = FBox(ForceInit);
    };

    static FPolygonSize MeasurePolygon(ARecastNavMesh* Nav, const FPathLinkNavSnapshot& Snapshot, int32 Index)
    {
        FPolygonSize Result;
        Result.Index = Index;
        Nav->GetPolyVerts(Snapshot.Refs[Index], Result.Vertices);
        for (int32 A = 0; A < Result.Vertices.Num(); ++A)
        {
            Result.Bounds += Result.Vertices[A];
            Result.Radius = FMath::Max(Result.Radius, FVector::Distance(Snapshot.Centers[Index], Result.Vertices[A]));
            for (int32 B = A + 1; B < Result.Vertices.Num(); ++B)
                Result.Diameter = FMath::Max(Result.Diameter, FVector::Distance(Result.Vertices[A], Result.Vertices[B]));
            if (A >= 2)
                Result.Area += FVector::CrossProduct(Result.Vertices[A-1] - Result.Vertices[0],
                    Result.Vertices[A] - Result.Vertices[0]).Length() * 0.5;
        }
        return Result;
    }

    static void PrintPolygon(FOutputDevice& Output, const FPathLinkNavSnapshot& Snapshot,
        const FPolygonSize& Size, int32 Rank = 0)
    {
        Output.Logf(TEXT("[PathLink][NavPoly] Rank=%d PolyIndex=%d Ref=%llu Island=%d Center=(%s) Diameter=%.3fm Radius=%.3fm Area=%.3fm2 BoundsMin=(%s) BoundsMax=(%s)"),
            Rank, Size.Index, uint64(Snapshot.Refs[Size.Index]), Snapshot.Island[Size.Index],
            *Snapshot.Centers[Size.Index].ToString(), Size.Diameter / 100, Size.Radius / 100,
            Size.Area / 10000, *Size.Bounds.Min.ToString(), *Size.Bounds.Max.ToString());
    }

    static void PrintLargePolygons(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        int32 Count = 10;
        const bool bDraw = Args.Contains(TEXT("draw"));
        if (Args.Num() > 2 || (Args.Num() && !LexTryParseString(Count, *Args[0]))
            || (Args.Num() == 2 && Args[1] != TEXT("draw")) || Count < 1 || Count > 50)
        { Output.Logf(TEXT("Usage: PathLink.NavLargePolys [count=10, 1..50] [draw]")); return; }
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
        { Output.Logf(TEXT("[PathLink][NavPoly] Editor navigation unavailable or building.")); return; }
        if (bDraw) PolygonOverlay.Begin(World);
        for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
        {
            FPathLinkNavSnapshot Snapshot; FString Error;
            if (!PathLinkBake::ReadNavigation(*It, Snapshot, Error)) { Output.Logf(TEXT("%s"), *Error); continue; }
            TArray<FPolygonSize> Sizes;
            for (int32 I = 0; I < Snapshot.Refs.Num(); ++I) Sizes.Add(MeasurePolygon(*It, Snapshot, I));
            Sizes.Sort([](const FPolygonSize& A, const FPolygonSize& B)
            { return A.Diameter != B.Diameter ? A.Diameter > B.Diameter : A.Index < B.Index; });
            Output.Logf(TEXT("[PathLink][NavPoly] NavData=%s Polys=%d DiameterMedian=%.3fm P95=%.3fm P99=%.3fm; sorted by 3D diameter, coordinates in cm. Draw colors alternate yellow/cyan for 180 seconds."),
                *It->GetName(), Sizes.Num(), Sizes[Sizes.Num()/2].Diameter/100,
                Sizes[FMath::FloorToInt((Sizes.Num()-1)*0.05)].Diameter/100,
                Sizes[FMath::FloorToInt((Sizes.Num()-1)*0.01)].Diameter/100);
            for (int32 I = 0; I < FMath::Min(Count, Sizes.Num()); ++I)
            {
                PrintPolygon(Output, Snapshot, Sizes[I], I+1);
                if (bDraw)
                {
                    const FColor Color = I % 2 ? FColor::Cyan : FColor::Yellow;
                    PolygonOverlay.Add(Sizes[I].Vertices, Snapshot.Centers[Sizes[I].Index], Color, 6, 18);
                }
            }
        }
        if (bDraw) PolygonOverlay.Commit(180);
    }

    static void PrintPolygonAt(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        double Values[6];
        if (Args.Num() != 3 && Args.Num() != 6)
        { Output.Logf(TEXT("Usage: PathLink.NavPolyAt x y z [targetX targetY targetZ] (cm)")); return; }
        for (int32 I = 0; I < Args.Num(); ++I)
            if (!LexTryParseString(Values[I], *Args[I]) || !FMath::IsFinite(Values[I])) return;
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World)) return;
        for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
        {
            FPathLinkNavSnapshot Snapshot; FString Error;
            if (!PathLinkBake::ReadNavigation(*It, Snapshot, Error)) continue;
            FNavLocation Locations[2]; int32 Indices[2] = {INDEX_NONE, INDEX_NONE};
            for (int32 P = 0; P < Args.Num()/3; ++P)
            {
                const FVector Point(Values[P*3], Values[P*3+1], Values[P*3+2]);
                if (!It->ProjectPoint(Point, Locations[P], FVector(150,150,400), Snapshot.GroundFilter)) continue;
                Indices[P] = Snapshot.Refs.IndexOfByKey(Locations[P].NodeRef);
                if (Indices[P] == INDEX_NONE) continue;
                PrintPolygon(Output, Snapshot, MeasurePolygon(*It, Snapshot, Indices[P]));
                Output.Logf(TEXT("[PathLink][NavPoly] Point=%d Projected=(%s) CenterOffset=%.3fm"), P,
                    *Locations[P].Location.ToString(), FVector::Distance(Locations[P].Location, Snapshot.Centers[Indices[P]])/100);
            }
            if (Args.Num() == 6 && Indices[0] != INDEX_NONE && Indices[1] != INDEX_NONE)
            {
                const auto Path = PathLinkNavigation::FindPath(World, Locations[0].Location, Locations[1].Location,
                    *It, Snapshot.GroundFilter, false);
                if (auto* Subsystem = World->GetSubsystem<UPathLinkSubsystem>())
                    if (const auto* Cache = Subsystem->GetValidatedRoutingCache(); Cache && Cache->NavigationSignature == Snapshot.Signature)
                    {
                        const double Estimate = Cache->EstimateNormalDistance(Indices[0], Locations[0].Location,
                            Indices[1], Locations[1].Location);
                        Output.Logf(TEXT("[PathLink][NavPoly] NormalExact=%.6fm NormalEstimate=%.6fm (negative means unavailable)"),
                            Path ? Path->GetLength()/100 : -1, Estimate >= 0 ? Estimate/100 : -1);
                    }
            }
        }
    }

    static FAutoConsoleCommandWithWorldArgsAndOutputDevice LargePolygonsCommand(TEXT("PathLink.NavLargePolys"),
        TEXT("Inspect largest loaded ground polygons: PathLink.NavLargePolys [count=10] [draw]"),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&PrintLargePolygons));
    static FAutoConsoleCommandWithWorldArgsAndOutputDevice PolygonAtCommand(TEXT("PathLink.NavPolyAt"),
        TEXT("Inspect polygon at a point; optional target compares normal exact/estimated lengths. Coordinates in cm."),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&PrintPolygonAt));
    static void PrintNavigationStats(const TArray<FString>& Args, UWorld* ConsoleWorld, FOutputDevice& Output)
    {
        const bool bDetails = Args.Num() == 1 && Args[0].Equals(TEXT("details"), ESearchCase::IgnoreCase);
        if (Args.Num() > 1 || (Args.Num() == 1 && !bDetails))
        {
            Output.Logf(TEXT("Usage: PathLink.NavStats [details]"));
            return;
        }

        // The command always describes the open editor map, even when a PIE console invoked it.
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (!World || World->WorldType != EWorldType::Editor)
        {
            Output.Logf(TEXT("[PathLink][NavStats] No open editor map."));
            return;
        }
        if (UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World))
        {
            Output.Logf(TEXT("[PathLink][NavStats] Navigation is building or locked. Wait for completion and retry."));
            return;
        }

        Output.Logf(TEXT("[PathLink][NavStats] Map=%s | Loaded navigation only | Traversable ground polygons; PathLinks and native off-mesh links excluded."),
            *World->GetOutermost()->GetName());
        int32 NavDataCount = 0;
        for (TActorIterator<ARecastNavMesh> It(World); It; ++It)
        {
            ++NavDataCount;
            const double Begin = FPlatformTime::Seconds();
            FPathLinkNavSnapshot Snapshot;
            FString Error;
            if (!PathLinkBake::ReadNavigation(*It, Snapshot, Error))
            {
                Output.Logf(TEXT("[PathLink][NavStats] NavData=%s | %s"), *It->GetName(), *Error);
                continue;
            }

            int32 Largest = 0;
            int64 TableBytes = 0;
            for (const TArray<int32>& Island : Snapshot.Components)
            {
                Largest = FMath::Max(Largest, Island.Num());
                TableBytes += int64(Island.Num()) * Island.Num() * sizeof(float);
            }
            Output.Logf(TEXT("[PathLink][NavStats] NavData=%s | NavPolys=%d | Islands=%d | LargestIslandPolys=%d | Float32Tables=%.6f MiB | Analysis=%.3f ms"),
                *It->GetName(), Snapshot.Refs.Num(), Snapshot.Components.Num(), Largest,
                double(TableBytes) / (1024.0 * 1024.0), (FPlatformTime::Seconds() - Begin) * 1000.0);
            if (bDetails)
                for (int32 Index = 0; Index < Snapshot.Components.Num(); ++Index)
                {
                    const int64 Count = Snapshot.Components[Index].Num();
                    Output.Logf(TEXT("[PathLink][NavStats] Island=%d | NavPolys=%lld | Float32Table=%.6f MiB"),
                        Index, Count, double(Count * Count * sizeof(float)) / (1024.0 * 1024.0));
                }
        }
        if (NavDataCount == 0)
            Output.Logf(TEXT("[PathLink][NavStats] No RecastNavMesh found. Load/build navigation for this map first."));
        else if (!bDetails)
            Output.Logf(TEXT("[PathLink][NavStats] Use PathLink.NavStats details for each island. Multiple NavData are reported separately."));
    }

    static FAutoConsoleCommandWithWorldArgsAndOutputDevice NavigationStatsCommand(
        TEXT("PathLink.NavStats"),
        TEXT("Print live NavPoly/island counts for the open editor map, without baking or modifying assets. Usage: PathLink.NavStats [details]"),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&PrintNavigationStats));
}

#endif

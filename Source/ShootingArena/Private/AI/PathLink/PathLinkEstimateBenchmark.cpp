#if WITH_EDITOR
#include "AI/PathLink/PathLinkSubsystem.h"
#include "AI/PathLink/PathLinkRoutingCache.h"
#include "AI/PathLink/Internal/PathLinkNavSnapshot.h"
#include "AI/PathLink/PathLink.h"
#include "Editor.h"
#include "Engine/World.h"
#include "NavMesh/RecastNavMesh.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

namespace
{
    struct FBlendSample { FVector A, B; FString Group; };
    FString RouteText(const FPathLinkRouteResult& Route)
    {
        FString Text;
        FPathLinkRouteResult::StaticStruct()->ExportText(Text, &Route, nullptr, nullptr, PPF_None, nullptr);
        return Text;
    }

    void BenchmarkBlend(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        if (!Args.IsEmpty() || !GEditor || GEditor->PlayWorld)
        { Output.Logf(TEXT("Stop PIE, then run PathLink.BenchmarkEstimateBlend (200 deterministic pairs).")); return; }
        UWorld* World = GEditor->GetEditorWorldContext().World();
        auto* Sub = World ? World->GetSubsystem<UPathLinkSubsystem>() : nullptr;
        if (!Sub || !Sub->PrepareRoutingCacheForEditor()) return;
        Sub->RefreshLinks();
        const auto* Cache = Sub->GetValidatedRoutingCache();
        if (!Cache) { Output.Logf(TEXT("No validated cache.")); return; }
        FPathLinkNavSnapshot Snapshot; FString Error;
        if (!PathLinkBake::ReadNavigation(Cache->NavigationData, Snapshot, Error)) return;
        FRandomStream Random(20261021);
        auto Point = [&](int32 Index)
        {
            TArray<FVector> Vertices;
            Cache->NavigationData->GetPolyVerts(Snapshot.Refs[Index], Vertices);
            if (Vertices.IsEmpty()) return Snapshot.Centers[Index];
            return FMath::Lerp(Snapshot.Centers[Index], Vertices[Random.RandRange(0, Vertices.Num()-1)], Random.FRandRange(0.1f,0.85f));
        };
        TArray<int32> Adjacent, Multi;
        for (int32 I=0; I<Snapshot.Refs.Num(); ++I)
        { if (!Snapshot.Neighbors[I].IsEmpty()) Adjacent.Add(I); if (Snapshot.Components[Snapshot.Island[I]].Num()>1) Multi.Add(I); }
        TArray<FBlendSample> Samples;
        Samples.Add({FVector(-670,-632,735),FVector(-838,-1805,746.037296),TEXT("prior57pct")});
        // Near-boundary points in neighboring polygons, including large polygon pairs.
        for (int32 I=0; I<60 && !Adjacent.IsEmpty(); ++I)
        {
            const int32 A=Adjacent[Random.RandRange(0,Adjacent.Num()-1)];
            const int32 B=Snapshot.Neighbors[A][Random.RandRange(0,Snapshot.Neighbors[A].Num()-1)];
            TArray<FVector> VA,VB,Shared;
            Cache->NavigationData->GetPolyVerts(Snapshot.Refs[A],VA);
            Cache->NavigationData->GetPolyVerts(Snapshot.Refs[B],VB);
            for (const FVector& V : VA) for (const FVector& W : VB) if (V.Equals(W,0.1)) { Shared.Add(V); break; }
            FVector Portal=(Snapshot.Centers[A]+Snapshot.Centers[B])*0.5;
            if (Shared.Num()>=2) Portal=(Shared[0]+Shared[1])*0.5;
            Samples.Add({FMath::Lerp(Portal,Snapshot.Centers[A],0.1),FMath::Lerp(Portal,Snapshot.Centers[B],0.1),TEXT("adjacent")});
        }
        for (int32 I=0; I<60 && !Multi.IsEmpty(); ++I)
        {
            int32 A=Multi[Random.RandRange(0,Multi.Num()-1)]; const auto& C=Snapshot.Components[Snapshot.Island[A]];
            int32 B=C[Random.RandRange(0,C.Num()-1)];
            Samples.Add({Point(A),Point(B),TEXT("sameIsland")});
        }
        for (int32 I=0; I<40 && !Cache->Traversals.IsEmpty(); ++I)
        {
            const auto& T=Cache->Traversals[Random.RandRange(0,Cache->Traversals.Num()-1)];
            const FVector Offset(Random.FRandRange(-75,75),Random.FRandRange(-75,75),0);
            Samples.Add({T.EntryNav+Offset,T.ExitNav-Offset,TEXT("linkDynamic")});
        }
        while (Samples.Num()<199)
            Samples.Add({Point(Random.RandRange(0,Snapshot.Refs.Num()-1)),Point(Random.RandRange(0,Snapshot.Refs.Num()-1)),TEXT("random")});
        Samples.Add({Snapshot.Centers[0],FVector(1e7,1e7,-1e7),TEXT("unreachable")});

        auto* Strength=IConsoleManager::Get().FindConsoleVariable(TEXT("PathLink.EstimateBlendStrength"));
        auto* Range=IConsoleManager::Get().FindConsoleVariable(TEXT("PathLink.EstimateBlendRangeCm"));
        const float OldS=Strength->GetFloat(),OldR=Range->GetFloat();
        ON_SCOPE_EXIT { Strength->SetWithCurrentPriority(OldS); Range->SetWithCurrentPriority(OldR); };
        Strength->SetWithCurrentPriority(0.0f);
        TArray<FPathLinkRouteResult> Exact; Exact.SetNum(Samples.Num());
        TArray<FString> ExactText;
        for (int32 I=0; I<Samples.Num(); ++I)
        { Sub->FindShortestRoute(Samples[I].A,Samples[I].B,Exact[I]); ExactText.Add(RouteText(Exact[I])); }
        TArray<FString> Rows;
        Rows.Add(TEXT("sample,group,ax,ay,az,bx,by,bz,strength,range_cm,exact_success,exact_cm,estimate_success,estimate_cm,estimate_us,path_queries"));
        for (float R : {500.0f,1000.0f,2000.0f}) for (float S : {0.0f,0.25f,0.5f,0.75f,1.0f})
        {
            if (S==0 && R!=500) continue;
            Strength->SetWithCurrentPriority(S); Range->SetWithCurrentPriority(R);
            for (int32 I=0; I<Samples.Num(); ++I)
            {
                const auto& P=Samples[I]; double Estimate=0;
                const bool bFound=Sub->GetEstimatedRouteDistance(P.A,P.B,Estimate);
                const auto Stats=Sub->GetLastEstimateStats();
                Rows.Add(FString::Printf(TEXT("%d,%s,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.2f,%.0f,%d,%.12g,%d,%.12g,%.6f,%lld"),
                    I,*P.Group,P.A.X,P.A.Y,P.A.Z,P.B.X,P.B.Y,P.B.Z,S,R,Exact[I].Success,Exact[I].TotalDistance,bFound,Estimate,
                    Stats.TotalMilliseconds*1000.0,int64(Stats.SynchronousNavQueries)));
            }
        }
        int32 Mismatches=0;
        for (int32 I=0; I<Samples.Num(); ++I)
        { FPathLinkRouteResult Check; Sub->FindShortestRoute(Samples[I].A,Samples[I].B,Check); Mismatches+=RouteText(Check)!=ExactText[I]; }
        const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("PathLinkBench/Blend200.csv"));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
        const bool bSaved=FFileHelper::SaveStringArrayToFile(Rows,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        Output.Logf(TEXT("[PathLink] Blend benchmark: samples=%d settings=13 detailed-output-mismatches=%d saved=%d %s"), Samples.Num(),Mismatches,bSaved,*Path);
    }
    FAutoConsoleCommandWithWorldArgsAndOutputDevice BlendBenchmarkCommand(TEXT("PathLink.BenchmarkEstimateBlend"),
        TEXT("Compare blend strengths/ranges against detailed routes for 200 deterministic pairs; write Saved/PathLinkBench/Blend200.csv."),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BenchmarkBlend));
}
#endif

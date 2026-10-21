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
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "CollisionQueryParams.h"

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

    void BenchmarkRandom(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        if (!GEditor || GEditor->PlayWorld || Args.Num()>3)
        { Output.Logf(TEXT("Stop PIE: PathLink.BenchmarkRandomRoutes [count=200] [seed=20261022] [playable]")); return; }
        const bool bPlayable=Args.Num()==3 && Args[2].Equals(TEXT("playable"),ESearchCase::IgnoreCase);
        if (Args.Num()==3 && !bPlayable) { Output.Logf(TEXT("Third argument must be playable.")); return; }
        const int32 Count=Args.Num()>0 ? FCString::Atoi(*Args[0]) : 200;
        const int32 Seed=Args.Num()>1 ? FCString::Atoi(*Args[1]) : 20261022;
        if (Count<1 || Count>10000) { Output.Logf(TEXT("Count must be 1..10000.")); return; }
        UWorld* World=GEditor->GetEditorWorldContext().World();
        auto* Sub=World ? World->GetSubsystem<UPathLinkSubsystem>() : nullptr;
        if (!Sub || !Sub->PrepareRoutingCacheForEditor()) { Output.Logf(TEXT("Routing cache preparation failed.")); return; }
        Sub->RefreshLinks(); const auto* Cache=Sub->GetValidatedRoutingCache();
        if (!Cache) { Output.Logf(TEXT("No validated routing cache.")); return; }
        FPathLinkNavSnapshot Snapshot; FString Error;
        if (!PathLinkBake::ReadNavigation(Cache->NavigationData,Snapshot,Error)) return;
        TArray<FVector> Anchors; double CapsuleRadius=0,CapsuleHalfHeight=0;
        if (bPlayable)
        {
            FString Json; TSharedPtr<FJsonObject> Object;
            const FString AnchorPath=FPaths::ProjectSavedDir()/TEXT("PathLinkBench/PlayableAnchors.json");
            if (!FFileHelper::LoadFileToString(Json,*AnchorPath) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Object)
                || !Object || Object->GetStringField(TEXT("map"))!=World->GetPathName())
            { Output.Logf(TEXT("Missing/mismatched PlayableAnchors.json; export spawn components and capsule through MCP first.")); return; }
            CapsuleRadius=Object->GetNumberField(TEXT("radius")); CapsuleHalfHeight=Object->GetNumberField(TEXT("half_height"));
            if (CapsuleRadius<=0 || CapsuleHalfHeight<CapsuleRadius) return;
            for (const auto& Value : Object->GetArrayField(TEXT("anchors")))
            {
                const auto& P=Value->AsObject()->GetArrayField(TEXT("position"));
                FNavLocation Location;
                if (P.Num()==3 && Cache->NavigationData->ProjectPoint(FVector(P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()),Location,
                    FVector(150,150,400),Snapshot.GroundFilter)) Anchors.Add(Location.Location);
            }
            if (Anchors.IsEmpty()) { Output.Logf(TEXT("No spawn anchors project onto NavMesh.")); return; }
        }
        struct FTriangle { FVector A,B,C; double Cumulative; int32 Poly; };
        TArray<FTriangle> Triangles; double Area=0;
        for (int32 P=0; P<Snapshot.Refs.Num(); ++P)
        {
            TArray<FVector> V; Cache->NavigationData->GetPolyVerts(Snapshot.Refs[P],V);
            for (int32 I=1; I+1<V.Num(); ++I)
            {
                const double Weight=FVector::CrossProduct(V[I]-V[0],V[I+1]-V[0]).Size()*0.5;
                if (Weight<=UE_SMALL_NUMBER) continue;
                Area+=Weight; Triangles.Add({V[0],V[I],V[I+1],Area,P});
            }
        }
        if (Triangles.IsEmpty()) return;
        FRandomStream Random(Seed);
        auto Sample=[&](int32& Poly)
        {
            const double Pick=Random.GetFraction()*Area;
            int32 Low=0,High=Triangles.Num()-1;
            while (Low<High) { const int32 Mid=(Low+High)/2; if (Triangles[Mid].Cumulative<Pick) Low=Mid+1; else High=Mid; }
            const auto& T=Triangles[Low]; Poly=T.Poly;
            const double U=FMath::Sqrt(Random.GetFraction()), V=Random.GetFraction();
            return (1-U)*T.A+U*(1-V)*T.B+U*V*T.C;
        };
        auto* Strength=IConsoleManager::Get().FindConsoleVariable(TEXT("PathLink.EstimateBlendStrength"));
        auto* Range=IConsoleManager::Get().FindConsoleVariable(TEXT("PathLink.EstimateBlendRangeCm"));
        const float OldS=Strength->GetFloat(),OldR=Range->GetFloat();
        ON_SCOPE_EXIT { Strength->SetWithCurrentPriority(OldS); Range->SetWithCurrentPriority(OldR); };
        TArray<FString> Rows;
        Rows.Add(TEXT("sample,group,ax,ay,az,bx,by,bz,strength,range_cm,exact_success,exact_cm,estimate_success,estimate_cm,estimate_us,path_queries,exact_us,used_links,start_poly,target_poly"));
        int32 ExactSuccess=0,Mismatches=0,Attempts=0,PhysicalRejects=0,SpawnRejects=0,PairRejects=0;
        const double TestBegin=FPlatformTime::Seconds();
        auto PlayablePoint=[&](FVector& Point,int32& Poly)
        {
            FNavLocation Projected;
            if (!Cache->NavigationData->ProjectPoint(Point,Projected,FVector(150,150,400),Snapshot.GroundFilter))
            { ++PhysicalRejects; return false; }
            Point=Projected.Location; Poly=Snapshot.Refs.IndexOfByKey(Projected.NodeRef);
            if (Poly==INDEX_NONE) { ++PhysicalRejects; return false; }
            FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(PathLinkPlayableBenchmark),false);
            const bool Ground=World->LineTraceSingleByObjectType(Hit,Point+FVector(0,0,40),Point-FVector(0,0,100),
                FCollisionObjectQueryParams(ECC_WorldStatic),Query);
            const FVector Center=Hit.ImpactPoint+FVector(0,0,CapsuleHalfHeight+5);
            if (!Ground || Hit.ImpactNormal.Z<0.7 || World->OverlapBlockingTestByChannel(Center,FQuat::Identity,ECC_Pawn,
                FCollisionShape::MakeCapsule(CapsuleRadius,CapsuleHalfHeight),Query))
            { ++PhysicalRejects; return false; }
            for (const FVector& Anchor : Anchors)
            { FPathLinkRouteResult Route; if (Sub->FindShortestRoute(Anchor,Point,Route)) return true; }
            ++SpawnRejects; return false;
        };
        for (int32 I=0; I<Count; ++I)
        {
            if (++Attempts>Count*1000 || FPlatformTime::Seconds()-TestBegin>120)
            { Output.Logf(TEXT("Stopped: could not collect requested playable pairs within attempt/time budget. Collected=%d"),I); return; }
            int32 PA,PB; FVector A=Sample(PA), B=Sample(PB);
            if (bPlayable && (!PlayablePoint(A,PA) || !PlayablePoint(B,PB))) { --I; continue; }
            Strength->SetWithCurrentPriority(0.0f);
            FPathLinkRouteResult Exact; Sub->FindShortestRoute(A,B,Exact);
            const double ExactUs=Sub->GetLastRouteStats().TotalMilliseconds*1000;
            if (bPlayable)
            {
                FPathLinkRouteResult Reverse;
                if (!Exact.Success || !Sub->FindShortestRoute(B,A,Reverse)) { ++PairRejects; --I; continue; }
            }
            const FString Before=RouteText(Exact); ExactSuccess+=Exact.Success;
            const TCHAR* Group=PA==PB ? TEXT("samePoly") : Snapshot.Island[PA]==Snapshot.Island[PB] ? TEXT("sameIsland") : TEXT("differentIsland");
            for (int32 Pass=0; Pass<2; ++Pass)
            {
                const float S=Pass==0 ? 0.0f : OldS; Strength->SetWithCurrentPriority(S);
                double Estimate=0; const bool Found=Sub->GetEstimatedRouteDistance(A,B,Estimate);
                const auto Stats=Sub->GetLastEstimateStats();
                Rows.Add(FString::Printf(TEXT("%d,%s,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.9g,%.9g,%d,%.17g,%d,%.17g,%.6f,%d,%.6f,%d,%d,%d"),
                    I,Group,A.X,A.Y,A.Z,B.X,B.Y,B.Z,S,OldR,Exact.Success,Exact.TotalDistance,Found,Estimate,
                    Stats.TotalMilliseconds*1000,Stats.SynchronousNavQueries,ExactUs,Exact.UsedLinks.Num(),PA,PB));
            }
            FPathLinkRouteResult Check; Sub->FindShortestRoute(A,B,Check); Mismatches+=Before!=RouteText(Check);
        }
        const FString Path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("PathLinkBench")/
            FString::Printf(TEXT("%s_%sRoutes%d_Seed%d.csv"),*World->GetName(),bPlayable ? TEXT("Playable") : TEXT("Random"),Count,Seed));
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
        const bool Saved=FFileHelper::SaveStringArrayToFile(Rows,*Path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
        if (bPlayable) Output.Logf(TEXT("[PathLink] Playable filter: anchors=%d capsule=%.1f/%.1f attempts=%d physical-rejects=%d spawn-rejects=%d pair-rejects=%d"),
            Anchors.Num(),CapsuleRadius,CapsuleHalfHeight,Attempts,PhysicalRejects,SpawnRejects,PairRejects);
        Output.Logf(TEXT("[PathLink] Random routes: map=%s pairs=%d seed=%d polys=%d islands=%d area=%.1fm2 exact-success=%d detailed-mismatches=%d strength=%.3f range=%.1fcm saved=%d file=%s"),
            *World->GetName(),Count,Seed,Snapshot.Refs.Num(),Snapshot.Components.Num(),Area/10000,ExactSuccess,Mismatches,OldS,OldR,Saved,*Path);
    }
    FAutoConsoleCommandWithWorldArgsAndOutputDevice RandomBenchmarkCommand(TEXT("PathLink.BenchmarkRandomRoutes"),
        TEXT("Area-weighted independent random NavMesh start/target pairs; compare current blend and disabled blend. [count=200] [seed=20261022] [playable]"),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BenchmarkRandom));
}
#endif

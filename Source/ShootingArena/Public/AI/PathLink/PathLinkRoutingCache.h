#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/PathLink/PathLinkTypes.h"
#include "PathLinkRoutingCache.generated.h"

class ARecastNavMesh;

USTRUCT()
struct FPathLinkBakedPoly
{
    GENERATED_BODY()
    UPROPERTY() FVector Center = FVector::ZeroVector;
    UPROPERTY() int32 Island = INDEX_NONE;
    UPROPERTY() int32 LocalIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FPathLinkIslandInfo
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bake") int32 PolygonCount = 0;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bake") double Float32TableMiB = 0;
    // Directed row-major N*N. -1 means no complete path; no waypoint arrays are serialized.
    UPROPERTY() TArray<float> Distances;
};

USTRUCT()
struct FPathLinkBakedTraversal
{
    GENERATED_BODY()
    UPROPERTY() TObjectPtr<APathLink> Link = nullptr;
    UPROPERTY() bool Reverse = false;
    UPROPERTY() EPathLinkType LinkType = EPathLinkType::Teleport;
    UPROPERTY() FVector Entry = FVector::ZeroVector;
    UPROPERTY() FVector Exit = FVector::ZeroVector;
    UPROPERTY() FVector EntryNav = FVector::ZeroVector;
    UPROPERTY() FVector ExitNav = FVector::ZeroVector;
    UPROPERTY() double LinkCost = 0;
    UPROPERTY() int32 EntryPoly = INDEX_NONE;
    UPROPERTY() int32 ExitPoly = INDEX_NONE;
};

/** One per fully loaded level, independent of AreaManager. Serialized properties are cooked with the map. */
UCLASS(NotBlueprintable)
class SHOOTINGARENA_API APathLinkRoutingCache : public AActor
{
    GENERATED_BODY()
public:
    APathLinkRoutingCache();

    UFUNCTION(CallInEditor, BlueprintCallable, Category = "PathLink|Bake") void AnalyzeNavigation();
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "PathLink|Bake") void BakeRoutingCache();
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "PathLink|Bake") void ClearRoutingCache();

    UPROPERTY(EditInstanceOnly, Category = "PathLink|Bake") TObjectPtr<ARecastNavMesh> NavigationData;
    UPROPERTY(EditInstanceOnly, Category = "PathLink|Bake", meta = (ClampMin = "1")) double MemoryBudgetMiB = 128;
    UPROPERTY(EditInstanceOnly, Category = "PathLink|Bake", meta = (ClampMin = "1")) double TimeBudgetSeconds = 120;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") int32 TotalPolygons = 0;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") int32 LargestIslandPolygons = 0;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") TArray<FPathLinkIslandInfo> Islands;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") double ExpectedTableMiB = 0;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") double EstimatedBakeSeconds = 0;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") double ActualBakeSeconds = 0;
    UPROPERTY(VisibleInstanceOnly, Category = "PathLink|Diagnostics") FString Diagnostic;

    UPROPERTY() int32 DataVersion = 0;
    UPROPERTY() FString NavigationSignature;
    UPROPERTY() FString LinkSignature;
    UPROPERTY() TArray<FPathLinkBakedPoly> Polygons;
    UPROPERTY() TArray<FPathLinkBakedTraversal> Traversals;
    // Same row-major transitions as FPathLinkStaticGraph, kept double to preserve detailed route decisions.
    UPROPERTY() TArray<double> TransitionDistances;
    UPROPERTY() bool bHasNativeOffMeshLinks = false;

    bool IsBaked() const;
    double EstimateNormalDistance(int32 FromPoly, const FVector& From, int32 ToPoly, const FVector& To) const;
    bool EstimateRoute(int32 StartPoly, const FVector& Start, int32 TargetPoly, const FVector& Target,
        double& OutDistance) const;
    void ExportStaticGraph(FPathLinkStaticGraph& Out) const;
    SIZE_T GetCacheAllocatedSize() const;
    static FString ComputeLinkSignature(UWorld* World);
};

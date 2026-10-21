#pragma once

#include "CoreMinimal.h"
#include "AI/PathLink/PathLinkTypes.h"

class AActor;
class APathLink;
class UWorld;

/**
 * UPathLinkSubsystem에서만 사용하는 순수 C++ 최단 경로 계산기입니다.
 * Area / Risk와 전혀 관계없이 NavMesh 거리와 PathLink 거리만 사용합니다.
 *
 * 최적화 구조:
 * - Link Exit -> 다른 Link Entry는 BuildStaticGraph에서 미리 계산합니다.
 * - 실제 Route 요청에서는 Start -> Link Entry / Link Exit -> Target / Start -> Target만 동적으로 계산합니다.
 * - 요청 내부의 Native Path를 재사용하며, 최종 선택된 Normal Segment만 PathPoints를 복사합니다.
 * - 거리 전용 요청은 출력 Segment/PathPoints를 생성하지 않습니다.
 */
class FPathLinkRouteFinder
{
public:
    explicit FPathLinkRouteFinder(UWorld* InWorld)
        : World(InWorld)
    {
    }

    /**
     * 현재 Link 배치와 PathfindingContext 기준으로 Link -> Link 정적 Graph를 구축합니다.
     * Context가 null이면 기본 Navigation 조건을 사용합니다.
     */
    bool BuildStaticGraph(
        const TArray<APathLink*>& Links,
        AActor* PathfindingContext,
        FPathLinkStaticGraph& OutGraph,
        bool bComputeTransitions = true) const;

    /**
     * 이미 구축된 Link -> Link Graph를 재사용하고, Start/Target 쪽은 Nearest 후보 + 근사 비용으로 줄인 뒤 선택 Endpoint만 NavMesh로 정밀 검증합니다.
     */
    bool FindShortestRoute(
        const FVector& StartLocation,
        const FVector& TargetLocation,
        const FPathLinkStaticGraph& StaticGraph,
        AActor* PathfindingContext,
        FPathLinkRouteResult& OutResult,
        bool bBuildSegments = true,
        FPathLinkQueryStats* OutStats = nullptr) const;

private:
    UWorld* World = nullptr;
};

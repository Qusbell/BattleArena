<!-- history-archive-20261105 -->
> 보관 문서입니다. 과거 개선 계획입니다. 현재 적용 완료 여부를 보장하는 문서가 아닙니다.
> 현행 기준은 [시스템 지도](../../repository-audit/architecture/SystemMap.md)와 [데이터 원본 정책](../../repository-audit/architecture/DataAuthority.md)를 확인합니다.

<!-- repository-audit-20261105:start -->
> 2026-11-05 저장소 조사: 과거 실험·수정 기록입니다. 아래 수치와 통과 결과는 당시 이력으로 보존하며 이번에 재실행하지 않았습니다.
> 현행 연결과 미확인 사항은 [시스템 지도](../../repository-audit/architecture/SystemMap.md), [데이터 원본 정책](../../repository-audit/architecture/DataAuthority.md)를 확인합니다.

> 현재 캐시 저장은 WorldSettings의 UPathLinkCacheStorageComponent이며 APathLinkRoutingCache Actor는 호환용 레거시입니다. 알고리즘의 과거 실험 조건과 현행 저장 구조를 구분합니다.
<!-- repository-audit-20261105:end -->

# Next accuracy improvements after selecting strength 1 / 40m

2026-10-21. Only the blend default was changed in this task; the refinements below are proposals.
Current same-200 sample: 36 overestimates and 7 underestimates exceed 10%. The target <=5%
means at most 10/200 outliers, so at least 33 current outliers must be eliminated without adding
new ones. Validate tuning on this fixed set, then on a new playable/reachable holdout set.

## Error sources

The dynamic start/target and fixed PathLink endpoints are represented via polygon centers.
The unblended cost is distance(A,centerA)+bakedLength(centerA,centerB)+distance(centerB,B),
not a chain through every intermediate polygon center. Long detours to the two forced centers
are a major source of overestimation. Their errors also affect which PathLinks win the graph solve.

Blending with Euclidean distance can underestimate obstacle and surface detours. Separately,
the approximate solver considers every baked traversal, while detailed routing uses bounded
nearest candidates, trace-based preliminary scores and selected-endpoint validation. Differences
in chosen Link sequences can survive even if one endpoint cost is improved. A center-selected
NavMesh corridor may also differ from the corridor appropriate for the real endpoints. Float32
rounding alone does not explain meter-scale errors. Exact baked Link-to-Link transitions and
Link traversal costs are already reused and should be retained.

## Recommended sequence

1. Instrument per-leg costs and selected traversal IDs for the 43 outliers. Compare the same
   endpoints and same Link sequence before attributing all error to endpoint representation.
2. Add a ground-only NavMesh raycast fast path. Verify the ray reaches the requested target
   polygon, handles ray corridor limits, filters, heights and separate floors. Physics visibility
   alone is insufficient. Clear flat corridors can use endpoint straight distance; sloped/detail
   surfaces may require accumulating surface-aware lengths. A raycast is an additional native
   query even though it avoids a full FindPathSync search. Measure that cost separately.
3. For blocked walking legs, bake enough compressed corridor/portal metadata to reconstruct
   a valid corridor, then string-pull/funnel using actual endpoints and accumulate length without
   exporting full Route PathPoints. Existing UE Recast FindStraightPath supports a supplied
   corridor. Ordinary shortest-corridor search can then stay in the editor. Do not persist live
   PolyRefs: use canonical baked indices and remap after signature validation. Deduplicate corridor
   metadata and enforce a memory budget rather than storing full waypoint arrays for all pairs.
   A single center-optimal corridor is not guaranteed to be endpoint-optimal: retain a few useful
   alternatives in ambiguous regions or perform bounded local refinement there. The sufficiency
   of a proposed next-hop encoding and its memory cost must be measured, not assumed.
4. If the geometry-based approximation still misses the target, use distance-only exact refinement
   for an uncertainty-selected subset (close ranking candidates, large endpoint offsets, ambiguous
   corridors), with an explicit per-frame query budget. Existing GetRouteDistance avoids output
   PathPoints but still invokes native searches; this is a measurable accuracy/performance tradeoff.
   Do not refine only the known failing test IDs or move all work asynchronously. The zero-full-
   path-search goal remains achievable for the common fast paths; universal zero-search and the
   5% statistical target are not established by the current center-only tables.

Prioritize the raycast experiment first because it reuses the current cache/graph and needs no
large additional all-pairs payload. Next evaluate blocked-path portal/funnel refinement against
accuracy, memory, p95 request latency, native raycast/path-search counts and frame spikes. Preserve
existing detailed route and physical movement behavior. A residual difference caused by the
reference detailed candidate policy must be reported rather than disguised as numerical error.

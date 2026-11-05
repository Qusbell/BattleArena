<!-- history-archive-20261105 -->
> 보관 문서입니다. 과거 측정·배치·원인 분석 기록; 현재 조건에서 재실행하지 않았습니다.
> 현행 기준은 [시스템 지도](../../repository-audit/architecture/SystemMap.md)와 [데이터 원본 정책](../../repository-audit/architecture/DataAuthority.md)를 확인합니다.

<!-- repository-audit-20261105:start -->
> 2026-11-05 저장소 조사: 과거 실험·수정 기록입니다. 아래 수치와 통과 결과는 당시 이력으로 보존하며 이번에 재실행하지 않았습니다.
> 현행 연결과 미확인 사항은 [시스템 지도](../../repository-audit/architecture/SystemMap.md), [데이터 원본 정책](../../repository-audit/architecture/DataAuthority.md)를 확인합니다.

> 현재 캐시 저장은 WorldSettings의 UPathLinkCacheStorageComponent이며 APathLinkRoutingCache Actor는 호환용 레거시입니다. 알고리즘의 과거 실험 조건과 현행 저장 구조를 구분합니다.
<!-- repository-audit-20261105:end -->

# PortalMap random route validation

2026-10-21. Current estimator: strength 1, blend range 2000cm. Lvl_02Map is excluded from interpretation at the user's request because it has no installed PathLinks. This report evaluates the 200 additional PortalMap pairs only; no replacement 200 pairs were added.

## Sampling

Seed 20261022. Each start and target is sampled independently from the currently loaded, default-filter-traversable ground NavMesh. Convex polygons are triangulated, triangles selected proportional to their 3D surface area, and uniform barycentric coordinates sampled inside them. This avoids weighting every tiny polygon equally. No boundary stress cases, same-island constraint, authored link anchors, forced successful routes, or reused previous cases are injected.

The loaded PortalMap had 910 ground polygons, 210 islands, and about 5970.4 square meters of sampled polygon surface. All loaded navigation surfaces participate, including islands that may not be part of normal gameplay. Therefore these results are a map-wide geometric sample, not a frequency-weighted simulation of AI target selection.

## Reachability

Of 200 pairs, 78 have successful detailed routes and estimates; both methods reject the remaining 122. There are zero false positives or false negatives relative to the current detailed API in this sample. All distance-error statistics below use the 78 comparable routes; failures are not counted as zero error.

24 pairs are within the same island and all 24 succeed. Of 176 cross-island pairs, 54 succeed using PathLinks and 122 fail. 58 of the successful routes use one or more PathLinks (including four same-island routes). The mean detailed distance of successful routes is 75.52m. The detailed API's existing bounded candidate policy is the reference, not a guarantee of unrestricted global optimality.

## Accuracy

| Metric | Blend disabled | Current strength 1 / 20m |
|---|---:|---:|
| Mean absolute distance error | 7.75m | 6.52m |
| Mean absolute relative error | 14.95% | 10.59% |
| Median absolute relative error | 10.52% | 8.02% |
| P95 absolute relative error | 38.50% | 26.52% |
| Maximum overestimate | 197.15% | 44.06% |
| Maximum underestimate | 13.67% | 13.67% |

Among the 78 comparable pairs, 45 improve, 1 worsen, and 32 are unchanged in absolute error. More-than-10% overestimates fall from 40 to 31; more-than-10% underestimates remain one.

The largest underestimate is sample 112: detailed 139.626m versus estimated 120.533m (-13.67%). The uncorrected estimate is exactly the same 120.533m, so that case is not introduced by blending. It still matters when judging suitability of the estimate as a whole.

The current mean absolute relative error is 11.57% for the 24 same-island successes and 10.15% for the 54 cross-island successes. The improvement supports this setting as a rough candidate-scoring heuristic on PortalMap; the remaining 44.06% maximum overestimate and 13.67% maximum underestimate do not support treating it as an exact distance or a tight error bound.

## Execution checks

The random test repeated each detailed result and compared all reflected route fields (including segments, links, and PathPoints): zero repeat mismatches. This is a repeatability check at the current setting. The earlier dedicated 200-pair blend sweep separately verified detailed output invariance between disabled and strongest correction.

Both estimator passes made zero synchronous pathfinding calls. Current warm native lookup averaged about 5.85 microseconds across all 200 pairs; one pass per setting, no frame/spike performance claim and no significance claim for differences from the baseline pass. Sampling/benchmark work runs only in the editor. The production routing logic and current blend controls were not changed for this test. The editor build passed.

## Reproduce and raw data

`PathLink.BenchmarkRandomRoutes 200 20261022` with PortalMap loaded and PIE stopped.

`python Scripts/AnalyzePathLinkRandom.py --maps Lvl_PortalMap`

Raw endpoints, route lengths, results, controls, groups and per-request timings: `Saved/PathLinkBench/Lvl_PortalMap_RandomRoutes200_Seed20261022.csv`. Summary: `Saved/PathLinkBench/Random400Summary.json` (currently filtered to PortalMap only). Control values are restored after testing. The starting editor map, Lvl_02Map, was restored afterward.

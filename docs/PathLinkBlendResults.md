# PathLink distance blend results

PortalMap, 2026-10-21. Reference: existing detailed FindShortestRoute, including its candidate policy; this is not a proof of unrestricted global optimality.

## Method

Seed 20261021; exactly 200 directed start/target pairs: 60 adjacent polygon boundary cases, 60 same-island pairs, 40 PathLink endpoint pairs with dynamic offsets, 38 arbitrary polygon pairs, the previous 57% case, and one deliberately unreachable far point. Polygon samples interpolate inside convex polygons and are projected by the normal API. Some random pairs are disconnected. All settings use the identical pairs.

169 pairs have a successful detailed route and estimate; 31 fail in both APIs. There were no reachability disagreements in these samples. Relative statistics exclude routes <=1cm (none among the 169 compared routes).

This is deliberately a stress-weighted sample: 60 cases place endpoints near opposite sides of a shared boundary, some only centimeters apart. Whole-sample percentage averages must not be compared directly to the older 42-pair benchmark or treated as typical gameplay error.

`w = Strength * clamp(1 - BakedCenterDistance / Range, 0, 1)`; `estimate = lerp(oldEstimate, endpointStraightDistance, w)`. Strength is clamped to [0,1]. Only approximate normal endpoint costs change. Exact link transitions/costs and detailed route generation are unchanged.

## All 13 settings

| Strength | Range m | Mean absolute error m | Mean absolute relative % | P95 absolute relative % | Max over % | Max under % | Previous case estimate m |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 0.00 | 5 | 3.387 | 714.71 | 2080.97 | 3693.02 | 0.00 | 19.462 |
| 0.25 | 5 | 3.182 | 632.52 | 1900.02 | 2889.03 | 0.00 | 18.693 |
| 0.50 | 5 | 2.977 | 550.32 | 1791.07 | 2235.48 | 0.00 | 17.923 |
| 0.75 | 5 | 2.771 | 468.13 | 1657.28 | 2235.48 | 0.00 | 17.153 |
| 1.00 | 5 | 2.566 | 385.93 | 1535.73 | 2235.48 | 0.00 | 16.384 |
| 0.25 | 10 | 2.950 | 585.51 | 1721.71 | 2829.40 | 0.00 | 18.189 |
| 0.50 | 10 | 2.513 | 456.30 | 1424.80 | 1965.77 | 0.00 | 16.916 |
| 0.75 | 10 | 2.076 | 327.09 | 1102.15 | 1419.01 | 0.00 | 15.642 |
| 1.00 | 10 | 1.640 | 197.91 | 767.86 | 1231.20 | 1.60 | 14.369 |
| 0.25 | 20 | 2.810 | 560.95 | 1643.78 | 2799.58 | 0.00 | 17.937 |
| 0.50 | 20 | 2.233 | 407.19 | 1202.94 | 1906.14 | 0.00 | 16.412 |
| 0.75 | 20 | 1.656 | 253.43 | 826.24 | 1012.70 | 0.28 | 14.887 |
| 1.00 | 20 | 1.081 | 99.74 | 383.93 | 615.60 | 5.51 | 13.361 |

Default selected: strength 1, range 2000cm, the lowest mean absolute/relative error among these tested settings. This is an empirical default for this map, not a guaranteed optimum for all levels. Strength 0 disables correction without rebaking.

## Remaining short-distance error

| Detailed route length | Compared pairs | Original mean absolute relative % | Selected mean absolute relative % |
|---|---:|---:|---:|
| under5m | 90 | 1327.92 | 181.25 |
| 5to20m | 46 | 22.35 | 6.62 |
| over20m | 33 | 7.42 | 7.24 |

The previous case has a 12.35399m detailed route: 19.46219m (57.54% over) becomes 13.36142m (8.15% over). The selected maximum underestimate is 5.51%; no sample underestimated by more than 10%.

Large residuals remain near polygon boundaries: selected worst relative overestimate is 615.60% (a 0.684m detailed path estimated as 4.895m). Because the weight is gated by center-to-center distance, a very short endpoint distance does not imply a weight near 1. This directly reflects the limitation of the requested blending rule. Shared-portal/visibility corrections would require a separate geometric refinement.

## Validation and runtime cost

All 200 detailed route outputs, including segments, used links and PathPoints, were exported and compared between strength 0 and the strongest tested setting: zero mismatches. The native BakedRouting automation test passed, including directed unreachable sentinel preservation, exact baked-distance gating, zero-range disabling, serialization and migration.

All 2600 estimate evaluations made zero synchronous pathfinding queries and produced no PathPoints. Measured warm native estimate mean was approximately 4 microseconds per request across settings on this machine; one pass per setting, not a representative game-frame or spike benchmark. No statistically significant performance comparison is claimed.

Editor and Win64 Development game builds passed. Cooked runtime execution was not performed.

## Reproduce

With PIE stopped, run `PathLink.BenchmarkEstimateBlend`, then `python Scripts/AnalyzePathLinkBlend.py`. Raw per-pair/configuration results: `Saved/PathLinkBench/Blend200.csv`. Aggregates: `Saved/PathLinkBench/Blend200Summary.json`. The command restores both prior blend controls after testing. It may prepare/bake the editor cache if stale; save the map normally afterward.

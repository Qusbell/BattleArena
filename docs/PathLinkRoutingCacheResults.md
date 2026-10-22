# PathLink verification — 2026-10-20

Environment: UE 5.6.1, Development Editor and Win64 Development game builds.
Map: `/Game/QuakeLike_Base/Lvl_PortalMap` (the map open when work began).
Only the new native cache actor's external package was added to the map. Existing Blueprint assets
and physical movement implementation were not edited.

## Bake

| Statistic | Result |
| --- | ---: |
| Traversable ordinary NavMesh polygons | 910 |
| Ground-polygon connected components (islands) | 210 |
| Largest island | 165 polygons |
| Directed float32 island tables | 0.2183 MiB |
| Baked data including PathLinks | 0.256 MiB |
| Runtime cache including subsystem allocations | 0.2898 MiB |
| Sampled bake estimate | about 0.24 seconds |
| Measured bake | 0.619 seconds |
| Usable PathLink actors / directed traversals | 10 / 12 |

Statistics cover the loaded navigation state, with default-filter traversable ground polygons.
All 210 per-island sizes and table estimates are available in the cache actor Details and bake log.
The source map contains other invalid PathLinks rejected by existing placement validation; those
placements were left unchanged and are not included in the usable traversal graph.

## Route/output regression and timings

42 fixed start/target pairs were recorded from the original loaded binary, including link endpoints,
offset targets representing dynamic items, equal endpoints, and a far unreachable target. After the
change and reloading the saved cache, **all detailed outputs matched exactly**: success, total
distance, segment order/types/endpoints/distances, Link identity/direction, and every PathPoint.

| Request | Original mean / max | Final mean / max |
| --- | ---: | ---: |
| Detailed route | 0.6846 / 7.8311 ms | 0.3752 / 4.1832 ms |
| Exact distance only | 0.5338 / 1.7187 ms | 0.2698 / 0.5782 ms |
| Baked estimate | N/A | 0.01345 / 0.02920 ms |

These are small Editor/Python samples and include cold initialization. They are not a production
frame-time benchmark and should not be treated as a guaranteed speedup.

Final detailed requests averaged 10.12 synchronous path searches (max 24), with candidate work
0.0927ms, graph solve 0.0136ms, and final PathPoints work 0.00426ms on average. Exact-distance
requests generated no output PathPoints. The baseline binary did not expose phase/query counters.
The baked estimator made **zero synchronous path searches in all 42 requests**; projection is
counted separately by design.

Estimates succeeded for all 41 reachable samples, with no false positives or false negatives in this
fixture. Compared with detailed route lengths, mean absolute error was 441.17cm, maximum 1687.50cm,
mean relative error **7.64%**, and maximum relative error **57.54%**. This is the representative-center
approximation's observed limitation; its scores must not be substituted for exact route lengths.

Four fresh equivalent Actor contexts returned identical exact distances with the same query count
as the default Context (9 for that input), and runtime cache memory did not grow. This checks that
equivalent contexts actually share the default graph rather than building four new graphs.

## Runtime and failure paths

- PIE authority world validated the saved bake and evaluated all sample coordinates using zero
  synchronous path queries. A real `BP_AIShooter_C` Pawn Context also returned an estimate.
- PIE client returned no navigation estimates, consistent with existing server-only navigation.
- The observed PIE maximum detailed request was 0.9102ms; maximum detailed-route work in a frame
  was 1.9446ms. Maximum observed engine frame delta was 2442.57ms, including PIE startup and Editor
  activity. No pre-change gameplay-frame capture exists, so no reduction in whole-frame spikes is
  claimed. Use an equivalent steady-state Unreal Insights run for that comparison.
- Clearing the cache made estimates unavailable immediately.
- A deliberately tiny test budget rejected All-Pairs before allocation; the rejected bake remained
  unavailable. The normal 128MiB budget was restored, cache rebaked, validated, and saved.
- `ShootingArena.PathLink.BakedRouting` passed. It covers asymmetric directions, same/different
  islands, unreachable entries, competing parallel links, reverse traversal, several hops, zero-cost
  teleport, dynamic target offsets, same-island shortcuts, and serialization/version rejection.
- Editor and Win64 game builds passed; `git diff --check` passed. A full cooked package was not run.

Raw measurements are in `Saved/PathLinkBench/baseline.json`, `final.json`, and `pie.json` in the local
workspace. These Saved files are not runtime dependencies and are not checked-in bake data.

## Adoption boundary

Existing Blueprint calls retain their interfaces and detailed behavior. Bulk target-scoring callers
can adopt `GetEstimatedRouteDistance`; selected targets continue through `FindShortestRoute`.
No Blueprint call sites were automatically switched to approximate scoring. Other gameplay levels
need their own analysis/bake. This implementation supports one selected NavData per world and
rejects mismatched/partially streamed navigation instead of querying an incorrect table.

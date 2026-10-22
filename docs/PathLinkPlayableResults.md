# PortalMap playable, mutually reachable random route test

2026-10-21. Supersedes the unrestricted-map random sample for the user's intended playable-route evaluation. Lvl_02Map remains excluded. Production estimator controls remain strength 1, range 2000cm.

## Playable-point definition

There is no separate authored playable-area volume in this map. The explicit operational proxy for this test is:

- On the currently loaded, default-filter-traversable ground NavMesh.
- A static ground hit within the test's +40/-100cm vertical trace, with ground normal Z >= 0.7.
- An upright capsule of radius 34cm / half-height 88cm fits at the ground hit plus half-height +5cm, without a blocking ECC_Pawn overlap. Dimensions were read from the actual BP_AIShooter class through Unreal MCP. This is a geometric clearance check, not a full simulation of movement.
- A complete detailed PathLink route exists from at least one of the six authored BP_PlayerSpawnPoint SpawnPoint components, projected onto navigation.
- After endpoint filtering, detailed routes must succeed in BOTH directions between A and B. Estimate success is deliberately not a selection criterion.

Spawn references and capsule dimensions were read/exported exclusively through Unreal MCP to Saved/PathLinkBench/PlayableAnchors.json. No Blueprint edits or runtime movement changes were made. Gameplay-specific exclusions not represented by these checks are not inferred.

## Sampling and acceptance

Seed 20261024; fresh area-weighted triangle and uniform barycentric sampling, as in the previous map-wide sampler. Reject candidates failing the above criteria until exactly 200 accepted pairs are collected. No stress-case injection or forced close/distant ratio. Failed attempts are not counted as error-free tests.

529 candidate pairs were attempted: 142 rejected at a physical-point check, 187 at spawn-route reachability, zero at the final forward/reverse pair-route check. All 200 accepted pairs pass both directional route checks. These counters classify each rejected attempt by its first failing check; they are not counts of distinct bad polygons.

The accepted set contains 126 cross-island pairs, 73 different-poly same-island pairs and one same-poly pair. 136 detailed forward routes use PathLinks (126 cross-island plus 10 same-island shortcuts). Mean detailed forward length is 72.81m.

## Accuracy on all 200 accepted forward routes

| Metric | Blend disabled | Current strength 1 / 20m |
|---|---:|---:|
| Mean absolute error | 6.48m | 5.37m |
| Mean absolute relative error | 13.24% | 9.57% |
| Median absolute relative error | 8.21% | 6.04% |
| P95 absolute relative error | 43.09% | 26.30% |
| Maximum overestimate | 171.94% | 61.46% |
| Maximum underestimate | 11.66% | 11.66% |

104 pairs improve, 2 worsen, 94 are unchanged in absolute error. More-than-10% overestimates decrease from 85 to 64; more-than-10% underestimates increase from 2 to 3. The maximum underestimate is unchanged, so the maximum alone does not capture every regression.

The worst underestimate is sample 31: detailed 124.655m versus estimate 110.120m. Its disabled-blend estimate is 110.120m.

Current mean absolute relative error is 8.41% for the 126 cross-island routes and 11.72% for the 73 different-poly same-island routes. The one same-poly route has zero error. No 200-pair test can establish a worst-case bound, and actual movement success was not simulated.

## Distance buckets

| Detailed distance | Pairs | Original mean absolute relative % | Current mean absolute relative % |
|---|---:|---:|---:|
| under5m | 2 | 159.81 | 47.85 |
| 5to20m | 20 | 26.44 | 16.47 |
| over20m | 178 | 10.11 | 8.37 |

## Validation

All 200 forward estimates succeeded: zero estimate/detailed reachability disagreements among the accepted set. Estimate failures would have been retained if present. These success-selected pairs cannot measure false positives on unreachable targets.

Detailed outputs (all reflected route properties, including PathPoints and used links) were compared between disabled blending and current blending: zero mismatches. Both estimate passes total zero synchronous path searches. Current native estimate mean was about 4.02 microseconds on this machine; not a representative frame/spike benchmark. The editor build passed. Changes in this task are editor benchmark/analysis tooling only.

## Reproduce

With PortalMap open and PIE stopped, run Scripts/ExportPathLinkPlayableAnchors.py through Unreal MCP execute_python (Blueprint reads must use MCP), then run:

`PathLink.BenchmarkRandomRoutes 200 20261024 playable`

`python Scripts/AnalyzePathLinkRandom.py --maps Lvl_PortalMap --kind Playable`

Raw data: Saved/PathLinkBench/Lvl_PortalMap_PlayableRoutes200_Seed20261024.csv. Summary: Saved/PathLinkBench/PlayableSummary.json. Current controls are restored after the benchmark. The editor is left on PortalMap.

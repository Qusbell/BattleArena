# Same 200 playable pairs: strength/range sweep

PortalMap, 2026-10-21. Replays exactly the 200 accepted endpoints in Lvl_PortalMap_PlayableRoutes200_Seed20261024.csv; no random resampling, reachability filtering, or replacement of difficult pairs. Original playable-point checks and bidirectional reachability remain documented in PathLinkPlayableResults.md.

The reference detailed distance for every pair was rechecked against the earlier CSV (0.001cm tolerance): zero changes. All 13 settings returned estimates for all 200 pairs. Detailed outputs, including segments, links and PathPoints, were compared between strength 0 and strength 1/range 80m: zero mismatches. All 2600 estimate calls performed zero synchronous path searches.

## All settings

| Strength | Range m | Mean absolute error m | Mean absolute relative % | P95 relative % | Max over m | Max under m | Max over % | Max under % | Under by >10% /200 |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 20 | 6.48 | 13.24 | 43.09 | 23.78 | 14.53 | 171.94 | 11.66 | 2 |
| 0.5 | 10 | 6.28 | 12.49 | 35.23 | 23.78 | 14.53 | 138.25 | 11.66 | 2 |
| 0.75 | 10 | 6.18 | 12.12 | 33.69 | 23.78 | 14.53 | 121.41 | 11.66 | 2 |
| 1 | 10 | 6.09 | 11.74 | 32.78 | 23.78 | 14.53 | 104.56 | 11.66 | 2 |
| 0.5 | 20 | 5.92 | 11.37 | 33.37 | 23.78 | 14.53 | 112.11 | 11.66 | 2 |
| 0.75 | 20 | 5.64 | 10.45 | 28.83 | 23.78 | 14.53 | 82.20 | 11.66 | 3 |
| 1 | 20 | 5.37 | 9.57 | 26.30 | 23.78 | 14.53 | 61.46 | 11.66 | 3 |
| 0.5 | 40 | 4.93 | 9.37 | 28.50 | 21.24 | 15.81 | 99.04 | 12.69 | 3 |
| 0.75 | 40 | 4.31 | 7.76 | 20.79 | 19.97 | 16.45 | 62.59 | 18.84 | 5 |
| 1 | 40 | 3.84 | 6.38 | 19.83 | 19.52 | 17.09 | 45.13 | 30.00 | 7 |
| 0.5 | 80 | 3.98 | 7.76 | 23.33 | 17.79 | 18.83 | 92.51 | 15.47 | 10 |
| 0.75 | 80 | 3.94 | 6.86 | 17.98 | 14.80 | 20.97 | 52.79 | 27.48 | 25 |
| 1 | 80 | 4.55 | 6.91 | 19.69 | 11.80 | 23.12 | 13.07 | 41.52 | 42 |

Maximum meter errors and maximum percentage errors can belong to different pairs. Do not interpret one as the percentage of the other.

## Interpretation

- The lowest mean absolute meter error and mean absolute relative error among these settings occur at strength 1 / range 40m: 3.84m and 6.38%, versus the current 5.37m / 9.57%. But the maximum underestimate rate rises from 11.66% to 30.00%, and >10% underestimates rise from 3 to 7.
- Strength 0.75 / range 40m is an intermediate choice: 4.31m mean absolute error, 7.76% mean absolute relative error, 18.84% maximum underestimate, 5 >10% underestimates.
- Strength 0.5 / range 40m makes a smaller change: mean absolute error 4.93m, mean absolute relative error 9.37%, maximum overestimate 21.24m and maximum underestimate rate 12.69% (3 >10% underestimates). It reduces large absolute overestimates somewhat but increases the maximum relative overestimate elsewhere.
- Strength 1 / range 80m minimizes maximum meter overestimation here (11.80m), but >10% underestimates rise to 42/200 and maximum underestimate rate is 41.52%. Mean meter error also worsens relative to 40m. Stronger/wider correction is therefore not unconditionally better.
- This is a tuning comparison on the same finite sample, not independent validation of a newly selected setting. No production/default setting was changed: strength 1 and range 2000cm were restored in a finally block.

## Previously highlighted samples

| Sample | Detailed m | Estimate @1/20m | Estimate @0.75/40m | Estimate @1/40m | Estimate @1/80m |
|---:|---:|---:|---:|---:|---:|
| 63 | 105.74 | 129.52 | 125.72 | 124.45 | 117.55 |
| 85 | 121.09 | 140.61 | 140.61 | 140.61 | 126.62 |
| 146 | 65.83 | 84.33 | 79.16 | 77.44 | 67.52 |
| 148 | 172.52 | 190.05 | 187.64 | 186.83 | 182.96 |
| 182 | 121.62 | 137.78 | 134.41 | 133.29 | 127.16 |
| 56 | 25.61 | 41.34 | 38.21 | 37.16 | 27.75 |
| 7 | 69.95 | 85.55 | 82.39 | 80.50 | 73.93 |
| 84 | 105.40 | 120.92 | 116.72 | 115.32 | 109.05 |
| 51 | 148.58 | 163.76 | 162.43 | 161.99 | 152.12 |
| 162 | 81.09 | 95.64 | 95.06 | 94.49 | 85.56 |
| 31 | 124.65 | 110.12 | 108.20 | 107.56 | 101.54 |
| 181 | 110.24 | 97.98 | 97.22 | 96.96 | 89.27 |
| 74 | 111.57 | 99.73 | 98.94 | 98.08 | 93.10 |
| 141 | 140.45 | 129.32 | 128.36 | 128.04 | 123.09 |
| 147 | 20.88 | 19.43 | 16.95 | 14.62 | 12.21 |
| 108 | 16.88 | 16.45 | 16.03 | 15.73 | 15.36 |
| 116 | 16.62 | 16.62 | 16.25 | 15.72 | 15.27 |

## Reproduce

With PortalMap open, run Scripts/SweepPathLinkPlayableBlend.py through Unreal MCP execute_python. The script reads original exact endpoint coordinates, checks detailed references, measures every setting and restores prior controls. It aborts rather than replacing any sample whose detailed reference changed.

Raw per-pair results: Saved/PathLinkBench/PlayableBlendSweep200.csv. Aggregates: Saved/PathLinkBench/PlayableBlendSweep200Summary.json. Native timings in the JSON are one-pass measurements and are not a frame/spike or statistically controlled performance comparison. This task changed only the replay script and report, not runtime navigation logic.

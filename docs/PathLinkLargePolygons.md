# Portal map polygon size / maximum-error investigation

Inspected live `/Game/QuakeLike_Base/Lvl_PortalMap` on 2026-10-21 via editor MCP.
910 traversable ground polygons. No map, Blueprint or baked-data changes were made.
Size means maximum 3D vertex-to-vertex diameter, not polygon count per island.

Diameter distribution: median **3.743m**, 95th percentile **10.084m**, 99th percentile **11.432m**.

## Largest polygons

Centers are Unreal world coordinates in **cm**. Polygon indices and island IDs describe the current
NavMesh state and can change after regeneration. Nearby mesh labels are spatial references, not polygon owners.

| Rank | Poly index | Island | Center (X,Y,Z) cm | Diameter m | Area m² | Nearby mesh |
| --- | ---: | ---: | --- | ---: | ---: | --- |
| 1 | 427 | 4 | (-503.500, -1600.750, 223.000) | 13.972 | 72.272 | Floors/Maya_LandingPlace109 |
| 2 | 454 | 4 | (-494.000, -2470.000, 223.000) | 13.972 | 97.614 | Floors/Maya_LandingPlace110 |
| 3 | 229 | 13 | (604.833, -1548.500, 2473.000) | 12.820 | 70.828 | Floors/Maya_LandingPlace121 |
| 4 | 94 | 31 | (1444.000, -2375.000, 223.000) | 12.700 | 78.193 | Floors/SM_Floor179 |
| 5 | 96 | 4 | (1358.500, -2417.750, 703.000) | 12.350 | 65.467 | Block02/콜리전/Box_wall127 vicinity |
| 6 | 248 | 31 | (668.800, -2508.000, 223.000) | 12.199 | 51.713 | |
| 7 | 775 | 176 | (-3572.000, -452.200, 2433.000) | 12.017 | 63.392 | |
| 8 | 780 | 4 | (-3277.500, -494.000, 753.000) | 11.804 | 61.948 | |
| 9 | 212 | 4 | (581.400, -361.000, 223.000) | 11.502 | 49.024 | |
| 10 | 257 | 13 | (674.500, -2470.000, 2343.000) | 11.432 | 52.410 | |

The first two are on the low floor at Z=223cm. Their respective XY bounds are
[-988, 0]×[-1976, -988] and [-988, 0]×[-2964, -1976] cm. The third is on the high
floor at Z=2473cm with XY bounds [0,988]×[-1843,-988] cm.

## Why the recorded maximum relative error was 57.54%

Baseline sample 37 in `Saved/PathLinkBench/final.json`:

- Start: (-670,-632,735) cm.
- Target: (-838,-1805,746.037296) cm.
- Detailed route uses `BP_PathLink16`, in `Block02/플랫폼요소`.
- Its entry is (-624,-717,737) cm; exit is (-875,-1784,731.037296) cm.

| Portion | Detailed distance m | Estimate m |
| --- | ---: | ---: |
| Start → BP_PathLink16 entry | 0.966488 | 8.074693 |
| PathLink traversal | 10.961412 | 10.961412 |
| Exit → target | 0.426089 | 0.426089 |
| Total | 12.353990 | 19.462194 |

The extra **7.1082m** is explained by the first ordinary movement segment. The live ground-only
query and the existing baked table reproduce the recorded discrepancy.

The start projects to polygon **402**, center (-639.667,-475,743), diameter **5.293m**, area 4.513m².
The nearby entry projects to polygon **403**, center (-356.250,-470.250,743), diameter **9.918m**,
area 30.667m². Both are in island 4, near `Floors/Maya_LandingPlace77`.

Although the actual endpoints are only about **0.97m** apart, the estimate sums:

`start → center402 ≈ 1.599m + center402 → center403 ≈ 2.835m + center403 → entry ≈ 3.641m`.

These are different adjacent polygons. Their remote representative centers cause a large detour
in the estimate. Neither polygon is one of the ten largest, and polygon 403 is below the 95th
diameter percentile. Therefore an exceptional maximum-size polygon is not necessary for this error.
Large polygons make this effect more likely, but forcing endpoint-center detours across a nearby
polygon boundary is the directly demonstrated cause in this sample. This small benchmark does not
establish how often the effect occurs in production.

The exit and target both project to polygon **446**, so that segment takes the same-polygon direct
distance and contributes no discrepancy here.

## Repeat the inspection

Editor Output Log Cmd:

```text
PathLink.NavLargePolys 10 draw
PathLink.NavPolyAt -670 -632 735 -624 -717 737
PathLink.NavPolyAt -875 -1784 731.037296132 -838 -1805 746.037296132
```

The first outlines the ten largest polygons and marks their centers in alternating yellow/cyan for
180 seconds, without modifying assets. The other commands print projected polygon geometry,
center offsets, and exact versus baked normal distances. Pair comparisons require a matching bake.
This adds diagnostic commands only; the approximation and detailed routing algorithms are unchanged.

# PathLink navigation cost cache

## Existing behavior and changes

| Entry point | Previous cost | Current behavior |
| --- | --- | --- |
| `UPathLinkSubsystem::FindShortestRoute` | Context graph lookup/build followed by detailed routing | Public signature/result unchanged. Adds full-request measurements and bake validation/import. |
| `FPathLinkRouteFinder::FindShortestRoute` | Two projections, candidate sort/traces, synchronous endpoint paths, repeated graph construction/Dijkstra, final paths | Preserves candidate settings, expansion rules, traversal order, tolerance and tie behavior. Builds adjacency once per request, reuses solve buffers, retains native paths within the request. |
| `BuildStaticGraph` | Ordered traversal pairs perform up to O(T²) synchronous paths per Context | Existing algorithm retained as fallback. Optional traversal-only mode supports editor bake. A validated bake supplies the same directed transitions without runtime pair searches. |
| `FPathLinkStaticGraph` | Ordered traversals plus dense T×T reachability/double-distance matrix | Retained. TwoWay has two traversal nodes; doubling T quadruples dense storage. |
| `GetRouteDistance` | Builds a complete result and copies all final PathPoints | Runs the same route selection and exact final distances, without output segments, UsedLinks or PathPoints copies. Still performs native pathfinding; use the new API for bulk scoring. |
| `EnsureRouteGraphCache` | Allocates RouteLinks and sweeps Context map even on cache hits; each AI builds a graph | RouteLinks only allocated on a miss. Dead Context cleanup moves to the subsystem tick. Equivalent default NavData/filter contexts share the default graph only with a single NavData and no native off-mesh links. Otherwise original per-Actor caching remains. |

Native `FindPathSync` uses the same Context/NavData/filter selection as UE 5.6.1's
`FindPathToLocationSynchronously`. It avoids temporary `UNavigationPath` UObjects. A request-local
cache retains `FNavPathSharedPtr`, including failed queries; it never reuses dynamic endpoints across
requests. Detailed queries keep their original partial-path rejection, 1cm shortcut and distance sum.
No PathLink traversal mechanics or AddMovementInput/physical movement code is changed.

The detailed route finder already uses bounded nearest candidates and trace-adjusted estimates.
Its final selected endpoints are validated with NavMesh. This change preserves that selection
policy; it does not claim that the existing bounded search is an unrestricted global optimum.

## Editor analysis and bake

For a read-only count of the currently open editor map, enter `PathLink.NavStats` in the
editor Output Log's **Cmd** console. Use `PathLink.NavStats details` for per-island counts.
No cache actor or bake is required; this reads the currently loaded NavMesh, reports each
Recast NavData separately, and does not dirty the map or change saved cache data. Counts
include default-filter-traversable ground polygons and exclude off-mesh links. For World
Partition/streamed levels, load all intended navigation tiles before treating this as a full-map count.

`PathLink.NavLargePolys 10` ranks polygons by their 3D vertex-to-vertex diameter, printing center,
bounds (cm), center-to-vertex radius, area (m²), and diameter percentiles. Add `draw` to outline
those polygons in alternating yellow/cyan for 180 seconds: `PathLink.NavLargePolys 10 draw`.
`PathLink.NavPolyAt x y z` inspects the polygon projected from a world coordinate (cm). Supply a
second XYZ triple to compare exact normal NavMesh distance with the baked normal estimate.
These diagnostics read current navigation; polygon indices/refs are only valid for that navigation state.

`PathLink.NavDrawPolys` draws **every** loaded traversable ground polygon's outline and representative
center until `PathLink.NavClearPolys` is run. An optional positive duration, e.g. `PathLink.NavDrawPolys 180`,
expires using editor wall-clock time rather than editor-world simulation ticks. Clear removes both
outlines and center points from this diagnostic batch. The largest-polygon drawing uses that same
batch and a wall-clock 180-second timeout. Each drawing command replaces the previous NavPoly
overlay; repeated calls do not accumulate geometry. Other systems' debug batches are retained.

1. Fully load the gameplay level, navigation tiles and PathLinks. Build Navigation normally.
2. Play/Simulate, or save the map. `UPathLinkCacheEditorSubsystem` automatically asks the world
   `UPathLinkSubsystem` to validate the NavMesh and PathLink signatures before world duplication/save.
3. On a cache hit, reuse the tables. On a miss, invalidate them first and automatically bake the current
   default Recast NavData. A pending Recast build is completed before checking.
4. The subsystem creates a native storage component on the existing WorldSettings automatically.
   No routing actor needs to be placed. Its instanced UObject payload is serialized/cooked with the map.
   A single legacy routing actor is migrated once; an invalid managed cache never falls back to it.
5. Save the map after Play if the automatic bake dirtied it. Map-save also checks/bakes automatically;
   it does not silently save unrelated changes. Cook uses the saved data, with no runtime bake.
6. Memory/time budgets remain 128MiB / 120 seconds by default. Oversized or cancelled bakes remain
   invalid, log their diagnostic and allow exact routing to continue; approximate scoring returns false.
   No all-pairs generation happens during game execution. Runtime mismatches block the old bake.

For an explicit forced rebuild, stop PIE/Simulate and enter `PathLink.BuildRoutingCache` in the
editor Output Log's Cmd console. It clears the managed bake and regenerates it from the current
NavMesh and PathLinks using the same budgets/validation as automatic preparation. Save the map
afterward. This command builds the routing cache; use Unreal's normal Build Navigation for terrain
changes that have not yet been reflected in NavMesh. A failed/cancelled forced build stays invalid.

The old actor class remains only for serialized-map compatibility. Runtime managed-cache queries
use the UObject directly. Editor hooks do not run in cook commandlets. A never-saved/new or changed
map must be saved normally before packaging. Load the intended full level before checking streamed
maps; runtime validation refuses signatures from a different loaded navigation/link set.

Islands are weak connected components of ordinary, default-filter-traversable Recast polygons.
Cross-tile polygon adjacency is included. Native off-mesh links are excluded, as are project
PathLinks. This has no dependency on AI Area actors or Recast tile cluster IDs. Directed path
queries within each component can still be unreachable; those ordered table entries contain -1.

Each N×N island table contains **float32 lengths in cm**, for paths between polygon representative
centers. Ordered pairs are queried separately. Diagonal entries are zero. No pair waypoint lists
are serialized. These are lengths of engine-selected NavMesh paths, consistent with TotalDistance;
area-weighted filter cost is not substituted for geometric length.

PathLink traversals retain the actual Actor, entry/exit, projected endpoints, endpoint polygon IDs,
OneWay/TwoWay/Reverse, type and movement cost (teleport=0). Directed Link-to-Link transitions retain
double distances to preserve detailed route comparisons. They are reused rather than collapsed to
island reachability. Several parallel links, several hops, and a shortcut within one island all remain
available in the estimate graph.

Analysis samples at most 32 real searches. Its time estimate is indicative, not a promise; search
cost depends on distance, island shape, failure cases and editor load. The actual bake also has a
wall-clock limit. An incomplete/cancelled/refused bake cannot be used. A node-limit search failure is
also stored as unavailable (-1), so estimated failure is not a proof of topological unreachability.

## Runtime API and accuracy

`GetEstimatedRouteDistance(Start, Target, OutDistance, Context)` returns a bool and a distance in cm.
After initial validation it performs two NavMesh **projections**, table lookups and a small traversal
Dijkstra. It performs **zero synchronous path searches**, builds no PathPoints, and checks all baked
PathLink endpoints. It compares normal movement with Link routes even within the same island.

Use it to rank dynamic targets, including arbitrary dropped-item coordinates. Once a target is
chosen, call the existing `FindShortestRoute` and move using its detailed result. Re-evaluate/retry
other targets if that detailed call fails. Existing Blueprint callers are unchanged; target-scoring
Blueprints must explicitly adopt the new API. They are not automatically rewritten.

For two different polygons the normal estimate is:

`distance(Start, CenterA) + BakedLength(A,B) + distance(CenterB, Target)`

For the same convex polygon it uses the projected endpoints' straight distance. Center detours,
surface detail, float32 rounding, weighted corridor choice and the existing detailed candidate policy
can produce differences from a detailed route. Close points on opposite polygon edges can have large
relative errors. This is neither an exact metric at arbitrary coordinates nor a certified error bound.
Projection uses the existing 150×150×400cm extent, so an off-mesh item can project to nearby ground.

The representative-center formula does **not** connect every intermediate polygon center.
`BakedLength(A,B)` is a regular engine-generated NavMesh path from CenterA to CenterB, including
its normal corridor/string-pulling behavior. Only the first and last polygon centers are forced
anchors. On a flat unobstructed surface, that middle path can be a straight line across many polygons.

False means unavailable, unsupported Context/filter, projection failure, or no complete estimated
route. It never silently invokes expensive pathfinding. `GetRoutingCacheStatus` diagnoses cache and
Context problems; callers choose any fallback. OutDistance is zero on failure.

## Validity and packaging

The bake signature includes engine/data version, agent dimensions, query filter costs/flags/search
limit/heuristic, canonical tile coordinates, vertices/detail meshes, polygon flags and directed ground
adjacency. PolyRefs/salts are **not** persistent identities. The current NavMesh is fingerprinted once
and current refs are mapped to baked array indices after a signature match. PIE/cook object reference
remapping preserves PathLink references. Link positions/direction/type/enabled state have a separate
signature and are then checked allocation-free against validated placements.

Navigation generation and Recast mesh-update delegates invalidate the runtime caches, including tile
streaming changes. Missing/partial/different loaded navigation rejects the bake. Native off-mesh
links are excluded from cost tables; their owner-dependent permissions prevent graph sharing/import
for detailed queries. Different NavData/filter contexts are rejected by the estimate API instead of
using an incorrect table. This initial implementation supports **one selected NavData/cache per world**.

Static terrain does not automatically imply a static runtime NavMesh. The currently inspected map
uses Dynamic runtime generation. Any runtime rebuild must finish and reproduce the baked signature;
otherwise rebake/static navigation configuration is needed. The implementation does not change that
project setting. Full validation is a cold-start operation and can cost O(polygons), outside the warm
lookup timings. Persistent cache memory counters do not include transient validation/pathfinding memory.

## Measurements and regression

- `GetLastRouteStats`: full request time (including cold graph work), candidate selection/traces,
  graph solve, final PathPoints generation, actual synchronous path-query count, solve count.
- `GetLastEstimateStats`: estimated request/solve time; synchronous query count remains zero.
- `GetRuntimeCacheMiB`: baked tables/graph, subsystem graphs, registry and ref-map allocations.
- `GetMaxFrameMilliseconds`: maximum observed engine frame delta during game-world ticks.
- `GetMaxRouteMilliseconds` and `GetMaxRouteWorkPerFrameMilliseconds`: route request spikes and
  combined detailed-route work in a frame. Use Unreal Insights to attribute total-frame stalls.
- CPU trace scopes: `PathLink_RouteRequest`, `PathLink_DetailedRoute`, `PathLink_Dijkstra`,
  `PathLink_EstimatedDistance`, `PathLink_ValidateBake`.

`Scripts/PathLinkBenchmark.py` runs through MCP `execute_python`, records full route outputs and
per-request timings to Saved, accepts a baseline file, and reports exact output mismatches plus
estimate absolute/relative error. Run the baseline before replacing the loaded binary, then compare
the same coordinates in the same navigation state. Its Python/Editor timings include call overhead
and cannot establish production frame-time improvements.

Automation: `ShootingArena.PathLink.BakedRouting` covers directed/asymmetric tables, disconnected
and unreachable targets, parallel links, explicit reverse, multiple hops, teleport cost, dynamic
coordinates, same-island shortcuts and property serialization. The live-map regression complements
these synthetic fixtures. Representative gameplay with the intended AI/target count is still needed
for a production maximum-frame/spike comparison.

## Automatic lifecycle verification (2026-10-21)

Editor and Win64 Development game builds passed. `ShootingArena.PathLink.BakedRouting` passed,
including UObject serialization and migration from the legacy actor. On PortalMap, first PIE migrated
910 polygons / 210 islands without another bake. Clearing the managed cache then starting PIE
triggered an automatic 0.640-second bake. Moving a runtime PathLink invalidated the estimate
immediately. The 42 baseline detailed routes remained identical (`Saved/PathLinkBench/managed.json`);
41 estimates succeeded with zero synchronous path searches and unchanged approximation errors.

The legacy actor was removed from PortalMap after migration. Saving/reloading retained the managed
component and tables without an actor. Navigation changed on reload in this map's dynamic navigation
configuration: cold validation correctly rejected that signature, and the next PIE automatically baked
again in 0.609 seconds. The PIE authority validated the actor-free cache. The final map was saved.
A cooked-package execution has not been performed; the non-editor game target compiles successfully.

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

1. Fully load the gameplay level, its navigation tiles and PathLinks; build Navigation and wait for completion.
2. Place one native **PathLinkRoutingCache** actor in the world. It is non-spatial in World Partition.
3. Choose the Recast NavigationData, or let **Analyze Navigation** select the default.
4. Inspect Details and Output Log: ordinary traversable ground polygons, connected islands, per-island
   counts, largest island, float32 directed-table MiB, and sampled bake-time estimate.
5. **Bake Routing Cache** enforces memory/time budgets (128MiB / 120 seconds by default), supports
   cancellation, records actual time, and refuses oversized N² arrays before allocating them.
6. Save the cache actor/map. These are regular serialized UPROPERTY fields, not editor-only data.
   Cook the map normally. No external Saved-directory file is required at runtime.

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

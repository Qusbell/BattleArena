"""Run through Unreal MCP after BenchmarkPathLinkLazySearch.py ('after'). Independent all-exact oracle."""
import heapq
import json
import math
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name() == "/Game/QuakeLike_Base/Lvl_PortalMap.Lvl_PortalMap"
assert not unreal.NavigationSystemV1.is_navigation_being_built_or_locked(world), "Wait for editor NavMesh initialization"
sub = next(o for o in unreal.ObjectIterator(unreal.PathLinkSubsystem) if o.get_world() == world)
folder = Path(unreal.Paths.project_saved_dir()) / "PathLinkBench"
data = json.loads((folder / "LazySearch_after.json").read_text(encoding="utf-8"))

def xyz(v): return (v.x, v.y, v.z)
projections, ground = {}, {}
def project(v):
    key = xyz(v)
    if key not in projections:
        p = unreal.NavigationSystemV1.project_point_to_navigation(world, v, None, None, unreal.Vector(150,150,400))
        if p is None: raise RuntimeError("Projection failed")
        projections[key] = p
    return projections[key]
def normal(a, b):
    a, b = project(a), project(b)
    if all(abs(x-y)<=1 for x,y in zip(xyz(a),xyz(b))): return 0.0
    key = xyz(a)+xyz(b)
    if key not in ground:
        p = unreal.NavigationSystemV1.find_path_to_location_synchronously(world, a, b)
        ground[key] = p.get_path_length() if p and p.is_valid() and not p.is_partial() else math.inf
    return ground[key]

traversals = []
for link in sub.get_enabled_links():
    a, b = link.get_entry_location(), link.get_exit_location()
    for reverse in ([False,True] if link.is_two_way() else [False]):
        entry, exit = (b,a) if reverse else (a,b)
        traversals.append(dict(link=link,label=link.get_actor_label(),reverse=reverse,entry=entry,exit=exit,
                               cost=link.get_travel_distance(reverse)))
n = len(traversals)
transition = [[math.inf]*n for _ in range(n)]
for i,a in enumerate(traversals):
    for j,b in enumerate(traversals):
        if a["link"] != b["link"]: transition[i][j] = normal(a["exit"], b["entry"])

def exhaustive(a, b):
    starts = [normal(a,t["entry"])+t["cost"] for t in traversals]
    ends = [normal(t["exit"],b) for t in traversals]
    distances, previous = list(starts), [-1]*n
    best, last = normal(a,b), -1
    queue = [(v,i) for i,v in enumerate(starts) if math.isfinite(v)]
    heapq.heapify(queue)
    while queue:
        distance,i = heapq.heappop(queue)
        if distance != distances[i] or distance >= best: continue
        if distance+ends[i] < best: best, last = distance+ends[i], i
        for j,t in enumerate(traversals):
            value = distance+transition[i][j]+t["cost"]
            if value < distances[j]:
                distances[j], previous[j] = value, i
                heapq.heappush(queue,(value,j))
    chain = []
    while last >= 0: chain.append(last); last = previous[last]
    chain.reverse()
    return best, [traversals[i]["label"] for i in chain]

results = []
for row in data["cases"]:
    a,b = unreal.Vector(*row["a"]),unreal.Vector(*row["b"])
    best,links = exhaustive(a,b)
    results.append(dict(group=row["group"],sample=row["sample"],reference_cm=best,
                        detailed_cm=row["distance_cm"],delta_cm=row["distance_cm"]-best,reference_links=links))

# One generated native graph is used throughout; verify native navigation links are absent.
native = [a.get_path_name() for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
          if a.get_components_by_class(unreal.NavLinkComponent) or a.get_components_by_class(unreal.NavLinkCustomComponent)]
assert not native, native
output = dict(cases=results,normal_queries=len(ground),native_link_actors=native,
              mismatches=sum(abs(r["delta_cm"])>.01 for r in results),
              max_abs_delta_cm=max(abs(r["delta_cm"]) for r in results))
(folder / "LazySearch_ExactReference.json").write_text(json.dumps(output,indent=2),encoding="utf-8")
print(json.dumps({k:v for k,v in output.items() if k!="cases"}))
assert output["mismatches"] == 0, "Detailed routes differ from exhaustive reference"

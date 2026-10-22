"""Run through Unreal MCP. Replay four outliers plus 200 fresh fixed playable pairs before/after C++ change."""
import csv
import json
import statistics as st
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name() == "/Game/QuakeLike_Base/Lvl_PortalMap.Lvl_PortalMap"
assert not unreal.NavigationSystemV1.is_navigation_being_built_or_locked(world), "Wait for editor NavMesh initialization"
sub = next(o for o in unreal.ObjectIterator(unreal.PathLinkSubsystem) if o.get_world() == world)
folder = Path(unreal.Paths.project_saved_dir()) / "PathLinkBench"
phase = globals().get("PATHLINK_LAZY_PHASE", "before")
assert phase in ("before", "after")
old = list(csv.DictReader((folder / "PlayableDirectionSweep200.csv").open(encoding="utf-8-sig")))
fresh = list(csv.DictReader((folder / "Lvl_PortalMap_PlayableRoutes200_Seed20261026.csv").open(encoding="utf-8-sig")))
outliers = {r["sample"]: r for r in old if r["sample"] in ("31", "74", "141", "181")}
new_pairs = {r["sample"]: r for r in fresh}
assert len(outliers) == 4 and len(new_pairs) == 200
sub.refresh_links()

def xyz(v): return [v.x, v.y, v.z]
def signature(route):
    return [route.success, route.total_distance,
            [[str(s.segment_type), xyz(s.start_location), xyz(s.end_location), s.distance,
              s.link.get_path_name() if s.link else None, s.reverse, [xyz(p) for p in s.path_points]] for s in route.segments]]

measurements = []
for group, pairs in (("outlier", outliers), ("fresh", new_pairs)):
    for sample, row in pairs.items():
        a = unreal.Vector(*(float(row[k]) for k in ("ax", "ay", "az")))
        b = unreal.Vector(*(float(row[k]) for k in ("bx", "by", "bz")))
        route = sub.find_shortest_route(a, b)  # warm up outside measured repetitions
        assert route and route.success, (group, sample)
        expected = signature(route)
        stats = []
        for _ in range(3):
            check = sub.find_shortest_route(a, b)
            assert signature(check) == expected, ("nondeterministic", group, sample)
            s = sub.get_last_route_stats()
            stats.append(dict(us=s.total_milliseconds*1000, queries=s.synchronous_nav_queries,
                              candidate_us=s.candidate_milliseconds*1000, solve_us=s.dijkstra_milliseconds*1000,
                              points_us=s.path_points_milliseconds*1000, solves=s.solve_count))
        estimate = sub.get_estimated_route_distance(a, b)
        es = sub.get_last_estimate_stats()
        distance = sub.get_route_distance(a, b)
        ds = sub.get_last_route_stats()
        assert abs(distance-route.total_distance) < .001, ("distance-only mismatch", group, sample)
        assert ds.path_points_milliseconds == 0, ("distance-only points", group, sample)
        measurements.append(dict(group=group, sample=int(sample), a=xyz(a), b=xyz(b),
             distance_cm=route.total_distance, estimate_cm=estimate,
             links=[l.get_actor_label() for l in route.used_links],
             reverse=[s.reverse for s in route.segments if s.link],
             link_types=[str(s.link_type) for s in route.segments if s.link],
             stats=stats, estimate_us=es.total_milliseconds*1000, estimate_queries=es.synchronous_nav_queries,
             distance_only_us=ds.total_milliseconds*1000, distance_only_queries=ds.synchronous_nav_queries))

output = folder / f"LazySearch_{phase}.json"
output.write_text(json.dumps(dict(phase=phase, seed=20261026, cases=measurements), indent=2), encoding="utf-8")
print(json.dumps(dict(phase=phase, count=len(measurements), output=str(output),
                     outliers=[{k:r[k] for k in ("sample","distance_cm","estimate_cm","links")} for r in measurements[:4]],
                     mean_us=st.mean(s["us"] for r in measurements[4:] for s in r["stats"]),
                     mean_queries=st.mean(s["queries"] for r in measurements[4:] for s in r["stats"]))))

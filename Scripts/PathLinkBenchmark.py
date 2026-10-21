"""Run through Unreal MCP execute_python in an editor world. No asset/Blueprint edits.

Set PATHLINK_BENCH_OUTPUT and optionally PATHLINK_BENCH_BASELINE in the executing globals.
Records full route outputs for comparison as well as per-request times. Frame spikes must be
measured separately in representative PIE/gameplay using Unreal Insights and subsystem counters.
"""
import json
import os
import statistics
import time
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
subsystem = next(o for o in unreal.ObjectIterator(unreal.PathLinkSubsystem) if o.get_outer() == world)
subsystem.refresh_links()
links = sorted(subsystem.get_enabled_links(), key=lambda a: a.get_path_name())

def xyz(v):
    return [v.x, v.y, v.z]

def encode(result):
    if result is None:
        return None
    return {"success": result.success, "distance": result.total_distance,
            "segments": [{"type": str(s.segment_type), "start": xyz(s.start_location),
                          "end": xyz(s.end_location), "distance": s.distance,
                          "link": s.link.get_name() if s.link else None, "reverse": s.reverse,
                          "points": [xyz(p) for p in s.path_points]} for s in result.segments]}

baseline_path = globals().get("PATHLINK_BENCH_BASELINE")
if baseline_path:
    with open(baseline_path, encoding="utf-8") as source:
        baseline = json.load(source)
    pairs = [(row["start"], row["target"]) for row in baseline["routes"]]
else:
    anchors = []
    for link in links:
        anchors.extend([xyz(link.get_entry_location()), xyz(link.get_exit_location())])
    pairs = []
    for i, anchor in enumerate(anchors):
        pairs.append((anchor, anchors[(i * 7 + 5) % len(anchors)]))
        # Targets offset from authored anchors model arbitrary dropped-item coordinates.
        target = anchors[(i * 3 + 1) % len(anchors)]
        pairs.append((anchor, [target[0] + 37, target[1] - 21, target[2] + 15]))
    if anchors:
        pairs.extend([(anchors[0], anchors[0]), (anchors[0], [10000000, 10000000, -10000000])])
    pairs = pairs[:130]

routes = []
for start, target in pairs:
    a, b = unreal.Vector(*start), unreal.Vector(*target)
    begin = time.perf_counter()
    result = subsystem.find_shortest_route(a, b)
    elapsed = (time.perf_counter() - begin) * 1000
    row = {"start": start, "target": target, "route": encode(result), "route_ms": elapsed}
    if hasattr(subsystem, "get_last_route_stats"):
        stats = subsystem.get_last_route_stats()
        row.update(detail_queries=stats.synchronous_nav_queries,
                   candidate_ms=stats.candidate_milliseconds,
                   dijkstra_ms=stats.dijkstra_milliseconds,
                   points_ms=stats.path_points_milliseconds,
                   solves=stats.solve_count)
    begin = time.perf_counter()
    row["distance_only"] = subsystem.get_route_distance(a, b)
    row["distance_ms"] = (time.perf_counter() - begin) * 1000
    if hasattr(subsystem, "get_last_route_stats"):
        stats = subsystem.get_last_route_stats()
        row["distance_queries"] = stats.synchronous_nav_queries
        row["distance_points_ms"] = stats.path_points_milliseconds
    if hasattr(subsystem, "get_estimated_route_distance"):
        begin = time.perf_counter()
        row["estimate"] = subsystem.get_estimated_route_distance(a, b)
        row["estimate_ms"] = (time.perf_counter() - begin) * 1000
        row["estimate_queries"] = subsystem.get_last_estimate_stats().synchronous_nav_queries
    routes.append(row)

report = {"world": world.get_path_name(), "enabled_links": len(links), "routes": routes}
for key in ("route_ms", "distance_ms", "estimate_ms", "detail_queries", "distance_queries",
            "candidate_ms", "dijkstra_ms", "points_ms", "estimate_queries"):
    values = [r[key] for r in routes if key in r]
    if values:
        report[key] = {"mean": statistics.mean(values), "max": max(values), "count": len(values)}
if baseline_path:
    mismatches = []
    for i, (old, new) in enumerate(zip(baseline["routes"], routes)):
        if old["route"] != new["route"]:
            mismatches.append(i)
    report["route_output_mismatches"] = mismatches
errors = []
for row in routes:
    if row.get("estimate") is not None and row["route"] and row["route"]["success"]:
        actual = row["route"]["distance"]
        absolute = abs(row["estimate"] - actual)
        errors.append((absolute, absolute / max(1.0, actual)))
if errors:
    report["estimate_error"] = {"samples": len(errors), "mean_absolute_cm": statistics.mean(e[0] for e in errors),
                                "max_absolute_cm": max(e[0] for e in errors),
                                "mean_relative": statistics.mean(e[1] for e in errors), "max_relative": max(e[1] for e in errors)}
if any("estimate" in row for row in routes):
    report["estimate_successes"] = sum(row.get("estimate") is not None for row in routes)
    report["estimate_false_negatives"] = [i for i, row in enumerate(routes) if row.get("estimate") is None
                                            and row["route"] and row["route"]["success"]]
    report["estimate_false_positives"] = [i for i, row in enumerate(routes) if row.get("estimate") is not None
                                            and (not row["route"] or not row["route"]["success"])]
output = globals().get("PATHLINK_BENCH_OUTPUT", unreal.Paths.project_saved_dir() + "PathLinkBenchmark.json")
os.makedirs(os.path.dirname(output), exist_ok=True)
with open(output, "w", encoding="utf-8") as destination:
    json.dump(report, destination, indent=2, ensure_ascii=False)
print(json.dumps({k: v for k, v in report.items() if k != "routes"}, ensure_ascii=False))
print("PATHLINK_BENCHMARK", output)

"""Run via Unreal MCP. Compare directional offset correction on the fixed 200 playable pairs."""
import csv
import json
import math
import statistics as st
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if world.get_path_name() != "/Game/QuakeLike_Base/Lvl_PortalMap.Lvl_PortalMap":
    raise RuntimeError("Load PortalMap first")
sub = next(o for o in unreal.ObjectIterator(unreal.PathLinkSubsystem) if o.get_world() == world)
folder = Path(unreal.Paths.project_saved_dir()) / "PathLinkBench"
source_name=globals().get("PATHLINK_DIRECTION_SOURCE","Lvl_PortalMap_PlayableRoutes200_Seed20261024.csv")
output_stem=globals().get("PATHLINK_DIRECTION_STEM","PlayableDirectionSweep200")
source = list(csv.DictReader((folder / source_name).open(encoding="utf-8-sig")))
pairs = list({r["sample"]:r for r in source}.values())
assert len(pairs) == 200 and len({r["sample"] for r in pairs}) == 200
strength_name, range_name = "PathLink.EstimateBlendStrength", "PathLink.EstimateBlendRangeCm"
old_strength = unreal.SystemLibrary.get_console_variable_float_value(strength_name)
old_range = unreal.SystemLibrary.get_console_variable_float_value(range_name)
direction_name="PathLink.EstimateDirectionStrength"
old_direction=unreal.SystemLibrary.get_console_variable_float_value(direction_name)
sub.refresh_links()

def set_controls(strength, distance, direction=0):
    unreal.SystemLibrary.execute_console_command(world,f"{direction_name} {direction:.9g}")
    unreal.SystemLibrary.execute_console_command(world, f"{strength_name} {strength:.9g}")
    unreal.SystemLibrary.execute_console_command(world, f"{range_name} {distance:.9g}")
    if abs(unreal.SystemLibrary.get_console_variable_float_value(strength_name) - strength) > 1e-6:
        raise RuntimeError("Strength control failed")
    if abs(unreal.SystemLibrary.get_console_variable_float_value(range_name) - distance) > 1e-6:
        raise RuntimeError("Range control failed")

def endpoints(row):
    return unreal.Vector(*(float(row[k]) for k in ("ax","ay","az"))), unreal.Vector(*(float(row[k]) for k in ("bx","by","bz")))

def xyz(v):
    return (v.x, v.y, v.z)

def route_value(route):
    if route is None:
        return None
    return (route.success, route.total_distance,
            tuple((str(s.segment_type), xyz(s.start_location), xyz(s.end_location), s.distance,
                   s.link.get_path_name() if s.link else None, s.reverse, tuple(xyz(p) for p in s.path_points))
                  for s in route.segments), tuple(link.get_path_name() for link in route.used_links))

configs = globals().get("PATHLINK_DIRECTION_CONFIGS",[(0,1,4000)] + [(d,s,4000) for s in (0,1) for d in (.25,.5,.75,1)])
raw, summary, reference = [], [], []
reference_mismatches, detailed_mismatches = [], []
try:
    set_controls(0, 2000)
    for row in pairs:
        a,b = endpoints(row)
        route = sub.find_shortest_route(a,b)
        if route is None or not route.success:
            raise RuntimeError(f"Previously reachable sample {row['sample']} now fails; abort rather than replace it")
        if abs(route.total_distance-float(row["exact_cm"])) > .001:
            reference_mismatches.append(int(row["sample"]))
        reference.append(route_value(route))
    if reference_mismatches:
        raise RuntimeError(f"Detailed reference changed since prior test: {reference_mismatches}")

    for direction, strength, distance in configs:
        set_controls(strength, distance, direction)
        measurements = []
        for row in pairs:
            a,b = endpoints(row)
            estimate = sub.get_estimated_route_distance(a,b)
            stats = sub.get_last_estimate_stats()
            item = dict(row, direction_strength=direction, strength=strength, range_cm=distance, estimate_success=int(estimate is not None),
                        estimate_cm=estimate if estimate is not None else 0,
                        estimate_us=stats.total_milliseconds*1000, path_queries=stats.synchronous_nav_queries)
            raw.append(item)
            measurements.append(item)
        valid = [x for x in measurements if x["estimate_success"] and float(x["exact_cm"]) > 1]
        errors = [(float(x["estimate_cm"])-float(x["exact_cm"]))/100 for x in valid]
        relative = [e/(float(x["exact_cm"])/100)*100 for e,x in zip(errors,valid)]
        absolute_pct = sorted(abs(e) for e in relative)
        worst_over = max(valid,key=lambda x:float(x["estimate_cm"])-float(x["exact_cm"]))
        worst_under = min(valid,key=lambda x:float(x["estimate_cm"])-float(x["exact_cm"]))
        result = dict(direction_strength=direction,strength=strength,range_m=distance/100,successes=len(valid),
                      mean_absolute_m=st.mean(abs(e) for e in errors),
                      mean_absolute_pct=st.mean(absolute_pct),median_absolute_pct=st.median(absolute_pct),
                      p95_absolute_pct=absolute_pct[math.ceil(.95*len(absolute_pct))-1],
                      max_over_m=max([0]+errors),max_under_m=-min([0]+errors),
                      max_over_pct=max([0]+relative),max_under_pct=-min([0]+relative),
                      over10=sum(e>10 for e in relative),under10=sum(e < -10 for e in relative),
                      worst_over_sample=int(worst_over["sample"]),worst_under_sample=int(worst_under["sample"]),
                      path_queries=sum(int(x["path_queries"]) for x in measurements),
                      mean_estimate_us=st.mean(float(x["estimate_us"]) for x in measurements))
        summary.append(result)

    # The last configuration is the strongest/longest tested. Compare complete detailed outputs.
    for i,row in enumerate(pairs):
        a,b = endpoints(row)
        if route_value(sub.find_shortest_route(a,b)) != reference[i]:
            detailed_mismatches.append(int(row["sample"]))
finally:
    set_controls(old_strength,old_range,old_direction)

with (folder/(output_stem+".csv")).open("w",newline="",encoding="utf-8") as out:
    writer=csv.DictWriter(out,fieldnames=list(raw[0])); writer.writeheader(); writer.writerows(raw)
report=dict(source=source_name,world=world.get_path_name(),pairs=200,configurations=len(configs),
            reference_mismatches=reference_mismatches,detailed_mismatches=detailed_mismatches,
            restored_strength=old_strength,restored_range_cm=old_range,restored_direction=old_direction,results=summary)
(folder/(output_stem+"Summary.json")).write_text(json.dumps(report,indent=2),encoding="utf-8")
print(json.dumps(report))

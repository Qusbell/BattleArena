"""Run through Unreal MCP. Read-only graph reconstruction; temporary candidate-setting probes restored in finally."""
import csv
import heapq
import json
import math
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name()=="/Game/QuakeLike_Base/Lvl_PortalMap.Lvl_PortalMap"
sub=next(o for o in unreal.ObjectIterator(unreal.PathLinkSubsystem) if o.get_world()==world)
sub.refresh_links()
folder=Path(unreal.Paths.project_saved_dir())/"PathLinkBench"
rows=[r for r in csv.DictReader((folder/"PlayableDirectionSweep200.csv").open())
      if float(r["direction_strength"])==1 and float(r["strength"])==0 and r["sample"] in ("31","74","141","181")]

def vec(v):return unreal.Vector(*v)
def xyz(v):return [v.x,v.y,v.z]
def project(v):
    p=unreal.NavigationSystemV1.project_point_to_navigation(world,v,None,None,unreal.Vector(150,150,400))
    if p is None:raise RuntimeError("Projection failed")
    return p
path_cache={}
def normal(a,b):
    a,b=project(a),project(b)
    if all(abs(x-y)<=1 for x,y in zip(xyz(a),xyz(b))):return 0.0
    key=tuple(xyz(a)+xyz(b))
    if key not in path_cache:
        path=unreal.NavigationSystemV1.find_path_to_location_synchronously(world,a,b)
        path_cache[key]=path.get_path_length() if path and path.is_valid() and not path.is_partial() else math.inf
    return path_cache[key]

traversals=[]
for link in sub.get_enabled_links():
    a,b=link.get_entry_location(),link.get_exit_location()
    for reverse in ([False,True] if link.is_two_way() else [False]):
        entry,exit=(b,a) if reverse else (a,b)
        traversals.append(dict(link=link,label=link.get_actor_label(),reverse=reverse,entry=entry,exit=exit,
                               cost=link.get_travel_distance(reverse)))
n=len(traversals)
transitions=[[math.inf]*n for _ in range(n)]
for i,a in enumerate(traversals):
    for j,b in enumerate(traversals):
        if a["link"]!=b["link"]:transitions[i][j]=normal(a["exit"],b["entry"])

def exact_all(a,b):
    distances=[normal(a,t["entry"])+t["cost"] for t in traversals]
    previous=[-1]*n
    direct=normal(a,b);best=direct;last=-1
    queue=[(v,i) for i,v in enumerate(distances) if math.isfinite(v)];heapq.heapify(queue)
    while queue:
        distance,i=heapq.heappop(queue)
        if distance!=distances[i] or distance>=best:continue
        end=normal(traversals[i]["exit"],b)
        if distance+end<best:best=distance+end;last=i
        for j,t in enumerate(traversals):
            new=distance+transitions[i][j]+t["cost"]
            if new<distances[j]:distances[j]=new;previous[j]=i;heapq.heappush(queue,(new,j))
    chain=[]
    while last>=0:chain.append(last);last=previous[last]
    chain.reverse()
    legs=[];start=a
    for index in chain:
        t=traversals[index]
        legs.append(dict(kind="normal",from_cm=xyz(start),to_cm=xyz(t["entry"]),cm=normal(start,t["entry"])))
        legs.append(dict(kind="link",label=t["label"],reverse=t["reverse"],from_cm=xyz(t["entry"]),to_cm=xyz(t["exit"]),cm=t["cost"]))
        start=t["exit"]
    legs.append(dict(kind="normal",from_cm=xyz(start),to_cm=xyz(b),cm=normal(start,b)))
    return dict(distance_cm=best,links=[traversals[i]["label"] for i in chain],indices=chain,legs=legs)

settings=unreal.get_default_object(unreal.load_class(None,"/Script/ShootingArena.PathLinkSettings"))
old_count=settings.get_editor_property("DynamicCandidateCount")
old_multiplier=settings.get_editor_property("BlockedTraceDistanceMultiplier")
report=dict(world=world.get_path_name(),traversals=n,original_candidates=old_count,cases=[])
try:
    for row in rows:
        a,b=(vec([float(row[k]) for k in keys]) for keys in (("ax","ay","az"),("bx","by","bz")))
        original=sub.find_shortest_route(a,b)
        checks=[abs(normal(s.start_location,s.end_location)-s.distance) for s in original.segments if not s.link]
        if max(checks,default=0)>.01:raise RuntimeError(f"Ground path reference mismatch: {checks}")
        exact=exact_all(a,b)
        settings.set_editor_property("DynamicCandidateCount",max(32,n))
        expanded=sub.find_shortest_route(a,b)
        settings.set_editor_property("BlockedTraceDistanceMultiplier",1.0)
        no_penalty=sub.find_shortest_route(a,b)
        settings.set_editor_property("BlockedTraceDistanceMultiplier",old_multiplier)
        settings.set_editor_property("DynamicCandidateCount",old_count)
        ranks=[]
        for start_side in (True,False):
            anchor=a if start_side else b
            candidates=sorted(sub.get_enabled_links(),key=lambda link:min(
                math.dist(xyz(anchor),xyz(endpoint)) for endpoint in
                ([link.get_entry_location(),link.get_exit_location()] if link.is_two_way() else
                 [link.get_entry_location() if start_side else link.get_exit_location()])))
            ranks.append([dict(rank=i+1,label=link.get_actor_label()) for i,link in enumerate(candidates)])
        report["cases"].append(dict(sample=int(row["sample"]),estimate_cm=sub.get_estimated_route_distance(a,b),
             detailed_cm=original.total_distance,detailed_links=[l.get_actor_label() for l in original.used_links],
             all_candidates_detailed_cm=expanded.total_distance,all_candidates_links=[l.get_actor_label() for l in expanded.used_links],
             all_candidates_no_penalty_cm=no_penalty.total_distance,all_candidates_no_penalty_links=[l.get_actor_label() for l in no_penalty.used_links],
             exact_all=exact,start_ranking=ranks[0],target_ranking=ranks[1],ground_reference_max_delta_cm=max(checks,default=0)))
finally:
    settings.set_editor_property("DynamicCandidateCount",old_count)
    settings.set_editor_property("BlockedTraceDistanceMultiplier",old_multiplier)
report["normal_path_queries"]=len(path_cache)
(folder/"OutlierExactGraphDiagnosis.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
print(json.dumps({**report,"cases":[{k:v for k,v in c.items() if k not in ("start_ranking","target_ranking","exact_all")}|
    {"exact_all_cm":c["exact_all"]["distance_cm"],"exact_all_links":c["exact_all"]["links"]} for c in report["cases"]]}))

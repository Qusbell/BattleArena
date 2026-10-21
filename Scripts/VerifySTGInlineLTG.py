"""Run only via Unreal MCP. Invoke the actual Blueprint filter on transient actors, then clean up."""
import json
from pathlib import Path
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
sub = next(o for o in unreal.ObjectIterator(unreal.PathLinkSubsystem) if o.get_world() == world)
folder = Path(unreal.Paths.project_saved_dir()) / "PathLinkBench"
data = json.loads((folder / "LazySearch_after.json").read_text(encoding="utf-8"))
pairs = [r for r in data["cases"] if r["group"] == "fresh"]
temporary = []
checks = []
sys = unreal.get_default_object(unreal.SystemLibrary.static_class())

def spawn(cls):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(0,0,-5000), transient=True)
    if not actor: raise RuntimeError("Test actor spawn failed")
    temporary.append(actor)
    return actor

def assign(actor, name, value):
    sys.call_method("SetObjectPropertyByName", (actor,name,value))

def move(actor, location):
    actor.set_actor_location(unreal.Vector(*location), False, True)

def selected(controller, goals):
    values = controller.call_method("Filtering STG", (goals,))
    return list(values[1])  # Blueprint input array is a reference; tuple contains Goals and Result.

try:
    c = spawn(unreal.load_class(None,"/Game/QuakeLike_1_0/AI/AIC/BP_QuakeAIController2.BP_QuakeAIController2_C"))
    p = spawn(unreal.load_class(None,"/Game/QuakeLike_1_0/Character/BP_ShooterBase.BP_ShooterBase_C"))
    p.set_actor_enable_collision(False)
    ltg, stg = spawn(unreal.Actor), spawn(unreal.Actor)
    assign(c,"MyCharacter",p)
    assign(c,"LTG",ltg)
    # Isolate the LTG comparison; arrival-time behavior itself is the user's existing section.
    sys.call_method("SetDoublePropertyByName", (c,"TravelTimeLimit",100000.0))
    assert c.get_editor_property("TravelTimeLimit") == 100000.0
    for i in range(60):
        row = pairs[i]
        move(p,row["a"]); move(ltg,row["b"]); move(stg,pairs[(i*17+3)%len(pairs)]["a"])
        a,b,t = p.get_actor_location(),stg.get_actor_location(),ltg.get_actor_location()
        ab,at,bt = (sub.get_estimated_route_distance(x,y) for x,y in ((a,b),(a,t),(b,t)))
        expected = all(v is not None for v in (ab,at,bt)) and ab<at and bt<at
        actual = selected(c,[stg]) == [stg]
        checks.append(dict(case=i,expected=expected,actual=actual,bot_stg_cm=ab,bot_ltg_cm=at,stg_ltg_cm=bt))
        assert actual == expected, checks[-1]
    accepted = sum(r["actual"] for r in checks)
    assert 0<accepted<len(checks), "Need both accepted and rejected samples"
    assert selected(c,[]) == []
    assign(c,"LTG",None)
    assert selected(c,[stg]) == [], "Missing LTG must reject"
    assign(c,"LTG",ltg)
    move(stg,[0,0,-50000])
    assert sub.get_estimated_route_distance(p.get_actor_location(),stg.get_actor_location()) is None
    assert selected(c,[stg]) == [], "Failed estimate must not be treated as zero distance"
    report = dict(cases=checks,accepted=accepted,rejected=len(checks)-accepted,mismatches=0,
                  empty_list=True,missing_ltg=True,failed_estimate=True)
    (folder/"FilteringSTGInlineLTGVerification.json").write_text(json.dumps(report,indent=2),encoding="utf-8")
    print(json.dumps({k:v for k,v in report.items() if k!="cases"}))
finally:
    for actor in reversed(temporary):
        if actor: actors.destroy_actor(actor)

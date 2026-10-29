"""Read-only saved-package verification in a separate Unreal Python commandlet.

Set PORTAL_COLLISION_REPORT to the simplification report's absolute filename.
Run with UnrealEditor-Cmd <project> -run=pythonscript -script=<this file>
-unattended -nullrhi. The interactive editor's unsaved work is not reloaded.
"""

import json
import os
import traceback

import unreal


def vector(v):
    return unreal.Vector(*v)


def main(report):
    world = unreal.EditorLoadingAndSavingUtils.load_map(report["map"].split(".")[0])
    if not world:
        raise RuntimeError("Could not load saved PortalMap")
    actors = {a.get_path_name(): a for a in
              unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()}
    failures = []
    for plan in report["proxies"]:
        actor = actors.get(plan["proxy_actor"])
        c = actor.get_component_by_class(unreal.InstancedStaticMeshComponent) if actor else None
        if not c or plan["instance_index"] >= c.get_instance_count():
            failures.append({"kind": "missing_instance", "plan": plan["id"]})
            continue
        t = c.get_instance_transform(plan["instance_index"], True)
        p, y, r = plan["rotation"]
        q = unreal.Rotator(pitch=p, yaw=y, roll=r).quaternion()
        rotation_error = 1-abs(sum(getattr(q,k)*getattr(t.rotation,k) for k in "xyzw"))
        if (t.translation-vector(plan["location"])).length() > .01 \
                or (t.scale3d-vector(plan["scale"])).length() > .001 or rotation_error > .000001:
            failures.append({"kind": "instance_transform", "plan": plan["id"]})
        if c.get_collision_enabled() != unreal.CollisionEnabled.QUERY_AND_PHYSICS \
                or str(c.get_collision_profile_name()) != "BlockAll":
            failures.append({"kind": "instance_collision", "actor": actor.get_path_name()})
        if c.static_mesh.get_path_name() != "/Engine/BasicShapes/Cube.Cube":
            failures.append({"kind": "instance_mesh", "actor": actor.get_path_name()})
    for state in report.get('preserved_sources',[]):
        actor=actors.get(state['actor'])
        if not actor:
            failures.append({'kind':'preserved_source_missing','actor':state['actor']})
            continue
        t=actor.get_actor_transform();expected=state['transform'];p,y,r=expected['rotation']
        q=unreal.Rotator(pitch=p,yaw=y,roll=r).quaternion()
        if (t.translation-vector(expected['location'])).length()>.001 \
                or (t.scale3d-vector(expected['scale'])).length()>.00001 \
                or 1-abs(sum(getattr(q,k)*getattr(t.rotation,k) for k in 'xyzw'))>.000001 \
                or actor.static_mesh_component.static_mesh.get_path_name()!=state['mesh'] \
                or actor.static_mesh_component.get_collision_enabled().value!=state['collision_enabled']:
            failures.append({'kind':'preserved_source_changed','actor':state['actor']})
    for path in report.get("retired",[]):
        actor = actors.get(path)
        c = actor.get_component_by_class(unreal.InstancedStaticMeshComponent) if actor else None
        if c and (c.get_instance_count() or c.get_collision_enabled() != unreal.CollisionEnabled.NO_COLLISION):
            failures.append({"kind": "retired_collision", "actor": path})
    for path, state in report["baseline_components"].items():
        c = unreal.find_object(None, state["actor"]+"."+state["component"])
        expected = state.get('expected_enabled', unreal.CollisionEnabled.NO_COLLISION.value)
        if not c or c.get_collision_enabled().value != expected:
            failures.append({"kind": "source_collision", "component": path})
        if c and state.get('expected_profile') and str(c.get_collision_profile_name()) != state['expected_profile']:
            failures.append({"kind":"source_profile","component":path})
        if c and 'expected_navigation_relevant' in state and \
                c.get_editor_property('can_ever_affect_navigation')!=state['expected_navigation_relevant']:
            failures.append({'kind':'source_navigation_relevance','component':path})
        if c and state.get('expected_no_character_base') and (
                c.get_walkable_slope_override().walkable_slope_behavior != unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_UNWALKABLE
                or c.get_editor_property('can_character_step_up_on') != unreal.CanBeCharacterBase.ECB_NO):
            failures.append({"kind":"native_wall_base","component":path})
    if 'wall_no_character_base' in report['validation']:
        wall_components = {p['proxy_actor'] for p in report['proxies'] if p['kind']=='Wall'}
        for path in wall_components:
            actor=actors.get(path)
            c=actor.get_component_by_class(unreal.InstancedStaticMeshComponent) if actor else None
            if not c or c.get_walkable_slope_override().walkable_slope_behavior != unreal.WalkableSlopeBehavior.WALKABLE_SLOPE_UNWALKABLE \
                    or c.get_editor_property('can_character_step_up_on') != unreal.CanBeCharacterBase.ECB_NO:
                failures.append({'kind':'wall_character_base','actor':path})
    gameplay = report["validation"].get("gameplay_preservation",{}).get("before",{})
    editor_only = set(report["validation"].get("gameplay_preservation",{}).get("editor_only_components", []))
    skipped_editor_only = []
    for path, before in gameplay.items():
        c = unreal.find_object(None, path)
        if c is None and path in editor_only and before[0] == str(unreal.CollisionEnabled.NO_COLLISION):
            skipped_editor_only.append(path)
            continue
        current = [str(c.get_collision_enabled()),str(c.get_collision_profile_name()),
                   str(c.get_collision_response_to_channel(unreal.CollisionChannel.ECC_PAWN)),
                   c.get_editor_property("generate_overlap_events")] if c else None
        if current != before:
            failures.append({"kind": "gameplay_component", "component": path, "actual": current})
    ray_count = 0
    for sample in report["validation"].get("expected_wall_samples",[]):
        p, n = vector(sample["center"]), vector(sample["normal"])
        for channel in ("Pawn", "Projectile", "RailGun"):
            if channel == "RailGun":
                hit = unreal.SystemLibrary.line_trace_single(world,p+n*30,p-n*30,
                    unreal.TraceTypeQuery.ECC_RAIL_GUN_TRACE_CHANNEL,False,[],unreal.DrawDebugTrace.NONE)
            else:
                hit = unreal.SystemLibrary.line_trace_single_by_profile(world,p+n*30,p-n*30,
                    channel,False,[],unreal.DrawDebugTrace.NONE)
            ray_count += 1
            if not hit:
                failures.append({"kind": "wall_ray", "center": sample["center"], "channel": channel})
    nav_failures = []
    for case in report["validation"]["navigation_paths"]:
        path = unreal.NavigationSystemV1.find_path_to_location_synchronously(
            world,vector(case["ends"][0]),vector(case["ends"][1]))
        if not path or not path.is_valid() or path.is_partial():
            nav_failures.append(case.get("name",str(case.get('lane'))))
    failures.extend({"kind":"navigation_path","name":name} for name in nav_failures)
    walking_links=report['validation'].get('walking_nav_links',[])
    for state in walking_links:
        actor=actors.get(state['actor'])
        if not isinstance(actor,unreal.NavLinkProxy):
            failures.append({'kind':'walking_link_actor','actor':state['actor']});continue
        actual=list(actor.get_editor_property('point_links'))
        if actor.get_editor_property('smart_link_is_relevant') or actor.is_smart_link_enabled() \
                or len(actual)!=len(state['links']):
            failures.append({'kind':'walking_link_mode','actor':state['actor']})
        for link,expected in zip(actual,state['links']):
            if (link.get_editor_property('left')-vector(expected['left'])).length()>.01 \
                    or (link.get_editor_property('right')-vector(expected['right'])).length()>.01 \
                    or link.get_editor_property('direction')!=unreal.NavLinkDirection.BOTH_WAYS \
                    or link.get_editor_property('area_class')!=unreal.NavArea_Default.static_class() \
                    or link.get_editor_property('max_fall_down_length')!=0.:
                failures.append({'kind':'walking_link_settings','actor':state['actor']})
    dense = report['validation'].get('dense_stair_navigation',{})
    for sample in dense.get('samples',[]):
        point=vector(sample['collision'])
        projected=unreal.NavigationSystemV1.project_point_to_navigation(
            world,point,None,None,unreal.Vector(1,1,80))
        if projected is None or abs(projected.z-point.z)>=25:
            failures.append({'kind':'dense_stair_navigation','local':sample['local']})
    resolution=report['validation'].get('local_navigation_resolution')
    if resolution:
        proxy=actors.get(resolution['proxy'])
        modifier=proxy.get_component_by_class(unreal.NavModifierComponent) if proxy else None
        nav=actors.get(resolution['nav'])
        params=nav.get_editor_property('nav_mesh_resolution_params') if nav else []
        actual=[[p.get_editor_property(k) for k in ('cell_size','cell_height','agent_max_step_height')]
                for p in params]
        if not modifier or modifier.get_editor_property('nav_mesh_resolution')!=unreal.NavigationDataResolution.HIGH \
                or actual!=resolution['parameters']:
            failures.append({'kind':'local_navigation_resolution','actual':actual})
    return {"loaded_map":world.get_path_name(),"actors":len(actors),
            "instances_tested":len(report["proxies"]),"gameplay_components":len(gameplay)-len(skipped_editor_only),
            "preserved_sources_tested":len(report.get('preserved_sources',[])),
            "dense_navigation_samples":len(dense.get('samples',[])),
            "walking_navigation_links":sum(len(s['links']) for s in walking_links),
            "omitted_editor_only_billboards":skipped_editor_only,
            "wall_rays":ray_count,"navigation_paths":len(report["validation"]["navigation_paths"]),
            "failures":failures,"passed":not failures}


if __name__ == "__main__":
    filename = os.environ["PORTAL_COLLISION_REPORT"]
    output = os.path.join(os.path.dirname(filename),"disk_verification.json")
    try:
        with open(filename,encoding="utf-8") as stream:
            result = main(json.load(stream))
    except Exception:
        result = {"passed":False,"error":traceback.format_exc()}
    with open(output,"w",encoding="utf-8") as stream:
        json.dump(result,stream,ensure_ascii=False,indent=2)
    unreal.log("PORTAL_COLLISION_DISK_VERIFY " + json.dumps(result,ensure_ascii=False))
    if not result["passed"]:
        raise RuntimeError("Saved collision verification failed; see "+output)

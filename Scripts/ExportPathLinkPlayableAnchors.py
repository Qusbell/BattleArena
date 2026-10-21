"""Run ONLY through Unreal MCP execute_python. Read authored Blueprint spawn components/capsule; export test input."""
import json
import os
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
anchors = []
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() != "BP_PlayerSpawnPoint_C":
        continue
    component = next((c for c in actor.get_components_by_class(unreal.SceneComponent) if c.get_name() == "SpawnPoint"), None)
    point = component.get_world_location() if component else actor.get_actor_location()
    anchors.append(dict(label=actor.get_actor_label(), position=[point.x, point.y, point.z]))
pawn = unreal.get_default_object(unreal.load_class(None, "/Game/QuakeLike_1_0/Character/BP_AIShooter.BP_AIShooter_C"))
capsule = pawn.get_component_by_class(unreal.CapsuleComponent)
data = dict(map=world.get_path_name(), anchors=anchors, radius=capsule.get_scaled_capsule_radius(),
            half_height=capsule.get_scaled_capsule_half_height())
path = unreal.Paths.project_saved_dir() + "PathLinkBench/PlayableAnchors.json"
os.makedirs(os.path.dirname(path), exist_ok=True)
with open(path, "w", encoding="utf-8") as destination:
    json.dump(data, destination, indent=2)
print(json.dumps(data, ensure_ascii=False))

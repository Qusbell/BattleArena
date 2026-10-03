"""Run explicitly in UE Python with the editor closed; backs up data before migration.

Editor-only native helpers modify graphs/structs. No maps or damage/drop rules are edited.
"""
import json
from pathlib import Path
import unreal

ROOT = "/Game/QuakeLike_1_0/"
OUT = Path(unreal.Paths.project_saved_dir()) / "ItemSoundMigration"
OUT.mkdir(parents=True, exist_ok=True)
KINDS = ("HeldItem", "Buff", "LifeItem", "AmmoItem")
DROP_NAMES = ("HoldableItem", "BuffItem", "LifeItem", "AmmoItem")
helper = unreal.ItemSoundMigrationLibrary

def asset(path):
    value = unreal.load_asset(path)
    assert value, path
    return value

def checked(ok, label):
    assert ok, label
    unreal.log("ITEM_SOUND_MIGRATION " + label)

tables = [asset(ROOT + "Data/Item/DropTable/DT_" + kind + "Table") for kind in KINDS]
structs = [table.get_editor_property("row_struct") for table in tables]
original = {table.get_name(): json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)) for table in tables}
snapshot = OUT / "Before.json"
if snapshot.exists():
    assert json.loads(snapshot.read_text(encoding="utf-8")) == original, "Assets differ from the original snapshot; do not rerun migration."
else:
    snapshot.write_text(json.dumps(original, ensure_ascii=False, indent=2), encoding="utf-8")

# Load direct struct/interface Blueprint referencers BEFORE changing signatures so UE can reconstruct them.
# Map referencers are deliberately not loaded or saved.
interface = asset(ROOT + "Spawner/BPI_ItemSpawner")
blueprints = {}
for value in [*structs, interface]:
    for path in unreal.EditorAssetLibrary.find_package_referencers_for_asset(value.get_path_name(), False):
        data = unreal.EditorAssetLibrary.find_asset_data(path)
        if str(data.asset_class_path.asset_name) in ("Blueprint", "WidgetBlueprint"):
            bp = asset(path)
            blueprints[bp.get_path_name()] = bp
spawners = [asset(ROOT + "Spawner/Component/BPC_" + kind + "Spawner") for kind in KINDS]
drops = [asset(ROOT + "DropItem/BP_" + name) for name in DROP_NAMES]
held = asset(ROOT + "HeldItem/BP_HeldItem")
spawnpoint = asset(ROOT + "Spawner/SpawnPoint/BP_ItemSpawnPoint")
for bp in [interface, *spawners, *drops, held, spawnpoint]:
    blueprints[bp.get_path_name()] = bp

for kind, struct in zip(KINDS, structs):
    checked(helper.configure_sound_fields(struct, kind), "fields " + kind)

for kind, table in zip(KINDS, tables):
    rows = json.loads(json.dumps(original[table.get_name()]))
    columns = [str(c) for c in unreal.DataTableFunctionLibrary.get_data_table_column_export_names(table)]
    actual = {name.lower(): name for name in columns}
    for row in rows:
        if kind == "HeldItem":
            row[actual["spawnsound"]] = row.pop("sound")
            data = asset(row["Item DA"].split("'")[1])
            sound = data.get_editor_property("equipSound")
            row[actual["pickupsound"]] = sound.get_path_name() if sound else "None"
            row[actual["pickupvolume"]] = data.get_editor_property("equipVolumeMultiplier")
        elif kind == "Buff":
            row[actual["spawnsound"]] = row["Sound"]
            row[actual["pickupsound"]] = row.pop("Sound")
            row[actual["pickupvolume"]] = row.pop("soundVolume")
        elif kind == "AmmoItem":
            row[actual["spawnsound"]] = row["sound"]
            row[actual["pickupsound"]] = row.pop("sound")
        else:
            row[actual["spawnsound"]] = "None"  # No existing Life spawn cue; do not invent art choices.
        row[actual["spawnvolume"]] = 1.0
    checked(unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows), table.get_editor_property("row_struct")), "restore all row data " + kind)
    restored = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    by_name = {row["Name"]: row for row in restored}
    for previous in original[table.get_name()]:
        for name, value in previous.items():
            if name not in ("sound", "Sound", "soundVolume"):
                assert by_name[previous["Name"]][name] == value, (kind, previous["Name"], name)

checked(helper.extend_spawn_feedback(interface), "BPI spawn volume")
checked(helper.refresh_and_compile(interface), "compile BPI")
for bp, table in zip(spawners, tables):
    checked(helper.wire_spawner_sound(bp, table), "spawn sound " + bp.get_name())
for bp, table in zip(drops, tables):
    checked(helper.wire_pickup_sound(bp, table), "pickup sound " + bp.get_name())
checked(helper.wire_spawn_volume(spawnpoint), "multicast spawn volume")
checked(helper.separate_weapon_pickup_sound(held, drops[0]), "separate weapon equip/pickup")

for bp in blueprints.values():
    checked(helper.refresh_and_compile(bp), "compile " + bp.get_name())
# All graphs must compile before saving any of the affected packages.
for struct in structs:
    checked(unreal.EditorAssetLibrary.save_loaded_asset(struct, False), "save " + struct.get_name())
for table in tables:
    checked(unreal.EditorAssetLibrary.save_loaded_asset(table, False), "save " + table.get_name())
for bp in blueprints.values():
    checked(unreal.EditorAssetLibrary.save_loaded_asset(bp, False), "save " + bp.get_name())
after = {table.get_name(): json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table)) for table in tables}
(OUT / "After.json").write_text(json.dumps(after, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log("ITEM_SOUND_MIGRATION_COMPLETE")

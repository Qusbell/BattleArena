"""Run ONLY via Unreal MCP execute_python. Registry metadata only; no asset loads/saves.

Call collect(run_directory). Completed roots can be resumed with the same directory.
"""
import collections
import csv
import datetime
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import unreal


def collect(run_directory):
    root = Path(unreal.Paths.project_dir()).resolve()
    run = resolve_audit_path(run_directory)
    if not run.is_relative_to(root / "docs/repository-audit/audit/generated"):
        raise ValueError("Output must be in this project's docs/repository-audit/audit/generated")
    manifest = json.loads((run / "run.json").read_text(encoding="utf-8"))
    if Path(manifest["project_root"]).resolve() != root:
        raise ValueError("MCP is connected to a different project")
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    if ar.is_loading_assets():
        raise RuntimeError("Asset Registry is still loading. Retry after discovery finishes.")
    def dirty_packages():
        return {"content": sorted(str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()),
                "maps": sorted(str(p.get_name()) for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())}
    dirty_before = dirty_packages()
    state_path = run / "asset-registry-state.json"
    collector_hash = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    state = json.loads(state_path.read_text(encoding="utf-8")) if state_path.exists() else {}
    if state.get("collector_sha256") != collector_hash:
        state = {"collector_sha256": collector_hash, "completed": {}, "errors": {}}
    fields = ["object_path", "package_name", "package_path", "asset_name", "asset_class", "mount"]

    def save(path, value):
        temp = path.with_suffix(path.suffix + ".tmp")
        temp.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        temp.replace(path)

    for candidate in manifest["mount_candidates"]:
        mount = candidate["mount"]
        if mount in state["completed"]:
            continue
        output = run / ("registry-" + mount[1:] + ".csv")
        temp = output.with_suffix(".csv.tmp")
        try:
            assets = ar.get_assets_by_path(mount, recursive=True, include_only_on_disk_assets=True)
            with temp.open("w", newline="", encoding="utf-8-sig") as f:
                writer = csv.writer(f)
                writer.writerow(fields)
                for asset in sorted(assets, key=lambda a: (str(a.package_name), str(a.asset_name))):
                    class_path = str(asset.asset_class_path.package_name) + "." + str(asset.asset_class_path.asset_name)
                    writer.writerow([str(asset.package_name) + "." + str(asset.asset_name), str(asset.package_name), str(asset.package_path), str(asset.asset_name), class_path, mount])
            temp.replace(output)
            state["completed"][mount] = {"rows": len(assets), "completed_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "note": "Zero rows can mean no content or an unmounted/disabled plugin."}
            state["errors"].pop(mount, None)
        except Exception as error:
            state["errors"][mount] = str(error)
        save(state_path, state)
    classes, mounts = collections.Counter(), collections.Counter()
    packages, objects = set(), set()
    with (run / "asset-registry.csv").open("w", newline="", encoding="utf-8-sig") as f:
        writer = csv.DictWriter(f, fieldnames=fields)
        writer.writeheader()
        for mount in sorted(state["completed"]):
            with (run / ("registry-" + mount[1:] + ".csv")).open(encoding="utf-8-sig", newline="") as part:
                for row in csv.DictReader(part):
                    writer.writerow(row)
                    classes[row["asset_class"]] += 1
                    mounts[row["mount"]] += 1
                    packages.add(row["package_name"])
                    objects.add(row["object_path"])
    summary = {"status": "complete" if not state["errors"] else "partial", "completed_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
               "transport": "Unreal MCP util.execute_python", "engine_version": unreal.SystemLibrary.get_engine_version(),
               "project_root": str(root), "on_disk_only": True, "asset_rows": sum(classes.values()), "unique_objects": len(objects), "unique_packages": len(packages),
               "dirty_packages_before": dirty_before, "dirty_packages_after": dirty_packages(),
               "mount_counts": dict(mounts), "class_counts": dict(classes.most_common()), "errors": state["errors"],
               "scope": "Project /Game and local plugin mount candidates; excludes /Engine, engine plugin content, unsaved-only assets and runtime actor instances. No graph/role/dependency analysis yet."}
    save(run / "asset-registry-summary.json", summary)
    print(json.dumps({"status": summary["status"], "assets": summary["asset_rows"], "packages": len(packages), "errors": state["errors"]}, ensure_ascii=False))

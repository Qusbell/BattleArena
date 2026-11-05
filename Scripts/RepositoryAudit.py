"""Filesystem-only audit. Never opens Unreal asset binaries or deletes project files."""
import argparse
import collections
import csv
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import subprocess

ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / "docs/repository-audit/audit/generated"


def now():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def write_json(path, value):
    path = Path(path)
    temp = path.with_suffix(path.suffix + ".tmp")
    temp.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    temp.replace(path)


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT).decode("utf-8").rstrip("\n")


def inventory():
    run = GENERATED / dt.datetime.now(dt.timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    run.mkdir(parents=True, exist_ok=False)
    started = now()
    tracked = set(git("ls-files", "-z").split("\0")) - {""}
    untracked = set(git("ls-files", "--others", "--exclude-standard", "-z").split("\0")) - {""}
    baseline = {
        "schema_version": 1, "started_utc": started, "project_root": str(ROOT),
        "git_commit": git("rev-parse", "HEAD"), "git_branch": git("branch", "--show-current"),
        "git_status_before": git("status", "--porcelain=v1", "--untracked-files=all"),
        "scope": "All workspace file metadata except .git, audit generated outputs and links/reparse points. No asset binary contents read.",
        "size_semantics": "Logical file lengths, not allocated disk space; live workspace, not an atomic snapshot.",
        "tracked_files": len(tracked),
        "tracked_asset_files": sum(p.endswith((".uasset", ".umap")) for p in tracked),
        "stages": {"filesystem": "running", "asset_registry": "pending", "dependencies": "not_started", "roles": "not_started", "cleanup": "not_started"},
    }
    write_json(run / "run.json", baseline)
    errors, skipped = [], []
    folders, saved, extensions = (collections.defaultdict(lambda: [0, 0]) for _ in range(3))
    tracking = collections.Counter()
    key_for = str.casefold if os.name == "nt" else str
    tracked_keys = {key_for(p) for p in tracked}
    untracked_keys = {key_for(p) for p in untracked}
    observed_tracked = set()
    plugin_mounts = []
    for descriptor in sorted((ROOT / "Plugins").glob("*/*.uplugin")):
        plugin_mounts.append({"mount": "/" + descriptor.stem, "directory": descriptor.parent.relative_to(ROOT).as_posix() + "/Content", "descriptor": descriptor.relative_to(ROOT).as_posix()})
    mounts = [{"mount": "/Game", "directory": "Content"}] + plugin_mounts
    baseline["mount_candidates"] = mounts
    total_bytes = 0
    count = 0
    disk_packages = set()

    def walk_error(error):
        errors.append({"path": str(error.filename), "error": str(error)})

    with (run / "files.csv").open("w", newline="", encoding="utf-8-sig") as f, (run / "disk-assets.csv").open("w", newline="", encoding="utf-8-sig") as a, (run / "documents.csv").open("w", newline="", encoding="utf-8-sig") as d:
        files = csv.writer(f)
        assets = csv.writer(a)
        docs = csv.writer(d)
        files.writerow(["path", "bytes", "modified_utc", "git_status"])
        assets.writerow(["path", "package_candidate", "bytes", "git_status"])
        docs.writerow(["path", "bytes", "modified_utc", "git_status"])
        for directory, dirs, names in os.walk(ROOT, followlinks=False, onerror=walk_error):
            parent = Path(directory)
            keep = []
            for name in sorted(dirs):
                path = parent / name
                rel = path.relative_to(ROOT).as_posix()
                if rel == ".git" or path == GENERATED:
                    skipped.append({"path": rel, "reason": "explicit_exclusion"})
                elif path.is_symlink() or path.is_junction():
                    skipped.append({"path": rel, "reason": "link_or_junction"})
                else:
                    keep.append(name)
            dirs[:] = keep
            for name in sorted(names):
                path = parent / name
                rel = path.relative_to(ROOT).as_posix()
                try:
                    if path.is_symlink():
                        skipped.append({"path": rel, "reason": "file_symlink"})
                        continue
                    stat = path.stat()
                except OSError as error:
                    walk_error(error)
                    continue
                path_key = key_for(rel)
                status = "tracked" if path_key in tracked_keys else "untracked" if path_key in untracked_keys else "ignored"
                if status == "tracked":
                    observed_tracked.add(path_key)
                stamp = dt.datetime.fromtimestamp(stat.st_mtime, dt.timezone.utc).isoformat()
                files.writerow([rel, stat.st_size, stamp, status])
                count += 1
                total_bytes += stat.st_size
                tracking[status] += 1
                for bucket, key in [(folders, rel.split("/")[0]), (extensions, path.suffix.lower() or "<none>")]:
                    bucket[key][0] += 1
                    bucket[key][1] += stat.st_size
                if rel.startswith("Saved/"):
                    key = rel.split("/")[1] if rel.count("/") >= 2 else "<root>"
                    saved[key][0] += 1
                    saved[key][1] += stat.st_size
                if path.suffix.lower() in (".uasset", ".umap"):
                    package = ""
                    for mount in mounts:
                        prefix = mount["directory"] + "/"
                        if rel.startswith(prefix):
                            package = mount["mount"] + "/" + rel[len(prefix):].rsplit(".", 1)[0]
                            disk_packages.add(package)
                            break
                    assets.writerow([rel, package, stat.st_size, status])
                if path.suffix.lower() == ".md" or (rel.split("/")[0] in ("Manual", "perf_docs", "docs", "GamePlayDB") and path.suffix.lower() in (".txt", ".pdf", ".docx")):
                    docs.writerow([rel, stat.st_size, stamp, status])
    for name, bucket in [("folders", folders), ("saved-folders", saved), ("extensions", extensions)]:
        with (run / (name + ".csv")).open("w", newline="", encoding="utf-8-sig") as f:
            writer = csv.writer(f)
            writer.writerow(["name", "files", "bytes"])
            writer.writerows((key, *value) for key, value in sorted(bucket.items(), key=lambda pair: (-pair[1][1], pair[0])))
    roots = []
    markers = ("GameDefaultMap", "ServerDefaultMap", "GlobalDefaultGameMode", "GameInstanceClass", "TransitionMap", "MapsToCook", "DirectoriesToAlwaysCook", "PrimaryAsset")
    for path in sorted((ROOT / "Config").rglob("*.ini")):
        for line_no, line in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
            if not line.lstrip().startswith((";", "#")) and any(marker in line for marker in markers):
                roots.append({"file": path.relative_to(ROOT).as_posix(), "line": line_no, "setting": line})
    write_json(run / "entrypoints.json", roots)
    write_json(run / "filesystem-errors.json", errors)
    write_json(run / "excluded-paths.json", skipped)
    write_json(run / "tracked-not-observed.json", sorted(p for p in tracked if key_for(p) not in observed_tracked))
    baseline.update({"filesystem_completed_utc": now(), "file_count": count, "logical_bytes": total_bytes,
                     "tracking_counts": dict(tracking), "disk_package_candidates": len(disk_packages),
                     "filesystem_error_count": len(errors), "excluded_count": len(skipped)})
    baseline["stages"]["filesystem"] = "complete" if not errors else "partial"
    write_json(run / "run.json", baseline)
    print(json.dumps({"run": run.relative_to(ROOT).as_posix(), "files": count, "bytes": total_bytes, "errors": len(errors)}, ensure_ascii=False))


def finalize(run):
    run = resolve_audit_path(run)
    if not run.is_relative_to(GENERATED.resolve()):
        raise ValueError("Run must be inside docs/repository-audit/audit/generated")
    data = json.loads((run / "run.json").read_text(encoding="utf-8"))
    registry_summary = run / "asset-registry-summary.json"
    if registry_summary.exists():
        registry = json.loads(registry_summary.read_text(encoding="utf-8"))
        data["asset_registry"] = registry
        data["stages"]["asset_registry"] = registry["status"]
        with (run / "disk-assets.csv").open(encoding="utf-8-sig", newline="") as f:
            disk = {r["package_candidate"] for r in csv.DictReader(f) if r["package_candidate"]}
        with (run / "asset-registry.csv").open(encoding="utf-8-sig", newline="") as f:
            packages = {r["package_name"] for r in csv.DictReader(f)}
        comparison = {"disk_only": sorted(disk - packages), "registry_only": sorted(packages - disk),
                      "note": "Differences require investigation; not proof of unused or invalid assets. Plugin mount candidates may be disabled/unmounted."}
        write_json(run / "registry-comparison.json", comparison)
        data["comparison"] = {"disk_only": len(comparison["disk_only"]), "registry_only": len(comparison["registry_only"])}
    data["finalized_utc"] = now()
    data["run_directory"] = run.relative_to(ROOT).as_posix()
    data["collector_sha256"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path(__file__), ROOT / "Scripts/RepositoryAuditUnreal.py"]}
    data["artifacts"] = {p.name: {"bytes": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(run.iterdir()) if p.is_file() and p.name != "run.json"}
    write_json(run / "run.json", data)
    write_json(ROOT / "docs/repository-audit/audit/baseline.json", data)
    def table(filename, limit):
        with (run / filename).open(encoding="utf-8-sig", newline="") as f:
            rows = list(csv.DictReader(f))[:limit]
        return "\n".join(f"| `{r['name']}` | {int(r['files']):,} | {int(r['bytes']) / 2**30:.3f} |" for r in rows)

    report = f"""# 저장소 조사 기준선

- 수집 시작: {data['started_utc']} (UTC)
- 기준 Git 커밋: `{data['git_commit']}` / 브랜치 `{data['git_branch']}`
- 실행 데이터: `{data['run_directory']}` (로컬 재생성 자료, Git 제외)
- 파일: {data['file_count']:,}개 / 논리 용량 {data['logical_bytes'] / 2**30:.3f} GiB
- 파일 조회 오류: {data['filesystem_error_count']}건
- 디스크 프로젝트 패키지 후보: {data['disk_package_candidates']:,}개
- 에셋 객체: {data.get('asset_registry', {}).get('asset_rows', '미수집')}개
- 디스크/Registry 비교: {json.dumps(data.get('comparison'), ensure_ascii=False)}

## 상위 폴더 용량

| 폴더 | 파일 수 | GiB |
|---|---:|---:|
{table('folders.csv', 15)}

## Saved 하위 폴더 용량

| 폴더 | 파일 수 | GiB |
|---|---:|---:|
{table('saved-folders.csv', 15)}

## 진행 상태

```json
{json.dumps(data['stages'], ensure_ascii=False, indent=2)}
```

이 기준선은 파일 메타데이터와 저장된 에셋의 Registry 목록 조사입니다. 사용 여부,
역할, 의존 관계와 삭제 안전성은 판정하지 않았습니다. Engine 및 엔진 플러그인 에셋,
저장 전 에셋, 실행 중 Actor 인스턴스는 Registry 수집 범위에 포함하지 않습니다.
용량은 압축/중복 제거를 고려한 실제 디스크 점유량이 아니며, 수집 중 Editor가
기록하는 로그 등은 변경될 수 있습니다. 제외 경로와 오류는 실행 데이터에 기록합니다.
"""
    (ROOT / "docs/repository-audit/audit/BASELINE.md").write_text(report, encoding="utf-8")
    print(json.dumps({"stages": data["stages"], "comparison": data.get("comparison"), "artifacts": len(data["artifacts"])}, ensure_ascii=False))


def verify(run):
    run = resolve_audit_path(run)
    if not run.is_relative_to(GENERATED.resolve()):
        raise ValueError("Run must be inside docs/repository-audit/audit/generated")
    data = json.loads((run / "run.json").read_text(encoding="utf-8"))
    for name, expected in data["artifacts"].items():
        path = run / name
        if path.stat().st_size != expected["bytes"] or hashlib.sha256(path.read_bytes()).hexdigest() != expected["sha256"]:
            raise ValueError("Artifact changed: " + name)
    for name, expected in data["collector_sha256"].items():
        if not recorded_hash_matches(ROOT / "Scripts" / name, expected):
            raise ValueError("Collector changed since finalize: " + name)
    with (run / "files.csv").open(encoding="utf-8-sig", newline="") as f:
        rows = list(csv.DictReader(f))
    if len(rows) != data["file_count"] or sum(int(r["bytes"]) for r in rows) != data["logical_bytes"]:
        raise ValueError("Filesystem totals do not match")
    if len({r["path"].casefold() for r in rows}) != len(rows):
        raise ValueError("Duplicate filesystem paths")
    with (run / "asset-registry.csv").open(encoding="utf-8-sig", newline="") as f:
        assets = list(csv.DictReader(f))
    if len(assets) != data["asset_registry"]["asset_rows"] or len({r['object_path'] for r in assets}) != len(assets):
        raise ValueError("Asset row count or uniqueness mismatch")
    # Blueprint-generated classes can live under /Game or plugin mounts.
    if any(not r["asset_class"].startswith("/") or "." not in r["asset_class"] or "<Struct" in r["asset_class"] for r in assets):
        raise ValueError("Invalid class path serialization")
    if dict(collections.Counter(r["asset_class"] for r in assets)) != data["asset_registry"]["class_counts"]:
        raise ValueError("Class totals do not match")
    if data["filesystem_error_count"] or data["asset_registry"]["errors"]:
        raise ValueError("Collection has unresolved errors")
    print(json.dumps({"verified": True, "artifacts": len(data["artifacts"]), "files": len(rows), "assets": len(assets)}))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--finalize", metavar="RUN_DIRECTORY")
    mode.add_argument("--verify", metavar="RUN_DIRECTORY")
    args = parser.parse_args()
    if args.finalize:
        finalize(args.finalize)
    elif args.verify:
        verify(args.verify)
    else:
        inventory()

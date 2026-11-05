"""MCP-only native T3D read export for graphs outside the JSON helper's coverage."""
import datetime
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import unreal


def collect(run, limit=300):
    root = Path(unreal.Paths.project_dir()).resolve()
    if root != Path(__file__).resolve().parents[1]:
        raise RuntimeError('Wrong project')
    run = resolve_audit_path(run)
    if not run.is_relative_to(root / 'docs/repository-audit/audit/generated'):
        raise ValueError('Invalid output path')
    inputs = [(row['object_path'], g['path']) for file in sorted(run.glob('blueprints-*.json'))
              for row in json.loads(file.read_text(encoding='utf-8'))['rows'] for g in row['unsupported_graphs']]
    candidates, excluded = [], []
    for asset, graph in inputs:
        if ':ExecuteUbergraph_' in graph or '_MERGED' in graph or ':EvaluateGraphExposedInputs_' in graph or graph.endswith('__AnimFunc'):
            excluded.append({'asset': asset, 'path': graph, 'reason': 'compiled/merged derivative; authored top-level graphs are separately captured'})
        else:
            candidates.append((asset, graph))
    candidates.sort()
    fingerprint = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    state_path = run / 'supplement-state.json'
    state = json.loads(state_path.read_text(encoding='utf-8')) if state_path.exists() else {
        'collector_sha256': fingerprint, 'next': 0, 'total': len(candidates), 'excluded': len(excluded), 'status': 'running'}
    if not recorded_hash_matches(Path(__file__), state['collector_sha256']):
        raise RuntimeError('Supplement collector changed')
    (run / 'supplement-excluded.json').write_text(json.dumps(excluded, ensure_ascii=False, indent=2), encoding='utf-8')
    start, stop = state['next'], min(state['next'] + limit, len(candidates))
    selected = candidates[start:stop]
    loaded = [unreal.load_asset(asset) for asset in sorted({a for a, _ in selected})]
    wanted = {g for _, g in selected}
    objects = {g.get_path_name(): g for g in unreal.ObjectIterator(unreal.EdGraph) if g.get_path_name() in wanted}
    folder = run / 'graph-text'
    folder.mkdir(exist_ok=True)
    records = []
    for asset, path in selected:
        record = {'asset': asset, 'path': path}
        try:
            graph = objects.get(path)
            if graph is None:
                raise RuntimeError('Graph object no longer present after loading; may be compiler-generated')
            ident = hashlib.sha256(path.encode()).hexdigest()[:24]
            output = folder / (ident + '.t3d')
            task = unreal.AssetExportTask()
            task.object = graph
            task.exporter = unreal.ObjectExporterT3D()
            task.filename = str(output)
            task.automated = True
            task.prompt = False
            task.replace_identical = True
            if not unreal.Exporter.run_asset_export_task(task):
                raise RuntimeError('Native T3D export returned false')
            record.update({'status': 'exported', 'artifact': 'graph-text/' + ident + '.t3d',
                           'bytes': output.stat().st_size, 'sha256': hashlib.sha256(output.read_bytes()).hexdigest()})
        except Exception as error:
            record.update({'status': 'unresolved', 'error': str(error)})
        records.append(record)
    (run / ('supplement-%06d.json' % start)).write_text(json.dumps(records, ensure_ascii=False, indent=2), encoding='utf-8')
    state.update({'next': stop, 'status': 'complete' if stop == len(candidates) else 'running', 'updated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat()})
    state_path.write_text(json.dumps(state, indent=2), encoding='utf-8')
    print(json.dumps({'stage': 'graph_text', 'next': stop, 'total': len(candidates), 'exported_batch': sum(r['status']=='exported' for r in records), 'unresolved_batch': sum(r['status']=='unresolved' for r in records)}))

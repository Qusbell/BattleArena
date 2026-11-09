"""Whole-project static audit. Invoke ONLY through Unreal MCP util.execute_python.

prepare(), dependencies(run, limit), blueprints(run, limit), bindings(run).
Every batch is checkpointed; no compile, asset save, JSON apply or world load.
"""
import collections
import csv
import datetime as dt
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import subprocess
import time
import unreal

FLAGS = {'hard_package': 'include_hard_package_references', 'soft_package': 'include_soft_package_references',
         'hard_management': 'include_hard_management_references', 'soft_management': 'include_soft_management_references'}


def stamp():
    return dt.datetime.now(dt.timezone.utc).isoformat()


def save(path, data):
    temp = path.with_suffix(path.suffix + '.tmp')
    temp.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    temp.replace(path)


def dirty():
    return {'content': sorted(p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()),
            'maps': sorted(p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())}


def context(run):
    root = Path(unreal.Paths.project_dir()).resolve()
    if root != Path(__file__).resolve().parents[1]:
        raise RuntimeError('MCP project does not match collector project')
    run = resolve_audit_path(run)
    if not run.is_relative_to(root / 'docs/repository-audit/audit/generated'):
        raise ValueError('Run must be inside docs/repository-audit/audit/generated')
    state = json.loads((run / 'state.json').read_text(encoding='utf-8'))
    if not recorded_hash_matches(Path(__file__), state['collector_sha256']):
        raise RuntimeError('Collector changed. Start a new run to avoid mixing evidence formats.')
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    if ar.is_loading_assets():
        raise RuntimeError('Wait for Asset Registry discovery')
    return root, run, state, ar


def prepare():
    root = Path(unreal.Paths.project_dir()).resolve()
    if root != Path(__file__).resolve().parents[1]:
        raise RuntimeError('Wrong project')
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    if ar.is_loading_assets():
        raise RuntimeError('Asset Registry is loading')
    run = root / 'docs/repository-audit/audit/generated' / ('whole-' + dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
    run.mkdir(parents=True)
    mounts = ['/Game'] + sorted({'/' + p.stem for p in (root / 'Plugins').glob('*/*.uplugin')})
    rows = []
    for mount in mounts:
        for a in ar.get_assets_by_path(mount, recursive=True, include_only_on_disk_assets=True):
            tags = {key: a.get_tag_value(key) for key in ['ParentClass', 'NativeParentClass', 'GeneratedClass', 'BlueprintType']}
            rows.append({'object_path': str(a.package_name) + '.' + str(a.asset_name), 'package': str(a.package_name),
                         'name': str(a.asset_name), 'class': str(a.asset_class_path.package_name) + '.' + str(a.asset_class_path.asset_name),
                         'mount': mount, 'tags': tags})
    rows.sort(key=lambda r: r['object_path'])
    packages = sorted({r['package'] for r in rows})
    bps = [r for r in rows if r['class'].rsplit('.', 1)[-1] in ('Blueprint', 'WidgetBlueprint', 'AnimBlueprint', 'EditorUtilityBlueprint', 'EditorUtilityWidgetBlueprint')]
    opts = {}
    for kind, enabled in FLAGS.items():
        values = {flag: False for flag in [*FLAGS.values(), 'include_searchable_names']}
        values[enabled] = True
        option = unreal.AssetRegistryDependencyOptions(**values)
        opts[kind] = {k: option.get_editor_property(k) for k in values}
    state = {'schema_version': 1, 'started_utc': stamp(), 'project_root': str(root),
             'git_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root).decode().strip(),
             'git_status_before': subprocess.check_output(['git', 'status', '--porcelain=v1', '--untracked-files=all'], cwd=root).decode('utf-8'),
             'engine': unreal.SystemLibrary.get_engine_version(), 'collector_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
             'transport': 'Unreal MCP util.execute_python', 'scope': 'Project /Game and local plugin mount candidates; on-disk registry assets. Engine/external mounts are boundary references.',
             'mounts': mounts, 'asset_rows': len(rows), 'packages': len(packages), 'blueprints': len(bps),
             'query_flags': opts, 'dependencies_next': 0, 'blueprints_next': 0,
             'dependencies_status': 'pending', 'blueprints_status': 'pending', 'bindings_status': 'pending',
             'dirty_before': dirty(), 'errors': [], 'unsupported_graphs': 0}
    save(run / 'assets.json', rows)
    save(run / 'packages.json', packages)
    save(run / 'blueprint-inputs.json', bps)
    save(run / 'state.json', state)
    print(json.dumps({'run': run.relative_to(root).as_posix(), 'assets': len(rows), 'packages': len(packages), 'blueprints': len(bps)}))


def dependencies(run, limit=1000):
    root, run, state, ar = context(run)
    packages = json.loads((run / 'packages.json').read_text(encoding='utf-8'))
    start = state['dependencies_next']
    stop = min(start + limit, len(packages))
    opts = {kind: unreal.AssetRegistryDependencyOptions(**flags) for kind, flags in state['query_flags'].items()}
    rows = []
    started = time.monotonic()
    for package in packages[start:stop]:
        for kind, option in opts.items():
            for dep in ar.get_dependencies(unreal.Name(package), option) or []:
                rows.append([package, str(dep), kind])
    rows.sort()
    save(run / ('dependencies-%06d.json' % start), {'start': start, 'stop': stop, 'rows': rows})
    state['dependencies_next'] = stop
    state['dependencies_status'] = 'complete' if stop == len(packages) else 'running'
    state['updated_utc'] = stamp()
    save(run / 'state.json', state)
    print(json.dumps({'stage': 'dependencies', 'next': stop, 'total': len(packages), 'rows_in_batch': len(rows), 'seconds': round(time.monotonic()-started, 2)}))


def blueprints(run, limit=15):
    root, run, state, ar = context(run)
    rows = json.loads((run / 'blueprint-inputs.json').read_text(encoding='utf-8'))
    start = state['blueprints_next']
    stop = min(start + limit, len(rows))
    loaded, results = {}, []
    for row in rows[start:stop]:
        try:
            bp = unreal.load_asset(row['object_path'])
            if not isinstance(bp, unreal.Blueprint):
                raise RuntimeError('Not a Blueprint or failed to load')
            loaded[bp.get_path_name()] = (bp, row)
        except Exception as error:
            state['errors'].append({'stage': 'blueprints', 'asset': row['object_path'], 'error': str(error)})
    graphs_by_bp = collections.defaultdict(list)
    for graph in unreal.ObjectIterator(unreal.EdGraph):
        # Graph paths use '<Blueprint object path>:<subobject path>'.
        owner = graph.get_path_name().split(':', 1)[0]
        if owner in loaded:
            graphs_by_bp[owner].append(graph)
    raw_folder = run / 'graphs'
    raw_folder.mkdir(exist_ok=True)
    for object_path, (bp, row) in loaded.items():
        record = {**row, 'graphs': [], 'unsupported_graphs': [], 'variables': {}, 'components': {}}
        try:
            record['variables'] = json.loads(unreal.MCPythonHelper.list_blueprint_variables(bp))
            record['components'] = json.loads(unreal.MCPythonHelper.list_blueprint_components(bp))
            seen = set()
            for graph in sorted(graphs_by_bp[object_path], key=lambda g: g.get_path_name()):
                name = graph.get_name()
                if name in seen:
                    record['unsupported_graphs'].append({'path': graph.get_path_name(), 'reason': 'duplicate name; helper accepts only graph name'})
                    continue
                seen.add(name)
                data = json.loads(unreal.MCPythonHelper.get_blueprint_graph_info(bp, name))
                if not data.get('success'):
                    record['unsupported_graphs'].append({'path': graph.get_path_name(), 'reason': data.get('message', 'helper cannot export nested/macro/compiled graphs')})
                    continue
                ident = hashlib.sha256((object_path + ':' + name).encode()).hexdigest()[:24]
                save(raw_folder / (ident + '.json'), data)
                nodes = data.get('nodes', [])
                record['graphs'].append({'name': name, 'path': graph.get_path_name(), 'artifact': 'graphs/' + ident + '.json',
                    'node_count': data['node_count'], 'node_classes': dict(collections.Counter(n['node_class'] for n in nodes)),
                    'events': [{'node': n['node_name'], 'title': n['node_title']} for n in nodes if 'Event' in n['node_class'] or n['node_class'] == 'K2Node_FunctionEntry'],
                    'calls': [{'node': n['node_name'], 'title': n['node_title']} for n in nodes if 'Call' in n['node_class']],
                    'object_defaults': sorted({p['default_object'] for n in nodes for p in n.get('pins', []) if p.get('default_object')})})
            state['unsupported_graphs'] += len(record['unsupported_graphs'])
        except Exception as error:
            state['errors'].append({'stage': 'blueprints', 'asset': object_path, 'error': str(error)})
            record['error'] = str(error)
        results.append(record)
    save(run / ('blueprints-%06d.json' % start), {'start': start, 'stop': stop, 'rows': results})
    state['blueprints_next'] = stop
    state['blueprints_status'] = 'complete' if stop == len(rows) else 'running'
    state['dirty_after_blueprints'] = dirty()
    state['updated_utc'] = stamp()
    save(run / 'state.json', state)
    print(json.dumps({'stage': 'blueprints', 'next': stop, 'total': len(rows), 'graphs_in_batch': sum(len(r['graphs']) for r in results), 'errors': len(state['errors']), 'unsupported_graphs': state['unsupported_graphs'], 'dirty_count': sum(len(v) for v in state['dirty_after_blueprints'].values())}))


def bindings(run):
    root, run, state, ar = context(run)
    registry = unreal.load_asset('/Game/JsonAssetSync/DA_JsonApplyRegistry')
    configs = [('data_table_bindings', 'target_data_table', 'DataTables'), ('data_asset_bindings', 'target_data_asset', 'DataAssets'),
               ('curve_table_bindings', 'target_curve_table', 'CurveTables'), ('float_curve_bindings', 'target_float_curve', 'FloatCurves')]
    records = []
    for group, prop, folder in configs:
        for binding in registry.get_editor_property(group):
            target = binding.get_editor_property(prop)
            relative = binding.get_editor_property('json_relative_path')
            source = root / 'Content' / folder / relative
            record = {'type': group, 'source': 'Content/' + folder + '/' + relative,
                      'target': target.get_path_name() if target else None, 'source_exists': source.is_file()}
            if target and group == 'data_table_bindings':
                record['row_names'] = [str(x) for x in unreal.DataTableFunctionLibrary.get_data_table_row_names(target)]
                record['columns'] = [str(x) for x in unreal.DataTableFunctionLibrary.get_data_table_column_export_names(target)]
                record['row_struct'] = target.get_editor_property('row_struct').get_path_name()
            records.append(record)
    save(run / 'bindings.json', records)
    state['bindings_status'] = 'complete'
    state['dirty_after'] = dirty()
    state['updated_utc'] = stamp()
    save(run / 'state.json', state)
    print(json.dumps({'stage': 'bindings', 'bindings': len(records), 'missing_sources': sum(not r['source_exists'] for r in records), 'missing_targets': sum(not r['target'] for r in records)}))

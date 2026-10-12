"""Read-only audit. Run only through Unreal MCP execute_python, inside the Editor."""
import unreal
import json
import os
from collections import Counter, deque

OUT = os.path.join(unreal.Paths.project_dir(), 'Docs/LogicAudit/Evidence')
os.makedirs(OUT, exist_ok=True)
AR = unreal.AssetRegistryHelpers.get_asset_registry()
OPTIONS = unreal.AssetRegistryDependencyOptions(
    include_soft_package_references=True, include_hard_package_references=True,
    include_searchable_names=False, include_soft_management_references=False,
    include_hard_management_references=False)

def write_json(name, obj):
    with open(os.path.join(OUT, name), 'w', encoding='utf-8') as f:
        json.dump(obj, f, ensure_ascii=False, indent=2)

def inventory():
    assets = AR.get_assets_by_path('/Game', recursive=True)
    rows = []
    for a in assets:
        cls = str(a.asset_class_path.asset_name)
        if any(s in cls for s in ['Blueprint', 'BehaviorTree', 'BlackboardData', 'World',
                                 'DataTable', 'DataAsset', 'InputAction', 'InputMappingContext',
                                 'StateTree', 'CurveTable', 'EnvQuery']) or str(a.package_name).startswith('/Game/QuakeLike_1_0/Data/'):
            rows.append({'path': str(a.package_name), 'class': cls,
                         'parent': str(a.get_tag_value('ParentClass') or ''),
                         'native_parent': str(a.get_tag_value('NativeParentClass') or '')})
    write_json('asset-inventory.json', rows)
    roots = ['/Game/QuakeLike_1_0/GameFlow/MainLevel/MainMenu_Level',
             '/Game/QuakeLike_1_0/GameMode/BP_QuakeGameInstance',
             '/Game/QuakeLike_1_0/Lobby/StartLobby', '/Game/QuakeLike_1_0/Lobby/TryLogin',
             '/Game/QuakeLike_1_0/Lobby/TransitionMap',
             '/Game/QuakeLike_1_0/GameFlow/Lobby/Lobby_Level',
             '/Game/QuakeLike_Base/Lvl_QuakeMap_01', '/Game/QuakeLike_Base/Lvl_QuakeMap_03',
             '/Game/QuakeLike_1_0/GameMode/BP_MultiplayerAIGameMode',
             '/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode']
    reached, queue, edges, predecessor = set(), deque(roots), {}, {}
    while queue:
        p = queue.popleft()
        if p in reached:
            continue
        reached.add(p)
        deps = sorted(str(x) for x in (AR.get_dependencies(p, OPTIONS) or []) if str(x).startswith('/Game/'))
        edges[p] = deps
        for d in deps:
            if d not in reached:
                predecessor.setdefault(d, p)
                queue.append(d)
    for a in rows:
        p = a['path']
        if p not in edges:
            edges[p] = sorted(str(x) for x in (AR.get_dependencies(p, OPTIONS) or []) if str(x).startswith('/Game/'))
        a['reachable'] = p in reached
        a['referencers'] = sorted(str(x) for x in (AR.get_referencers(p, OPTIONS) or []) if str(x).startswith('/Game/'))
    write_json('asset-inventory.json', rows)
    write_json('asset-reachability.json', {'roots': roots, 'reached': sorted(reached), 'predecessor': predecessor, 'dependencies': edges})
    selected = [a for a in rows if 'Blueprint' in a['class'] and
                (a['path'].startswith('/Game/QuakeLike_1_0/') or a['reachable'])]
    write_json('blueprint-targets.json', selected)
    print(json.dumps({'total_assets': len(assets), 'inventory': len(rows), 'reachable_packages': len(reached),
                      'blueprints_to_inspect': len(selected), 'classes': dict(Counter(a['class'] for a in rows))}, ensure_ascii=False))

def collect_batch(start, count):
    with open(os.path.join(OUT, 'blueprint-targets.json'), encoding='utf-8') as f:
        selected = json.load(f)[start:start+count]
    loaded, errors = [], []
    for a in selected:
        try:
            bp = unreal.load_asset(a['path'])
            if bp:
                loaded.append((a, bp))
            else:
                errors.append({'path': a['path'], 'error': 'load_asset returned None'})
        except Exception as e:
            errors.append({'path': a['path'], 'error': str(e)})
    graphs = {}
    for g in unreal.ObjectIterator(unreal.EdGraph):
        outer = g.get_outer()
        if outer:
            graphs.setdefault(outer.get_path_name(), []).append(g.get_name())
    results = []
    for a, bp in loaded:
        record = dict(a)
        record['graphs'] = []
        record['unsupported_graphs'] = []
        try:
            record['variables'] = json.loads(unreal.MCPythonHelper.list_blueprint_variables(bp))
            record['components'] = json.loads(unreal.MCPythonHelper.list_blueprint_components(bp))
            names = sorted(set(graphs.get(bp.get_path_name(), [])))
            for name in names:
                graph = json.loads(unreal.MCPythonHelper.get_blueprint_graph_info(bp, name))
                if graph.get('success'):
                    record['graphs'].append(graph)
                else:
                    record['unsupported_graphs'].append({'graph': name, 'reason': graph.get('message')})
            gc = bp.generated_class()
            if gc:
                cdo = unreal.get_default_object(gc)
                record['defaults'] = {}
                props = [v['name'] for v in record['variables'].get('variables', [])]
                props += ['default_pawn_class', 'player_controller_class', 'player_state_class', 'game_state_class',
                          'hud_class', 'ai_controller_class', 'b_replicates', 'replicates', 'bgm_settings', 'b_is_campaign_mode']
                for prop in props:
                    try:
                        value = cdo.get_editor_property(prop)
                        record['defaults'][prop] = value.get_path_name() if isinstance(value, unreal.Object) else str(value)
                    except Exception:
                        pass
        except Exception as e:
            record['error'] = str(e)
        results.append(record)
    write_json('blueprints-%03d.json' % start, {'blueprints': results, 'errors': errors})
    print(json.dumps({'start': start, 'assets': len(results), 'graphs': sum(len(r['graphs']) for r in results),
                      'nodes': sum(g['node_count'] for r in results for g in r['graphs']), 'errors': errors}, ensure_ascii=False))

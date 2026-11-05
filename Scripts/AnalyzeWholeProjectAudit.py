"""Analyze exported MCP evidence and authored text. Never reads asset binary contents."""
import argparse
import ast
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
import re

ROOT = Path(__file__).resolve().parents[1]
DECODINGS = {}


def text_of(path):
    raw = path.read_bytes()
    for encoding in (['utf-16'] if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else ['utf-8-sig', 'cp949']):
        try:
            result = raw.decode(encoding)
            DECODINGS[path.relative_to(ROOT).as_posix()] = encoding
            return result
        except UnicodeDecodeError:
            pass
    raise ValueError('Unrecognized text encoding: ' + str(path))


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def csvfile(path, columns, rows):
    with path.open('w', newline='', encoding='utf-8-sig') as f:
        writer = csv.writer(f)
        writer.writerow(columns)
        writer.writerows(rows)


def system(path):
    value = path.lower()
    for label, terms in [
        ('AI.Navigation', ['pathlink', '/ai/area', '/ai_area', 'virtualinputaimove', 'navpath']),
        ('AI.Decision', ['/ai/', 'aicontroller', 'blackboard', 'behaviortree', 'bt_quake']),
        ('Movement.World', ['teleport', 'portal', 'jumppad', 'mapgimmick', 'spawnselectionsystem']),
        ('Combat.Weapons', ['/weapon/', 'helditem', 'projectile', '/fire/', 'firstpersonrendering', 'recentattacker']),
        ('Items.Buffs', ['/items/', '/buff/', '/spawner/', '/dropitem/', '/data/item', 'itemspawn', '/life', '/inventory']),
        ('Character', ['/character/', '/player/', 'appearances', 'shooterdisplay', 'shooternickname', 'shooteroutline']),
        ('GameFlow.Network', ['/loading/', '/lobby', '/gamemode/', '/controller/', 'myplayer', 'matchrules', 'reconnection', 'localdedicated', 'multirules']),
        ('Audio.Camera', ['/audio/', '/camera/', '/sound', '/bgm', 'deathcam']),
        ('UI', ['/ui/', '/gameflow/', '/gameprogressflow/', 'commonui']),
        ('Telemetry.Debug', ['/analyzer/', '/debug/', 'arenamatchlog', 'arenaanalyze']),
        ('Data.Config', ['/data/', 'datatable', 'dataasset', 'jsonassetsync']),
    ]:
        if any(t in value for t in terms):
            return label
    return 'Other'


def package_of(value):
    if not isinstance(value, str):
        return None
    match = re.search(r'(/Game/[^\s\"\'\],)}<>]+)', value)
    return match.group(1).split('.')[0].rstrip('/') if match else None


def asset_role(a):
    kind = a['class'].rsplit('.', 1)[-1]
    roles = {'Blueprint':'Blueprint class/logic definition', 'WidgetBlueprint':'UI widget class/logic definition',
             'AnimBlueprint':'animation pose/state logic', 'StaticMeshActor':'serialized world mesh Actor record',
             'StaticMesh':'static render/collision geometry resource', 'SkeletalMesh':'skinned character/weapon geometry resource',
             'Material':'surface/shader graph', 'MaterialInstanceConstant':'material parameter variant',
             'MaterialFunction':'reusable material graph', 'Texture2D':'2D image/texture resource',
             'SoundCue':'audio routing/playback graph', 'SoundWave':'audio sample resource',
             'DataTable':'named rows of structured data', 'World':'level/world package',
             'ObjectRedirector':'asset path forwarding record', 'BehaviorTree':'AI behavior hierarchy',
             'BlackboardData':'AI shared key schema', 'AnimSequence':'authored animation clip',
             'Skeleton':'skeletal hierarchy and animation bindings', 'PhysicsAsset':'skeletal collision/physics bodies'}
    return roles.get(kind, 'Serialized ' + kind + ' object; business responsibility requires system evidence')


def analyze(run):
    run = resolve_audit_path(run)
    if not run.is_relative_to(ROOT / 'docs/repository-audit/audit/generated'):
        raise ValueError('Invalid output')
    state = read(run / 'state.json')
    if any(state[k] != 'complete' for k in ['dependencies_status', 'blueprints_status', 'bindings_status']):
        raise RuntimeError('Finish MCP collection before analysis')
    assets = read(run / 'assets.json')
    packages = set(read(run / 'packages.json'))
    edges = [tuple(row) for f in sorted(run.glob('dependencies-*.json')) for row in read(f)['rows']]
    unique_edges = sorted(set(edges))
    adjacency = collections.defaultdict(set)
    reverse = collections.defaultdict(set)
    for source, target, kind in unique_edges:
        adjacency[source].add(target)
        reverse[target].add(source)
    bps = [row for f in sorted(run.glob('blueprints-*.json')) for row in read(f)['rows']]
    binding_rows = read(run / 'bindings.json')
    roots = []
    text_refs = []
    text_hashes = {}
    source_catalog, native_types, modules, workflows = [], [], [], []
    text_files = sorted(set([*ROOT.glob('Config/**/*.ini'), *ROOT.glob('Source/**/*.h'), *ROOT.glob('Source/**/*.cpp'),
                            *ROOT.glob('Source/**/*.cs'), *ROOT.glob('Plugins/*/Source/**/*.h'), *ROOT.glob('Plugins/*/Source/**/*.cpp'),
                            *ROOT.glob('Plugins/*/Source/**/*.cs'), *ROOT.glob('Content/DataTables/**/*.json'),
                            *ROOT.glob('Content/DataAssets/**/*.json'), *ROOT.glob('Content/CurveTables/**/*.json'),
                            *ROOT.glob('Content/FloatCurves/**/*.json'), *ROOT.glob('ExternalData/*.json'),
                            *ROOT.glob('Scripts/**/*.py'), *ROOT.glob('Tools/**/*.py'), *ROOT.glob('Tools/**/*.ps1'),
                            *ROOT.glob('Tools/**/*.bat'), *ROOT.glob('GamePlayDB/*.py'), *ROOT.glob('GamePlayDB/*.sql'),
                            *ROOT.glob('*.bat'), *ROOT.glob('*.ps1')]))
    for file in text_files:
        rel = file.relative_to(ROOT).as_posix()
        if any(part in rel.split('/') for part in ['Intermediate','Binaries','.venv','venv','__pycache__','node_modules']):
            continue
        text = text_of(file)
        text_hashes[rel] = hashlib.sha256(file.read_bytes()).hexdigest()
        is_test = '/Tests/' in rel
        is_editor = 'Editor' in rel or rel.startswith('Plugins/GameplayValidator') or rel.startswith('Plugins/UnrealMCPython')
        context = 'tests' if is_test else 'editor_tool' if is_editor else 'authored_data' if file.suffix == '.json' else 'runtime_source' if rel.startswith(('Source/', 'Plugins/')) else 'config' if file.suffix=='.ini' else 'tool_source'
        if context=='tool_source':
            doc=''
            if file.suffix=='.py':
                try:doc=ast.get_docstring(ast.parse(text)) or ''
                except SyntaxError:doc='Python source could not be parsed by this interpreter; inspect manually.'
            workflows.append({'file':rel,'description_from_source':doc,'role_evidence':'module docstring' if doc else 'filename only; workflow purpose requires inspection',
                              'lines':len(text.splitlines()),'system_inference':system(rel)})
        for line_no, line in enumerate(text.splitlines(), 1):
            if file.suffix == '.ini' and line.lstrip().startswith((';', '#')):
                continue
            for match in re.finditer(r'/Game/[^\s\"\'\],)}<>]+', line):
                package = package_of(match.group())
                match_type = 'package' if package in packages else 'directory' if any(p.startswith(package + '/') for p in packages) else 'unresolved_literal'
                item = {'file': rel, 'line': line_no, 'value': match.group(), 'package': package, 'context': context,
                        'exists': match_type != 'unresolved_literal', 'match_type': match_type}
                text_refs.append(item)
                if context == 'config' and any(key in line for key in ['GameDefaultMap', 'ServerDefaultMap', 'GameInstanceClass', 'GlobalDefaultGameMode', 'TransitionMap', 'MapsToCook', 'registryAsset']):
                    roots.append({**item, 'policy': 'configured_entrypoint'})
                if context == 'config' and 'DirectoriesToAlwaysCook' in line:
                    for p in sorted(packages):
                        if p == package or p.startswith(package + '/'):
                            roots.append({'package': p, 'file': rel, 'line': line_no, 'policy': 'always_cook_directory'})
        if rel.startswith(('Source/', 'Plugins/')):
            includes = re.findall(r'^\s*#include\s+"([^"]+)"', text, re.M)
            source_catalog.append({'file': rel, 'system_inference': system(rel), 'context': context, 'lines': len(text.splitlines()), 'includes': includes})
        if file.suffix == '.h':
            for match in re.finditer(r'\b(enum\s+class|enum|class|struct)\s+(?:\w+_API\s+)?(\w+)(?:\s+final)?\s*(?::\s*([^;{}]+))?\s*\{', text):
                prefix = text[:match.start()]
                comments = list(re.finditer(r'/\*\*([\s\S]*?)\*/', prefix))
                comment = comments[-1] if comments and match.start() - comments[-1].end() < 400 else None
                brief = re.sub(r'\s+', ' ', re.sub(r'^\s*\*\s?', '', comment.group(1), flags=re.M)).strip() if comment else ''
                # Header-wide symbols are a search aid, not a C++ parser's member ownership result.
                plain = re.sub(r'U(?:FUNCTION|PROPERTY|CLASS|STRUCT)\((?:[^()]|\([^()]*\))*\)', '', text)
                methods = sorted({m for m in re.findall(r'\b(\w+)\s*\([^;{}]*\)\s*(?:const)?\s*(?:override)?\s*(?:;|\{)', plain)
                                  if not m.isupper() and m not in ('if','for','while','switch','return','sizeof')})
                native_types.append({'name': match.group(2), 'kind': match.group(1), 'bases': (match.group(3) or '').strip(), 'file': rel,
                                     'line': text.count('\n', 0, match.start()) + 1, 'comment': brief,
                                     'header_function_symbols': methods,
                                     'system_inference': system(rel), 'context': context,
                                     'evidence': 'header declaration/comment; semantic ownership not inferred from reachability'})
        if file.name.endswith('.Build.cs'):
            modules.append({'file': rel, 'module': file.name[:-9], 'dependency_literals': re.findall(r'"(\w+)"', text),
                            'note': 'Literal extraction includes conditional modules; not a resolved platform build graph.'})
    for row in binding_rows:
        if row['target']:
            roots.append({'package': row['target'].split('.')[0], 'policy': 'registered_json_target', 'file': row['source']})
    manifest = read(ROOT / 'ExternalData/JsonAssetSyncManifest.json')
    manifest_bindings = [r for group in manifest.values() if isinstance(group,list) for r in group if isinstance(r,dict) and r.get('target')]
    actual_pairs = {(r['source'],r['target']) for r in binding_rows}
    manifest_pairs = {(r['source'],r['target']) for r in manifest_bindings}
    binding_comparison = {'actual_count':len(actual_pairs),'manifest_count':len(manifest_pairs),
                          'only_registry':sorted(actual_pairs-manifest_pairs),'only_manifest':sorted(manifest_pairs-actual_pairs)}
    write(run / 'binding-manifest-comparison.json', binding_comparison)
    include_edges = []
    for row in source_catalog:
        for include in row['includes']:
            local = str((ROOT / row['file']).parent.relative_to(ROOT) / include).replace('\\','/')
            candidates = [r['file'] for r in source_catalog if r['file'] == local or r['file'].endswith('/' + include)]
            include_edges.append({'source':row['file'],'include':include,'candidates':candidates,
                                  'status':'resolved' if len(candidates)==1 else 'ambiguous' if candidates else 'engine_or_external'})
    write(run / 'native-include-edges.json', include_edges)
    configured = {r['package'] for r in roots if r['package'] in packages}
    # Conservative authored-data/source roots include literal references but do not claim execution.
    conservative = configured | {r['package'] for r in text_refs if r['package'] in packages and r['context'] in ('runtime_source', 'authored_data', 'config')}

    def closure(seeds):
        seen = set(seeds)
        pending = list(seeds)
        while pending:
            for target in adjacency[pending.pop()]:
                if target in packages and target not in seen:
                    seen.add(target)
                    pending.append(target)
        return seen

    reachable = closure(configured)
    potential = closure(conservative)
    by_package = collections.defaultdict(list)
    for a in assets:
        by_package[a['package']].append(a)
    world_packages = {a['package'] for a in assets if a['class'].endswith('.World')}
    # External actor/object package relationships may not be represented by ordinary map dependencies.
    external = {p for p in packages if p.startswith(('/Game/__ExternalActors__/', '/Game/__ExternalObjects__/'))}
    csvfile(run / 'asset-roles.csv', ['object_path','package','class','role_from_class','system_inference','evidence','configured_reachable','conservative_reachable','referencer_count','is_external_actor_or_object'],
            ([a['object_path'],a['package'],a['class'],asset_role(a),system(a['package']),'class/path metadata; semantic role requires graph/source evidence',a['package'] in reachable,a['package'] in potential,len(reverse[a['package']]),a['package'] in external] for a in assets))
    csvfile(run / 'package-reachability.csv', ['package','configured_reachable','conservative_reachable','referencer_count','review_status'],
            ([p,p in reachable,p in potential,len(reverse[p]),'external_actor_review' if p in external else 'reachable' if p in potential else 'unresolved_not_delete_safe'] for p in sorted(packages)))
    csvfile(run / 'typed-dependencies.csv', ['source','target','kind'], unique_edges)
    system_edges = collections.Counter((system(a),system(b),kind) for a,b,kind in unique_edges if b in packages)
    csvfile(run / 'system-dependencies.csv',['source_system_inference','target_system_inference','kind','edges'],
            ((*key,value) for key,value in sorted(system_edges.items())))
    write(run / 'roots.json', roots)
    write(run / 'text-references.json', text_refs)
    write(run / 'native-types.json', native_types)
    write(run / 'native-sources.json', source_catalog)
    write(run / 'module-dependencies.json', modules)
    write(run / 'blueprint-catalog.json', bps)
    write(run / 'unresolved-packages.json', sorted(packages - potential - external))
    write(run / 'maps.json', [{'package': p, 'configured_reachable': p in reachable, 'conservative_reachable': p in potential,
                              'dependencies': sorted(adjacency[p]), 'referencers': sorted(reverse[p])} for p in sorted(world_packages)])
    supplement = [r for f in sorted(run.glob('supplement-*.json')) if f.name != 'supplement-state.json' and f.name != 'supplement-excluded.json' for r in read(f)]
    # Doc inventory is limited to authored project docs, not dependencies/caches/generated outputs.
    doc_files = [*ROOT.glob('docs/*.md'), *ROOT.glob('GamePlayDB/*.md'), *ROOT.glob('Manual/*.txt'), *ROOT.glob('perf_docs/*.md')]
    doc_records = []
    for file in sorted(doc_files):
        text = text_of(file)
        name = file.name
        category = 'historical_experiment' if any(x in name for x in ['Results','Sweep','Repair','Review','PlacementCheck','Outlier','Stabilization','AccuracyNext','Checklist','DirectionCorrection','LargePolygons','PlayableBlend','27Placement','Random','LazySearchResults']) else 'current_feature_reference'
        if file.parent.name == 'Manual':
            category = 'legacy_manual'
        doc_records.append({'path': file.relative_to(ROOT).as_posix(), 'category': category,
                            'headings': re.findall(r'^#{1,3} (.+)$', text, re.M), 'sha256_before_reconciliation': hashlib.sha256(file.read_bytes()).hexdigest(),
                            'review_scope': 'Static source/asset cross-reference; historical runtime results not rerun.'})
    if (run/'document-review.json').exists():
        doc_records=read(run/'document-review.json')
    else:
        write(run / 'document-review.json', doc_records)
    report = {'schema_version':1, 'analyzed_utc':dt.datetime.now(dt.timezone.utc).isoformat(), 'run_directory':run.relative_to(ROOT).as_posix(),
              'git_commit':state['git_commit'], 'collector_state':state, 'asset_objects':len(assets),'packages':len(packages),
              'typed_edges':len(unique_edges),'edge_kinds':dict(collections.Counter(e[2] for e in unique_edges)),
              'blueprints':len(bps),'json_graphs':sum(len(b['graphs']) for b in bps),'json_graph_nodes':sum(g['node_count'] for b in bps for g in b['graphs']),
              'text_graph_exports':sum(r['status']=='exported' for r in supplement),'text_graph_unresolved':sum(r['status']=='unresolved' for r in supplement),
              'native_types':len(native_types),'native_source_files':len(source_catalog),'modules':len(modules),'bindings':len(binding_rows),
              'native_include_edges':len(include_edges),'workflow_files':len(workflows),'binding_manifest_comparison':binding_comparison,
              'configured_roots':len(configured),'conservative_roots':len(conservative),'configured_reachable':len(reachable),'conservative_reachable':len(potential),
              'external_actor_object_packages':len(external),'unresolved_non_external_packages':len(packages-potential-external),
              'zero_referencer_packages':sum(not reverse[p] for p in packages),'maps':len(world_packages),'documents':len(doc_records),
              'text_input_hashes':text_hashes, 'text_decodings':DECODINGS, 'limitations':[
                  'Reachability is static inclusion evidence, not proof of runtime use or unused content.',
                  'AlwaysCook roots intentionally overestimate use. External actor ownership and dynamic string construction require manual validation.',
                  'Blueprint catalogs record graph/call/variable/component facts; folder-based system labels are explicitly inference.',
                  'No map load, PIE, cook, build, asset compile/save or cleanup performed.',
                  'Engine and external plugin content is treated as an outgoing boundary, not recursively audited.']}
    write(run / 'analysis.json', report)
    durable = ROOT / 'docs/repository-audit/architecture'
    write(durable / 'audit-summary.json', report)
    write(durable / 'native-types.json', native_types)
    csvfile(durable / 'blueprint-index.csv', ['object_path','parent_class_tag','native_parent_tag','system_inference','graphs','nodes','function_or_event_names','component_names','raw_record'],
        ([b['object_path'],b['tags'].get('ParentClass'),b['tags'].get('NativeParentClass'),system(b['package']),len(b['graphs']),sum(g['node_count'] for g in b['graphs']),
          '|'.join(g['name'] for g in b['graphs']), '|'.join(str(c.get('name',c.get('variable_name',''))) for c in b.get('components',{}).get('components',[])),
          report['run_directory'] + '/blueprint-catalog.json'] for b in bps))
    write(durable / 'data-bindings.json', binding_rows)
    write(durable / 'module-dependencies.json', modules)
    write(durable / 'maps.json', read(run / 'maps.json'))
    write(durable / 'document-review.json', doc_records)
    write(durable / 'workflows.json', workflows)
    project = read(ROOT / 'ShootingArena.uproject')
    plugin_overrides = {p['Name']:p for p in project.get('Plugins',[])}
    plugins = []
    for descriptor in sorted((ROOT/'Plugins').glob('*/*.uplugin')):
        d=read(descriptor)
        plugins.append({'name':descriptor.stem,'descriptor':descriptor.relative_to(ROOT).as_posix(),
                        'project_override':plugin_overrides.get(descriptor.stem),
                        'enabled_by_default_declaration':d.get('EnabledByDefault'), 'modules':d.get('Modules',[]),
                        'dependencies':d.get('Plugins',[]), 'note':'Descriptor/project declarations; not a runtime plugin activation probe.'})
    write(durable/'plugins.json',plugins)
    print(json.dumps({k:v for k,v in report.items() if isinstance(v,int)}))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run')
    analyze(p.parse_args().run)

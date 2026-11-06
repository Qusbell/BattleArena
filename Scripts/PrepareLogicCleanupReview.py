"""Prepare logic-only cleanup candidates from existing MCP evidence; never opens asset binaries."""
import collections
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import runpy
import subprocess

ROOT=Path(__file__).resolve().parents[1]
RESOLVE=runpy.run_path(str(ROOT/'Scripts/AuditPaths.py'))['resolve_audit_path']
ENTRY='/Game/QuakeLike_1_0/GameFlow/MainLevel/TestMainMenu_Level'
FLAGS=['include_hard_package_references','include_soft_package_references','include_hard_management_references','include_soft_management_references','include_searchable_names']


def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def prepare():
    summary=read(ROOT/'docs/repository-audit/architecture/audit-summary.json')
    old=RESOLVE(summary['run_directory'])
    changed=subprocess.check_output(['git','diff',summary['git_commit'],'HEAD','--name-only','--','*.uasset','*.umap','Source/**','Config/**','Content/DataTables/**','Content/DataAssets/**'],cwd=ROOT).decode()
    if changed.strip():raise RuntimeError('Historical asset/source evidence changed; refresh affected scope first: '+changed)
    run=ROOT/'docs/repository-audit/audit/generated'/('cleanup-'+dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
    run.mkdir()
    assets=read(old/'assets.json');packages={r['package'] for r in assets}
    canonical={p.casefold():p for p in packages}
    bp={r['package']:r for r in read(old/'blueprint-catalog.json')}
    edges=[(a,b,k) for file in sorted(old.glob('dependencies-*.json')) for a,b,k in read(file)['rows']]
    forward=collections.defaultdict(set);reverse=collections.defaultdict(set)
    for a,b,k in edges:forward[a].add(b);reverse[b].add(a)
    worlds={r['package'] for r in assets if r['class'].endswith('.World')}
    external_owner={}
    for package in packages:
        if package.startswith(('/Game/__ExternalActors__/','/Game/__ExternalObjects__/')):
            suffix='/Game/'+package.split('/',3)[3]
            owner=next((w for w in sorted(worlds,key=len,reverse=True) if suffix.startswith(w+'/')),None)
            if owner:external_owner[package]=owner;forward[owner].add(package)
    generated={p+'.'+r['name']+'_C':r for p,r in bp.items()}
    allowed={'Blueprint','WidgetBlueprint','AnimBlueprint','EditorUtilityWidgetBlueprint','EditorUtilityBlueprint','UserDefinedStruct','UserDefinedEnum','DataTable','BehaviorTree','BlackboardData','EnvQuery','StateTree','StateTreeSchema','World','InputAction','InputMappingContext'}
    excluded_styles=['CommonTextStyle','CommonButtonStyle','CommonBorderStyle','SlateWidgetStyle','CameraShake','MatineeCameraShake','TextStyle','ButtonStyle']
    logic={}
    for a in assets:
        p=a['package'];kind=a['class'].rsplit('.',1)[-1]
        if p.startswith(('/Game/__ExternalActors__/','/Game/__ExternalObjects__/')):continue
        definition=bp.get(p) or generated.get(a['class'])
        parent=str((definition or {}).get('tags',{}).get('NativeParentClass') or (definition or {}).get('tags',{}).get('ParentClass') or '')
        if any(term in parent for term in excluded_styles):continue
        is_data='DataAsset' in parent or 'DataAsset' in kind
        if kind not in allowed and not is_data:continue
        # Environment-art construction Blueprints are outside this logic review.
        art_roots=('SuperGrid','Geometry','Scifi_Arsenal_Vol2','MSPresets','EngineSky','DragonCave','Nanite_Env_Bundle_1','SciFi_Hanger','SoulCave','Sun_Fire_VFX','Watermills','Realistic_Starter_VFX_Pack_Vol2','Sci-FI_Troopers_Collection','ParagonSteel','Characters')
        if any(p.startswith('/Game/'+name+'/') for name in art_roots):continue
        logic[p]={**a,'native_parent':parent,'graph_count':len((bp.get(p) or {}).get('graphs',[])),
                  'node_count':sum(g['node_count'] for g in (bp.get(p) or {}).get('graphs',[]))}
    by_name=collections.defaultdict(set)
    for p,r in logic.items():by_name[r['name']].add(p)
    def path_tokens(value):
        result=set()
        for match in re.findall(r'/Game/[^\s\"\'(),<>]+',value):
            p=canonical.get(match.split('.')[0].rstrip('\\').casefold())
            if p:result.add(p)
        return result
    string_edges=set();literal_sites=collections.defaultdict(list)
    # Read MCP-exported pin literals, including values that Asset Registry cannot track.
    for owner,b in bp.items():
        for g in b['graphs']:
            raw=read(old/g['artifact'])
            for n in raw.get('nodes',[]):
                for pin in n.get('pins',[]):
                    value=pin.get('default_value','')
                    targets=path_tokens(value)
                    if value in by_name:targets|=by_name[value]
                    for target in targets:
                        if target!=owner:
                            string_edges.add((owner,target));literal_sites[target].append({'asset':owner,'graph':g['name'],'node':n['node_name'],'pin':pin['pin_name'],'value':value})
    # Native text exports cover macros/nested graphs omitted by the JSON helper.
    for file in sorted(old.glob('supplement-*.json')):
        if file.name in ('supplement-state.json','supplement-excluded.json'):continue
        for item in read(file):
            if item['status']!='exported':continue
            raw=(old/item['artifact']).read_bytes()
            text=raw.decode('utf-16') if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else raw.decode('utf-8-sig')
            owner=item['asset'].split('.')[0]
            for line in text.splitlines():
                if 'DefaultValue=' not in line:continue
                for target in path_tokens(line):
                    if target!=owner:string_edges.add((owner,target))
    for a,b in string_edges:forward[a].add(b)
    source_sites=collections.defaultdict(list)
    sources=[]
    for rel in summary['text_input_hashes']:
        if not rel.startswith(('Source/','Plugins/','Scripts/','Tools/')):continue
        if '/__pycache__/' in rel or '/.venv/' in rel or 'Audit' in Path(rel).name or 'CleanupReview' in Path(rel).name or Path(rel).name=='WriteWholeProjectCatalogs.py':continue
        raw=(ROOT/rel).read_bytes()
        try:text=raw.decode('utf-8-sig')
        except UnicodeDecodeError:text=raw.decode('cp949')
        sources.append((rel,text))
    for rel,text in sources:
        context='test' if '/Tests/' in rel else 'editor_or_tool' if rel.startswith(('Scripts/','Tools/')) or 'Editor' in rel else 'runtime'
        for number,line in enumerate(text.splitlines(),1):
            targets=path_tokens(line)
            for name in re.findall(r'[\"\']([A-Za-z_][A-Za-z0-9_]*)[\"\']',line):targets|=by_name.get(name,set())
            for p in targets:source_sites[p].append({'file':rel,'line':number,'context':context})
    # Authored external data and exact configuration references remain explicit protected roots.
    text_refs=read(old/'text-references.json')
    configured_roots={ENTRY}
    for ref in text_refs:
        if ref['package'] in packages and ref['context']=='config' and ref['match_type']=='package':configured_roots.add(ref['package'])
    binding_roots={r['target'].split('.')[0] for r in read(old/'bindings.json') if r['target']}
    data_roots={r['package'] for r in text_refs if r['package'] in packages and r['context']=='authored_data'}
    runtime_roots={p for p,sites in source_sites.items() if any(s['context']=='runtime' for s in sites)}
    # Verified runtime registry scan in ArenaMatchLogSubsystem.cpp.
    runtime_roots|={p for p,r in logic.items() if p.startswith('/Game/QuakeLike_1_0/Data/Item/') and r['class'].endswith('.DataTable')}
    def closure(seeds):
        seen=set(seeds);pending=list(seeds)
        while pending:
            for p in forward[pending.pop()]:
                if p in packages and p not in seen:seen.add(p);pending.append(p)
        return seen
    entry_reachable=closure({ENTRY})
    protected=closure(configured_roots|binding_roots|data_roots|runtime_roots)
    string_reverse=collections.defaultdict(set)
    for a,b in string_edges:string_reverse[b].add(a)
    def reverse_impact(package):
        seen={package};pending=[package]
        while pending:
            current=pending.pop()
            for parent in reverse[current]|string_reverse[current]:
                if parent in packages and parent not in seen:seen.add(parent);pending.append(parent)
        seen.discard(package)
        return seen
    candidates=[];protected_logic=[]
    for p,a in sorted(logic.items()):
        refs=reverse[p]|string_reverse[p]
        active_refs=refs&protected
        row={**a,'registry_referencers':sorted(reverse[p]),'literal_referencers':sorted(string_reverse[p]),'source_mentions':source_sites[p],
             'entry_reachable':p in entry_reachable,'protected_reachable':p in protected,'active_referencers':sorted(active_refs),
             'outgoing_packages':sorted(forward[p]),'incoming_count':len(refs),
             'legacy_signal':p.startswith(('/Game/QuakeLike_Base/','/Game/FirstPerson/','/Game/Variant_Shooter/','/Game/CompShooterHUD/')) or any(x in p.lower() for x in ['/test/','/democontent/','/example','_old','_deprecated'])}
        if p in protected or active_refs:
            protected_logic.append(row);continue
        impact=reverse_impact(p)
        row.update({'transitive_dependents':sorted(impact),'transitive_logic_dependents':sorted(impact&logic.keys()),
                    'transitive_dependent_count':len(impact),'transitive_logic_dependent_count':len(impact&logic.keys())})
        # A source/test/tool literal is a dependency even when it is not a shipping entrypoint.
        if not refs and not row['source_mentions']:tier=1
        elif len(refs)<=2 and len(impact)<=10 and len(impact&logic.keys())<=5:tier=2
        else:tier=3
        row['proposed_tier']=tier
        candidates.append(row)
    state={'schema_version':1,'started_utc':dt.datetime.now(dt.timezone.utc).isoformat(),'entrypoint':ENTRY,
           'entry_game_mode':'/Game/QuakeLike_1_0/GameFlow/Lobby/BP_MenuGameMode.BP_MenuGameMode_C',
           'baseline_run':old.relative_to(ROOT).as_posix(),'baseline_commit':summary['git_commit'],
           'current_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT).decode().strip(),
           'asset_or_runtime_changes_since_baseline':changed,'logic_assets':len(logic),'candidate_count':len(candidates),'protected_logic_count':len(protected_logic),
           'entry_reachable_packages':len(entry_reachable),'protected_packages':len(protected),'external_owner_edges':len(external_owner),
           'blueprint_literal_edges':len(string_edges),'proposed_tiers':dict(collections.Counter(r['proposed_tier'] for r in candidates)),
           'collector_status':'pending_live_revalidation','deletions':'none','art_scope':'No mesh/material/texture/audio/animation-clip assets, UI style or environment-art construction BPs.'}
    write(run/'state.json',state);write(run/'candidates-preliminary.json',candidates);write(run/'protected-logic.json',protected_logic)
    write(run/'literal-reference-sites.json',literal_sites);write(run/'runtime-roots.json',{'configured':sorted(configured_roots),'bindings':sorted(binding_roots),'authored_data':sorted(data_roots),'runtime_literals':sorted(runtime_roots)})
    print(json.dumps({'run':run.relative_to(ROOT).as_posix(),**{k:state[k] for k in ['logic_assets','candidate_count','protected_logic_count','entry_reachable_packages','protected_packages','proposed_tiers']}},ensure_ascii=False))


if __name__=='__main__':prepare()

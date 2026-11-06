"""Read-only candidate revalidation. Call collect(run) ONLY through Unreal MCP."""
import datetime as dt
import json
from pathlib import Path
import unreal


def collect(run):
    root=Path(unreal.Paths.project_dir()).resolve()
    if root!=Path(__file__).resolve().parents[1]:raise RuntimeError('Wrong project')
    run=Path(run).resolve()
    if not run.is_relative_to(root/'docs/repository-audit/audit/generated'):raise ValueError('Invalid output')
    state=json.loads((run/'state.json').read_text(encoding='utf-8'))
    rows=json.loads((run/'candidates-preliminary.json').read_text(encoding='utf-8'))
    ar=unreal.AssetRegistryHelpers.get_asset_registry()
    if ar.is_loading_assets():raise RuntimeError('Registry is loading')
    flags={'hard_package':'include_hard_package_references','soft_package':'include_soft_package_references',
           'hard_management':'include_hard_management_references','soft_management':'include_soft_management_references'}
    def dirty():return {'content':[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()],
                         'maps':[p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages()]}
    before=dirty();live=[];changes=[]
    for r in rows:
        typed={};all_refs=set();outgoing=set()
        for kind,flag in flags.items():
            options={x:False for x in [*flags.values(),'include_searchable_names']};options[flag]=True
            opt=unreal.AssetRegistryDependencyOptions(**options)
            typed[kind]=sorted(str(x) for x in ar.get_referencers(unreal.Name(r['package']),opt) or [])
            all_refs.update(typed[kind])
            outgoing.update(str(x) for x in ar.get_dependencies(unreal.Name(r['package']),opt) or [])
        exists=unreal.EditorAssetLibrary.does_asset_exist(r['object_path'])
        if set(r['registry_referencers'])!=all_refs:changes.append({'package':r['package'],'previous':r['registry_referencers'],'current':sorted(all_refs)})
        live.append({'package':r['package'],'exists':exists,'referencers_by_kind':typed,'referencers':sorted(all_refs),
                     'dependencies':sorted(outgoing)})
    world=unreal.load_asset(state['entrypoint'])
    settings=[]
    for o in unreal.ObjectIterator(unreal.WorldSettings):
        if o.get_path_name().startswith(world.get_path_name()+':'):
            mode=o.get_editor_property('default_game_mode')
            settings.append({'path':o.get_path_name(),'default_game_mode':mode.get_path_name() if mode else None})
    registry=unreal.load_asset('/Game/JsonAssetSync/DA_JsonApplyRegistry')
    bindings=[]
    for group,prop in [('data_table_bindings','target_data_table'),('data_asset_bindings','target_data_asset'),
                       ('curve_table_bindings','target_curve_table'),('float_curve_bindings','target_float_curve')]:
        for b in registry.get_editor_property(group):
            target=b.get_editor_property(prop)
            bindings.append({'kind':group,'target':target.get_path_name() if target else None,'source':b.get_editor_property('json_relative_path')})
    state.update({'live_checked_utc':dt.datetime.now(dt.timezone.utc).isoformat(),'live_candidate_count':len(live),'live_reference_changes':changes,
                  'live_missing_candidates':[r['package'] for r in live if not r['exists']],'entry_world_settings':settings,
                  'dirty_before':before,'dirty_after':dirty(),'collector_status':'complete'})
    for name,value in [('candidates-live.json',live),('bindings-live.json',bindings),('state.json',state)]:
        (run/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'candidates':len(live),'reference_changes':len(changes),'missing':len(state['live_missing_candidates']),
                      'entry_world_settings':settings,'dirty_after':state['dirty_after']},ensure_ascii=False))

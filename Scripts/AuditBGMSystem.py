"""Read-only pilot collector. Execute collect() ONLY through Unreal MCP execute_python.

Loads selected assets for inspection; never calls gameplay functions, compiles, applies JSON or saves assets.
"""
import datetime
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import subprocess
import unreal

SETTINGS = '/Game/QuakeLike_1_0/Data/BGM/DA_BGMSetting_Default'
GI = '/Game/QuakeLike_1_0/GameMode/BP_QuakeGameInstance'
REGISTRY = '/Game/JsonAssetSync/DA_JsonApplyRegistry'
MENU = '/Game/QuakeLike_1_0/GameProgressFlow/WBP_SettingsMenu'
PC = '/Game/QuakeLike_1_0/Controller/BP_QuakePlayerController'
GRAPHS = {GI: ['EventGraph', 'LoadSettings', 'SaveSettings', 'FillDefaultsFromDT'],
          MENU: ['EventGraph', 'ApplyBGMVolume', 'InitSlider'],
          '/Game/QuakeLike_1_0/GameFlow/UI/03_Widget/WBP_Setting': ['EventGraph'],
          PC: ['EventGraph', 'ApplyAudioSettings']}
REFERENCE_OPTIONS = {
    'hard_package': 'include_hard_package_references',
    'soft_package': 'include_soft_package_references',
    'hard_management': 'include_hard_management_references',
    'soft_management': 'include_soft_management_references',
}


def collect():
    root = Path(unreal.Paths.project_dir()).resolve()
    if root != Path(__file__).resolve().parents[1]:
        raise RuntimeError('MCP is connected to another project')
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    if ar.is_loading_assets():
        raise RuntimeError('Wait for Asset Registry discovery to finish')
    started = datetime.datetime.now(datetime.timezone.utc)
    output = root / 'docs/repository-audit/audit/generated' / ('bgm-' + started.strftime('%Y%m%dT%H%M%S%fZ'))
    output.mkdir(parents=True, exist_ok=False)

    def save(path, data):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

    def dirty():
        return {'content': sorted(p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_content_packages()),
                'maps': sorted(p.get_name() for p in unreal.EditorLoadingAndSavingUtils.get_dirty_map_packages())}

    def path_of(obj):
        return obj.get_path_name() if obj else None

    def options(*enabled):
        # UE 5.6.1 defaults hard+soft package references to True. Set every flag.
        flags = {key: False for key in [*REFERENCE_OPTIONS.values(), 'include_searchable_names']}
        flags.update({key: True for key in enabled})
        return unreal.AssetRegistryDependencyOptions(**flags)

    def track(obj):
        return {key: (path_of(obj.get_editor_property(prop)) if key == 'sound' else obj.get_editor_property(prop))
                for key, prop in [('sound', 'sound'), ('volumeMultiplier', 'volume_multiplier'), ('bLoop', 'loop'), ('fadeInTime', 'fade_in_time'), ('fadeOutTime', 'fade_out_time')]}

    before = dirty()
    state = {'schema_version': 1, 'system': 'BGM', 'status': 'running', 'started_utc': started.isoformat(),
             'git_commit': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root).decode().strip(),
             'engine': unreal.SystemLibrary.get_engine_version(), 'transport': 'Unreal MCP util.execute_python',
             'run_directory': output.relative_to(root).as_posix(), 'errors': [], 'dirty_before': before}
    save(output / 'state.json', state)
    settings = unreal.load_asset(SETTINGS)
    if not settings:
        raise RuntimeError('BGM settings not found')
    settings_data = {'mainMenuBGM': track(settings.get_editor_property('main_menu_bgm')),
                     'gameplayBGMByMap': {str(k): track(v) for k, v in settings.get_editor_property('gameplay_bgm_by_map').items()}}
    for key, prop in [('defaultFadeInTime', 'default_fade_in_time'), ('defaultFadeOutTime', 'default_fade_out_time'),
                      ('loadingVolumeMultiplier', 'loading_volume_multiplier'), ('loadingVolumeRestoreTime', 'loading_volume_restore_time'),
                      ('soundMix', 'sound_mix'), ('bGMSoundClass', 'bgm_sound_class')]:
        value = settings.get_editor_property(prop)
        settings_data[key] = path_of(value) if key in ('soundMix', 'bGMSoundClass') else value
    registry = unreal.load_asset(REGISTRY)
    bindings = [{'source': b.get_editor_property('json_relative_path'), 'target': path_of(b.get_editor_property('target_data_asset'))}
                for b in registry.get_editor_property('data_asset_bindings') if path_of(b.get_editor_property('target_data_asset')) == settings.get_path_name()]
    gi_class = unreal.load_class(None, GI + '.BP_QuakeGameInstance_C')
    gi_default = unreal.get_default_object(gi_class)
    sync_settings = unreal.get_default_object(unreal.load_class(None, '/Script/JsonAssetSync.JsonAssetSyncSettings'))
    live = {'settings': settings_data, 'game_instance_settings': path_of(gi_default.get_editor_property('BGMSettings')),
            'registry_bindings': bindings, 'json_sync': {
                'registryAsset': path_of(sync_settings.get_editor_property('registryAsset')),
                'applyOnEditorStartup': sync_settings.get_editor_property('applyOnEditorStartup'),
                'editorApplyMode': str(sync_settings.get_editor_property('editorApplyMode')),
                'applyOnRuntimeStartup': sync_settings.get_editor_property('applyOnRuntimeStartup')}}
    save(output / 'live-settings.json', live)
    source_path = root / 'Content/DataAssets/DA_BGMSetting_Default.json'
    authored = json.loads(source_path.read_text(encoding='utf-8-sig'))

    def normalize(value):
        if isinstance(value, dict):
            return {k: normalize(v) for k, v in value.items()}
        if isinstance(value, str):
            if value == 'None':
                return None
            if "'" in value and value.endswith("'"):
                return value.split("'", 1)[1][:-1]
        return value

    comparison = {'match': normalize(authored) == normalize(settings_data),
                  'note': 'Editor memory values compared with authored JSON after normalizing None/object-path wrappers. Not a disk reload or packaged runtime test.'}
    save(output / 'settings-comparison.json', comparison)
    graph_summary = []
    for asset_path, names in GRAPHS.items():
        bp = unreal.load_asset(asset_path)
        for name in names:
            graph = json.loads(unreal.MCPythonHelper.get_blueprint_graph_info(bp, name))
            save(output / ('graph-' + bp.get_name() + '-' + name + '.json'), graph)
            if not graph.get('success'):
                state['errors'].append({'asset': asset_path, 'graph': name, 'result': graph})
                continue
            hits = [n for n in graph['nodes'] if any(term in json.dumps(n, ensure_ascii=False).lower().replace(' ', '') for term in ('bgm', 'loadsettings', 'savesettings', 'applyaudiosettings'))]
            names_near = {n['node_name'] for n in hits}
            for node in hits:
                for pin in node['pins']:
                    names_near.update(link['node_name'] for link in pin.get('linked_to', []))
            graph_summary.append({'asset': asset_path, 'graph': name, 'node_count': graph['node_count'],
                                  'matching_nodes': [n['node_name'] for n in hits],
                                  'nodes': [n for n in graph['nodes'] if n['node_name'] in names_near]})
    save(output / 'graph-evidence.json', graph_summary)
    tracks = [settings_data['mainMenuBGM'], *settings_data['gameplayBGMByMap'].values()]
    audio_settings = {}
    for value in tracks:
        if value['sound']:
            sound = unreal.load_asset(value['sound'])
            audio_settings[value['sound']] = {'sound_class': path_of(sound.get_editor_property('sound_class_object'))}
    save(output / 'audio-settings.json', audio_settings)
    seeds = sorted(set([SETTINGS, GI, REGISTRY, MENU, *GRAPHS] +
                       [t['sound'].split('.')[0] for t in tracks if t['sound']] +
                       [settings_data[k].split('.')[0] for k in ('soundMix', 'bGMSoundClass') if settings_data[k]]))
    dependencies = []
    for package in seeds:
        for kind, option in REFERENCE_OPTIONS.items():
            opts = options(option)
            for other in ar.get_dependencies(unreal.Name(package), opts) or []:
                dependencies.append({'source': package, 'target': str(other), 'kind': kind, 'query': 'dependencies'})
            for other in ar.get_referencers(unreal.Name(package), opts) or []:
                dependencies.append({'source': str(other), 'target': package, 'kind': kind, 'query': 'referencers'})
    dependencies.sort(key=lambda e: (e['source'], e['target'], e['kind'], e['query']))
    save(output / 'typed-dependencies.json', dependencies)
    # Follow only audio resources, keeping gameplay/registry dependencies bounded.
    audio = {}
    pending = sorted({t['sound'].split('.')[0] for t in tracks if t['sound']})
    while pending:
        package = pending.pop(0)
        if package in audio:
            continue
        if len(audio) >= 100:
            state['errors'].append({'audio_closure': '100-package cap reached'})
            break
        opts = options('include_hard_package_references', 'include_soft_package_references')
        deps = sorted(str(x) for x in ar.get_dependencies(unreal.Name(package), opts) or [])
        audio[package] = deps
        for dep in deps:
            if dep.startswith('/Game/') and dep not in audio:
                data = unreal.EditorAssetLibrary.find_asset_data(dep)
                if str(data.asset_class_path.asset_name) in ('SoundCue', 'SoundWave'):
                    pending.append(dep)
    save(output / 'audio-closure.json', audio)
    state.update({'completed_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'dirty_after': dirty(),
                  'dependency_seeds': seeds, 'dependency_rows': len(dependencies), 'graphs_inspected': len(graph_summary),
                  'audio_packages_inspected': len(audio), 'settings_match_authored_json': comparison['match']})
    state['query_flags'] = {kind: {flag: flag == enabled for flag in [*REFERENCE_OPTIONS.values(), 'include_searchable_names']}
                            for kind, enabled in REFERENCE_OPTIONS.items()}
    state['status'] = 'complete' if not state['errors'] else 'partial'
    evidence = {'run': state, 'settings': live, 'comparison': comparison,
                'settings_dependencies': [e for e in dependencies if e['source'] == SETTINGS or e['target'] == SETTINGS],
                'audio_closure': audio, 'audio_settings': audio_settings, 'graphs': graph_summary,
                'scope': 'Selected BGM boundary only. Direct typed package/management queries, bounded audio closure; graph matching is not whole-project call analysis.'}
    save(root / 'docs/repository-audit/systems/evidence/bgm.json', evidence)
    inputs = [Path(__file__), source_path, root / 'ExternalData/JsonAssetSyncManifest.json', root / 'Config/DefaultGame.ini', root / 'Config/DefaultEngine.ini']
    inputs += sorted((root / 'Source/ShootingArena').rglob('BGM*.h')) + sorted((root / 'Source/ShootingArena').rglob('BGM*.cpp'))
    inputs += [root / p for p in ['Source/ShootingArena/Public/ShootingArenaGameInstance.h',
               'Source/ShootingArena/Private/Loading/LoadingScreenSubsystem.cpp', 'Source/ShootingArena/Private/Loading/LoadingBlueprintLibrary.cpp',
               'Plugins/JsonAssetSync/Source/JsonAssetSync/Private/JsonAssetSyncSubsystem.cpp']]
    state['input_sha256'] = {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    state['artifact_sha256'] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in output.iterdir() if p.is_file() and p.name != 'state.json'}
    save(output / 'state.json', state)
    # Save evidence again so the durable summary includes provenance hashes.
    save(root / 'docs/repository-audit/systems/evidence/bgm.json', evidence)
    print(json.dumps({'status': state['status'], 'run': state['run_directory'], 'graphs': state['graphs_inspected'],
                      'dependency_rows': len(dependencies), 'settings_match_json': comparison['match'], 'errors': state['errors']}, ensure_ascii=False))

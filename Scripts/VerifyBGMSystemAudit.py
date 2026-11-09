"""Verify pilot evidence and provenance without opening Unreal assets."""
import collections
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']


def verify():
    root = Path(__file__).resolve().parents[1]
    evidence = json.loads((root / 'docs/repository-audit/systems/evidence/bgm.json').read_text(encoding='utf-8'))
    run = evidence['run']
    output = resolve_audit_path(run['run_directory'])
    assert output.is_relative_to(root / 'docs/repository-audit/audit/generated')
    assert run == json.loads((output / 'state.json').read_text(encoding='utf-8'))
    assert run['status'] == 'complete' and not run['errors']
    assert run['dirty_before'] == run['dirty_after'], 'Inspection changed dirty package state'
    for relative, expected in run['input_sha256'].items():
        assert recorded_hash_matches(root / relative, expected), 'Changed input: ' + relative
    for relative, expected in run['artifact_sha256'].items():
        assert hashlib.sha256((output / relative).read_bytes()).hexdigest() == expected, 'Changed artifact: ' + relative
    for kind, flags in run['query_flags'].items():
        assert sum(flags.values()) == 1, 'Reference kinds are mixed: ' + kind
    for graph in evidence['graphs']:
        name = 'graph-' + graph['asset'].rsplit('/', 1)[-1] + '-' + graph['graph'] + '.json'
        raw = json.loads((output / name).read_text(encoding='utf-8'))
        assert raw['success'] and raw['node_count'] == graph['node_count']
        raw_nodes = {n['node_name']: n for n in raw['nodes']}
        assert all(raw_nodes[n['node_name']] == n for n in graph['nodes'])
        assert set(graph['matching_nodes']) <= {n['node_name'] for n in graph['nodes']}
    deps = json.loads((output / 'typed-dependencies.json').read_text(encoding='utf-8'))
    assert len(deps) == run['dependency_rows']
    assert all(e['kind'] in run['query_flags'] for e in deps)
    assert all(e in deps for e in evidence['settings_dependencies'])
    settings_path = '/Game/QuakeLike_1_0/Data/BGM/DA_BGMSetting_Default'
    gi = '/Game/QuakeLike_1_0/GameMode/BP_QuakeGameInstance'
    assert any(e['source'] == gi and e['target'] == settings_path and e['kind'] == 'soft_package' for e in deps)
    for track in evidence['settings']['settings']['gameplayBGMByMap'].values():
        package = track['sound'].split('.')[0]
        assert any(e['source'] == settings_path and e['target'] == package and e['kind'] == 'soft_package' for e in deps)
        assert package in evidence['audio_closure']
    assert run['settings_match_authored_json'] and evidence['comparison']['match']
    unique_edges = {(e['source'], e['target'], e['kind']) for e in deps}
    summary = {'verified': True, 'graphs': len(evidence['graphs']), 'dependency_query_rows': len(deps),
               'unique_typed_edges': len(unique_edges), 'kinds': dict(collections.Counter(e[2] for e in unique_edges)),
               'input_hashes': len(run['input_sha256']), 'artifact_hashes': len(run['artifact_sha256'])}
    print(json.dumps(summary))
    return summary


if __name__ == '__main__':
    verify()

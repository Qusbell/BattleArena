"""Verify cleanup classification evidence without changing or opening Unreal assets."""
import collections
import csv
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))


def verify():
    folder=ROOT/'docs/repository-audit/cleanup';summary=read(folder/'summary.json');run=ROOT/summary['run_directory']
    rows=read(folder/'candidates.json');state=read(run/'state.json');live={r['package']:r for r in read(run/'candidates-live.json')}
    assert not state['live_reference_changes'] and not state['live_missing_candidates']
    assert len(live)==summary['live_revalidated_assets']
    assert len(rows)==summary['candidate_count'] and len({r['package'] for r in rows})==len(rows)
    assert {str(k):v for k,v in collections.Counter(r['tier'] for r in rows).items()}==summary['tiers']
    assert summary['entrypoint']=='/Game/QuakeLike_1_0/GameFlow/MainLevel/TestMainMenu_Level'
    assert any(x['default_game_mode']=='/Game/QuakeLike_1_0/GameFlow/Lobby/BP_MenuGameMode.BP_MenuGameMode_C' for x in state['entry_world_settings'])
    assert state['dirty_before']==state['dirty_after']=={'content':[],'maps':[]}
    for r in rows:
        assert not r['entry_reachable'] and not r['protected_reachable']
        assert live[r['package']]['exists'] and sorted(live[r['package']]['referencers'])==r['registry_referencers']
        assert r['live_referencers_by_kind']==live[r['package']]['referencers_by_kind']
        if r['tier']==1:
            assert not r['registry_referencers'] and not r['literal_referencers'] and not r['source_mentions']
            assert r['transitive_dependent_count']==r['transitive_logic_dependent_count']==0
            assert 'EditorUtility' not in r['class']
        if r['tier']==2:
            assert r['incoming_count']<=2 and r['transitive_logic_dependent_count']<=5 and r['transitive_dependent_count']<=10
    protected=read(run/'protected-logic.json')
    protected_packages={r['package'] for r in protected}|{r['package'] for r in summary['retained_manual_entries']}
    assert len(protected_packages)==summary['protected_logic_assets']
    assert not protected_packages&{r['package'] for r in rows}
    for path,digest in summary['script_sha256'].items():
        assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,'Collector changed: '+path
    for name,digest in summary['evidence_sha256'].items():
        assert hashlib.sha256((run/name).read_bytes()).hexdigest()==digest,'Evidence changed: '+name
    actual_bindings={r['target'] for r in read(run/'bindings-live.json') if r['target']}
    assert not {r['package'] for r in rows}&{r.split('.')[0] for r in actual_bindings}
    changed=subprocess.check_output(['git','diff',summary['baseline_commit'],'HEAD','--name-only','--','*.uasset','*.umap','Source/**','Config/**','Content/DataTables/**','Content/DataAssets/**'],cwd=ROOT).decode().strip()
    assert not changed,'Asset/source/config baseline changed'
    with (folder/'candidates.csv').open(encoding='utf-8-sig',newline='') as f:
        csv_rows=list(csv.DictReader(f))
    assert len(csv_rows)==len(rows)
    assert {r['package'] for r in csv_rows}=={r['package'] for r in rows}
    for tier in (1,2,3):
        text=(folder/'lists'/f'tier-{tier}.md').read_text(encoding='utf-8')
        assert all('`'+r['package']+'`' in text for r in rows if r['tier']==tier)
    report=(folder/'README.md').read_text(encoding='utf-8')
    assert str(len(rows))+'개' in report
    output={'verified':True,'candidates':len(rows),'tiers':summary['tiers'],'protected_or_retained':len(protected_packages),
            'live_revalidated':len(live),'reference_changes':0,'deleted_assets':0,'runtime_regression':'not_run'}
    (folder/'verification.json').write_text(json.dumps(output,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(output,ensure_ascii=False))


if __name__=='__main__':verify()

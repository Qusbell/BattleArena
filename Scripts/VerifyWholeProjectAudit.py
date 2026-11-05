"""Verify full static-audit coverage, evidence, reachability and documents; no Unreal access."""
import collections
import csv
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import posixpath
import re
import subprocess

ROOT=Path(__file__).resolve().parents[1]


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def verify():
    summary=read(ROOT/'docs/repository-audit/architecture/audit-summary.json')
    run=resolve_audit_path(summary['run_directory'])
    assert run.is_relative_to(ROOT/'docs/repository-audit/audit/generated')
    state=read(run/'state.json')
    assert not state['errors']
    assert state['dependencies_status']==state['blueprints_status']==state['bindings_status']=='complete'
    assert recorded_hash_matches(ROOT/'Scripts/WholeProjectAuditUnreal.py', state['collector_sha256'])
    assert state['dirty_before']==state['dirty_after']==state['dirty_after_blueprints']
    for flags in state['query_flags'].values():
        assert sum(flags.values())==1
    assets=read(run/'assets.json')
    packages=set(read(run/'packages.json'))
    assert len(assets)==summary['asset_objects'] and len(packages)==summary['packages']
    assert len({a['object_path'] for a in assets})==len(assets)

    def batches(prefix,total):
        next_start=0
        rows=[]
        for path in sorted(run.glob(prefix+'-*.json')):
            data=read(path)
            assert data['start']==next_start and data['stop']>=data['start'], 'Checkpoint gap/overlap: '+path.name
            next_start=data['stop']
            rows+=data['rows']
        assert next_start==total
        return rows

    deps=batches('dependencies',len(packages))
    assert len({tuple(e) for e in deps})==len(deps)==summary['typed_edges']
    assert all(e[0] in packages and e[2] in state['query_flags'] for e in deps)
    bps=batches('blueprints',state['blueprints'])
    assert {b['object_path'] for b in bps}=={b['object_path'] for b in read(run/'blueprint-inputs.json')}
    graphs=nodes=0
    for bp in bps:
        assert not bp.get('error')
        for graph in bp['graphs']:
            raw=read(run/graph['artifact'])
            assert raw['success'] and raw['graph_name']==graph['name'] and raw['node_count']==graph['node_count']
            names={n['node_name'] for n in raw['nodes']}
            assert len(names)==len(raw['nodes'])
            assert all(link['node_name'] in names for node in raw['nodes'] for pin in node.get('pins',[]) for link in pin.get('linked_to',[]))
            graphs+=1
            nodes+=raw['node_count']
    assert graphs==summary['json_graphs'] and nodes==summary['json_graph_nodes']
    supplement=read(run/'supplement-state.json')
    assert supplement['status']=='complete' and supplement['next']==supplement['total']
    assert recorded_hash_matches(ROOT/'Scripts/WholeProjectGraphSupplement.py', supplement['collector_sha256'])
    extra=[]
    for path in sorted(run.glob('supplement-*.json')):
        if path.name not in ('supplement-state.json','supplement-excluded.json'):
            extra+=read(path)
    assert len(extra)==supplement['total'] and len({r['path'] for r in extra})==len(extra)
    for row in extra:
        assert row['status']=='exported',row
        raw=(run/row['artifact']).read_bytes()
        assert len(raw)==row['bytes'] and hashlib.sha256(raw).hexdigest()==row['sha256']
        text=raw.decode('utf-16') if raw.startswith((b'\xff\xfe',b'\xfe\xff')) else raw.decode('utf-8-sig')
        assert text.startswith('Begin Object') and row['path'] in text.splitlines()[0]
    adjacency=collections.defaultdict(set)
    for a,b,_ in deps:
        adjacency[a].add(b)
    seeds={r['package'] for r in read(run/'roots.json') if r['package'] in packages}
    literals={r['package'] for r in read(run/'text-references.json') if r['package'] in packages and r['context'] in ('runtime_source','authored_data','config')}
    def closure(start):
        seen=set(start)
        todo=list(start)
        while todo:
            for value in adjacency[todo.pop()]:
                if value in packages and value not in seen:
                    seen.add(value);todo.append(value)
        return seen
    assert len(closure(seeds))==summary['configured_reachable']
    assert len(closure(seeds|literals))==summary['conservative_reachable']
    for filename,count in [('asset-roles.csv',len(assets)),('package-reachability.csv',len(packages))]:
        with (run/filename).open(encoding='utf-8-sig',newline='') as f:
            assert sum(1 for _ in csv.DictReader(f))==count
    assert summary['binding_manifest_comparison']['only_registry']==summary['binding_manifest_comparison']['only_manifest']==[]
    for rel,expected in summary['text_input_hashes'].items():
        assert recorded_hash_matches(ROOT/rel, expected),'Changed input: '+rel
    docs=read(ROOT/'docs/repository-audit/architecture/document-review.json')
    for doc in docs:
        if doc.get('sha256_after_reconciliation'):
            assert recorded_hash_matches(ROOT/doc['path'],doc['sha256_after_reconciliation'])
    tracked=set(filter(None,subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode('utf-8').split('\0')))
    tracked.update(filter(None,subprocess.check_output(['git','ls-files','--others','--exclude-standard','-z'],cwd=ROOT).decode('utf-8').split('\0')))
    check_docs=[*ROOT.glob('docs/repository-audit/architecture/**/*.md'),ROOT/'docs/repository-audit/README.md',ROOT/'docs/README.md',ROOT/'docs/repository-audit/audit/README.md']
    links=0
    for path in check_docs:
        rel=path.relative_to(ROOT).as_posix()
        text=re.sub(r'```.*?```','',path.read_text(encoding='utf-8'),flags=re.S)
        for target in re.findall(r'\]\(([^)]+)\)',text):
            if '://' in target or target.startswith('#'):
                continue
            resolved=posixpath.normpath(posixpath.join(posixpath.dirname(rel),target.split('#')[0]))
            assert resolved in tracked,(rel,resolved,'missing or wrong Git path case')
            links+=1
    result={'verified':True,'packages':len(packages),'assets':len(assets),'typed_edges':len(deps),'blueprints':len(bps),
            'json_graphs':graphs,'text_graphs':len(extra),'input_hashes':len(summary['text_input_hashes']),'document_links':links,
            'runtime_validation':'not_run','cleanup':'not_started'}
    (ROOT/'docs/repository-audit/verification-current.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(result))


if __name__=='__main__':
    verify()

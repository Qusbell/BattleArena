"""Build navigable Markdown evidence from MCP snapshots; never reads .uasset files."""
from pathlib import Path
import json
import re
import html
from collections import Counter

ROOT = Path(__file__).resolve().parents[3]
DOC = ROOT / 'Docs/LogicAudit'
EVIDENCE = DOC / 'Evidence'
inventory = json.loads((EVIDENCE / 'asset-inventory.json').read_text(encoding='utf-8'))
blueprints = []
snapshot_for = {}
for file in sorted(EVIDENCE.glob('blueprints-*.json')):
    for record in json.loads(file.read_text(encoding='utf-8'))['blueprints']:
        blueprints.append(record)
        snapshot_for[record['path']] = file.name

def escaped(value):
    return html.escape(str(value), quote=False).replace('|', '\\|').replace('\n', '<br/>')

def md_path(path):
    return 'Blueprints/' + path.removeprefix('/Game/') + '.md'

def node_label(value):
    return html.escape(value, quote=True).replace('\n', '<br/>')

for bp in blueprints:
    file = DOC / md_path(bp['path'])
    file.parent.mkdir(parents=True, exist_ok=True)
    lines = [f"# {bp['path'].rsplit('/', 1)[-1]}", '', f"에셋: `{bp['path']}`", '',
             f"부모: `{bp['parent']}`", '',
             '수집일: 2026-10-12. MCP에서 읽은 그래프·핀 연결의 자동 전사입니다. 상속된 그래프는 부모 문서를 함께 확인합니다.', '',
             f"정적 루트 참조 도달: {'있음' if bp['reachable'] else '없음'}. 실행 여부 또는 삭제 가능 여부의 확정이 아닙니다.", '',
             '컴포넌트 목록은 이 BP가 직접 정의한 SCS만 포함합니다. 부모·네이티브 생성 컴포넌트, 인스턴스 설정은 별도입니다.', '']
    components = bp.get('components', {}).get('components', [])
    if components:
        lines += ['## 직접 정의한 컴포넌트', '', '| 이름 | 클래스 | 부모 컴포넌트 |', '|---|---|---|']
        lines += [f"| `{c['variable_name']}` | `{c['class']}` | `{c['parent']}` |" for c in components]
        lines += ['']
    variables = bp.get('variables', {}).get('variables', [])
    if variables:
        lines += ['## 변수와 기본값', '', '| 이름 | 타입 | CDO 기본값 |', '|---|---|---|']
        for v in variables:
            default = bp.get('defaults', {}).get(v['name'], '조회되지 않음')
            # Addresses in Unreal repr are irrelevant to authored configuration.
            default = re.sub(r' \(0x[0-9A-Fa-f]+\)', '', default)
            lines += [f"| `{escaped(v['name'])}` | `{v.get('sub_type', v['type'])}` | {escaped(default)} |"]
        lines += ['']
    lines += ['## 그래프 목록', '', '| 그래프 | 전체 노드 수 |', '|---|---|']
    lines += [f"| {escaped(g['graph_name'])} | {g['node_count']} |" for g in bp['graphs']]
    lines += ['', '## 그래프별 실행 연결', '',
              '실선은 출력 exec 핀의 실제 연결입니다. 여러 분기는 출력 핀 이름을 표시합니다. 함수 호출·delegate·타이머의 다음 그래프 실행은 이 그림에서 자동으로 이어 붙이지 않습니다.', '',
              '순수 계산 노드 및 데이터 핀은 아래 목록과 원본 JSON을 확인합니다. 연결되지 않은 이벤트도 표시되므로, 노드가 있다는 이유만으로 실행된다고 판단하면 안 됩니다.', '']
    for g in bp['graphs']:
        lines += [f"### {g['graph_name']}", '']
        nodes = g['nodes']
        exec_nodes = [n for n in nodes if any(p['type'] == 'exec' for p in n['pins'])]
        ids = {n['node_name']: 'n'+str(i) for i,n in enumerate(exec_nodes)}
        if exec_nodes:
            lines += ['<details>', '<summary>실행 핀 플로우차트</summary>', '', '```mermaid', 'flowchart LR']
            for n in exec_nodes:
                lines += [f'  {ids[n["node_name"]]}["{node_label(n["node_title"])}"]']
            for n in exec_nodes:
                for p in n['pins']:
                    if p['type'] == 'exec' and p['direction'] == 'Output':
                        for link in p.get('linked_to', []):
                            if link['node_name'] in ids:
                                lines += [f'  {ids[n["node_name"]]} -->|"{node_label(p["pin_name"])}"| {ids[link["node_name"]]}']
            lines += ['```', '', '</details>', '']
        lines += ['<details>', '<summary>전체 노드·기본 입력 목록</summary>', '', '| 노드 ID | 제목 | 미연결 기본 입력 / 에셋 입력 |', '|---|---|---|']
        for n in nodes:
            values = [f"{p['pin_name']}={p.get('default_object', p.get('default_value'))}" for p in n['pins']
                      if p.get('default_object') or (p.get('default_value') and not p.get('linked_to'))]
            lines += [f"| `{n['node_name']}` | {escaped(n['node_title'])} | {escaped('; '.join(values))} |"]
        lines += ['', '</details>', '']
    skipped = bp.get('unsupported_graphs', [])
    if skipped:
        lines += ['## 조회 범위의 한계', '', 'ObjectIterator로 발견했으나 MCP helper가 반환하지 않은 그래프입니다. delegate signature·컴파일된 중간 그래프·macro·애니메이션 내부 그래프가 섞일 수 있습니다.', '']
        lines += [f"- `{s['graph']}`: {s['reason']}" for s in skipped]
    rel = Path(__import__('os').path.relpath(EVIDENCE / snapshot_for[bp['path']], file.parent)).as_posix()
    lines += ['', f'[원본 MCP 스냅샷]({rel})', '']
    file.write_text('\n'.join(lines), encoding='utf-8')

lines = ['# Blueprint 조사 색인', '', '정적 도달은 시작 설정·패키징 설정·동적 경기 모드·로비 진입점을 루트로 한 hard/soft package 참조입니다. 참조가 있어도 실제 노드가 호출되지 않을 수 있습니다. 참조가 없어도 문자열 로드가 가능하므로 삭제 판정으로 사용할 수 없습니다.', '',
         '| 에셋 | 부모 | 루트 참조 | 그래프 / 노드 | 상세 |', '|---|---|---|---|---|']
for b in sorted(blueprints, key=lambda x:x['path']):
    lines += [f"| `{b['path']}` | `{b['parent']}` | {'있음' if b['reachable'] else '없음'} | {len(b['graphs'])} / {sum(g['node_count'] for g in b['graphs'])} | [조사 기록]({md_path(b['path'])}) |"]
(DOC / 'BlueprintIndex.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')

lines = ['# 전체 로직 에셋 분류', '', 'MCP Asset Registry 기준. 외부 에셋 팩·템플릿은 목록만 조사했으며, 상세 그래프 조사 범위는 BlueprintIndex에 표시합니다. 여기서 도달 여부는 정적 package 참조이며 실행 판정이 아닙니다.', '',
         '| 경로 | 타입 | 루트 참조 | 직접 참조자 수 | 상세 조사 |', '|---|---|---|---|---|']
bp_paths = {b['path'] for b in blueprints}
for a in sorted(inventory,key=lambda x:x['path']):
    link = f"[상세]({md_path(a['path'])})" if a['path'] in bp_paths else '목록 / 별도 시스템 문서'
    lines += [f"| `{a['path']}` | `{a['class']}` | {'있음' if a['reachable'] else '없음'} | {len(a['referencers'])} | {link} |"]
(DOC / 'AssetInventory.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')

source_files = sorted((ROOT / 'Source').rglob('*'))
lines = ['# C++ 조사 색인', '', '실제 프로젝트 루트의 Source만 포함합니다. Assignment/Source는 배포 복사본이므로 별도 구현으로 중복 집계하지 않았습니다. Runtime 구현 / 테스트 / Editor 도구를 구분합니다.', '']
contracts = ['# 문자열·Reflection 결합 지점', '', '클래스·함수·프로퍼티 이름을 변경할 때 호출자도 함께 이행해야 하는 정적 검색 결과입니다. 아래는 원문 위치이며, 모든 문자열이 런타임에 쓰인다는 뜻은 아닙니다.', '', '| 파일 | 줄 | 원문 |', '|---|---|---|']
for file in source_files:
    if file.suffix not in ['.h','.cpp']:continue
    text = file.read_text(encoding='utf-8',errors='replace')
    rel = file.relative_to(ROOT).as_posix()
    category = 'Editor 도구' if 'ShootingArenaEditorTools' in rel else ('테스트' if '/Tests/' in rel else 'Runtime')
    lines += [f'## {file.name}', '', f'분류: {category}. [소스](../../{rel})', '']
    declarations = []
    for i,line in enumerate(text.splitlines(),1):
        if re.search(r'^\s*(?:class|struct|enum class)\s+\w',line) or re.search(r'^\w[\w\s:*&<>]*::\w+\(',line):
            declarations.append(f'- {i}: `{line.strip()}`')
        if any(token in line for token in ['FindFProperty','FindFunction','ProcessEvent','LoadObject<','LoadClass<','/Game/QuakeLike','StartsWith(TEXT("BP_','GetName() == ClassName']):
            contracts += [f'| [{file.name}](../../{rel}) | {i} | `{escaped(line.strip())}` |']
    lines += declarations or ['- 선언·구현 항목이 없거나 매크로로 정의됩니다.']
    lines += ['']
(DOC / 'CppIndex.md').write_text('\n'.join(lines),encoding='utf-8')
(DOC / 'ReflectionContracts.md').write_text('\n'.join(contracts)+'\n',encoding='utf-8')
summary = {'blueprints':len(blueprints),'graphs':sum(len(b['graphs']) for b in blueprints),
           'nodes':sum(g['node_count'] for b in blueprints for g in b['graphs']),
           'blueprint_load_errors':sum(len(json.loads(p.read_text(encoding='utf-8'))['errors']) for p in EVIDENCE.glob('blueprints-*.json')),
           'record_errors':[{'path':b['path'],'error':b['error']} for b in blueprints if 'error' in b],
           'inventory_assets':len(inventory),'cpp_files':len([p for p in source_files if p.suffix in ['.h','.cpp']]),
           'unsupported_graph_entries':sum(len(b.get('unsupported_graphs',[])) for b in blueprints)}
(EVIDENCE / 'coverage.json').write_text(json.dumps(summary,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(summary,ensure_ascii=False))

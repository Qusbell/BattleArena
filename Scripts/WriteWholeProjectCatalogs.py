"""Generate navigable catalogs and review annotations from a completed static audit."""
import collections
import csv
import hashlib
import json
from pathlib import Path
import runpy as _audit_runpy
_audit_paths = _audit_runpy.run_path(str(Path(__file__).with_name('AuditPaths.py')))
resolve_audit_path = _audit_paths['resolve_audit_path']
recorded_hash_matches = _audit_paths['recorded_hash_matches']
import re
import posixpath

ROOT = Path(__file__).resolve().parents[1]


def get(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def put(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text.replace('\r\n','\n'), encoding='utf-8', newline='\n')


def cell(value):
    return str(value or '').replace('|',' / ').replace('\n',' ')


def build():
    summary = get(ROOT/'docs/repository-audit/architecture/audit-summary.json')
    run = resolve_audit_path(summary['run_directory'])
    native = get(run/'native-types.json')
    bps = get(run/'blueprint-catalog.json')
    # Same explicitly heuristic routing used by the analyzer.
    import runpy
    classify = runpy.run_path(str(ROOT/'Scripts/AnalyzeWholeProjectAudit.py'))['system']
    systems = sorted({r['system_inference'] for r in native} | {classify(r['package']) for r in bps})
    links = []
    for name in systems:
        filename = name.replace('.','-') + '.md'
        ns = [r for r in native if r['system_inference']==name]
        bs = [r for r in bps if classify(r['package'])==name]
        links.append(f'| [{name}](catalogs/{filename}) | {len(ns)} | {len(bs)} |')
        lines = [f'# {name} 객체 카탈로그\n',
                 '이 그룹은 경로 키워드에 따른 **분류 추론**입니다. 소유권·실행 여부를 증명하지 않습니다.\n'
                 '아래 역할 요약은 클래스 선언 주석, 상속 태그와 실제 그래프 이름을 근거로 하며, 이름만으로 동작을 확정하지 않습니다.\n',
                 '## C++ 선언\n', '| 타입 | 역할 근거 / 헤더 함수 검색어 | 선언 위치 |', '|---|---|---|']
        for r in ns:
            desc = r['comment'] or ('선언 종류: ' + r['kind'] + '; 상속: ' + (r['bases'] or '없음') + '; 업무 역할 미서술')
            methods = ', '.join(r['header_function_symbols'][:8])
            lines.append(f"| `{r['name']}` | {cell(desc)} / 헤더 검색어: {cell(methods)} | [{r['file']}:{r['line']}](../../../../{r['file']}) |")
        lines += ['\n헤더 검색어는 같은 헤더에 선언된 다른 타입의 함수도 포함할 수 있는 탐색 보조 자료입니다.\n',
                  '## Blueprint 정의\n', '| 객체 경로 | 상속 / 정의 역할 | 확인한 그래프 |', '|---|---|---|']
        for r in bs:
            parent = r['tags'].get('ParentClass') or r['tags'].get('NativeParentClass') or '미기록'
            graph_names = ', '.join(g['name'] for g in r['graphs']) or '최상위 그래프 없음; 상속/데이터 중심 정의 가능'
            lines.append(f"| `{cell(r['object_path'])}` | {cell(r['class'].split('.')[-1])}; {cell(parent)} | {cell(graph_names)} |")
        lines.append('\n호출 노드·변수·컴포넌트 상세는 로컬 실행 폴더의 `blueprint-catalog.json`과 `graphs/`에 있습니다.\n'
                     'JSON 조회기 외 그래프는 `graph-text/`의 Unreal 텍스트 내보내기로 보충했으며, 컴파일 파생 그래프는 별도 제외 목록에 기록합니다.\n')
        put(ROOT/'docs/repository-audit/architecture/catalogs'/filename, '\n'.join(lines))
    intro = '# 전수조사 객체 카탈로그\n\n'
    intro += f"기준 커밋 `{summary['git_commit']}` / 조사일 2026-11-05 / [진행 상태](../audit/progress.json)\n\n"
    intro += '소스 선언과 Blueprint 정의는 아래 그룹에서 찾습니다. 리소스·맵·외부 Actor 레코드를 포함한 전체 에셋은\n'
    intro += f"`{summary['run_directory']}/asset-roles.csv`에 클래스 기반 역할, 분류 추론, 도달 관계를 함께 기록했습니다.\n\n"
    intro += '| 분류 추론 | C++ 타입 | Blueprint |\n|---|---:|---:|\n'+'\n'.join(links)+'\n\n'
    intro += '## 해석 기준\n\n'
    intro += '- C++ 타입에는 class·struct·enum 선언이 포함됩니다. 자동 추출이며 완전한 C++ AST 분석은 아닙니다.\n'
    intro += '- Blueprint는 클래스 정의입니다. 레벨에 배치된 인스턴스의 설정과 같다고 가정하지 않습니다.\n'
    intro += '- 외부 Actor/Object 패키지는 Asset Registry 레코드와 의존 관계로 조사했습니다. 맵의 모든 인스턴스 속성을 로드하지 않았습니다.\n'
    intro += '- Texture·Mesh·Material·Sound 등은 클래스별 리소스 역할을 전수 기록했습니다. 미술적 의도는 이름만으로 확정하지 않았습니다.\n'
    intro += '- 동적 문자열 로딩·실제 실행·Cook 성공은 별도 검증이 필요합니다. 도달하지 못한 에셋을 삭제 대상으로 확정하지 않습니다.\n\n'
    intro += '## 자료\n\n| 자료 | 내용 |\n|---|---|\n'
    for file,desc in [('SystemMap.md','사람이 검토한 시스템 경계와 실제 연결'),('DataAuthority.md','JSON과 에셋 편집 기준'),('Findings.md','정리 전 확인할 구체적 발견'),
                      ('blueprint-index.csv','Blueprint 경로·부모·그래프 색인'),('native-types.json','소스 타입·주석·위치'),('data-bindings.json','실제 Registry binding'),
                      ('module-dependencies.json','Build.cs 모듈 의존 문자열'),('plugins.json','로컬 플러그인 선언'),('workflows.json','스크립트·실행 도구의 원본 설명/파일 색인'),('maps.json','전체 World 패키지 연결'),('document-review.json','기존 문서 분류·대조 기록'),('audit-summary.json','범위·수치·소스 해시')]:
        intro += f'| [{file}]({file}) | {desc} |\n'
    put(ROOT/'docs/repository-audit/architecture/README.md',intro)

    # Preserve original runtime measurements while making current authority and verification scope visible.
    reviews=get(run/'document-review.json')
    archive_index=ROOT/'docs/history/index.json'
    archived={r['source'].casefold():r for r in get(archive_index)['documents']} if archive_index.exists() else {}
    asset_rows=get(run/'assets.json')
    asset_names=collections.Counter(a['name'] for a in asset_rows)
    source_text='\n'.join(p.read_text(encoding='utf-8-sig',errors='replace') for p in (ROOT/'Source').rglob('*.cpp'))
    changed=[]
    for r in reviews:
        if r['path']=='docs/README.md':
            continue
        path=resolve_audit_path(r['path'])
        r['current_path']=path.relative_to(ROOT).as_posix()
        if r['path'].casefold() in archived:
            item=archived[r['path'].casefold()]
            r.update({'archive_path':item['target'],'archive_category':item['category'],'archive_reason':item['reason'],'current_use':False})
        raw=path.read_bytes()
        encoding='utf-8-sig'
        try:text=raw.decode(encoding)
        except UnicodeDecodeError:
            encoding='cp949';text=raw.decode(encoding)
        named_assets=sorted(set(re.findall(r'\b(?:BP|BPC|BPI|BTT|BTS|WBP|DT|DA|PDA|BT|BB|ABP|EUW)_[A-Za-z0-9_]+\b',text)))
        named_paths=sorted(set(re.findall(r'(?:Source|Scripts|Tools|Plugins|Content|Saved)/[^\s`|()<>\"\']+',text)))
        r['asset_name_checks']=[{'name':n,'matching_registry_objects':asset_names[n]} for n in named_assets]
        r['path_checks']=[{'path':p.rstrip('.,;'),'exists_in_workspace':(ROOT/p.rstrip('.,;')).exists()} for p in named_paths]
        tests=sorted(set(re.findall(r'ShootingArena\.[A-Za-z0-9_.]+',text)))
        r['test_name_checks']=[{'name':name,'literal_or_filter_prefix_in_source':name in source_text,'runtime_rerun':False} for name in tests]
        r['review_limit']='Reference existence and source symbols were checked; full prose semantics and historical runtime assertions were not revalidated.'
        marker='<!-- repository-audit-20261105:start -->'
        if marker in text:
            r['annotation_applied']=True
            r['sha256_after_reconciliation']=hashlib.sha256(raw).hexdigest()
            continue
        prefix=posixpath.relpath('docs/repository-audit/architecture',path.parent.relative_to(ROOT).as_posix())+'/'
        # ../docs works from both Git's Docs/ and docs/ paths on case-sensitive systems.
        kind=r['category']
        if kind=='historical_experiment':
            note='과거 실험·수정 기록입니다. 아래 수치와 통과 결과는 당시 이력으로 보존하며 이번에 재실행하지 않았습니다.'
        elif kind=='legacy_manual':
            note='레거시 사용 설명서입니다. 아래 QuakeLike_Base/LJH 경로를 현재 설정으로 사용하지 않습니다. 현재 Life 설정은 Content/DataAssets/DA_DefaultLife.json → /Game/QuakeLike_1_0/Data/Chracter/DA_DefaultLife입니다. 현재 스폰 흐름은 BP_ItemSpawnPoint와 BPC_SpawnerManager 카탈로그를 확인합니다.'
        else:
            note='현재 소스·에셋과 연결을 정적 대조한 참고 문서입니다. 아래 과거 빌드·자동 테스트·실행 결과를 이번 검증 결과로 해석하지 않습니다.'
        extra=''
        if any(word in path.name for word in ['ItemSound','BuffWarning']):
            extra='\n> 아이템/버프 DT는 JsonAssetSync에 등록되어 있습니다. 지속할 설정 변경은 원본 JSON에도 반영해야 하며, Editor 시작 및 패키징 실행 시작 시 JSON이 다시 적용됩니다.\n'
        if path.name.startswith('PathLink'):
            extra+='\n> 현재 캐시 저장은 WorldSettings의 UPathLinkCacheStorageComponent이며 APathLinkRoutingCache Actor는 호환용 레거시입니다. 알고리즘의 과거 실험 조건과 현행 저장 구조를 구분합니다.\n'
        if path.suffix=='.txt':
            annotation=f'{marker}\n2026-11-05 조사: {note}\n{prefix}SystemMap.md 및 DataAuthority.md를 확인합니다.\n<!-- repository-audit-20261105:end -->\n\n'
        else:
            annotation=f'{marker}\n> 2026-11-05 저장소 조사: {note}\n> 현행 연결과 미확인 사항은 [시스템 지도]({prefix}SystemMap.md), [데이터 원본 정책]({prefix}DataAuthority.md)를 확인합니다.\n{extra}<!-- repository-audit-20261105:end -->\n\n'
        put(path,annotation+text)
        r['annotation_applied']=True
        r['encoding_before']=encoding
        r['sha256_after_reconciliation']=hashlib.sha256(path.read_bytes()).hexdigest()
        changed.append(r['path'])
    write_path=ROOT/'docs/repository-audit/architecture/document-review.json'
    write_path.write_text(json.dumps(reviews,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'catalog_groups':len(systems),'native_types':len(native),'blueprints':len(bps),'documents_annotated':len(changed)},ensure_ascii=False))


if __name__=='__main__':
    build()

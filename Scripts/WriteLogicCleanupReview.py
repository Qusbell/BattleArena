"""Classify live-validated logic cleanup candidates; creates reports only, never deletes assets."""
import argparse
import collections
import csv
import datetime as dt
import hashlib
import json
from pathlib import Path
import runpy

ROOT=Path(__file__).resolve().parents[1]
RESOLVE=runpy.run_path(str(ROOT/'Scripts/AuditPaths.py'))['resolve_audit_path']
MANUAL_KEEP={
 '/Game/QuakeLike_1_0/EUW_ArenaAnalyzer':'Current documented analysis UI; manually invoked Editor entrypoint.',
 '/Game/EditorTools_YJH/EUW_UnusedAssetFinder':'Manually invoked maintenance utility; no evidence that its purpose is retired.',
}
ROLES={
 'BPC_AreaRiskComponent':'Area Manager에 유닛 인식·전투 위험을 전달하는 미연결 컴포넌트',
 'BPC_AIWeaponSelection1':'무기 점수 계산·선택을 가진 별도 AI 컴포넌트',
 'BP_QuakeAIController':'현행 AI/AIC/BP_QuakeAIController2와 별개인 Controller 구현',
 'BTS_SelectRetreatMovePosition':'Area 위험도를 이용해 후퇴 위치를 정하는 BT Service',
 'BTT_SelectRetreatMovePosition':'후퇴 이동 위치를 정하는 BT Task',
 'BTT_ClearRetreatState':'후퇴 Blackboard 상태를 초기화하는 BT Task',
 'BP_DroneTarget':'별도 Controller가 참조하는 타깃 Actor 정의',
 'BTS_UpdateDroneLocation':'구형 BT_AIaction에서 사용하는 Drone 위치 갱신 Service',
 'BTTask_MoveToTarget_HBC':'구형 BT_AIaction에서 사용하는 이동 Task',
 'BTTask_TryUse':'구형 BT_AIaction에서 사용하는 사용 시도 Task',
 'BP_LJH_GameState':'구형 스폰·재스폰 큐와 안전 위치 선택 GameState',
 'BPC_BGH_Life':'구형 피해·회복·사망·과충전 Life 컴포넌트',
 'BP_BGH_BaseChar':'BGH/HBC/LJH/PJH/YJS 레거시 구현이 공유하는 캐릭터 기반',
 'BP_HeldItemBase':'PJH 구형 장착 아이템 기반',
 'BP_PJH_ProjectileBase':'PJH 구형 투사체 기반',
 'BP_PJH_ProjectileWeaponBase':'PJH 구형 발사 무기 기반',
 'BPC_TargetPriorityStack':'LTG/STG·가중치·경로 효율을 계산하는 별도 목표 스택',
 'EUW_JsonImporter':'설정 기반 DT JSON 생성·검증·수동 가져오기 Editor Utility',
}


def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def write(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8',newline='\n')


def build(run):
    run=RESOLVE(run)
    if not run.is_relative_to(ROOT/'docs/repository-audit/audit/generated'):raise ValueError('Invalid output')
    state=read(run/'state.json');rows=read(run/'candidates-preliminary.json');live={r['package']:r for r in read(run/'candidates-live.json')}
    assert state['collector_status']=='complete' and not state['live_reference_changes'] and not state['live_missing_candidates']
    assert state['dirty_before']==state['dirty_after']=={'content':[],'maps':[]}
    old=RESOLVE(state['baseline_run']);bp={r['package']:r for r in read(old/'blueprint-catalog.json')}
    forward=collections.defaultdict(set)
    for f in old.glob('dependencies-*.json'):
        for a,b,_ in read(f)['rows']:forward[a].add(b)
    for r in rows:forward[r['package']].update(r['outgoing_packages'])
    keep=set(MANUAL_KEEP);pending=list(keep)
    while pending:
        for p in forward[pending.pop()]:
            if p not in keep:keep.add(p);pending.append(p)
    held=[];result=[]
    for r in rows:
        if r['package'] in keep:
            held.append({'package':r['package'],'reason':MANUAL_KEEP.get(r['package'],'Dependency of retained manual Editor entrypoint')});continue
        r={**r,'tier':r['proposed_tier'],'live_referencers_by_kind':live[r['package']]['referencers_by_kind']}
        assert r['entry_reachable'] is False and r['protected_reachable'] is False
        assert not (set(live[r['package']]['referencers'])&keep)
        if r['name']=='EUW_JsonImporter' or '/JsonParser/' in r['package'] or r['name']=='PDA_JsonImporterConfig':
            r['tier']=3;r['special_review']='Manual legacy JSON importer feature, not an ordinary zero-referencer runtime asset.'
        r['role']=ROLES.get(r['name'],'')
        if not r['role']:
            kind=r['class'].rsplit('.',1)[-1]
            graphs=[g['name'] for g in bp.get(r['package'],{}).get('graphs',[]) if g['name'] not in ('EventGraph','UserConstructionScript')]
            r['role']={'World':'현재 진입점 밖의 World 정의',
                       'UserDefinedStruct':'사용자 데이터 구조 정의','UserDefinedEnum':'사용자 열거형 정의',
                       'DataTable':'구조화된 행 데이터','BehaviorTree':'AI 행동 트리','BlackboardData':'AI Blackboard 키 정의',
                       'EnvQuery':'EQS 공간·상황 질의 정의','StateTree':'상태 기계 로직 정의',
                       'AnimBlueprint':'애니메이션 상태·포즈 제어 Blueprint','WidgetBlueprint':'UI 위젯 로직 정의',
                       'InputAction':'입력 동작 정의','InputMappingContext':'입력 매핑 정의'}.get(kind,'Blueprint/설정 정의')
            if graphs:r['role']+='; 그래프: '+', '.join(graphs[:4])
        r['classification_basis']={1:'No package/string referencers or known authored source/test/tool literals; outside protected entry flow.',
                                   2:'Outside protected flow; at most 2 direct, 5 logic and 10 total transitive dependents.',
                                   3:'Outside protected flow; wider legacy/prototype dependency cluster or manual legacy feature.'}[r['tier']]
        r['confidence']='static_isolation' if r['tier']==1 else 'thin_cold_dependency' if r['tier']==2 else 'legacy_or_unconnected_prototype_hypothesis'
        result.append(r)
    result.sort(key=lambda r:(r['tier'],r['package']))
    output=ROOT/'docs/repository-audit/cleanup';(output/'lists').mkdir(parents=True,exist_ok=True)
    counts=dict(collections.Counter(r['tier'] for r in result))
    compact=[]
    for r in result:
        compact.append({k:r[k] for k in ['tier','package','name','class','role','classification_basis','confidence','registry_referencers','literal_referencers','source_mentions','incoming_count','transitive_dependent_count','transitive_logic_dependent_count','legacy_signal','entry_reachable','protected_reachable','live_referencers_by_kind']})
    write(run/'candidates-final.json',result);write(output/'candidates.json',compact);write(output/'retained-manual-tools.json',held)
    columns=['tier','package','class','role','incoming_count','transitive_dependent_count','transitive_logic_dependent_count','legacy_signal','registry_referencers','literal_referencers','source_mentions']
    with (output/'candidates.csv').open('w',encoding='utf-8-sig',newline='') as f:
        writer=csv.writer(f);writer.writerow(columns)
        for r in compact:writer.writerow([json.dumps(r[k],ensure_ascii=False) if isinstance(r[k],(list,dict)) else r[k] for k in columns])
    for tier in (1,2,3):
        text=[f'# 분류 {tier} 전체 목록\n',
              '판정 기준과 실행 검증 한계는 [삭제 후보 명세](../README.md)를 먼저 확인합니다. 이 목록은 삭제 실행 기록이 아닙니다.\n',
              '| 패키지 경로 | 역할 근거 | 직접 역참조 | 간접 로직 영향 | 참조자/소스 |','|---|---|---:|---:|---|']
        for r in result:
            if r['tier']!=tier:continue
            refs=r['registry_referencers']+r['literal_referencers']
            notes=', '.join('`'+p+'`' for p in refs[:3])
            if len(refs)>3:notes+=' 외 '+str(len(refs)-3)+'개'
            if r['source_mentions']:notes+='; 소스 '+', '.join(s['file']+':'+str(s['line']) for s in r['source_mentions'][:2])
            text.append('| `'+r['package']+'` | '+r['role'].replace('|',' / ')+' | '+str(r['incoming_count'])+' | '+str(r['transitive_logic_dependent_count'])+' | '+(notes or '없음')+' |')
        (output/'lists'/f'tier-{tier}.md').write_text('\n'.join(text)+'\n',encoding='utf-8',newline='\n')
    summary={'schema_version':1,'reviewed_utc':state['live_checked_utc'],'entrypoint':state['entrypoint'],
             'entry_world_settings':state['entry_world_settings'],'run_directory':run.relative_to(ROOT).as_posix(),
             'baseline_commit':state['baseline_commit'],'current_commit':state['current_commit'],
             'candidate_count':len(result),'tiers':counts,'protected_logic_assets':state['logic_assets']-len(result),'retained_manual_entries':held,
             'live_revalidated_assets':len(live),'reference_changes':len(state['live_reference_changes']),
             'dirty_after':state['dirty_after'],'deletions':'none','runtime_tests':'not_run','cook_tests':'not_run',
             'limits':['Tier 1 means static isolation in the defined entry flow, not an unconditional proof against all future/direct/manual loads.',
                       'Manual Editor entrypoints can have zero referencers and still be useful; retained separately.',
                       'Tier 2 counts transitive impact, not only direct referencers. Delete/refactor callers first.',
                       'Tier 3 is a legacy/prototype hypothesis, never an instruction to remove a whole folder.',
                       'Simple art resources, UI style BPs and environment construction BPs excluded. AnimBPs are executable pose/state definitions, not animation clips.']}
    scripts=['Scripts/PrepareLogicCleanupReview.py','Scripts/CollectLogicCleanupReview.py','Scripts/WriteLogicCleanupReview.py','Scripts/VerifyLogicCleanupReview.py']
    summary['script_sha256']={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in scripts}
    summary['evidence_sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in run.iterdir() if p.is_file()}
    write(output/'summary.json',summary)
    print(json.dumps({'candidates':len(result),'tiers':counts,'held':len(held),'protected_logic_assets':summary['protected_logic_assets']},ensure_ascii=False))


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run');build(p.parse_args().run)

"""경기 기록 검증 스크립트 (완료 조건 1~4).

사용:
    python verify_match.py [DB경로] [MatchId]

- DB경로 생략: ../Saved/Analyze/ShootingArenaLog.db
- MatchId 생략: 가장 최근에 시작한 경기
"""
import sqlite3
import sys
from pathlib import Path

default_db = Path(__file__).resolve().parent.parent / 'Saved' / 'Analyze' / 'ShootingArenaLog.db'
db_path = Path(sys.argv[1]) if len(sys.argv) > 1 else default_db
con = sqlite3.connect(db_path)
q = lambda sql, *a: con.execute(sql, a).fetchall()

matches = q('SELECT MatchId, MapId, GameBuildVersion, SettingsVersion FROM match ORDER BY rowid')
print(f'DB: {db_path}')
print(f'기록된 경기 수: {len(matches)}')
for m in matches:
    print(f'  {m[0]}  {m[1]}  settings={m[3]}')
if not matches:
    sys.exit(1)

match_id = sys.argv[2] if len(sys.argv) > 2 else matches[-1][0]
print(f'\n검증 대상: {match_id}\n')

print('[1] 테이블별 행 수')
for t in ['event', 'match', 'match_end', 'participant', 'participant_result', 'spawn', 'spawn_weapon', 'spawn_end',
          'fire', 'damage', 'death', 'weapon_equip', 'item_spawn', 'pickup', 'item_remove', 'buff']:
    print(f'  {t:20s}{q(f"SELECT COUNT(*) FROM {t} WHERE MatchId=?", match_id)[0][0]:>8}')

print('\n[2] 누락·참조 검증 (v_record_issues)')
issues = q('SELECT Code, Severity, COUNT(*) FROM v_record_issues WHERE MatchId=? GROUP BY 1,2', match_id)
print('  문제 없음' if not issues else '\n'.join(f'  {c} [{s}] x{n}' for c, s, n in issues))
fk = q('PRAGMA foreign_key_check')
print(f'  외래키 위반: {len([r for r in fk if True])}건 (전체 DB 기준)')

print('\n[3] 이벤트 순서 연속성')
gap = q('SELECT COUNT(*), MAX(EventSequence) FROM event WHERE MatchId=?', match_id)[0]
print(f'  이벤트 {gap[0]}개, 최대 순서 {gap[1]} -> {"연속" if gap[0] == gap[1] else "누락 있음"}')

print('\n[4] 게임이 확정한 최종 결과 vs 기록에서 센 값')
rows = q('''SELECT r.ParticipantId, p.CharacterId, r.FinalKills,
                   (SELECT COUNT(*) FROM death d WHERE d.MatchId=r.MatchId AND d.KillerParticipantId=r.ParticipantId),
                   r.FinalDeaths,
                   (SELECT COUNT(*) FROM death d WHERE d.MatchId=r.MatchId AND d.VictimParticipantId=r.ParticipantId)
            FROM participant_result r JOIN participant p USING(MatchId, ParticipantId)
            WHERE r.MatchId=? ORDER BY r.ParticipantId''', match_id)
if not rows:
    print('  participant_result 없음 (경기가 정상 종료되지 않았을 수 있습니다)')
bad = 0
print('  참가자  캐릭터                  킬(최종/기록)  사망(최종/기록)')
for pid, ch, fk_, ck, fd, cd in rows:
    flag = '' if (fk_, fd) == (ck, cd) else '  <-- 불일치'
    bad += flag != ''
    print(f'  {pid:>4}   {ch:22s}  {fk_:>4}/{ck:<4}      {fd:>4}/{cd:<4}{flag}')
end = q('SELECT MatchEndReason FROM match_end WHERE MatchId=?', match_id)
print(f'\n종료 사유: {end[0][0] if end else "(종료 기록 없음)"}')
print('결과: ' + ('모두 일치' if rows and not bad else f'불일치 {bad}건' if rows else '확인 불가'))

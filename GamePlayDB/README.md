# GamePlayDB — 전투 데이터 DB 연결 (2026-10-29)

지시서(`261028_전투데이터_DB연결_지시서.md`)에 따라 한 경기에서 발생한 전투 데이터를 SQLite에 저장합니다.

## 폴더 구성

| 파일 | 내용 |
|---|---|
| `README.md` | 이 문서. 사용법과 검증 방법 |
| `연결표.md` | 저장 테이블별 실제 코드 연결 위치와 실행 시점 (지시서 10-2 제출 항목 1) |
| `구현상태.md` | 확정 필요 사항별 현재 구현 상태와 알려진 한계 (제출 항목 5, 4) |
| `aggregates.sql` | 집계 조회 5종(`weapon_stats`, `character_weapon_stats`, `weapon_totals`, `character_stats`, `matchup_stats`) |
| `verify_match.py` | 경기 기록 검증 스크립트 (누락·참조·최종 결과 대조) |
| `ShootingArenaLog_sample.db` | 실제 경기 1개가 기록된 샘플 DB. 집계 뷰까지 들어 있어 바로 조회할 수 있습니다 |

## DB 위치

- 기본: `Saved/Analyze/ShootingArenaLog.db` (프로젝트 폴더 기준, 서버/PIE/단독 실행 모두 같은 규칙)
- 바꾸기: 실행 인자 `-ArenaLogDb=D:/경로/파일.db`
- 스키마 템플릿: `Content/Data/ShootingArenaLog.db` (커밋된 빈 DB). 실행 시에는 코드에 내장된 같은 스키마로 자동 생성하며 템플릿 파일에는 쓰지 않습니다.
- 스키마를 바꿨다면 `Tools/ArenaMatchLog/GenerateSchema.py`로 `ArenaMatchLogSchema.inc`를 다시 만드세요.

## 기록이 만들어지는 조건

- 서버 또는 단독 실행(클라이언트 제외)에서만 기록합니다.
- 경기용 맵(게임모드가 경기 시계를 쓰는 맵)에서만 기록합니다. 로비·메인메뉴는 기록하지 않습니다.
- 첫 캐릭터가 스폰될 때 경기가 시작되고 `MatchId`가 만들어집니다. 경기마다 새 `MatchId`를 씁니다.
- 경기 종료는 게임의 종료 신호(`bIsMatchEnded`)로 감지해 `TimeLimit` 또는 `KillTarget`으로 기록합니다. 종료 신호 없이 월드가 닫히면 `Forced`로 기록합니다.

## 저장 실패 시

- 일시적 오류는 자동 재시도합니다. 끝내 못 쓴 문장과 DB가 거부한 문장은 `<DB 파일>.failed.sql`에 오류와 함께 남습니다. 이 파일이 생겼다면 기록에 빠진 부분이 있다는 뜻입니다.
- 경기가 끝나고 저장이 모두 끝나면 로그(`LogArenaMatchLog`)에 `validated: no missing events or references` 또는 발견된 문제가 한 줄로 남습니다.

## 사용법

### 특정 경기의 데이터 조회 (완료 조건 2)

```sql
-- 경기 목록
SELECT MatchId, MapId, SettingsVersion FROM match;

-- 한 경기의 사망 기록 (킬러, 인정 방식, 근거 피해 포함)
SELECT e.ElapsedSeconds, d.VictimParticipantId, d.DeathCause, d.KillCreditSource, d.KillerParticipantId
FROM death d JOIN event e USING (MatchId, EventSequence)
WHERE d.MatchId = '<MatchId>' ORDER BY e.EventSequence;
```

### 여러 경기 구분 (완료 조건 3)
모든 테이블에 `MatchId`가 있고, 경기 안에서만 유일한 값(참가자 ID, 공격 ID, 생성 아이템 ID)은 항상 `MatchId`와 함께 씁니다.

### 집계 조회
```bash
sqlite3 ShootingArenaLog.db < GamePlayDB/aggregates.sql
```
```sql
SELECT * FROM weapon_stats WHERE MatchId = '<MatchId>' AND ParticipantId = 6;
SELECT * FROM character_weapon_stats ORDER BY CharacterId, EquipRank;
SELECT * FROM matchup_stats;
```
`sqlite3`가 없으면 Python으로 `executescript`하면 됩니다. 샘플 DB에는 이미 들어 있습니다.

### 실제 플레이와 비교 (완료 조건 4)
```bash
python GamePlayDB/verify_match.py                    # 가장 최근 경기
python GamePlayDB/verify_match.py <DB경로> <MatchId>
```
출력: 테이블별 행 수, `v_record_issues` 결과, 이벤트 순서 연속성, 게임이 확정한 최종 킬/사망 수와 기록에서 센 값의 비교.

## 검증 결과 (실제 경기, 2026-10-29)

`ShootingArenaLog_sample.db`는 110초짜리 경기 1개(종료 사유 `Forced`)입니다:

- 이벤트 1347개, 순서 누락 없음
- `v_record_issues` 문제 없음, 외래키 위반 0건
- 7명 모두 최종 킬/사망이 기록에서 센 값과 일치
- 생애 25개 모두 `spawn_weapon`이 있음, 샷건 한 발은 `ShotElementCount` 16

그 밖에 이전 실행에서 확인한 것(샘플 DB에는 들어 있지 않음):
- 시간 제한 종료(`TimeLimit`) 경기에서 7명 전원의 최종 킬/사망이 기록과 일치, 무기별 킬 합계 = 킬러가 있는 사망 수
- 버프는 직접 주운 시각과 `Gain`이 일치하고 `Expire`가 30초 뒤 또는 사망 시각에 찍힘

확인하지 못한 것은 `구현상태.md`에 있습니다.

# 블루프린트 연결 가이드 (수집 형식 버전 1)

탄약·발사 조건·적용 설정·버프 연결은 C++만으로 기록되므로 블루프린트 수정이 필요 없습니다. 직접 해야 하는 일은 **플레이어 행동 기록(조건 4)을 위한 `BP_PlayerShooter` 수정**입니다.

> 입력과 화면 정보는 **플레이어의 PC(클라이언트)에서만** 알 수 있습니다. 서버+클라이언트로 분리된 구조(PIE 분리 서버, 실제 배포)에서도 기록되도록, C++ 컴포넌트 `ArenaBehaviorReporter` 가 클라이언트에서 서버로 전달합니다. 이 컴포넌트를 쓰지 않고 `Record Attack Input` 같은 라이브러리 노드를 클라이언트 이벤트에서 직접 부르면 분리된 서버에서는 아무것도 기록되지 않습니다.

> 2026-11-11 MCP로 실제 그래프를 읽어 확인한 내용입니다. 핀 이름과 앞뒤 노드 이름으로 찾으세요.

## 먼저 확인된 것: BPC_Buff 는 수정할 필요 없음

`ApplyBuff`, `ApplyDroppedBuff` 둘 다 `Set currentBuffRowName` 이 `Record Buff Applied` 보다 먼저 실행됩니다. 추가 작업이 없습니다.

## 관측 켜기

관측 주기를 정하지 않으면 컴포넌트가 아무것도 보내지 않고 `observation_config` 도 생기지 않습니다(정상).
- `Content/QuakeLike_1_0/Data/DA_ArenaLogging` 의 **Behavior Sample Interval Seconds** 에 초 단위 값(0 = 끔), 또는
- 서버 실행 인자 `-ArenaBehaviorSampleInterval=<초>` (에셋 값보다 우선)
- `Logging Enabled` 도 켜져 있어야 합니다. 서버가 주기를 정하면 컴포넌트가 소유 클라이언트에 전달하므로, 클라이언트 쪽 설정은 필요 없습니다.

## 1. 컴포넌트 추가 (1회)

1. `Content/QuakeLike_1_0/Character/BP_PlayerShooter` 를 엽니다(Viewport/Components 탭).
2. Components 패널 **Add** → 검색창에 `Arena Behavior Reporter` → 추가. 이름은 그대로 `ArenaBehaviorReporter` 로 둡니다.
3. 별도 설정은 없습니다. 복제(Replicates)는 컴포넌트가 스스로 켭니다. 단, `BP_PlayerShooter` 자체의 **Replicates** 가 켜져 있어야 합니다(이미 멀티플레이 캐릭터이므로 켜져 있을 것입니다).
4. 컴파일하고 저장합니다.

## 2. 공격 입력 — Event Graph / `IA_Shoot`

현재 연결:
- `Ongoing` → `Get Selected Item (BPI Inventory)` → `Is Valid` → `Try Use (BPI Usable)`  ← 누르는 동안 계속
- `Canceled` → (재경유 노드) → `Get Selected Item` → `Is Valid` → `Stop Using`  ← 뗄 때
- `Started`, `Triggered`, `Completed` 는 비어 있음

노드 3개를 끼워 넣습니다. 변수 목록에서 `ArenaBehaviorReporter` 를 끌어다 Get 으로 놓고, 거기서 선을 끌어 `Report Attack Input` 을 만듭니다.

| # | 위치 | 설정 |
|---|---|---|
| ① | `Ongoing` 과 `Get Selected Item` 사이 | `Report Attack Input` — `Target`=ArenaBehaviorReporter, `b Pressed`=체크 |
| ② | `Canceled` 과 재경유 노드 사이 | `Report Attack Input` — `b Pressed`=해제 |
| ③ | `Completed` (비어 있음) | `Report Attack Input` 하나 더 — `b Pressed`=해제 |

- `Ongoing` 이 매 프레임 호출돼도 됩니다. 같은 상태가 반복되면 컴포넌트가 서버로 보내지 않습니다.
- ③은 트리거 설정에 따라 뗄 때 `Completed` 가 올 수도 있어서 넣습니다.

## 3. 점프 입력 — Event Graph / `IA_Jump`

현재: `Started` → `Jump` → `Switch on EMovementMode`, `Completed` → `Stop Jumping`.

- `Started` 와 `Jump` 사이에 **`Report Jump Input`** (`Target`=ArenaBehaviorReporter) 을 끼웁니다: `Started` → `Report Jump Input` → `Jump`.
- 반드시 `Jump` 보다 **앞**에 두세요(입력 순간의 "점프 가능 여부"를 서버가 기록합니다).
- 점프가 실제로 실행됐는지는 서버(C++)가 지면→공중 전환으로 따로 기록합니다(`JumpKind='Executed'`).

## 4. 강제 이동 (선택) — `Record Forced Movement`

플레이어의 점프가 아닌 원인으로 공중에 뜨는 곳에 서버에서 실행되는 경로로 알립니다. 서버에서 실행되는 곳이면 이 라이브러리 노드를 그대로 쓰면 됩니다.

| 원인 | 상태 |
|---|---|
| 점프패드 | **C++로 연결함** (`JumpPad`) |
| 포탈 출구 발사 | **C++로 연결함** (`Teleport`) |
| 넉백 | 직접 연결 필요 — 밀어내는 노드(`Launch Character`, `Add Impulse` 등) 바로 앞에 `Record Forced Movement` (`Pawn`=밀리는 캐릭터, `Cause`=`Knockback`). 서버에서 실행되는 이벤트 안에 둡니다 |
| 텔레포트 스킬 | 직접 연결 필요 — 같은 방법, `Cause`=`TeleportSkill` |

연결하지 않으면 그 이륙이 플레이어의 점프(`Executed`)로 기록될 수 있습니다. 1~3번에는 영향이 없습니다.

## 확인

```sql
-- 입력: Start / End 가 번갈아 나오는지
SELECT * FROM attack_input WHERE MatchId = '<MatchId>' ORDER BY EventSequence;
-- 점프: InputPressed / Executed / Forced 구분
SELECT JumpKind, bGrounded, bCanJump, ForcedCause FROM jump_observation WHERE MatchId = '<MatchId>';
-- 노출·가림
SELECT * FROM exposure_change WHERE MatchId = '<MatchId>' LIMIT 20;
-- 한 시간순 목록
SELECT ElapsedSeconds, Kind, Detail FROM v_behavior_timeline WHERE MatchId = '<MatchId>' AND ParticipantId = <사람 참가자 ID> ORDER BY ElapsedSeconds, EventSequence;
```

## 참고

- 노출(`exposure_change`)은 플레이어의 PC 가 "화면 안에 있다 / 벽에 가려졌다"를 판정해 서버로 보냅니다. 거리와 높이차는 서버가 실제 위치로 직접 계산합니다. 클라이언트 값이므로 변조 가능성이 있는 분석용 기록입니다.
- 탄약 `ammo_change` : 서버 C++이 무기 액터의 `currentUsageCount` 를 읽습니다.
- 발사 목표 `fire_context` : AI 는 컨트롤러의 `CurrentTarget` (`BP_QuakeAIController2`)을 읽습니다. 사람 플레이어는 목표 열이 NULL 입니다.

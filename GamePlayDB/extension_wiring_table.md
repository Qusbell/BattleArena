# 확장 연결표 — 탄약·발사 조건·적용 설정·버프 효과·행동 관측 (수집 형식 버전 1)

기준 지시서: `261111_전투데이터_DB연결_지시서.md`. 경로는 프로젝트 루트 기준이며 기록 담당은 `Source/ShootingArena/Private/Analyzer/ArenaMatchLogSubsystem.cpp`(이하 `MatchLog`)입니다. 스키마 원본은 `Tools/ArenaMatchLog/extension_v1.sql`, `extension_views_v1.sql` 이고 `apply_extension.py` 가 템플릿 DB(`Content/Data/ShootingArenaLog.db`)에 적용해 `ArenaMatchLogSchema.inc` 를 다시 만듭니다. 기존 DB 파일은 열 때 `MatchLog::OpenDatabase` 가 컬럼·테이블·뷰를 자동으로 보강합니다(기존 행은 NULL 유지, 새 경기는 `match.CollectionVersion=1`).

## 1. 탄약

| 저장 | 실행 시점 | 연결 위치 |
|---|---|---|
| `spawn_weapon.InitialAmmo`, `bUnlimitedAmmo` | 초기 무기 지급 직후 | `MatchLog::CompleteSpawn` → `CollectWeaponAmmo`(무기 액터 `currentUsageCount`) |
| `ammo_change` (Fire) | 발사 성립 직후 | `MatchLog::RecordFire` → `WriteAmmoChange`. 발사 이벤트를 먼저 발급하고 탄약 이벤트의 `CauseEventSequence` 로 연결 |
| `ammo_change` (Pickup) | 아이템 획득 효과 적용 후 | `MatchLog::CommitItemPickup`. 탄약 아이템은 `targetItemId` 태그가 일치하는 무기, 무기 아이템은 새로 생기거나 변한 무기 |
| 무한 탄약 | — | `maxUsageCount` 가 0 이하이거나 99999 이상이면 무한(`DA_Pistol` 이 99999). 무한이면 전후 수량 NULL |

- 무기 블루프린트가 발사를 보고하기 전에 `Set Current Count` 로 탄약을 먼저 깎으므로, `AmmoAfter` 는 보고 시점 값이고 `AmmoBefore` 는 로그가 마지막으로 본 값입니다. 중간 변경을 놓치면 `v_ammo_history.bContinuityBreak=1` 로 드러납니다.
- 이 게임에는 `Other`(발사·획득 외) 탄약 변경을 일으키는 경로를 확인하지 못해 기록하지 않습니다.

## 2. 발사 조건

| 저장 | 실행 시점 | 연결 위치 |
|---|---|---|
| `fire_context.EquippedWeaponId` | 발사 성립 시 | `MatchLog::RecordFire` (`EquippedByLife`: 스폰·`weapon_equip` 로 갱신) |
| `fire_context.Target*` | 발사 성립 시 | `MatchLog::FindTargetPawnLife`: AI 컨트롤러의 `CurrentTarget` 만 사용. 사람/목표 없음은 NULL |
| `TargetDistance` | 발사 성립 시 | 발사자와 목표 폰의 액터 위치 직선 거리(cm) |

행은 `fire` 와 같은 키로 `FlushPendingFires` 에서 함께 저장됩니다.

## 3. 참가자별 실제 적용 설정 (`participant_settings`)

시점: 참가자 첫 스폰 확정 시 `MatchLog::TryWriteParticipant` → `ArchiveParticipantSettings`.

| SourceTable | 내용 |
|---|---|
| `Pawn:LifeInfo` | 폰의 `BPC_Life.LifeInfo` 구조체 |
| `Pawn:CharacterMovement` | 걷기/웅크리기 속도, 가속, 점프, 중력, 공중 제어, 마찰, 제동 |
| `DataAsset:<클래스>` | 폰·컴포넌트·컨트롤러가 가리키는 데이터 에셋 전체(`DA_DefaultLife`, `DA_MovementOption`, 봇의 `DA_DefaultAggression` 등) |
| `DataTable:DT_Characteristic` | 봇 성격 행 하나 |
| `DataTable:DT_WeaponChange_Weight`, `DT_Item_Weight` | 봇 무기 교체·아이템 가중치 표 전체 |

참가자별 보정 로직을 코드에서 찾지 못해 "보정 후 값"은 위 실제 객체 값과 같습니다. 성격 9개 값은 `bot_settings` 참조를 유지하고 복사하지 않습니다. 사람 참가자는 봇 전용 행(`DataTable:*`, 컨트롤러 에셋)을 만들지 않습니다.

## 4. 버프 효과

| 저장 | 실행 시점 | 연결 위치 |
|---|---|---|
| `buff_instance` | 새 효과 적용(Gain/Replace) | `MatchLog::RecordBuffApplied` (블루프린트 `BPC_Buff` 가 호출). `RowName` = 컴포넌트의 `currentBuffRowName`, 원본 표는 `CurrentBuffItem` 의 DataTable 이름(없으면 `DT_BuffTable`) |
| `buff.BuffId/PreviousBuffId` | 버프 변경 | 같은 효과 갱신은 기존 ID(Refresh), 다른 행으로 교체는 새 ID + 이전 ID(Replace), 만료/사망 소실은 해제한 ID |
| `buff_instance.SourceEventSequence` | 획득 | 같은 프레임의 버프 아이템 `pickup` 이벤트. 콜백 순서가 뒤바뀌면 `MatchLog::RecordItemPickup` 이 나중에 `UPDATE` 로 연결 |
| `damage.bBuffsRecorded`, `damage_buff` | 피해 처리 | `UArenaAnalyzeSubsystem::ApplyShotDamage` 가 `MatchLog::CaptureBuffLinks` 로 ApplyDamage **직전**에 공격자·피해자의 활성 효과 확보 → `EndDamage` 에서 저장 |

- 공격자가 발사 이후 리스폰해 다른 생애가 되었다면 그 버프는 계산에 쓰인 값이 아니므로 `bBuffsRecorded=0` 으로 남기고 연결 행을 만들지 않습니다(없다고 가정하지 않음).
- 피해 계산이 명중 시점의 공격자 버프를 쓰는지(발사 시점이 아닌지)는 기존 `AttackerBuffTag` 와 같은 기준(명중 시점)으로 따릅니다.

## 5. 플레이어 행동 관측 (사람 참가자만, 표본 주기 설정 시)

| 저장 | 실행 시점 | 연결 위치 |
|---|---|---|
| `observation_config` | 경기 시작 | `MatchLog::BeginBehaviorObservation` (주기와 노출 판정 규칙 문자열 보존) |
| `observation_sample` | 설정한 주기마다 | `MatchLog::TickBehavior` → `ObserveHumanLife`. 조준 방향은 컨트롤러 조준(무기 산포 적용 전), 이동은 관측 대상 기준 전후/좌우. `SampleIntervalSeconds` 는 이전 표본과의 실제 간격 |
| `exposure_change` | 노출/가림 상태가 바뀔 때 | 플레이어 PC 의 `UArenaBehaviorReporterComponent::PollExposure` 가 판정(노출=대상 위치가 뷰포트 안에 투영, 가림=눈높이에서 대상으로의 시야선이 막힘)해 서버 RPC 로 보내면 `MatchLog::RecordExposureReport` 가 거리·높이를 서버 위치로 계산해 저장 |
| `fire_availability` | 발사 가능 여부가 바뀔 때 | 같은 곳. 현재 판정은 `BP_HeldItem.CanUse` 와 같은 "수량>0"(무한 무기는 항상 가능)이며 사유는 `NoAmmo`, `NoWeapon` 뿐 |
| `attack_input` | 입력 시작/종료 | `BP_PlayerShooter` 의 `ArenaBehaviorReporter.Report Attack Input` → 서버 RPC → `MatchLog::RecordAttackInput` (가이드 2번) |
| `jump_observation` | 입력/실행/강제 | `ArenaBehaviorReporter.Report Jump Input`(가이드 3번), 지면→공중 전환 감지(서버 C++), `Record Forced Movement`(가이드 4번, 점프패드·포탈은 C++ 연결) |

## 6. 검증과 조회

- `v_extension_issues` (수집 형식 버전 1 경기만 검사) → `v_record_issues` 에 합쳐져 경기 종료 후 검증 로그와 `v_valid_matches` 에 반영됩니다.
- 조건별 비교: `weapon_distance_stats`, `weapon_ammo_stats`, `weapon_buff_stats` (기존 비교 조건 키 + 캐릭터 + 무기).
- 행동 조회: `v_behavior_timeline`, `v_exposure_response`, `v_reaction_stimulus`, `v_observation_gaps`.
- 예시 쿼리: `GamePlayDB/extension_queries.sql`.

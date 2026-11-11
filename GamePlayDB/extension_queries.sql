-- 수집 형식 버전 1 확장 조회 예시. 모든 뷰는 DB 안에 이미 있으므로 SELECT 만 실행하면 됩니다.
-- 아래 :match 는 match.MatchId 값으로 바꿔 사용합니다.

-- [조건 1] 탄약: 한 참가자의 생애·무기별 탄약 이력 (초기 지급 + 이후 변경, 끊김 표시)
SELECT * FROM v_ammo_history WHERE MatchId = :match AND ParticipantId = 0 ORDER BY SpawnNumber, WeaponId, EventSequence;

-- [조건 1·2] 발사 1회 → 같은 발사에서 생긴 탄약 처리와 발사 조건 (원본 연결)
SELECT f.EventSequence AS FireSeq, f.WeaponId, c.EquippedWeaponId, c.TargetParticipantId, c.TargetDistance,
       a.AmmoBefore, a.AmmoAfter, a.bUnlimitedAmmo
FROM fire f
JOIN fire_context c ON c.MatchId = f.MatchId AND c.EventSequence = f.EventSequence
LEFT JOIN event ea ON ea.MatchId = f.MatchId AND ea.CauseEventSequence = f.EventSequence AND ea.EventType = 'AmmoChanged'
LEFT JOIN ammo_change a ON a.MatchId = ea.MatchId AND a.EventSequence = ea.EventSequence
WHERE f.MatchId = :match ORDER BY f.EventSequence;

-- [조건 1] 참가자에게 실제 적용된 설정 (원본 설정이 같아도 참가자별로 보존)
SELECT ParticipantId, SourceTable, RowName, "Values" FROM participant_settings WHERE MatchId = :match ORDER BY ParticipantId, SourceTable;

-- [조건 1·2] 버프 효과: 가산/배율 효과를 행 이름으로 구분하고, 피해에 쓰인 효과와 원인 획득 이벤트까지 추적
SELECT d.EventSequence AS DamageSeq, db.Role, bi.RowName, json_extract(bs."Values", '$.modifierType') AS Modifier,
       json_extract(bs."Values", '$.statValue') AS StatValue, bi.SourceEventSequence AS PickupSeq, d.InputDamage
FROM damage d
JOIN damage_buff db ON db.MatchId = d.MatchId AND db.DamageEventSequence = d.EventSequence
JOIN buff_instance bi ON bi.MatchId = db.MatchId AND bi.BuffId = db.BuffId
LEFT JOIN balance_settings bs ON bs.SettingsVersion = bi.SettingsVersion AND bs.SourceTable = bi.SourceTable AND bs.RowName = bi.RowName
WHERE d.MatchId = :match;

-- [조건 1·2] 버프 변경 이력 (갱신은 같은 BuffId, 교체는 PreviousBuffId 로 이전 효과 추적)
SELECT EventSequence, ParticipantId, BuffChangeType, BuffId, PreviousBuffId, BuffExpiresAtSeconds FROM buff WHERE MatchId = :match ORDER BY EventSequence;

-- [조건 3] 거리 구간별 무기 사용량·명중률 (구간 폭은 analysis_param.DistanceBandCm; 목표가 없는 발사는 DistanceBand NULL)
SELECT CharacterId, WeaponId, DistanceFromCm, FireCount, HitAttackCount, HitRate, TargetedFireCount, 1.0 * TargetedFireCount / FireCount AS TargetedShare
FROM weapon_distance_stats ORDER BY CharacterId, WeaponId, DistanceBand;

-- [조건 3] 남은 탄약 구간별 (유한 무기만 구간이 있고 무한 무기는 AmmoBand NULL)
SELECT CharacterId, WeaponId, AmmoBand, FireCount, HitRate FROM weapon_ammo_stats ORDER BY CharacterId, WeaponId, AmmoBand;

-- [조건 3] 공격 버프 효과별 (None = 버프 없이 가한 피해)
SELECT CharacterId, WeaponId, AttackerBuffRow, ModifierType, StatValue, DamageEvents, AppliedDamagePerEvent FROM weapon_buff_stats ORDER BY CharacterId, WeaponId, AttackerBuffRow;

-- [조건 3] 기존 비교 (캐릭터 성과 / 무기 선호 / 캐릭터 간 상대 성적)
SELECT * FROM character_stats;
SELECT * FROM character_weapon_stats ORDER BY CharacterId, EquipRank;
SELECT * FROM matchup_stats;

-- [조건 4] 플레이어의 행동과 당시 조건을 한 시간순 목록으로 (노출, 공격 입력, 발사 가능, 점프, 발사, 피격)
SELECT ElapsedSeconds, Kind, Detail FROM v_behavior_timeline WHERE MatchId = :match AND ParticipantId = 5 ORDER BY ElapsedSeconds, EventSequence;

-- [조건 4] 적이 보였을 때 공격했는가: 노출 구간별 공격 입력 수, 발사 수, 당시 체력·아머·장착 무기·탄약
SELECT * FROM v_exposure_response WHERE MatchId = :match ORDER BY StartSeconds;

-- [조건 4] 자극(노출/피격) 이후 공격 입력·발사까지 걸린 시간 (이미 공격 입력을 유지하던 경우는 bAttackAlreadyHeld=1 로 표본 제외)
SELECT * FROM v_reaction_stimulus WHERE MatchId = :match AND bAttackAlreadyHeld = 0;

-- [조건 4] 후진하며 사격: 발사 시각 직전 표본의 관측 대상 기준 전후 속도 (음수 = 대상에서 멀어짐)
SELECT f.EventSequence, f.WeaponId, s.ElapsedSeconds, s.RelForwardSpeed, s.RelLateralSpeed, s.ObservedDistance
FROM fire f JOIN event e ON e.MatchId = f.MatchId AND e.EventSequence = f.EventSequence
JOIN observation_sample s ON s.MatchId = f.MatchId AND s.ParticipantId = f.ShooterParticipantId AND s.SpawnNumber = f.ShooterSpawnNumber
 AND s.ElapsedSeconds = (SELECT MAX(s2.ElapsedSeconds) FROM observation_sample s2 WHERE s2.MatchId = s.MatchId AND s2.ParticipantId = s.ParticipantId
                         AND s2.SpawnNumber = s.SpawnNumber AND s2.ElapsedSeconds <= e.ElapsedSeconds)
WHERE f.MatchId = :match AND s.RelForwardSpeed < 0;

-- [조건 4] 관측 누락 구간과 정상적인 무행동 구분 (누락 구간은 무입력으로 해석하지 않음)
SELECT * FROM v_observation_gaps WHERE MatchId = :match;

-- [검증] 확장 기록 누락·연결 오류
SELECT * FROM v_extension_issues WHERE MatchId = :match;

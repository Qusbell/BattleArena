-- ShootingArena 전투 기록 집계 조회 (지시서 11-2)
-- 원본 이벤트에서 계산하는 읽기 전용 뷰입니다. 원본 테이블에는 쓰지 않습니다.
-- 사용: sqlite3 ShootingArenaLog.db < aggregates.sql   (한 번 실행하면 뷰가 DB에 생깁니다)

DROP VIEW IF EXISTS v_life_span;
DROP VIEW IF EXISTS v_damage_net;
DROP VIEW IF EXISTS weapon_stats;
DROP VIEW IF EXISTS weapon_totals;
DROP VIEW IF EXISTS v_match_condition;
DROP VIEW IF EXISTS character_stats;
DROP VIEW IF EXISTS character_weapon_stats;
DROP VIEW IF EXISTS character_weapon_stats_no_default;
DROP VIEW IF EXISTS matchup_stats;

-- 생존 구간(스폰 ~ 스폰 종료). 모든 시간 집계는 살아 있는 구간만 사용합니다.
CREATE VIEW v_life_span AS
SELECT s.MatchId, s.ParticipantId, s.SpawnNumber,
       es.ElapsedSeconds AS StartSeconds, ee.ElapsedSeconds AS EndSeconds
FROM spawn s
JOIN event es ON es.MatchId = s.MatchId AND es.EventSequence = s.EventSequence
JOIN spawn_end se ON se.MatchId = s.MatchId AND se.ParticipantId = s.ParticipantId AND se.SpawnNumber = s.SpawnNumber
JOIN event ee ON ee.MatchId = se.MatchId AND ee.EventSequence = se.EventSequence;

-- 피해별 실제 감소량. 체력 초과 피해(음수 체력)는 제외하고, 자기 피해는 상대 대상 집계에서 빠집니다.
CREATE VIEW v_damage_net AS
SELECT d.MatchId, d.EventSequence, d.AttackId, d.AttackerParticipantId, d.VictimParticipantId, d.bSelfDamage,
       MAX(0, d.HealthBefore - MAX(d.HealthAfter, 0)) AS HealthLoss,
       MAX(0, d.ArmorBefore - d.ArmorAfter)           AS ArmorLoss
FROM damage d;

-- 경기 + 참가자 + 무기
CREATE VIEW weapon_stats AS
WITH keys AS (
  SELECT MatchId, ParticipantId, WeaponId FROM v_owned_span
  UNION SELECT MatchId, ParticipantId, WeaponId FROM v_equip_span
  UNION SELECT MatchId, ShooterParticipantId, WeaponId FROM fire
  UNION SELECT MatchId, ParticipantId, WeaponId FROM v_kill_attribution WHERE WeaponId IS NOT NULL
), owned AS (
  SELECT MatchId, ParticipantId, WeaponId, SUM(EndSeconds - StartSeconds) AS Sec FROM v_owned_span GROUP BY 1,2,3
), equipped AS (
  SELECT MatchId, ParticipantId, WeaponId, SUM(EndSeconds - StartSeconds) AS Sec FROM v_equip_span GROUP BY 1,2,3
), alive AS (
  SELECT MatchId, ParticipantId, SUM(EndSeconds - StartSeconds) AS Sec FROM v_life_span GROUP BY 1,2
), fires AS (
  SELECT MatchId, ShooterParticipantId AS ParticipantId, WeaponId, COUNT(*) AS N FROM fire GROUP BY 1,2,3
), fire_total AS (
  SELECT MatchId, ShooterParticipantId AS ParticipantId, COUNT(*) AS N FROM fire GROUP BY 1,2
), hits AS (
  -- 명중 공격 수: 자기 피해를 뺀 원인 공격을 중복 제거
  SELECT f.MatchId, f.ShooterParticipantId AS ParticipantId, f.WeaponId, COUNT(DISTINCT f.AttackId) AS N
  FROM fire f JOIN damage d ON d.MatchId = f.MatchId AND d.AttackId = f.AttackId AND d.bSelfDamage = 0
  GROUP BY 1,2,3
), dmg AS (
  SELECT f.MatchId, f.ShooterParticipantId AS ParticipantId, f.WeaponId,
         SUM(n.HealthLoss) AS Health, SUM(n.ArmorLoss) AS Armor
  FROM fire f JOIN v_damage_net n ON n.MatchId = f.MatchId AND n.AttackId = f.AttackId AND n.bSelfDamage = 0
  GROUP BY 1,2,3
), kills AS (
  SELECT MatchId, ParticipantId, WeaponId, COUNT(*) AS N FROM v_kill_attribution WHERE WeaponId IS NOT NULL GROUP BY 1,2,3
), kill_total AS (
  SELECT MatchId, ParticipantId, COUNT(*) AS N FROM v_kill_attribution GROUP BY 1,2
), pickups AS (
  SELECT p.MatchId, p.ParticipantId, i.ItemRowName AS WeaponId, COUNT(*) AS N
  FROM pickup p JOIN item_spawn i ON i.MatchId = p.MatchId AND i.SpawnedItemId = p.SpawnedItemId
  WHERE i.ItemCategory = 'HeldItem' GROUP BY 1,2,3
)
SELECT k.MatchId, k.ParticipantId, pt.CharacterId, k.WeaponId,
       COALESCE(o.Sec, 0)  AS OwnedSeconds,
       COALESCE(e.Sec, 0)  AS EquippedSeconds,
       1.0 * COALESCE(e.Sec, 0) / NULLIF(a.Sec, 0)       AS EquipShare,
       1.0 * COALESCE(e.Sec, 0) / NULLIF(o.Sec, 0)       AS OwnedEquipShare,
       COALESCE(f.N, 0)    AS FireCount,
       1.0 * COALESCE(f.N, 0) / NULLIF(ft.N, 0)          AS FireShare,
       COALESCE(h.N, 0)    AS HitAttackCount,
       1.0 * COALESCE(h.N, 0) / NULLIF(f.N, 0)           AS HitRate,
       COALESCE(dm.Health, 0) AS HealthDamage,
       COALESCE(dm.Armor, 0)  AS ArmorDamage,
       COALESCE(kl.N, 0)   AS KillCount,
       1.0 * COALESCE(kl.N, 0) / NULLIF(kt.N, 0)         AS KillShare,
       COALESCE(kl.N, 0) / NULLIF(COALESCE(e.Sec, 0) / 60.0, 0) AS KillsPerEquippedMinute,
       COALESCE(pk.N, 0)   AS WeaponPickupCount
FROM keys k
JOIN participant pt ON pt.MatchId = k.MatchId AND pt.ParticipantId = k.ParticipantId
LEFT JOIN owned o ON o.MatchId = k.MatchId AND o.ParticipantId = k.ParticipantId AND o.WeaponId = k.WeaponId
LEFT JOIN equipped e ON e.MatchId = k.MatchId AND e.ParticipantId = k.ParticipantId AND e.WeaponId = k.WeaponId
LEFT JOIN alive a ON a.MatchId = k.MatchId AND a.ParticipantId = k.ParticipantId
LEFT JOIN fires f ON f.MatchId = k.MatchId AND f.ParticipantId = k.ParticipantId AND f.WeaponId = k.WeaponId
LEFT JOIN fire_total ft ON ft.MatchId = k.MatchId AND ft.ParticipantId = k.ParticipantId
LEFT JOIN hits h ON h.MatchId = k.MatchId AND h.ParticipantId = k.ParticipantId AND h.WeaponId = k.WeaponId
LEFT JOIN dmg dm ON dm.MatchId = k.MatchId AND dm.ParticipantId = k.ParticipantId AND dm.WeaponId = k.WeaponId
LEFT JOIN kills kl ON kl.MatchId = k.MatchId AND kl.ParticipantId = k.ParticipantId AND kl.WeaponId = k.WeaponId
LEFT JOIN kill_total kt ON kt.MatchId = k.MatchId AND kt.ParticipantId = k.ParticipantId
LEFT JOIN pickups pk ON pk.MatchId = k.MatchId AND pk.ParticipantId = k.ParticipantId AND pk.WeaponId = k.WeaponId;

-- 경기 + 무기: 처치자가 있는 사망만 집계. 발사 무기를 모르는 처치는 WeaponId = NULL(무기 미상)로 따로 셉니다.
CREATE VIEW weapon_totals AS
SELECT MatchId, WeaponId, COUNT(*) AS TotalWeaponKills FROM v_kill_attribution GROUP BY MatchId, WeaponId;

-- 같은 비교 조건: 게임 버전, 설정 버전, 맵, 상대 구성이 같은 경기끼리만 합산합니다.
--   * 정상 종료된 경기(TimeLimit, KillTarget)만 포함합니다. 강제 종료(Forced)와 종료 기록이 없는 경기는 제외됩니다.
--   * Composition: 그 경기 참가자 전원의 "캐릭터:봇 설정 해시"를 정렬해 이은 값. 상대 구성이 같은지 비교하는 데 씁니다.
CREATE VIEW v_match_condition AS
SELECT m.MatchId, m.GameBuildVersion, m.SettingsVersion, m.MapId,
       (SELECT group_concat(x, '|') FROM (SELECT p2.CharacterId || ':' || COALESCE(p2.BotSettingsHash, 'human') AS x
                                          FROM participant p2 WHERE p2.MatchId = m.MatchId ORDER BY 1)) AS Composition,
       p.ParticipantId, p.CharacterId, p.DifficultyId, p.BotSettingsHash
FROM match m
JOIN match_end me ON me.MatchId = m.MatchId AND me.MatchEndReason <> 'Forced'
JOIN participant p ON p.MatchId = m.MatchId;

-- 비교 조건 + 캐릭터. 비율은 단순 평균이 아니라 분자·분모를 각각 합산해서 계산하세요.
CREATE VIEW character_stats AS
WITH per AS (
  SELECT c.GameBuildVersion, c.SettingsVersion, c.MapId, c.Composition, c.CharacterId, c.DifficultyId, c.BotSettingsHash,
         c.MatchId, c.ParticipantId,
         (SELECT COUNT(*) FROM death d WHERE d.MatchId = c.MatchId AND d.KillerParticipantId = c.ParticipantId) AS Kills,
         (SELECT COUNT(*) FROM death d WHERE d.MatchId = c.MatchId AND d.VictimParticipantId = c.ParticipantId) AS Deaths,
         (SELECT COALESCE(SUM(n.HealthLoss),0) FROM v_damage_net n WHERE n.MatchId = c.MatchId AND n.AttackerParticipantId = c.ParticipantId AND n.bSelfDamage = 0) AS HealthDmg,
         (SELECT COALESCE(SUM(n.HealthLoss),0) FROM v_damage_net n WHERE n.MatchId = c.MatchId AND n.VictimParticipantId = c.ParticipantId AND n.bSelfDamage = 0) AS TakenHealth,
         (SELECT COALESCE(SUM(n.ArmorLoss),0) FROM v_damage_net n WHERE n.MatchId = c.MatchId AND n.AttackerParticipantId = c.ParticipantId AND n.bSelfDamage = 0) AS ArmorDmg,
         (SELECT COALESCE(SUM(n.ArmorLoss),0) FROM v_damage_net n WHERE n.MatchId = c.MatchId AND n.VictimParticipantId = c.ParticipantId AND n.bSelfDamage = 0) AS TakenArmor,
         (SELECT COALESCE(SUM(l.EndSeconds - l.StartSeconds),0) FROM v_life_span l WHERE l.MatchId = c.MatchId AND l.ParticipantId = c.ParticipantId) AS Alive
  FROM v_match_condition c
)
SELECT GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash,
       COUNT(*) AS ParticipantCount,
       SUM(Kills) AS KillCount, SUM(Deaths) AS DeathCount,
       SUM(HealthDmg) AS HealthDamage, SUM(TakenHealth) AS TakenHealthDamage,
       SUM(ArmorDmg) AS ArmorDamage, SUM(TakenArmor) AS TakenArmorDamage,
       SUM(Alive) AS AliveSeconds
FROM per GROUP BY GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash;

-- 비교 조건 + 캐릭터 + 무기. 순위는 사용량 합계로 따로 매기며 동점은 1, 1, 3 으로, 사용량 0 은 순위 없음(NULL).
CREATE VIEW character_weapon_stats AS
WITH sums AS (
  SELECT c.GameBuildVersion, c.SettingsVersion, c.MapId, c.Composition, c.CharacterId, c.DifficultyId, c.BotSettingsHash, w.WeaponId,
         SUM(w.EquippedSeconds) AS EquippedSeconds, SUM(w.FireCount) AS FireCount
  FROM weapon_stats w JOIN v_match_condition c ON c.MatchId = w.MatchId AND c.ParticipantId = w.ParticipantId
  GROUP BY 1,2,3,4,5,6,7,8
)
SELECT *,
  CASE WHEN EquippedSeconds > 0 THEN RANK() OVER (PARTITION BY GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash
                                              ORDER BY EquippedSeconds DESC) END AS EquipRank,
  CASE WHEN FireCount > 0 THEN RANK() OVER (PARTITION BY GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash
                                         ORDER BY FireCount DESC) END AS FireRank
FROM sums;

-- 비교 조건 + 기준 캐릭터 + 상대 캐릭터. 자기 피해와 자기 처치는 제외합니다.
CREATE VIEW matchup_stats AS
WITH pair AS (
  SELECT a.GameBuildVersion, a.SettingsVersion, a.MapId, a.Composition, a.CharacterId, a.DifficultyId, a.BotSettingsHash,
         b.CharacterId AS OpponentCharacterId, a.MatchId, a.ParticipantId AS A, b.ParticipantId AS B
  FROM v_match_condition a JOIN v_match_condition b ON b.MatchId = a.MatchId AND b.ParticipantId <> a.ParticipantId
)
SELECT p.GameBuildVersion, p.SettingsVersion, p.MapId, p.Composition, p.CharacterId, p.DifficultyId, p.BotSettingsHash, p.OpponentCharacterId,
  SUM((SELECT COUNT(*) FROM death d WHERE d.MatchId = p.MatchId AND d.KillerParticipantId = p.A AND d.VictimParticipantId = p.B)) AS KillCount,
  SUM((SELECT COUNT(*) FROM death d WHERE d.MatchId = p.MatchId AND d.KillerParticipantId = p.B AND d.VictimParticipantId = p.A)) AS DeathCount,
  SUM((SELECT COALESCE(SUM(n.HealthLoss),0) FROM v_damage_net n WHERE n.MatchId = p.MatchId AND n.AttackerParticipantId = p.A AND n.VictimParticipantId = p.B AND n.bSelfDamage = 0)) AS HealthDamage,
  SUM((SELECT COALESCE(SUM(n.HealthLoss),0) FROM v_damage_net n WHERE n.MatchId = p.MatchId AND n.AttackerParticipantId = p.B AND n.VictimParticipantId = p.A AND n.bSelfDamage = 0)) AS TakenHealthDamage,
  SUM((SELECT COALESCE(SUM(n.ArmorLoss),0) FROM v_damage_net n WHERE n.MatchId = p.MatchId AND n.AttackerParticipantId = p.A AND n.VictimParticipantId = p.B AND n.bSelfDamage = 0)) AS ArmorDamage,
  SUM((SELECT COALESCE(SUM(n.ArmorLoss),0) FROM v_damage_net n WHERE n.MatchId = p.MatchId AND n.AttackerParticipantId = p.B AND n.VictimParticipantId = p.A AND n.bSelfDamage = 0)) AS TakenArmorDamage
FROM pair p
GROUP BY p.GameBuildVersion, p.SettingsVersion, p.MapId, p.Composition, p.CharacterId, p.DifficultyId, p.BotSettingsHash, p.OpponentCharacterId;

-- 기본 무기(스폰 때 항상 지급되는 무기)를 빼고 매긴 선호 순위. 위 character_weapon_stats(기본 무기 포함)와 구분해서 쓰세요.
CREATE VIEW character_weapon_stats_no_default AS
WITH defaults AS (SELECT DISTINCT WeaponId FROM spawn_weapon WHERE bIsDefaultWeapon = 1),
sums AS (
  SELECT GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash, WeaponId, EquippedSeconds, FireCount
  FROM character_weapon_stats WHERE WeaponId NOT IN (SELECT WeaponId FROM defaults)
)
SELECT *,
  CASE WHEN EquippedSeconds > 0 THEN RANK() OVER (PARTITION BY GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash
                                              ORDER BY EquippedSeconds DESC) END AS EquipRank,
  CASE WHEN FireCount > 0 THEN RANK() OVER (PARTITION BY GameBuildVersion, SettingsVersion, MapId, Composition, CharacterId, DifficultyId, BotSettingsHash
                                         ORDER BY FireCount DESC) END AS FireRank
FROM sums;

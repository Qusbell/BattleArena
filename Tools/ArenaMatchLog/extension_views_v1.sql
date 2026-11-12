-- 확장 조회. 구분자 '-- @@' 로 한 문장씩 나눠 적용합니다 (apply_extension.py).

INSERT OR REPLACE INTO analysis_param (Name, Value) VALUES ('GapToleranceFactor', 2.0);
-- @@
CREATE VIEW v_ammo_history AS
WITH points AS (
 SELECT sw.MatchId,sw.ParticipantId,sw.SpawnNumber,sw.WeaponId,e.EventSequence AS Seq,e.ElapsedSeconds AS Seconds,
  'Initial' AS ChangeReason,NULL AS CauseEventSequence,sw.bUnlimitedAmmo,NULL AS AmmoBefore,sw.InitialAmmo AS AmmoAfter
 FROM spawn_weapon sw JOIN spawn s USING(MatchId,ParticipantId,SpawnNumber)
 JOIN event e ON e.MatchId=s.MatchId AND e.EventSequence=s.EventSequence
 UNION ALL
 SELECT a.MatchId,a.ParticipantId,a.SpawnNumber,a.WeaponId,a.EventSequence,e.ElapsedSeconds,
  a.ChangeReason,e.CauseEventSequence,a.bUnlimitedAmmo,a.AmmoBefore,a.AmmoAfter
 FROM ammo_change a JOIN event e ON e.MatchId=a.MatchId AND e.EventSequence=a.EventSequence
), chained AS (
 SELECT *,LAG(Seq) OVER w AS PreviousSeq,LAG(AmmoAfter) OVER w AS PreviousAmmoAfter
 FROM points WINDOW w AS (PARTITION BY MatchId,ParticipantId,SpawnNumber,WeaponId ORDER BY Seq)
)
SELECT MatchId,ParticipantId,SpawnNumber,WeaponId,Seq AS EventSequence,Seconds AS ElapsedSeconds,ChangeReason,
 CauseEventSequence,bUnlimitedAmmo,AmmoBefore,AmmoAfter,PreviousAmmoAfter,
 CASE WHEN ChangeReason<>'Initial' AND bUnlimitedAmmo=0 AND PreviousSeq IS NOT NULL
       AND AmmoBefore IS NOT PreviousAmmoAfter THEN 1 ELSE 0 END AS bContinuityBreak
FROM chained
-- @@
CREATE VIEW v_observation_gaps AS
SELECT MatchId,ParticipantId,SpawnNumber,PrevSeconds,ElapsedSeconds AS NextSeconds,
 ElapsedSeconds-PrevSeconds AS GapSeconds,ConfiguredInterval AS SampleIntervalSeconds
FROM (SELECT s.*,c.SampleIntervalSeconds AS ConfiguredInterval,
       LAG(s.ElapsedSeconds) OVER(PARTITION BY s.MatchId,s.ParticipantId,s.SpawnNumber ORDER BY s.ElapsedSeconds) AS PrevSeconds
      FROM observation_sample s JOIN observation_config c ON c.MatchId=s.MatchId)
WHERE PrevSeconds IS NOT NULL
 AND ElapsedSeconds-PrevSeconds > ConfiguredInterval*(SELECT Value FROM analysis_param WHERE Name='GapToleranceFactor')
-- @@
CREATE VIEW v_extension_issues AS
SELECT f.MatchId,'AMMO_FIRE_LINK_MISSING' AS Code,'ERROR' AS Severity,f.EventSequence,'발사에 연결된 탄약 처리 기록 없음' AS Detail
FROM fire f JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND NOT EXISTS(
 SELECT 1 FROM ammo_change a JOIN event e ON e.MatchId=a.MatchId AND e.EventSequence=a.EventSequence
 WHERE a.MatchId=f.MatchId AND a.ChangeReason='Fire' AND e.CauseEventSequence=f.EventSequence)
UNION ALL
SELECT a.MatchId,'AMMO_CAUSE_MISSING','ERROR',a.EventSequence,'발사 또는 획득 원인의 탄약 처리에 원인 이벤트 없음'
FROM ammo_change a JOIN match m USING(MatchId) JOIN event e ON e.MatchId=a.MatchId AND e.EventSequence=a.EventSequence
WHERE m.CollectionVersion=1 AND a.ChangeReason IN ('Fire','Pickup') AND e.CauseEventSequence IS NULL
UNION ALL
SELECT a.MatchId,'AMMO_VALUE_MISSING','ERROR',a.EventSequence,'유한 탄약 처리의 전후 수량 누락'
FROM ammo_change a JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND a.bUnlimitedAmmo=0 AND (a.AmmoBefore IS NULL OR a.AmmoAfter IS NULL)
UNION ALL
SELECT h.MatchId,'AMMO_CONTINUITY_BREAK','WARNING',h.EventSequence,'이전 변경 후 수량과 다음 변경 전 수량이 다름: 중간 탄약 변경 누락 확인'
FROM v_ammo_history h JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND h.bContinuityBreak=1
UNION ALL
SELECT sw.MatchId,'AMMO_INITIAL_MISSING','ERROR',s.EventSequence,'초기 보유 무기의 초기 탄약 또는 무한 탄약 여부 누락'
FROM spawn_weapon sw JOIN match m USING(MatchId)
JOIN spawn s ON s.MatchId=sw.MatchId AND s.ParticipantId=sw.ParticipantId AND s.SpawnNumber=sw.SpawnNumber
WHERE m.CollectionVersion=1 AND (sw.bUnlimitedAmmo IS NULL OR (sw.bUnlimitedAmmo=0 AND sw.InitialAmmo IS NULL))
UNION ALL
SELECT f.MatchId,'FIRE_CONTEXT_MISSING','ERROR',f.EventSequence,'발사 조건 행 없음'
FROM fire f JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND NOT EXISTS(SELECT 1 FROM fire_context c WHERE c.MatchId=f.MatchId AND c.EventSequence=f.EventSequence)
UNION ALL
SELECT p.MatchId,'PARTICIPANT_SETTINGS_MISSING','ERROR',NULL,'참가자별 적용 설정 없음'
FROM participant p JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND NOT EXISTS(SELECT 1 FROM participant_settings s WHERE s.MatchId=p.MatchId AND s.ParticipantId=p.ParticipantId)
UNION ALL
SELECT b.MatchId,'BUFF_INSTANCE_MISSING','ERROR',b.EventSequence,'버프 변경의 버프 적용 ID 또는 적용 효과 누락'
FROM buff b JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND (b.BuffId IS NULL OR NOT EXISTS(SELECT 1 FROM buff_instance i WHERE i.MatchId=b.MatchId AND i.BuffId=b.BuffId))
UNION ALL
SELECT i.MatchId,'BUFF_SOURCE_MISSING','WARNING',NULL,'버프 적용 ID '||i.BuffId||'의 원인 이벤트 없음'
FROM buff_instance i JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND i.SourceEventSequence IS NULL
UNION ALL
SELECT d.MatchId,'DAMAGE_BUFF_UNRECORDED','ERROR',d.EventSequence,'피해의 버프 연결 수집 완료 여부 없음'
FROM damage d JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND d.bBuffsRecorded IS NULL
UNION ALL
SELECT d.MatchId,'DAMAGE_BUFF_LINK_MISSING','ERROR',d.EventSequence,'버프 태그가 있는 피해에 효과 연결 없음'
FROM damage d JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1 AND d.bBuffsRecorded=1 AND (
 (d.AttackerBuffTag IS NOT NULL AND NOT EXISTS(SELECT 1 FROM damage_buff x WHERE x.MatchId=d.MatchId AND x.DamageEventSequence=d.EventSequence AND x.Role='Attacker'))
 OR (d.VictimBuffTag IS NOT NULL AND NOT EXISTS(SELECT 1 FROM damage_buff x WHERE x.MatchId=d.MatchId AND x.DamageEventSequence=d.EventSequence AND x.Role='Victim')))
UNION ALL
SELECT g.MatchId,'OBSERVATION_GAP','WARNING',NULL,'행동 관측 누락 구간 '||ROUND(g.PrevSeconds,2)||'초~'||ROUND(g.NextSeconds,2)||'초: 무입력으로 해석하지 않음'
FROM v_observation_gaps g JOIN match m USING(MatchId)
WHERE m.CollectionVersion=1
-- @@
CREATE VIEW v_behavior_timeline AS
SELECT x.MatchId,x.ParticipantId,x.SpawnNumber,e.ElapsedSeconds,x.EventSequence,'ExposureChanged' AS Kind,
 json_object('Observed',x.ObservedParticipantId,'bExposed',x.bExposed,'bOccluded',x.bOccluded,'Distance',x.Distance,'HeightDelta',x.HeightDelta) AS Detail
FROM exposure_change x JOIN event e USING(MatchId,EventSequence)
UNION ALL
SELECT x.MatchId,x.ParticipantId,x.SpawnNumber,e.ElapsedSeconds,x.EventSequence,'AttackInput',
 json_object('State',x.InputState,'Weapon',x.EquippedWeaponId)
FROM attack_input x JOIN event e USING(MatchId,EventSequence)
UNION ALL
SELECT x.MatchId,x.ParticipantId,x.SpawnNumber,e.ElapsedSeconds,x.EventSequence,'FireAvailability',
 json_object('bCanFire',x.bCanFire,'BlockReason',x.BlockReason,'Weapon',x.EquippedWeaponId)
FROM fire_availability x JOIN event e USING(MatchId,EventSequence)
UNION ALL
SELECT x.MatchId,x.ParticipantId,x.SpawnNumber,e.ElapsedSeconds,x.EventSequence,'Jump',
 json_object('Kind',x.JumpKind,'bGrounded',x.bGrounded,'bCanJump',x.bCanJump,'ForcedCause',x.ForcedCause)
FROM jump_observation x JOIN event e USING(MatchId,EventSequence)
UNION ALL
SELECT x.MatchId,x.ShooterParticipantId,x.ShooterSpawnNumber,e.ElapsedSeconds,x.EventSequence,'Fire',
 json_object('Weapon',x.WeaponId,'AttackId',x.AttackId,'Target',c.TargetParticipantId,'TargetDistance',c.TargetDistance)
FROM fire x JOIN event e USING(MatchId,EventSequence)
LEFT JOIN fire_context c USING(MatchId,EventSequence)
UNION ALL
SELECT x.MatchId,x.VictimParticipantId,x.VictimSpawnNumber,e.ElapsedSeconds,x.EventSequence,'DamageTaken',
 json_object('Attacker',x.AttackerParticipantId,'InputDamage',x.InputDamage,'HealthAfter',x.HealthAfter,'ArmorAfter',x.ArmorAfter)
FROM damage x JOIN event e USING(MatchId,EventSequence)
-- @@
CREATE VIEW v_exposure_response AS
WITH exposures AS (
 SELECT x.MatchId,x.EventSequence,x.ParticipantId,x.SpawnNumber,x.ObservedParticipantId,x.ObservedSpawnNumber,
  e.ElapsedSeconds AS StartSeconds,x.Distance,x.HeightDelta,x.bOccluded,
  (SELECT MIN(e2.ElapsedSeconds) FROM exposure_change x2 JOIN event e2 ON e2.MatchId=x2.MatchId AND e2.EventSequence=x2.EventSequence
   WHERE x2.MatchId=x.MatchId AND x2.ParticipantId=x.ParticipantId AND x2.SpawnNumber=x.SpawnNumber
    AND x2.ObservedParticipantId=x.ObservedParticipantId AND x2.ObservedSpawnNumber=x.ObservedSpawnNumber
    AND x2.bExposed=0 AND e2.ElapsedSeconds>=e.ElapsedSeconds AND x2.EventSequence>x.EventSequence) AS EndSeconds
 FROM exposure_change x JOIN event e USING(MatchId,EventSequence) WHERE x.bExposed=1
)
SELECT ex.MatchId,ex.EventSequence AS ExposureEventSequence,ex.ParticipantId,ex.SpawnNumber,ex.ObservedParticipantId,
 ex.StartSeconds,ex.EndSeconds,ex.Distance,ex.HeightDelta,ex.bOccluded,
 (SELECT SUM(1) FROM attack_input a JOIN event ea ON ea.MatchId=a.MatchId AND ea.EventSequence=a.EventSequence
   WHERE a.MatchId=ex.MatchId AND a.ParticipantId=ex.ParticipantId AND a.SpawnNumber=ex.SpawnNumber AND a.InputState='Start'
    AND ea.ElapsedSeconds>=ex.StartSeconds AND (ex.EndSeconds IS NULL OR ea.ElapsedSeconds<=ex.EndSeconds)) AS AttackStartsWhileExposed,
 (SELECT SUM(1) FROM fire f JOIN event ef ON ef.MatchId=f.MatchId AND ef.EventSequence=f.EventSequence
   WHERE f.MatchId=ex.MatchId AND f.ShooterParticipantId=ex.ParticipantId AND f.ShooterSpawnNumber=ex.SpawnNumber
    AND ef.ElapsedSeconds>=ex.StartSeconds AND (ex.EndSeconds IS NULL OR ef.ElapsedSeconds<=ex.EndSeconds)) AS FiresWhileExposed,
 (SELECT d.HealthAfter FROM damage d JOIN event ed ON ed.MatchId=d.MatchId AND ed.EventSequence=d.EventSequence
   WHERE d.MatchId=ex.MatchId AND d.VictimParticipantId=ex.ParticipantId AND d.VictimSpawnNumber=ex.SpawnNumber AND ed.ElapsedSeconds<=ex.StartSeconds
   ORDER BY ed.EventSequence DESC LIMIT 1) AS LastKnownHealth,
 (SELECT d.ArmorAfter FROM damage d JOIN event ed ON ed.MatchId=d.MatchId AND ed.EventSequence=d.EventSequence
   WHERE d.MatchId=ex.MatchId AND d.VictimParticipantId=ex.ParticipantId AND d.VictimSpawnNumber=ex.SpawnNumber AND ed.ElapsedSeconds<=ex.StartSeconds
   ORDER BY ed.EventSequence DESC LIMIT 1) AS LastKnownArmor,
 (SELECT q.WeaponId FROM v_equip_span q WHERE q.MatchId=ex.MatchId AND q.ParticipantId=ex.ParticipantId AND q.SpawnNumber=ex.SpawnNumber
   AND q.StartSeconds<=ex.StartSeconds AND (q.EndSeconds IS NULL OR q.EndSeconds>ex.StartSeconds) LIMIT 1) AS EquippedWeaponId,
 (SELECT h.AmmoAfter FROM v_ammo_history h
   JOIN v_equip_span q ON q.MatchId=ex.MatchId AND q.ParticipantId=ex.ParticipantId AND q.SpawnNumber=ex.SpawnNumber
    AND q.StartSeconds<=ex.StartSeconds AND (q.EndSeconds IS NULL OR q.EndSeconds>ex.StartSeconds)
   WHERE h.MatchId=ex.MatchId AND h.ParticipantId=ex.ParticipantId AND h.SpawnNumber=ex.SpawnNumber AND h.WeaponId=q.WeaponId
    AND h.ElapsedSeconds<=ex.StartSeconds ORDER BY h.EventSequence DESC LIMIT 1) AS AmmoAtExposure
FROM exposures ex
-- @@
CREATE VIEW v_reaction_stimulus AS
WITH stimuli AS (
 SELECT MatchId,ParticipantId,SpawnNumber,EventSequence,'Exposure' AS StimulusKind FROM exposure_change WHERE bExposed=1
 UNION ALL SELECT MatchId,VictimParticipantId,VictimSpawnNumber,EventSequence,'DamageTaken' FROM damage
 WHERE AttackerParticipantId IS NOT NULL AND bSelfDamage=0
)
SELECT s.MatchId,s.ParticipantId,s.SpawnNumber,s.EventSequence AS StimulusEventSequence,s.StimulusKind,es.ElapsedSeconds AS StimulusSeconds,
 (SELECT MIN(ea.ElapsedSeconds)-es.ElapsedSeconds FROM attack_input a JOIN event ea ON ea.MatchId=a.MatchId AND ea.EventSequence=a.EventSequence
   WHERE a.MatchId=s.MatchId AND a.ParticipantId=s.ParticipantId AND a.SpawnNumber=s.SpawnNumber AND a.InputState='Start'
    AND a.EventSequence>s.EventSequence) AS SecondsToAttackInput,
 (SELECT MIN(ef.ElapsedSeconds)-es.ElapsedSeconds FROM fire f JOIN event ef ON ef.MatchId=f.MatchId AND ef.EventSequence=f.EventSequence
   WHERE f.MatchId=s.MatchId AND f.ShooterParticipantId=s.ParticipantId AND f.ShooterSpawnNumber=s.SpawnNumber
    AND f.EventSequence>s.EventSequence) AS SecondsToFire,
 EXISTS(SELECT 1 FROM attack_input a JOIN event ea ON ea.MatchId=a.MatchId AND ea.EventSequence=a.EventSequence
   WHERE a.MatchId=s.MatchId AND a.ParticipantId=s.ParticipantId AND a.SpawnNumber=s.SpawnNumber AND a.EventSequence<s.EventSequence
    AND a.InputState='Start' AND NOT EXISTS(SELECT 1 FROM attack_input b WHERE b.MatchId=a.MatchId AND b.ParticipantId=a.ParticipantId
     AND b.SpawnNumber=a.SpawnNumber AND b.InputState='End' AND b.EventSequence>a.EventSequence AND b.EventSequence<s.EventSequence)) AS bAttackAlreadyHeld
FROM stimuli s JOIN event es ON es.MatchId=s.MatchId AND es.EventSequence=s.EventSequence
-- @@
CREATE VIEW weapon_distance_stats AS
WITH shots AS (
 SELECT f.MatchId,f.AttackId,f.ShooterParticipantId AS ParticipantId,f.WeaponId,c.TargetParticipantId,c.TargetDistance,
  CASE WHEN c.TargetDistance IS NULL THEN NULL
       ELSE CAST(c.TargetDistance/(SELECT Value FROM analysis_param WHERE Name='DistanceBandCm') AS INTEGER) END AS DistanceBand,
  EXISTS(SELECT 1 FROM damage d WHERE d.MatchId=f.MatchId AND d.AttackId=f.AttackId AND d.DamageSource='Weapon'
   AND d.bSelfDamage=0 AND d.VictimParticipantId<>f.ShooterParticipantId) AS bHit,
  COALESCE((SELECT SUM(MAX(0,MAX(d.HealthBefore,0)-MAX(d.HealthAfter,0))+MAX(0,MAX(d.ArmorBefore,0)-MAX(d.ArmorAfter,0)))
   FROM damage d WHERE d.MatchId=f.MatchId AND d.AttackId=f.AttackId AND d.DamageSource='Weapon' AND d.bSelfDamage=0),0) AS AppliedDamage
 FROM fire f JOIN fire_context c ON c.MatchId=f.MatchId AND c.EventSequence=f.EventSequence
)
SELECT x."GameBuildVersion",x."SettingsVersion",x."MapId",x."TimeLimitSeconds",x."KillTarget",x."MatchEndReason",x."RosterSignature",x."CharacterId",x."bIsBot",x."DifficultyId",x."CharacteristicId",x."BotSettingsHash",
 s.WeaponId,s.DistanceBand,
 s.DistanceBand*(SELECT Value FROM analysis_param WHERE Name='DistanceBandCm') AS DistanceFromCm,
 COUNT(*) AS FireCount,SUM(s.bHit) AS HitAttackCount,1.0*SUM(s.bHit)/COUNT(*) AS HitRate,SUM(s.AppliedDamage) AS AppliedDamage,
 SUM(s.TargetParticipantId IS NOT NULL) AS TargetedFireCount
FROM shots s JOIN v_participant_context x ON x.MatchId=s.MatchId AND x.ParticipantId=s.ParticipantId
GROUP BY x."GameBuildVersion",x."SettingsVersion",x."MapId",x."TimeLimitSeconds",x."KillTarget",x."MatchEndReason",x."RosterSignature",x."CharacterId",x."bIsBot",x."DifficultyId",x."CharacteristicId",x."BotSettingsHash",s.WeaponId,s.DistanceBand
-- @@
CREATE VIEW weapon_ammo_stats AS
WITH shots AS (
 SELECT f.MatchId,f.AttackId,f.ShooterParticipantId AS ParticipantId,f.WeaponId,a.bUnlimitedAmmo,a.AmmoBefore,
  CAST(json_extract(b."Values",'$.maxUsageCount') AS INTEGER) AS MaxAmmo,
  EXISTS(SELECT 1 FROM damage d WHERE d.MatchId=f.MatchId AND d.AttackId=f.AttackId AND d.DamageSource='Weapon'
   AND d.bSelfDamage=0 AND d.VictimParticipantId<>f.ShooterParticipantId) AS bHit
 FROM fire f
 JOIN event ef ON ef.MatchId=f.MatchId AND ef.CauseEventSequence=f.EventSequence AND ef.EventType='AmmoChanged'
 JOIN ammo_change a ON a.MatchId=ef.MatchId AND a.EventSequence=ef.EventSequence AND a.ChangeReason='Fire'
 JOIN match m ON m.MatchId=f.MatchId
 LEFT JOIN balance_settings b ON b.SettingsVersion=m.SettingsVersion AND b.SourceTable='DataAsset:PDA_WeaponBase_C' AND b.RowName=f.WeaponId
)
SELECT x."GameBuildVersion",x."SettingsVersion",x."MapId",x."TimeLimitSeconds",x."KillTarget",x."MatchEndReason",x."RosterSignature",x."CharacterId",x."bIsBot",x."DifficultyId",x."CharacteristicId",x."BotSettingsHash",
 s.WeaponId,s.bUnlimitedAmmo,
 CASE WHEN s.bUnlimitedAmmo=1 OR s.MaxAmmo IS NULL OR s.MaxAmmo<=0 THEN NULL
      WHEN s.AmmoBefore*4<=s.MaxAmmo THEN 'Q1_Low' WHEN s.AmmoBefore*2<=s.MaxAmmo THEN 'Q2'
      WHEN s.AmmoBefore*4<=s.MaxAmmo*3 THEN 'Q3' ELSE 'Q4_High' END AS AmmoBand,
 COUNT(*) AS FireCount,SUM(s.bHit) AS HitAttackCount,1.0*SUM(s.bHit)/COUNT(*) AS HitRate
FROM shots s JOIN v_participant_context x ON x.MatchId=s.MatchId AND x.ParticipantId=s.ParticipantId
GROUP BY x."GameBuildVersion",x."SettingsVersion",x."MapId",x."TimeLimitSeconds",x."KillTarget",x."MatchEndReason",x."RosterSignature",x."CharacterId",x."bIsBot",x."DifficultyId",x."CharacteristicId",x."BotSettingsHash",s.WeaponId,s.bUnlimitedAmmo,AmmoBand
-- @@
CREATE VIEW weapon_buff_stats AS
WITH hits AS (
 SELECT d.MatchId,d.EventSequence,f.ShooterParticipantId AS ParticipantId,f.WeaponId,d.InputDamage,
  MAX(0,MAX(d.HealthBefore,0)-MAX(d.HealthAfter,0))+MAX(0,MAX(d.ArmorBefore,0)-MAX(d.ArmorAfter,0)) AS AppliedDamage,
  d.bBuffsRecorded,bi.RowName AS AttackerBuffRow,m.SettingsVersion
 FROM damage d JOIN fire f ON f.MatchId=d.MatchId AND f.AttackId=d.AttackId
 JOIN match m ON m.MatchId=d.MatchId
 LEFT JOIN damage_buff db ON db.MatchId=d.MatchId AND db.DamageEventSequence=d.EventSequence AND db.Role='Attacker'
 LEFT JOIN buff_instance bi ON bi.MatchId=db.MatchId AND bi.BuffId=db.BuffId
 WHERE d.DamageSource='Weapon' AND d.bSelfDamage=0 AND d.bBuffsRecorded=1
)
SELECT x."GameBuildVersion",x."SettingsVersion",x."MapId",x."TimeLimitSeconds",x."KillTarget",x."MatchEndReason",x."RosterSignature",x."CharacterId",x."bIsBot",x."DifficultyId",x."CharacteristicId",x."BotSettingsHash",
 h.WeaponId,COALESCE(h.AttackerBuffRow,'None') AS AttackerBuffRow,
 json_extract(b."Values",'$.modifierType') AS ModifierType,json_extract(b."Values",'$.statValue') AS StatValue,
 COUNT(*) AS DamageEvents,SUM(h.InputDamage) AS InputDamage,SUM(h.AppliedDamage) AS AppliedDamage,1.0*SUM(h.AppliedDamage)/COUNT(*) AS AppliedDamagePerEvent
FROM hits h JOIN v_participant_context x ON x.MatchId=h.MatchId AND x.ParticipantId=h.ParticipantId
LEFT JOIN balance_settings b ON b.SettingsVersion=h.SettingsVersion AND b.SourceTable='DT_BuffTable' AND b.RowName=h.AttackerBuffRow
GROUP BY x."GameBuildVersion",x."SettingsVersion",x."MapId",x."TimeLimitSeconds",x."KillTarget",x."MatchEndReason",x."RosterSignature",x."CharacterId",x."bIsBot",x."DifficultyId",x."CharacteristicId",x."BotSettingsHash",h.WeaponId,AttackerBuffRow

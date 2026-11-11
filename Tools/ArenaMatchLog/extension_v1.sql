-- 전투 데이터 확장 (수집 형식 버전 1): 탄약, 발사 조건, 참가자 적용 설정, 버프 효과, 플레이어 행동 관측.
-- apply_extension.py가 템플릿 DB(Content/Data/ShootingArenaLog.db)에 적용합니다. 기존 테이블은 컬럼만 추가합니다.

-- The committed template predates match.StartedAtKst (the C++ schema had it patched in by hand), so it is added here.
ALTER TABLE match ADD COLUMN StartedAtKst TEXT;
ALTER TABLE match ADD COLUMN CollectionVersion INTEGER;
ALTER TABLE participant ADD COLUMN PlayerKey TEXT;
ALTER TABLE spawn_weapon ADD COLUMN InitialAmmo INTEGER;
ALTER TABLE spawn_weapon ADD COLUMN bUnlimitedAmmo INTEGER;
ALTER TABLE damage ADD COLUMN bBuffsRecorded INTEGER;
ALTER TABLE buff ADD COLUMN BuffId INTEGER;
ALTER TABLE buff ADD COLUMN PreviousBuffId INTEGER;

CREATE TABLE ammo_change (
    MatchId         TEXT    NOT NULL,
    EventSequence   INTEGER NOT NULL,
    ParticipantId   INTEGER NOT NULL,
    SpawnNumber     INTEGER NOT NULL,
    WeaponId        TEXT    NOT NULL,
    bUnlimitedAmmo  INTEGER NOT NULL,
    AmmoBefore      INTEGER,
    AmmoAfter       INTEGER,
    ChangeReason    TEXT    NOT NULL,
    PRIMARY KEY (MatchId, EventSequence),
    FOREIGN KEY (MatchId, EventSequence) REFERENCES event (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    CHECK (bUnlimitedAmmo IN (0,1)),
    CHECK (ChangeReason IN ('Fire','Pickup','Other')),
    CHECK (bUnlimitedAmmo=0 OR (AmmoBefore IS NULL AND AmmoAfter IS NULL))
);

CREATE TABLE fire_context (
    MatchId              TEXT    NOT NULL,
    EventSequence        INTEGER NOT NULL,
    TargetParticipantId  INTEGER,
    TargetSpawnNumber    INTEGER,
    TargetDistance       REAL,
    EquippedWeaponId     TEXT,
    PRIMARY KEY (MatchId, EventSequence),
    FOREIGN KEY (MatchId, EventSequence) REFERENCES fire (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, TargetParticipantId, TargetSpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    CHECK ((TargetParticipantId IS NULL) = (TargetSpawnNumber IS NULL)),
    CHECK (TargetParticipantId IS NOT NULL OR TargetDistance IS NULL)
);

CREATE TABLE participant_settings (
    MatchId        TEXT    NOT NULL,
    ParticipantId  INTEGER NOT NULL,
    SourceTable    TEXT    NOT NULL,
    RowName        TEXT    NOT NULL,
    "Values"       TEXT    NOT NULL CHECK (json_valid("Values") AND json_type("Values")='object'),
    PRIMARY KEY (MatchId, ParticipantId, SourceTable, RowName),
    FOREIGN KEY (MatchId, ParticipantId) REFERENCES participant (MatchId, ParticipantId) DEFERRABLE INITIALLY DEFERRED
);

CREATE TABLE buff_instance (
    MatchId              TEXT    NOT NULL,
    BuffId               INTEGER NOT NULL,
    ParticipantId        INTEGER NOT NULL,
    SpawnNumber          INTEGER NOT NULL,
    SettingsVersion      TEXT    NOT NULL,
    SourceTable          TEXT    NOT NULL,
    RowName              TEXT    NOT NULL,
    SourceEventSequence  INTEGER,
    PRIMARY KEY (MatchId, BuffId),
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (SettingsVersion, SourceTable, RowName)
        REFERENCES balance_settings (SettingsVersion, SourceTable, RowName) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, SourceEventSequence) REFERENCES event (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED
);

CREATE TABLE damage_buff (
    MatchId              TEXT    NOT NULL,
    DamageEventSequence  INTEGER NOT NULL,
    BuffId               INTEGER NOT NULL,
    BuffEventSequence    INTEGER NOT NULL,
    Role                 TEXT    NOT NULL,
    PRIMARY KEY (MatchId, DamageEventSequence, BuffId, Role),
    FOREIGN KEY (MatchId, DamageEventSequence) REFERENCES damage (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, BuffId) REFERENCES buff_instance (MatchId, BuffId) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, BuffEventSequence) REFERENCES buff (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    CHECK (Role IN ('Attacker','Victim'))
);

-- 플레이어 행동 관측 (사람 참가자만). 확인하지 못한 값은 NULL이며 0으로 채우지 않습니다.
CREATE TABLE observation_config (
    MatchId               TEXT    NOT NULL,
    ObservationVersion    INTEGER NOT NULL,
    SampleIntervalSeconds REAL,
    ExposureRule          TEXT    NOT NULL,
    PRIMARY KEY (MatchId),
    FOREIGN KEY (MatchId) REFERENCES match (MatchId) DEFERRABLE INITIALLY DEFERRED
);

CREATE TABLE observation_sample (
    MatchId                TEXT    NOT NULL,
    ParticipantId          INTEGER NOT NULL,
    SpawnNumber            INTEGER NOT NULL,
    ElapsedSeconds         REAL    NOT NULL,
    SampleIntervalSeconds  REAL    NOT NULL,
    ViewYaw                REAL,
    ViewPitch              REAL,
    AimX                   REAL,
    AimY                   REAL,
    AimZ                   REAL,
    SpeedXY                REAL,
    VelocityZ              REAL,
    bGrounded              INTEGER,
    bCrouched              INTEGER,
    EquippedWeaponId       TEXT,
    ObservedParticipantId  INTEGER,
    ObservedSpawnNumber    INTEGER,
    ObservedDistance       REAL,
    ObservedHeightDelta    REAL,
    ObservedRelX           REAL,
    ObservedRelY           REAL,
    ObservedRelZ           REAL,
    ObservedVelX           REAL,
    ObservedVelY           REAL,
    ObservedVelZ           REAL,
    RelForwardSpeed        REAL,
    RelLateralSpeed        REAL,
    PRIMARY KEY (MatchId, ParticipantId, SpawnNumber, ElapsedSeconds),
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ObservedParticipantId, ObservedSpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED
);

CREATE TABLE exposure_change (
    MatchId                TEXT    NOT NULL,
    EventSequence          INTEGER NOT NULL,
    ParticipantId          INTEGER NOT NULL,
    SpawnNumber            INTEGER NOT NULL,
    ObservedParticipantId  INTEGER NOT NULL,
    ObservedSpawnNumber    INTEGER NOT NULL,
    bExposed               INTEGER NOT NULL,
    bOccluded              INTEGER,
    Distance               REAL,
    HeightDelta            REAL,
    PRIMARY KEY (MatchId, EventSequence),
    FOREIGN KEY (MatchId, EventSequence) REFERENCES event (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ObservedParticipantId, ObservedSpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    CHECK (bExposed IN (0,1))
);

CREATE TABLE attack_input (
    MatchId           TEXT    NOT NULL,
    EventSequence     INTEGER NOT NULL,
    ParticipantId     INTEGER NOT NULL,
    SpawnNumber       INTEGER NOT NULL,
    InputState        TEXT    NOT NULL,
    EquippedWeaponId  TEXT,
    PRIMARY KEY (MatchId, EventSequence),
    FOREIGN KEY (MatchId, EventSequence) REFERENCES event (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    CHECK (InputState IN ('Start','End'))
);

CREATE TABLE fire_availability (
    MatchId           TEXT    NOT NULL,
    EventSequence     INTEGER NOT NULL,
    ParticipantId     INTEGER NOT NULL,
    SpawnNumber       INTEGER NOT NULL,
    EquippedWeaponId  TEXT,
    bCanFire          INTEGER NOT NULL,
    BlockReason       TEXT,
    PRIMARY KEY (MatchId, EventSequence),
    FOREIGN KEY (MatchId, EventSequence) REFERENCES event (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    CHECK (bCanFire IN (0,1)),
    CHECK ((bCanFire=1) = (BlockReason IS NULL))
);

CREATE TABLE jump_observation (
    MatchId        TEXT    NOT NULL,
    EventSequence  INTEGER NOT NULL,
    ParticipantId  INTEGER NOT NULL,
    SpawnNumber    INTEGER NOT NULL,
    JumpKind       TEXT    NOT NULL,
    bGrounded      INTEGER,
    bCanJump       INTEGER,
    ForcedCause    TEXT,
    PRIMARY KEY (MatchId, EventSequence),
    FOREIGN KEY (MatchId, EventSequence) REFERENCES event (MatchId, EventSequence) DEFERRABLE INITIALLY DEFERRED,
    FOREIGN KEY (MatchId, ParticipantId, SpawnNumber)
        REFERENCES spawn (MatchId, ParticipantId, SpawnNumber) DEFERRABLE INITIALLY DEFERRED,
    CHECK (JumpKind IN ('InputPressed','Executed','Forced')),
    CHECK ((JumpKind='Forced') = (ForcedCause IS NOT NULL))
);

-- 분석 조건 값 (수정 가능한 분석 매개변수; 수집 값이 아닙니다)
CREATE TABLE analysis_param (
    Name   TEXT NOT NULL PRIMARY KEY,
    Value  REAL NOT NULL
);
INSERT INTO analysis_param (Name, Value) VALUES ('DistanceBandCm', 500);

CREATE TRIGGER participant_settings_no_update BEFORE UPDATE ON participant_settings
WHEN NEW.MatchId IS NOT OLD.MatchId OR NEW.ParticipantId IS NOT OLD.ParticipantId OR NEW.SourceTable IS NOT OLD.SourceTable
  OR NEW.RowName IS NOT OLD.RowName OR NEW."Values" IS NOT OLD."Values"
BEGIN SELECT RAISE(ABORT,'participant_settings snapshots are immutable'); END;

CREATE TRIGGER participant_settings_no_replace BEFORE INSERT ON participant_settings
WHEN EXISTS (SELECT 1 FROM participant_settings b WHERE b.MatchId=NEW.MatchId AND b.ParticipantId=NEW.ParticipantId
 AND b.SourceTable=NEW.SourceTable AND b.RowName=NEW.RowName AND b."Values" IS NOT NEW."Values")
BEGIN SELECT RAISE(ABORT,'participant settings already have different applied values'); END;

CREATE INDEX ix_ammo_change_life ON ammo_change (MatchId, ParticipantId, SpawnNumber, WeaponId);
CREATE INDEX ix_ammo_change_cause ON ammo_change (MatchId, ChangeReason);
CREATE INDEX ix_damage_buff_buff ON damage_buff (MatchId, BuffId);
CREATE INDEX ix_buff_id ON buff (MatchId, BuffId);
CREATE INDEX ix_exposure_life ON exposure_change (MatchId, ParticipantId, SpawnNumber);
CREATE INDEX ix_attack_input_life ON attack_input (MatchId, ParticipantId, SpawnNumber);
CREATE INDEX ix_fire_availability_life ON fire_availability (MatchId, ParticipantId, SpawnNumber);
CREATE INDEX ix_jump_life ON jump_observation (MatchId, ParticipantId, SpawnNumber);
CREATE INDEX ix_observation_sample_time ON observation_sample (MatchId, ParticipantId, ElapsedSeconds);

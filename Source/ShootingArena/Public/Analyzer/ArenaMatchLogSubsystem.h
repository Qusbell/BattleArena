#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "Containers/Ticker.h"

#include "ArenaMatchLogSubsystem.generated.h"

class AActor;
class AController;
class APawn;
class APlayerState;
class FSQLiteDatabase;
class AMatchRulesInfo;

/** Health/armor/buff state read directly from the victim's life component. */
struct FArenaLifeSnapshot
{
    bool bValid = false;
    double Health = 0.0;
    double Armor = 0.0;
    FString BuffTag;
};

/** A buff effect a damage calculation used: the instance and the buff event that last set its state. */
struct FArenaBuffLink
{
    int32 BuffId = INDEX_NONE;
    int32 EventSequence = INDEX_NONE;
};

/** Everything the damage table needs; filled by the combat path around one ApplyDamage call. */
struct FArenaDamageRecord
{
    /** Attacker's current pawn (may already belong to a later life than the shot); null for environment damage. */
    TWeakObjectPtr<APawn> AttackerPawn;
    /** Filled by CaptureBuffLinks before ApplyDamage: the effects the calculation can use. */
    TArray<FArenaBuffLink> AttackerBuffs;
    TArray<FArenaBuffLink> VictimBuffs;
    bool bBuffsRecorded = false;
    int32 AttackId = INDEX_NONE;            // INDEX_NONE -> environment damage
    FString WeaponId;
    int32 AttackerParticipantId = INDEX_NONE;
    int32 AttackerSpawnNumber = INDEX_NONE;
    int32 VictimParticipantId = INDEX_NONE;
    int32 VictimSpawnNumber = INDEX_NONE;
    FString AttackerBuffTag;
    FString VictimBuffTag;
    double AppliedDamage = 0.0;
    TWeakObjectPtr<AActor> VictimActor;
    double RawDamage = 0.0;                 // damage handed to ApplyDamage, before health/armor distribution
    FArenaLifeSnapshot Before;
    FArenaLifeSnapshot After;
    double Distance = -1.0;                 // cm, < 0 -> unknown
};

/**
 * Writes one SQLite database row set per match (see Content/Data/ShootingArenaLog.db for the schema).
 * It only lives in authoritative Game/PIE match worlds. Every call returns quietly when no match
 * is being recorded; a failed write is logged and counted, never turned into a zero.
 */
UCLASS()
class SHOOTINGARENA_API UArenaMatchLogSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    static UArenaMatchLogSubsystem* Get(const UObject* WorldContext);

    /** Database file used by this process (-ArenaLogDb=<path> overrides Saved/Analyze/ShootingArenaLog.db). */
    static FString ResolveDatabasePath();

    bool IsRecording() const { return Db != nullptr && bMatchStarted && !bMatchEnded; }
    const FString& GetMatchId() const { return MatchId; }
    int32 GetFailedWriteCount() const { return FailedWrites; }
    /** Problems found by the post-save validation of the finished match, INDEX_NONE until it ran. */
    int32 GetValidationIssueCount() const { return LastValidationIssues; }

    // ---- lifecycle -------------------------------------------------------------------------
    /** Registers the controller as a participant (row is written when the first life completes). */
    void NoteController(int32 ParticipantId, AController* Controller);
    /** A pawn was possessed: start a life that becomes a spawn once its loadout is granted. */
    void NoteLifeBegin(APawn* Pawn, int32 ParticipantId, int32 SpawnNumber);
    /** Loadout finished: write spawn + spawn_weapon. EquippedWeapon empty -> first default weapon. */
    void CompleteSpawn(APawn* Pawn, const TArray<FString>& WeaponIds, const TArray<bool>& DefaultFlags, const FString& EquippedWeapon);
    void EndLife(APawn* Pawn, const TCHAR* Reason, int32 CauseSeq = INDEX_NONE);
    void EndMatch(const FString& Reason);

    // ---- combat ----------------------------------------------------------------------------
    int32 RecordFire(int32 AttackId, int32 ShooterParticipantId, int32 ShooterSpawnNumber, const FString& WeaponId, const UObject* WeaponData = nullptr, AActor* WeaponActor = nullptr);
    void SetFireElementCount(int32 AttackId, int32 Count);
    /** Resolves which buff effects the damage calculation of this attacker/victim pair uses; call right before ApplyDamage. */
    void CaptureBuffLinks(FArenaDamageRecord& Record, const APawn* Victim);
    /** Reserves the damage event before ApplyDamage so a death triggered inside can reference it. */
    int32 BeginDamage(int32 AttackId);
    void EndDamage(int32 DamageSeq, const FArenaDamageRecord& Record);
    int32 GetActiveDamageSeq() const { return ActiveDamageSeqs.IsEmpty() ? INDEX_NONE : ActiveDamageSeqs.Last(); }
    void RecordDeath(APawn* Victim, const FString& Cause, int32 KillerParticipantId, int32 KillerDamageSeq);
    /** Authoritative game-rule credit (may arrive before or after RecordDeath). */
    void RecordCredit(APawn* Victim, const FString& DeathCause, const FString& CreditSource, int32 CreditedParticipantId);
    FArenaLifeSnapshot SnapshotLife(AActor* Pawn) const;

    // ---- weapons / items / buffs (called from Blueprint through UArenaMatchLogLibrary) ----
    void RecordWeaponEquip(APawn* Pawn, const FString& Before, const FString& After, const FString& Reason);
    int32 RecordItemSpawn(AActor* Item, const FString& Category, const FString& RowName, const FString& Reason);
    /** Health/armor are read from the pawn at call time, so call before the effect is applied. */
    void RecordItemPickup(APawn* Pawn, AActor* Item, double ItemAmount, double AppliedAmount, bool bHasAmounts);
    void RecordItemRemove(AActor* Item, const FString& Reason);
    int32 RecordBuff(APawn* Pawn, const FString& BuffAfter, const FString& ChangeType, double ExpiresAtSeconds, bool bHasExpiry, int32 CauseSeq = INDEX_NONE,
        int32 BuffId = INDEX_NONE, int32 PreviousBuffId = INDEX_NONE);

    // ---- player behaviour observation (human participants; Blueprint hooks for what only the input graph knows) ----
    /** Fire input started (true) / released (false) on the pawn's controller. */
    void RecordAttackInput(APawn* Pawn, bool bPressed);
    /** The player's own machine judged that Target is on screen (and whether a wall hides it). Distances are derived here. */
    void RecordExposureReport(APawn* Observer, APawn* Target, bool bExposed, bool bOccluded);
    double GetBehaviorSampleInterval() const { return bBehaviorEnabled ? BehaviorSampleInterval : 0.0; }
    /** Jump input was pressed; whether it executed is observed separately. */
    void RecordJumpInput(APawn* Pawn);
    /** The next ground-to-air change of this pawn is not a jump the player made (JumpPad, Teleport, Knockback, ...). */
    void NoteForcedMovement(APawn* Pawn, const FString& Cause);

    // ---- Blueprint-driven helpers: the caller only passes actors, values are derived here ----
    /** BPC_Life: the damage amount right before it is split between health and armor (after buffs). */
    void RecordDamageInput(AActor* Victim, double InputDamage);
    void RecordDropItemSpawned(AActor* Item);
    void BeginItemPickup(AActor* Item, AActor* Picker);
    void CommitItemPickup(AActor* Item, AActor* Picker);
    void RecordWeaponSwap(AActor* Owner, AActor* BeforeItem, AActor* AfterItem);
    void RecordBuffApplied(UActorComponent* BuffComponent);
    void RecordBuffExpired(UActorComponent* BuffComponent);

    /** Resolves participant/spawn for a pawn, false when the pawn is not a tracked life. */
    bool FindLife(const APawn* Pawn, int32& OutParticipantId, int32& OutSpawnNumber) const;
    double ElapsedSeconds() const;

private:
    struct FLifeState
    {
        int32 ParticipantId = INDEX_NONE;
        int32 SpawnNumber = INDEX_NONE;
        TWeakObjectPtr<APawn> Pawn;
        bool bSpawnWritten = false;
        bool bEnded = false;
        double BeganAt = 0.0;
        int32 SpawnSeq = INDEX_NONE;
        int32 DeathSeq = INDEX_NONE;
        int32 LastDamageSeq = INDEX_NONE;
        TMap<int32, int32> LastDamageFrom;
        int32 DeathDamageSeq = INDEX_NONE;
        FString PendingInitialEquip;
        FString PendingDeathCause;
        FString PendingCreditSource;
        int32 PendingCreditedParticipant = INDEX_NONE;
        bool bHasPendingCredit = false;
    };

    struct FParticipantState
    {
        TWeakObjectPtr<AController> Controller;
        bool bWritten = false;
        bool bResultWritten = false;
        int32 CachedKills = INDEX_NONE;
        int32 CachedDeaths = INDEX_NONE;
    };

    struct FPendingFire
    {
        int32 EventSeq = INDEX_NONE;
        int32 ShooterParticipantId = INDEX_NONE;
        int32 ShooterSpawnNumber = INDEX_NONE;
        FString WeaponId;
        int32 ElementCount = 1;
        bool bCountFromData = false;
        // fire_context: the target the game selected for the shooter when the shot was fired (none for players)
        int32 TargetParticipantId = INDEX_NONE;
        int32 TargetSpawnNumber = INDEX_NONE;
        double TargetDistance = -1.0;
        FString EquippedWeaponId;
    };

    /** One held weapon actor's ammo reading. */
    struct FAmmoReading
    {
        TWeakObjectPtr<AActor> Weapon;
        FString WeaponId;
        FString ItemTag;
        int32 Count = 0;
        bool bUnlimited = false;
    };

    struct FActiveBuff
    {
        int32 BuffId = INDEX_NONE;
        int32 EventSequence = INDEX_NONE;
        FString Row;
        FString Tag;
    };

    struct FBehaviorPawnState
    {
        bool bGrounded = true;
        bool bHasPrevious = false;
        bool bAttackHeld = false;
        int32 CanFire = INDEX_NONE;     // INDEX_NONE = nothing recorded yet
        FString BlockReason;
        double ForcedSuppressUntil = -1.0;
        double LastSampleAt = -1.0;
        TMap<int64, int32> ExposureState; // other life key -> bit0 exposed, bit1 occluded, bit7 recorded
    };

    struct FFireTraits
    {
        double Damage = 0.0;
        double ExplosionRadius = 0.0;
    };

    UFUNCTION()
    void HandleItemDestroyed(AActor* DestroyedActor);

    struct FItemInfo
    {
        FString Category;
        FString RowName;
        UObject* RowOwner = nullptr;
    };
    bool DescribeDropItem(AActor* Item, FString& OutCategory, FString& OutRowName, double& OutAmount, bool& bOutHasAmount) const;

    static int64 LifeKey(int32 ParticipantId, int32 SpawnNumber) { return (int64(ParticipantId) << 32) | uint32(SpawnNumber); }

    bool OpenDatabase();
    void CloseDatabase();
    void EnsureMatchStarted();
    void ArchiveBalanceSettings(FString& OutSettingsVersion);
    bool TryWriteParticipant(int32 ParticipantId, FParticipantState& State, int32 SpawnNumber);
    int32 AllocateEvent(const TCHAR* EventType, int32 CauseSeq);
    void Enqueue(const FString& Sql);
    void FlushPendingFires();
    void Flush();
    void Tick();
    bool ReadMatchEnded();
    void CollectAmmo(const AActor* Holder, TMap<TWeakObjectPtr<AActor>, int32>& Out) const;
    bool ReadAmmo(const AActor* Weapon, FAmmoReading& Out) const;
    void CollectWeaponAmmo(const AActor* Holder, TArray<FAmmoReading>& Out) const;
    /** Writes one ammo_change event caused by CauseSeq and remembers the reading as the weapon's last known ammo. */
    void WriteAmmoChange(APawn* Pawn, const FAmmoReading& Now, const int32* Before, const TCHAR* Reason, int32 CauseSeq);
    void ArchiveParticipantSettings(int32 ParticipantId, AController* Controller, const FString& CharacteristicId);
    void EnqueueSetting(int32 ParticipantId, const FString& SourceTable, const FString& RowName, const FString& Json);
    int32 FindTargetPawnLife(AController* Controller, int32& OutSpawnNumber, double& OutDistance) const;
    FString ReadEquippedWeapon(int32 ParticipantId, int32 SpawnNumber) const;
    void NoteEquipped(int32 ParticipantId, int32 SpawnNumber, const FString& WeaponId);
    void BeginBehaviorObservation();
    bool TickBehavior(float DeltaTime);
    void ObserveHumanLife(FLifeState& Life, APawn* Pawn, double Now);
    void CollectHeldWeapons(APawn* Pawn, TArray<FString>& OutIds, TArray<bool>& OutDefaults) const;
    void TryAnchorClock();
    void Spool(const TArray<FString>& Statements, const FString& Reason);
    void MaybeValidate();

    FSQLiteDatabase* Db = nullptr;
    FString DbPath;
    FString MatchId;
    /** Real-time ticker (not a world timer): the game pauses the world when a match ends, which would stop a world timer. */
    FTSTicker::FDelegateHandle MatchTicker;
    double MatchStartSeconds = 0.0;
    int32 NextEventSeq = 1;
    int32 FailedWrites = 0;
    int32 FlushAttempts = 0;
    int32 SimulatedCommitFailures = 0; // test only: -ArenaLogSimulateCommitFailures=<n>
    int32 LastValidationIssues = INDEX_NONE;
    bool bClockAnchored = false;
    bool bValidated = false;
    TWeakObjectPtr<AMatchRulesInfo> RulesInfo;
    bool bMatchStarted = false;
    bool bMatchEnded = false;
    bool bBegunSqlTransaction = false;

    TArray<FString> PendingSql;
    TMap<int32, FParticipantState> Participants;
    TMap<int64, FLifeState> Lives;
    TMap<TWeakObjectPtr<APawn>, int64> PawnLives;
    TMap<int32, FPendingFire> PendingFires;
    struct FPendingLoadout
    {
        TArray<FString> Ids;
        TArray<bool> Defaults;
        FString Equipped;
    };
    TMap<TWeakObjectPtr<APawn>, FPendingLoadout> PendingLoadouts;
    /** Weapon equipped while the pawn had no open life (loadout granted before possession); becomes the initial weapon. */
    TMap<TWeakObjectPtr<APawn>, FString> PendingEquipByPawn;
    TMap<int32, FPendingLoadout> LastLoadoutByParticipant;
    TMap<TWeakObjectPtr<AActor>, FArenaLifeSnapshot> PickupBefore;
    TMap<TWeakObjectPtr<AActor>, uint64> PickupFrame;
    TMap<TWeakObjectPtr<AActor>, FString> BuffByPawn;
    FString SettingsVersionId;
    TMap<TWeakObjectPtr<AActor>, FAmmoReading> LastAmmo;                 // last ammo this log has seen per weapon actor
    TMap<int64, FString> EquippedByLife;                                 // currently equipped weapon per life
    TMap<int32, FString> ItemTargetTagById;                              // ammo items: the weapon tag they refill
    TMap<TWeakObjectPtr<AActor>, FActiveBuff> ActiveBuffs;               // pawn -> buff effect currently on it
    int32 NextBuffId = 1;
    TMap<TWeakObjectPtr<AActor>, TPair<int32, uint64>> BuffPickupByPawn; // pawn -> (pickup event, frame) of the latest buff item
    TMap<TWeakObjectPtr<AActor>, TPair<int32, uint64>> UnlinkedBuffByPawn; // pawn -> (buff id, frame) applied before its pickup was recorded
    int32 TimeLimitSeconds = 0;
    bool bSeenModeStarted = false;
    bool bWarnedEndMissing = false;
    double BehaviorSampleInterval = 0.0;
    bool bBehaviorEnabled = false;
    FTSTicker::FDelegateHandle BehaviorTicker;
    TMap<int64, FBehaviorPawnState> BehaviorStates;
    TMap<TWeakObjectPtr<AActor>, FString> ItemCategories;
    TMap<int32, FString> ItemCategoryById;
    TMap<int32, double> ItemAmountById;
    TMap<int32, bool> ItemHasAmountById;
    TMap<TWeakObjectPtr<AActor>, double> DamageInputs;
    TMap<int32, FFireTraits> FireTraits;
    TMap<TWeakObjectPtr<AActor>, TMap<TWeakObjectPtr<AActor>, int32>> AmmoBefore;
    TMap<int32, int32> FireSeqs;
    TMap<int32, int32> PickupSeqs;
    int32 LastDeathSeq = INDEX_NONE;
    bool bDisabled = false;
    bool bFlushWanted = false;
    TArray<int32> ActiveDamageSeqs;
    TMap<TWeakObjectPtr<AActor>, int32> ItemIds;
    TSet<int32> RemovedItems;
    TSet<int32> OpenItems;
    int32 NextItemId = 1;
};

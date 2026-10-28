#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "Analyzer/ArenaCombatTypes.h"
#include "UObject/ObjectKey.h"

#include "ArenaAnalyzeSubsystem.generated.h"

class AController;
class APawn;
class UDamageType;

USTRUCT(BlueprintType)
struct FArenaMovementSample
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    double TimeSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 ControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 TrackId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FArenaDamageSample
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 ShotId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FString WeaponId;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    bool bHasInstigatorLocation = false;

    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    double TimeSeconds = 0.0;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    float Damage = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FVector DamagedLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FVector InstigatorLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 DamagedControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 DamagedTrackId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 InstigatorControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 InstigatorTrackId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FArenaLineSegment
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FVector StartLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FVector EndLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 ControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    int32 TrackId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FArenaAnalyzeSession
{
    GENERATED_BODY()

    // Missing in legacy files: version 1.
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") int32 SchemaVersion = 1;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") FString SessionId;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") int32 ChunkIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") TArray<FArenaParticipant> Participants;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") TArray<FArenaShotSample> ShotSamples;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") TArray<FArenaHitSample> HitSamples;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze") TArray<FArenaDeathSample> DeathSamples;

    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    FString MapName;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    float SampleInterval = 0.5f;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    double SessionDuration = 0.0;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    TArray<FArenaMovementSample> MovementSamples;
    UPROPERTY(BlueprintReadOnly, Category = "Arena Analyze")
    TArray<FArenaDamageSample> DamageSamples;
};

UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

private:
    struct FControllerAnalyzeState
    {
        TWeakObjectPtr<AController> Controller;
        TWeakObjectPtr<APawn> CurrentPawn;
        int32 ControllerId = INDEX_NONE;
        int32 CurrentTrackId = INDEX_NONE;
        int32 NextTrackId = 0;
    };

    static constexpr float MovementSampleInterval = 0.5f;

    TArray<FControllerAnalyzeState> ControllerStates;
    TArray<FArenaMovementSample> MovementSamples;
    TArray<FArenaDamageSample> DamageSamples;
    FTimerHandle LoopTimerHandle;
    double SessionStartTime = 0.0;
    int32 NextControllerId = 0;
    int32 NextShotId = 0;
    int32 ChunkIndex = 0;
    FString SessionId;
    TArray<FArenaParticipant> Participants;
    TArray<FArenaShotSample> ShotSamples;
    TArray<FArenaHitSample> HitSamples;
    TArray<FArenaDeathSample> DeathSamples;
    TMap<TWeakObjectPtr<AActor>, FArenaShotSample> WeaponShots;
    TMap<TWeakObjectPtr<AActor>, FArenaShotSample> ProjectileShots;
    TArray<FArenaShotSample> DamageContexts;
    TMap<TWeakObjectPtr<APawn>, TPair<int32, int32>> PawnHistory;
    TSet<FObjectKey> DeadPawns;
    TMap<FObjectKey, int32> DeathIndices;
    TMap<FObjectKey, TPair<int32, FString>> PendingCredits;
    FArenaShotSample ActiveDamage;
    bool bApplyingDamage = false;
    TWeakObjectPtr<AActor> ActiveVictim;

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
    void RegisterController(AController* Controller);

    int32 RecordShot(AActor* Weapon, UObject* WeaponData, FVector Location, FVector Direction);
    void BindProjectile(AActor* Projectile, AActor* Weapon);
    void UpdateShotAim(AActor* Weapon, FVector Start, FVector End);
    void ForgetProjectile(AActor* Projectile);
    void PushProjectile(AActor* Projectile);
    void PopProjectile();
    void RecordHit(AActor* Source, const FHitResult& Hit, bool bDamageContact = false);
    float ApplyShotDamage(AActor* Victim, float Damage, AController* Instigator, AActor* Causer, TSubclassOf<UDamageType> Type, AActor* FiringWeapon = nullptr);
    void RecordDeath(APawn* Victim, AController* Instigator, AActor* Causer);
    void RecordCredit(APawn* Victim, class APlayerState* Credited, const FString& Source);
    UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
    FArenaAnalyzeSession Snapshot() const;

private:
    void AnalyzeControllers();
    void SaveSamplesToJson();
    FControllerAnalyzeState* FindStateForController(const AController* Controller);
    FControllerAnalyzeState* FindStateForPawn(const APawn* Pawn);
    void SetTrackedPawn(FControllerAnalyzeState& State, APawn* NewPawn);
    FArenaShotSample ResolveShot(AActor* Source) const;
    double Now() const;

    UFUNCTION()
    void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UFUNCTION()
    void OnPawnTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser);
};

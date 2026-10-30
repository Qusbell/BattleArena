#include "Analyzer/ArenaAnalyzeSubsystem.h"
#include "Analyzer/ArenaMatchLogSubsystem.h"
#include "Analyzer/ArenaLoggingSettings.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Guid.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
UArenaMatchLogSubsystem* MatchLog(const UWorld* World)
{
    return World ? World->GetSubsystem<UArenaMatchLogSubsystem>() : nullptr;
}
}

bool UArenaAnalyzeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    if (!IsArenaLoggingEnabled() || !World || World->GetNetMode() == ENetMode::NM_Client)
    {
        return false;
    }
    return World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
}

void UArenaAnalyzeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    SessionId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    SessionStartTime = World->GetTimeSeconds();
    World->GetTimerManager().SetTimer(
        LoopTimerHandle, this, &UArenaAnalyzeSubsystem::AnalyzeControllers,
        MovementSampleInterval, true);
}

void UArenaAnalyzeSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(LoopTimerHandle);
    }

    for (FControllerAnalyzeState& State : ControllerStates)
    {
        if (AController* Controller = State.Controller.Get())
        {
            Controller->OnPossessedPawnChanged.RemoveDynamic(
                this, &UArenaAnalyzeSubsystem::OnPossessedPawnChanged);
        }
        if (APawn* Pawn = State.CurrentPawn.Get())
        {
            Pawn->OnTakeAnyDamage.RemoveDynamic(
                this, &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage);
        }
    }

    SaveSamplesToJson();
    ControllerStates.Reset();
    Super::Deinitialize();
}

void UArenaAnalyzeSubsystem::RegisterController(AController* Controller)
{
    if (!IsValid(Controller) || Controller->GetWorld() != GetWorld() || !Controller->HasAuthority() || FindStateForController(Controller))
    {
        return;
    }

    FControllerAnalyzeState& State = ControllerStates.AddDefaulted_GetRef();
    State.Controller = Controller;
    State.ControllerId = NextControllerId++;
    FArenaParticipant& Participant = Participants.AddDefaulted_GetRef();
    Participant.ControllerId = State.ControllerId;
    Participant.bPlayer = Controller->IsPlayerController();
    Participant.Name = Controller->GetName();
    if (const APlayerState* PS = Controller->GetPlayerState<APlayerState>())
    {
        Participant.Name = PS->GetPlayerName();
        Participant.PlayerId = PS->GetUniqueId().ToString();
    }

    if (auto* Log = MatchLog(GetWorld()))
    {
        Log->NoteController(State.ControllerId, Controller);
    }

    Controller->OnPossessedPawnChanged.AddUniqueDynamic(
        this, &UArenaAnalyzeSubsystem::OnPossessedPawnChanged);

    if (APawn* Pawn = Controller->GetPawn())
    {
        SetTrackedPawn(State, Pawn);
    }
}

void UArenaAnalyzeSubsystem::AnalyzeControllers()
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Discovery covers Player/AI login and already-existing controllers. Registration is idempotent.
    for (TActorIterator<AController> It(GetWorld()); It; ++It) RegisterController(*It);
    for (auto It = WeaponShots.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    for (auto It = ProjectileShots.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    for (auto It = PawnHistory.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    const double NowTime = Now();
    for (FControllerAnalyzeState& State : ControllerStates)
    {
        AController* Controller = State.Controller.Get();
        if (!IsValid(Controller))
        {
            if (APawn* Pawn = State.CurrentPawn.Get())
            {
                Pawn->OnTakeAnyDamage.RemoveDynamic(
                    this, &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage);
            }
            State.CurrentPawn.Reset();
            State.CurrentTrackId = INDEX_NONE;
            continue;
        }

        APawn* Pawn = Controller->GetPawn();
        if (!IsValid(Pawn))
        {
            SetTrackedPawn(State, nullptr);
            continue;
        }

        // Handles registration/possession ordering without adding Tick work.
        if (State.CurrentPawn.Get() != Pawn)
        {
            SetTrackedPawn(State, Pawn);
        }

        if (DeadPawns.Contains(FObjectKey(Pawn))) continue;
        FArenaMovementSample& Sample = MovementSamples.AddDefaulted_GetRef();
        Sample.TimeSeconds = NowTime;
        Sample.Location = Pawn->GetActorLocation();
        Sample.ControllerId = State.ControllerId;
        Sample.TrackId = State.CurrentTrackId;
    }
    ControllerStates.RemoveAll([](const auto& State) { return !State.Controller.IsValid(); });
    for (auto It = DeadPawns.CreateIterator(); It; ++It) if (!IsValid(It->ResolveObjectPtr())) It.RemoveCurrent();
    for (auto It = PendingCredits.CreateIterator(); It; ++It) if (!IsValid(It.Key().ResolveObjectPtr())) It.RemoveCurrent();
    for (auto It = DeathIndices.CreateIterator(); It; ++It) if (!IsValid(It.Key().ResolveObjectPtr())) It.RemoveCurrent();
    // Bound raw event buffers. Each independently valid JSON chunk shares session/IDs.
    if (MovementSamples.Num() + DamageSamples.Num() + ShotSamples.Num() + HitSamples.Num() + DeathSamples.Num() >= 50000)
    {
        SaveSamplesToJson();
    }
}

void UArenaAnalyzeSubsystem::SaveSamplesToJson()
{
    FArenaAnalyzeSession Session = Snapshot();
    FString JsonString;
    if (!FJsonObjectConverter::UStructToJsonObjectString(Session, JsonString))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert ArenaAnalyze data to JSON"));
        return;
    }

    const FString Directory = FPaths::ProjectSavedDir() / TEXT("Analyze");
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString FilePath = Directory /
        FString::Printf(TEXT("ArenaAnalyze_%s_%06d.json"), *SessionId, ChunkIndex);

    if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("ArenaAnalyze saved: %s"), *FilePath);
        ++ChunkIndex;
        MovementSamples.Reset(); DamageSamples.Reset(); ShotSamples.Reset(); HitSamples.Reset(); DeathSamples.Reset();
        DeathIndices.Reset();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save ArenaAnalyze data: %s"), *FilePath);
    }
}

UArenaAnalyzeSubsystem::FControllerAnalyzeState*
UArenaAnalyzeSubsystem::FindStateForController(const AController* Controller)
{
    if (!Controller)
    {
        return nullptr;
    }
    return ControllerStates.FindByPredicate(
        [Controller](const FControllerAnalyzeState& State)
        {
            return State.Controller.Get() == Controller;
        });
}

UArenaAnalyzeSubsystem::FControllerAnalyzeState*
UArenaAnalyzeSubsystem::FindStateForPawn(const APawn* Pawn)
{
    if (!Pawn)
    {
        return nullptr;
    }
    return ControllerStates.FindByPredicate(
        [Pawn](const FControllerAnalyzeState& State)
        {
            return State.CurrentPawn.Get() == Pawn;
        });
}

void UArenaAnalyzeSubsystem::SetTrackedPawn(
    FControllerAnalyzeState& State, APawn* NewPawn)
{
    APawn* OldPawn = State.CurrentPawn.Get();
    if (OldPawn == NewPawn)
    {
        return;
    }

    if (IsValid(OldPawn))
    {
        OldPawn->OnTakeAnyDamage.RemoveDynamic(
            this, &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage);
    }

    State.CurrentPawn = NewPawn;
    State.CurrentTrackId = INDEX_NONE;
    if (IsValid(NewPawn))
    {
        State.CurrentTrackId = State.NextTrackId++;
        // main's character pool can possess the same Pawn object for another life.
        const FObjectKey PawnKey(NewPawn);
        DeadPawns.Remove(PawnKey);
        DeathIndices.Remove(PawnKey);
        PendingCredits.Remove(PawnKey);
        PawnHistory.Add(NewPawn, {State.ControllerId, State.CurrentTrackId});
        NewPawn->OnTakeAnyDamage.AddUniqueDynamic(
            this, &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage);
        if (auto* Log = MatchLog(GetWorld()))
        {
            Log->NoteLifeBegin(NewPawn, State.ControllerId, State.CurrentTrackId + 1);
        }
    }
}

void UArenaAnalyzeSubsystem::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    FControllerAnalyzeState* State = nullptr;
    if (NewPawn)
    {
        State = FindStateForController(NewPawn->GetController());
    }
    if (!State)
    {
        State = FindStateForPawn(OldPawn);
    }
    if (State)
    {
        SetTrackedPawn(*State, NewPawn);
    }
}

void UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage(
    AActor* DamagedActor, float Damage, const UDamageType* DamageType,
    AController* InstigatedBy, AActor* DamageCauser)
{
    if ((bApplyingDamage && ActiveVictim.Get() == DamagedActor) || !IsValid(DamagedActor))
    {
        return;
    }

    const UWorld* World = GetWorld();
    FArenaDamageSample& Sample = DamageSamples.AddDefaulted_GetRef();
    Sample.TimeSeconds = World ? World->GetTimeSeconds() - SessionStartTime : 0.0;
    Sample.Damage = Damage;
    Sample.DamagedLocation = DamagedActor->GetActorLocation();

    const APawn* DamagedPawn = Cast<APawn>(DamagedActor);
    if (const FControllerAnalyzeState* State = FindStateForPawn(DamagedPawn))
    {
        Sample.DamagedControllerId = State->ControllerId;
        Sample.DamagedTrackId = State->CurrentTrackId;
    }
    else if (const auto* Identity = PawnHistory.Find(DamagedPawn))
    {
        Sample.DamagedControllerId = Identity->Key; Sample.DamagedTrackId = Identity->Value;
    }

    if (IsValid(InstigatedBy))
    {
        if (const FControllerAnalyzeState* State = FindStateForController(InstigatedBy))
        {
            Sample.InstigatorControllerId = State->ControllerId;
            Sample.InstigatorTrackId = State->CurrentTrackId;
        }
        if (const APawn* InstigatorPawn = InstigatedBy->GetPawn(); IsValid(InstigatorPawn))
        {
            Sample.InstigatorLocation = InstigatorPawn->GetActorLocation();
            Sample.bHasInstigatorLocation = true;
        }
    }
}


double UArenaAnalyzeSubsystem::Now() const
{
    return GetWorld() ? FMath::Max(0.0, double(GetWorld()->GetTimeSeconds()) - SessionStartTime) : 0.0;
}

FArenaAnalyzeSession UArenaAnalyzeSubsystem::Snapshot() const
{
    FArenaAnalyzeSession S;
    S.SchemaVersion = 2; S.SessionId = SessionId; S.ChunkIndex = ChunkIndex;
    S.MapName = GetWorld()->GetMapName(); S.MapName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
    S.SampleInterval = MovementSampleInterval; S.SessionDuration = Now();
    S.Participants = Participants; S.MovementSamples = MovementSamples; S.DamageSamples = DamageSamples;
    S.ShotSamples = ShotSamples; S.HitSamples = HitSamples; S.DeathSamples = DeathSamples;
    return S;
}

int32 UArenaAnalyzeSubsystem::RecordShot(AActor* Weapon, UObject* Data, FVector Location, FVector Direction)
{
    if (!IsValid(Weapon) || !Weapon->HasAuthority() || GetWorld()->GetNetMode() == NM_Client
        || Weapon->GetWorld() != GetWorld() || Location.ContainsNaN() || Direction.ContainsNaN()) return INDEX_NONE;
    APawn* Pawn = Cast<APawn>(Weapon->GetOwner());
    if (!Pawn) Pawn = Weapon->GetInstigator();
    AController* Controller = Pawn ? Pawn->GetController() : nullptr;
    RegisterController(Controller);
    FArenaShotSample Shot;
    Shot.ShotId = NextShotId++; Shot.TimeSeconds = Now();
    Shot.WeaponId = IsValid(Data) ? Data->GetPathName() : Weapon->GetClass()->GetPathName();
    Shot.Location = Location; Shot.Direction = Direction.GetSafeNormal();
    if (auto* State = FindStateForController(Controller))
    {
        SetTrackedPawn(*State, Pawn);
        Shot.ControllerId = State->ControllerId; Shot.TrackId = State->CurrentTrackId;
    }
    ShotSamples.Add(Shot); WeaponShots.Add(Weapon, Shot);
    if (Shot.ControllerId != INDEX_NONE)
    {
        if (auto* Log = MatchLog(GetWorld())) Log->RecordFire(Shot.ShotId, Shot.ControllerId, Shot.TrackId + 1, Shot.WeaponId, Data);
    }
    return Shot.ShotId;
}

FArenaShotSample UArenaAnalyzeSubsystem::ResolveShot(AActor* Source) const
{
    if (!DamageContexts.IsEmpty()) return DamageContexts.Last();
    if (const auto* Shot = ProjectileShots.Find(Source)) return *Shot;
    if (const auto* Shot = WeaponShots.Find(Source)) return *Shot;
    return FArenaShotSample();
}

void UArenaAnalyzeSubsystem::BindProjectile(AActor* Projectile, AActor* Weapon)
{
    if (IsValid(Projectile))
    {
        FArenaShotSample Shot = ResolveShot(Weapon);
        Shot.Location = Projectile->GetActorLocation(); Shot.Direction = Projectile->GetActorForwardVector();
        ProjectileShots.Add(Projectile, Shot);
        if (auto* Buffered = ShotSamples.FindByPredicate([&](const auto& S) { return S.ShotId == Shot.ShotId; }))
        { Buffered->Location = Shot.Location; Buffered->Direction = Shot.Direction; }
    }
}
void UArenaAnalyzeSubsystem::ForgetProjectile(AActor* Projectile) { ProjectileShots.Remove(Projectile); }
void UArenaAnalyzeSubsystem::PushProjectile(AActor* Projectile)
{
    const auto* Shot = ProjectileShots.Find(Projectile);
    DamageContexts.Add(Shot ? *Shot : FArenaShotSample());
}
void UArenaAnalyzeSubsystem::PopProjectile() { if (!DamageContexts.IsEmpty()) DamageContexts.Pop(EAllowShrinking::No); }

void UArenaAnalyzeSubsystem::RecordHit(AActor* Source, const FHitResult& Hit, bool bDamageContact)
{
    if (!Hit.bBlockingHit && !Hit.GetActor()) return;
    const FArenaShotSample Shot = ResolveShot(Source);
    if (Shot.ShotId == INDEX_NONE) return;
    FArenaHitSample& Sample = HitSamples.AddDefaulted_GetRef();
    Sample.ShotId = Shot.ShotId; Sample.TimeSeconds = Now(); Sample.Location = Hit.ImpactPoint;
    Sample.Normal = Hit.ImpactNormal; Sample.bDamageContact = bDamageContact;
    if (AActor* Target = Hit.GetActor())
    {
        Sample.TargetName = Target->GetName();
        if (auto* Pawn = Cast<APawn>(Target))
        {
            RegisterController(Pawn->GetController());
            if (auto* State = FindStateForController(Pawn->GetController()))
            {
                SetTrackedPawn(*State, Pawn);
                Sample.TargetControllerId = State->ControllerId; Sample.TargetTrackId = State->CurrentTrackId;
            }
        }
    }
}

float UArenaAnalyzeSubsystem::ApplyShotDamage(AActor* Victim, float Damage, AController* Instigator,
    AActor* Causer, TSubclassOf<UDamageType> Type, AActor* FiringWeapon)
{
    if (!IsValid(Victim) || !Victim->HasAuthority()) return 0;
    const FArenaShotSample Shot = ResolveShot(FiringWeapon ? FiringWeapon : Causer);
    RegisterController(Cast<APawn>(Victim) ? Cast<APawn>(Victim)->GetController() : nullptr);
    if (Shot.ShotId != INDEX_NONE)
    {
        Instigator = nullptr;
        for (const auto& State : ControllerStates)
            if (State.ControllerId == Shot.ControllerId) { Instigator = State.Controller.Get(); break; }
    }
    // Snapshot before TakeDamage: death can unpossess/destroy either pawn synchronously.
    FArenaDamageSample Sample;
    Sample.TimeSeconds = Now(); Sample.DamagedLocation = Victim->GetActorLocation();
    Sample.ShotId = Shot.ShotId; Sample.WeaponId = Shot.WeaponId;
    Sample.InstigatorControllerId = Shot.ControllerId; Sample.InstigatorTrackId = Shot.TrackId;
    if (auto* State = FindStateForPawn(Cast<APawn>(Victim)))
    { Sample.DamagedControllerId = State->ControllerId; Sample.DamagedTrackId = State->CurrentTrackId; }
    const auto* FiringState = FindStateForController(Instigator);
    if (IsValid(Instigator) && IsValid(Instigator->GetPawn())
        && (Shot.ShotId == INDEX_NONE || (FiringState && FiringState->ControllerId == Shot.ControllerId && FiringState->CurrentTrackId == Shot.TrackId)))
    { Sample.InstigatorLocation = Instigator->GetPawn()->GetActorLocation(); Sample.bHasInstigatorLocation = true; }
    if (Shot.ShotId != INDEX_NONE)
    {
        FHitResult Contact(Victim, nullptr, Victim->GetActorLocation(), FVector::ZeroVector);
        Contact.bBlockingHit = true; RecordHit(FiringWeapon ? FiringWeapon : Causer, Contact, true);
    }
    // Match log: reserve the damage event first so a death raised inside ApplyDamage can reference it.
    auto* Log = MatchLog(GetWorld());
    int32 DamageSeq = INDEX_NONE;
    FArenaDamageRecord Record;
    if (Log && Sample.DamagedControllerId != INDEX_NONE)
    {
        Record.VictimParticipantId = Sample.DamagedControllerId;
        Record.VictimSpawnNumber = Sample.DamagedTrackId + 1;
        if (Shot.ShotId != INDEX_NONE && Shot.ControllerId != INDEX_NONE)
        {
            Record.AttackId = Shot.ShotId;
            Record.WeaponId = Shot.WeaponId;
            Record.AttackerParticipantId = Shot.ControllerId;
            Record.AttackerSpawnNumber = Shot.TrackId + 1;
            if (Sample.bHasInstigatorLocation) Record.Distance = FVector::Dist(Sample.InstigatorLocation, Sample.DamagedLocation);
            if (IsValid(Instigator)) Record.AttackerBuffTag = Log->SnapshotLife(Instigator->GetPawn()).BuffTag;
        }
        Record.RawDamage = Damage;
        Record.VictimActor = Victim;
        Record.Before = Log->SnapshotLife(Victim);
        Record.VictimBuffTag = Record.Before.BuffTag;
        DamageSeq = Log->BeginDamage(Record.AttackId);
    }
    const auto PreviousVictim = ActiveVictim;
    ActiveVictim = Victim;
    const bool PreviousApplying = bApplyingDamage;
    const auto PreviousDamage = ActiveDamage;
    bApplyingDamage = true; ActiveDamage = Shot;
    const float Applied = UGameplayStatics::ApplyDamage(Victim, Damage, Instigator, Causer, Type);
    ActiveDamage = PreviousDamage; bApplyingDamage = PreviousApplying; ActiveVictim = PreviousVictim;
    if (Log && Sample.DamagedControllerId != INDEX_NONE)
    {
        Record.AppliedDamage = Applied;
        Record.After = Log->SnapshotLife(Victim);
        Log->EndDamage(DamageSeq, Record);
    }
    if (Applied != 0)
    { Sample.Damage = Applied; DamageSamples.Add(MoveTemp(Sample)); }
    return Applied;
}

void UArenaAnalyzeSubsystem::RecordDeath(APawn* Victim, AController* Instigator, AActor* Causer)
{
    if (!IsValid(Victim) || !Victim->HasAuthority() || DeadPawns.Contains(FObjectKey(Victim))) return;
    RegisterController(Victim->GetController());
    DeadPawns.Add(FObjectKey(Victim));
    FArenaDeathSample Sample;
    Sample.TimeSeconds = Now(); Sample.VictimLocation = Victim->GetActorLocation();
    if (auto* State = FindStateForPawn(Victim))
    { Sample.VictimControllerId = State->ControllerId; Sample.VictimTrackId = State->CurrentTrackId; }
    const FArenaShotSample Shot = bApplyingDamage && ActiveVictim.Get() == Victim ? ActiveDamage : FArenaShotSample();
    // Only an explicit active damage context may identify the original firing life.
    Sample.ShotId = Shot.ShotId; Sample.WeaponId = Shot.WeaponId;
    Sample.KillerControllerId = Shot.ControllerId; Sample.KillerTrackId = Shot.TrackId;
    if (Shot.ShotId != INDEX_NONE)
        Sample.Cause = Shot.ControllerId == INDEX_NONE ? TEXT("Unknown")
            : Shot.ControllerId == Sample.VictimControllerId ? TEXT("Suicide") : TEXT("Player");
    else if ((IsValid(Instigator) && Instigator == Victim->GetController()) || Causer == Victim)
    {
        Sample.Cause = TEXT("Suicide");
        Sample.KillerControllerId = Sample.VictimControllerId; Sample.KillerTrackId = Sample.VictimTrackId;
    }
    else if (IsValid(Instigator))
    {
        RegisterController(Instigator);
        if (auto* State = FindStateForController(Instigator))
        { Sample.KillerControllerId = State->ControllerId; Sample.KillerTrackId = State->CurrentTrackId; }
        Sample.Cause = TEXT("Player");
    }
    else Sample.Cause = IsValid(Causer) ? TEXT("Environment") : TEXT("Unknown");
    if (Sample.Cause == TEXT("Suicide"))
    { Sample.KillerLocation = Sample.VictimLocation; Sample.bHasKillerLocation = true; }
    // Never use a respawned pawn as the old shooter's position.
    for (const auto& State : ControllerStates)
        if (State.ControllerId == Sample.KillerControllerId && State.CurrentTrackId == Sample.KillerTrackId
            && State.CurrentPawn.IsValid() && !DeadPawns.Contains(FObjectKey(State.CurrentPawn.Get())))
        { Sample.KillerLocation = State.CurrentPawn->GetActorLocation(); Sample.bHasKillerLocation = true; break; }
    if (const auto* Credit = PendingCredits.Find(FObjectKey(Victim)))
    { Sample.CreditedControllerId = Credit->Key; Sample.CreditSource = Credit->Value; }
    DeathIndices.Add(FObjectKey(Victim), DeathSamples.Add(Sample));
    if (auto* Log = MatchLog(GetWorld()))
    {
        const bool bDirect = Sample.Cause == TEXT("Player") && Sample.KillerControllerId != INDEX_NONE;
        Log->RecordDeath(Victim, Sample.Cause == TEXT("Unknown") ? FString(TEXT("Environment")) : Sample.Cause,
            bDirect ? Sample.KillerControllerId : INDEX_NONE,
            (bDirect && bApplyingDamage && ActiveVictim.Get() == Victim) ? Log->GetActiveDamageSeq() : INDEX_NONE);
    }
}

void UArenaAnalyzeSubsystem::RecordCredit(APawn* Victim, APlayerState* Credited, const FString& Source, const FString& DeathCause)
{
    if (!IsValid(Victim) || !Victim->HasAuthority()) return;
    int32 Id = INDEX_NONE;
    if (IsValid(Credited))
    {
        for (TActorIterator<AController> It(GetWorld()); It; ++It)
            if (It->GetPlayerState<APlayerState>() == Credited)
            { RegisterController(*It); if (auto* State = FindStateForController(*It)) Id = State->ControllerId; break; }
    }
    PendingCredits.Add(FObjectKey(Victim), {Id, Source});
    if (const int32* Index = DeathIndices.Find(FObjectKey(Victim)))
    { DeathSamples[*Index].CreditedControllerId = Id; DeathSamples[*Index].CreditSource = Source; }
    if (auto* Log = MatchLog(GetWorld()); Log && !DeathCause.IsEmpty())
        Log->RecordCredit(Victim, DeathCause, Source, Id);
}

void UArenaAnalyzeSubsystem::NoteShotElements(AActor* Weapon, int32 Count)
{
    if (const auto* Shot = WeaponShots.Find(Weapon))
        if (auto* Log = MatchLog(GetWorld())) Log->SetFireElementCount(Shot->ShotId, Count);
}

void UArenaAnalyzeSubsystem::UpdateShotAim(AActor* Weapon, FVector Start, FVector End)
{
    if (Start.ContainsNaN() || End.ContainsNaN() || Start.Equals(End)) return;
    if (auto* Shot = WeaponShots.Find(Weapon))
    {
        Shot->Location = Start; Shot->Direction = (End - Start).GetSafeNormal();
        if (auto* Buffered = ShotSamples.FindByPredicate([&](const auto& S) { return S.ShotId == Shot->ShotId; }))
        { Buffered->Location = Shot->Location; Buffered->Direction = Shot->Direction; }
    }
}

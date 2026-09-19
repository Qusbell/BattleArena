#include "Analyzer/ArenaAnalyzeSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool UArenaAnalyzeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    if (!World || World->GetNetMode() == ENetMode::NM_Client)
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
    if (!IsValid(Controller) || FindStateForController(Controller))
    {
        return;
    }

    FControllerAnalyzeState& State = ControllerStates.AddDefaulted_GetRef();
    State.Controller = Controller;
    State.ControllerId = NextControllerId++;

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

    const double NowTime = World->GetTimeSeconds() - SessionStartTime;
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
            continue;
        }

        // Handles registration/possession ordering without adding Tick work.
        if (State.CurrentPawn.Get() != Pawn)
        {
            SetTrackedPawn(State, Pawn);
        }

        FArenaMovementSample& Sample = MovementSamples.AddDefaulted_GetRef();
        Sample.TimeSeconds = NowTime;
        Sample.Location = Pawn->GetActorLocation();
        Sample.ControllerId = State.ControllerId;
        Sample.TrackId = State.CurrentTrackId;
    }
}

void UArenaAnalyzeSubsystem::SaveSamplesToJson()
{
    const UWorld* World = GetWorld();
    FArenaAnalyzeSession Session;
    Session.MapName = World ? World->GetMapName() : FString();
    if (World)
    {
        Session.MapName.RemoveFromStart(World->StreamingLevelsPrefix);
    }
    Session.SampleInterval = MovementSampleInterval;
    Session.SessionDuration = World
        ? FMath::Max(0.0, static_cast<double>(World->GetTimeSeconds()) - SessionStartTime)
        : 0.0;
    Session.MovementSamples = MovementSamples;
    Session.DamageSamples = DamageSamples;

    FString JsonString;
    if (!FJsonObjectConverter::UStructToJsonObjectString(Session, JsonString))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert ArenaAnalyze data to JSON"));
        return;
    }

    const FString Directory = FPaths::ProjectSavedDir() / TEXT("Analyze");
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString FilePath = Directory /
        (TEXT("ArenaAnalyze_") + FDateTime::Now().ToString() + TEXT(".json"));

    if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("ArenaAnalyze saved: %s"), *FilePath);
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
        NewPawn->OnTakeAnyDamage.AddUniqueDynamic(
            this, &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage);
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
    if (!IsValid(DamagedActor))
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
        }
    }
}

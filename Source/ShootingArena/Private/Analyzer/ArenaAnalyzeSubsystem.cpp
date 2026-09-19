
#include "Analyzer/ArenaAnalyzeSubsystem.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"


bool UArenaAnalyzeSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);

    // World 아니면 거르기
    if (!World)
    {
        return false;
    }

    // 클라이언트 거르기
    if (World->GetNetMode() == ENetMode::NM_Client)
    {
        return false;
    }

    return
        World->WorldType == EWorldType::Game ||
        World->WorldType == EWorldType::PIE;
}

void UArenaAnalyzeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UWorld* World = GetWorld();

    if (World == nullptr) { return; }

    World->GetTimerManager().SetTimer(
        LoopTimerHandle,
        this,
        &UArenaAnalyzeSubsystem::AnalyzeControllers,
        0.5f,   // 0.5초마다
        true    // 반복
    );
}

void UArenaAnalyzeSubsystem::Deinitialize()
{
    UWorld* World = GetWorld();

    if (World != nullptr)
    {
        World->GetTimerManager().ClearTimer(LoopTimerHandle);
    }

    // 저장
    SaveSamplesToJson();

    Super::Deinitialize();
}

void UArenaAnalyzeSubsystem::RegisterController(AController* controller)
{
    if (!IsValid(controller)) { return; }

	RegisteredControllers.AddUnique(controller);

    controller->OnPossessedPawnChanged.AddUniqueDynamic(
        this,
        &UArenaAnalyzeSubsystem::OnPossessedPawnChanged
    );

    // 등록 시점에 이미 Pawn을 가지고 있을 수도 있음
    if (APawn* Pawn = controller->GetPawn())
    {
        Pawn->OnTakeAnyDamage.AddUniqueDynamic(
            this,
            &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage
        );
    }
}


void UArenaAnalyzeSubsystem::AnalyzeControllers()
{
    double nowTime = GetWorld()->GetTimeSeconds();
    

    for (const TWeakObjectPtr<AController>& WeakController : RegisteredControllers)
    {
        const AController* controller = WeakController.Get();
        if (controller == nullptr) { continue; }

        const APawn* pawn = controller->GetPawn();
		if (pawn == nullptr) { continue; }

        // 정보 수집
        FArenaInfoSample& sample = AIMovementSamples.AddDefaulted_GetRef();
        sample.TimeSeconds = nowTime;
        sample.Location = pawn->GetActorLocation();
	}
}


void UArenaAnalyzeSubsystem::SaveSamplesToJson()
{
    FArenaAnalyzeSession Session;
    Session.AIMovementSamples = AIMovementSamples;
    Session.DamageSamples = DamageSamples;

    FString JsonString;

    if (!FJsonObjectConverter::UStructToJsonObjectString(Session, JsonString))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert ArenaAnalyze data to JSON"));
        return;
    }

    const FString Directory =
        FPaths::ProjectSavedDir() / TEXT("Analyze");

    IFileManager::Get().MakeDirectory(*Directory, true);

    const FString FileName =
        TEXT("ArenaAnalyze_")
        + FDateTime::Now().ToString()
        + TEXT(".json");

    const FString FilePath =
        Directory / FileName;

    if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("ArenaAnalyze saved: %s"),
            *FilePath
        );
    }
}


void UArenaAnalyzeSubsystem::OnPossessedPawnChanged(
    APawn* OldPawn,
    APawn* NewPawn)
{
    if (IsValid(OldPawn))
    {
        OldPawn->OnTakeAnyDamage.RemoveDynamic(
            this,
            &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage
        );
    }

    if (IsValid(NewPawn))
    {
        NewPawn->OnTakeAnyDamage.AddUniqueDynamic(
            this,
            &UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage
        );
    }
}


void UArenaAnalyzeSubsystem::OnPawnTakeAnyDamage(
    AActor* DamagedActor,
    float Damage,
    const UDamageType* DamageType,
    AController* InstigatedBy,
    AActor* DamageCauser)
{
    if (!IsValid(DamagedActor))
    {
        return;
    }

    if (!IsValid(InstigatedBy))
    {
        return;
    }

    APawn* InstigatorPawn = InstigatedBy->GetPawn();

    if (!IsValid(InstigatorPawn))
    {
        return;
    }

    FArenaDamageSample& Sample =
        DamageSamples.AddDefaulted_GetRef();

    Sample.TimeSeconds = GetWorld()->GetTimeSeconds();
    Sample.DamagedLocation = DamagedActor->GetActorLocation();
    Sample.InstigatorLocation = InstigatorPawn->GetActorLocation();
}
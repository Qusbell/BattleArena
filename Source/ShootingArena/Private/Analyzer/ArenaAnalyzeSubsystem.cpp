
#include "Analyzer/ArenaAnalyzeSubsystem.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

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

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("===== Arena Analyze Result =====")
    );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Total Samples: %d"),
        InfoSamples.Num()
    );

    for (const FArenaInfoSample& Sample : InfoSamples)
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("Time: %.2f | Location: %s"),
            Sample.TimeSeconds,
            *Sample.Location.ToString()
        );
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("==============================")
    );

    Super::Deinitialize();
}

void UArenaAnalyzeSubsystem::RegisterController(AController* controller)
{
    if (!IsValid(controller)) { return; }

	RegisteredControllers.AddUnique(controller);
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
        FArenaInfoSample& sample = InfoSamples.AddDefaulted_GetRef();
        sample.TimeSeconds = nowTime;
        sample.Location = pawn->GetActorLocation();
	}
}

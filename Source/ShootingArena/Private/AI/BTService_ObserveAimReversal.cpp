#include "AI/BTService_ObserveAimReversal.h"
#include "AI/AimReversalState.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

UBTService_ObserveAimReversal::UBTService_ObserveAimReversal()
{
    NodeName = TEXT("Observe Aim Reversal");
    bCreateNodeInstance = true;
    bNotifyBecomeRelevant = true;
    bNotifyCeaseRelevant = true;
    bNotifyTick = true;
    Interval = 0.1f;
    RandomDeviation = 0.f;
    EnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_ObserveAimReversal, EnemyKey), AActor::StaticClass());
    ReversalKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_ObserveAimReversal, ReversalKey));
    EnemyKey.SelectedKeyName = TEXT("Enemy");
    ReversalKey.SelectedKeyName = TEXT("TargetReversalPenalty");
    AimSettings = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/QuakeLike_1_0/Data/AI/DA_AIAimBoundary.DA_AIAimBoundary")));
}

void UBTService_ObserveAimReversal::ResetObservation(UBlackboardComponent* Blackboard)
{
    ObservedEnemy.Reset();
    PreviousVelocity = FVector::ZeroVector;
    LastValidTime = PenaltyUntil = -DBL_MAX;
    if (Blackboard) Blackboard->SetValueAsBool(ReversalKey.SelectedKeyName, false);
}

EBlackboardNotificationResult UBTService_ObserveAimReversal::OnEnemyChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey)
{
    ResetObservation(const_cast<UBlackboardComponent*>(&Blackboard));
    return EBlackboardNotificationResult::ContinueObserving;
}

void UBTService_ObserveAimReversal::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    if (const UObject* Settings = AimSettings.LoadSynchronous())
    {
        auto ReadNumber = [Settings](const TCHAR* Name, float Fallback)
        {
            const FNumericProperty* Property = FindFProperty<FNumericProperty>(Settings->GetClass(), Name);
            return Property && Property->IsFloatingPoint()
                ? static_cast<float>(Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Settings))) : Fallback;
        };
        Interval = FMath::Max(0.01f, ReadNumber(TEXT("ReversalSampleInterval"), 0.1f));
        VelocityMemorySeconds = FMath::Max(0.f, ReadNumber(TEXT("ReversalVelocityMemory"), 0.3f));
        PenaltySeconds = FMath::Max(0.f, ReadNumber(TEXT("ReversalPenaltyDuration"), 0.2f));
    }
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    ResetObservation(Blackboard);
    ObservedBlackboard = Blackboard;
    if (Blackboard)
    {
        const auto Key = Blackboard->GetKeyID(EnemyKey.SelectedKeyName);
        if (Key != FBlackboard::InvalidKey)
            Blackboard->RegisterObserver(Key, this, FOnBlackboardChangeNotification::CreateUObject(this, &ThisClass::OnEnemyChanged));
    }
}

void UBTService_ObserveAimReversal::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (UBlackboardComponent* Blackboard = ObservedBlackboard.Get())
    {
        Blackboard->UnregisterObserversFrom(this);
        ResetObservation(Blackboard);
    }
    ObservedBlackboard.Reset();
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTService_ObserveAimReversal::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
    UWorld* World = OwnerComp.GetWorld();
    AActor* Enemy = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(EnemyKey.SelectedKeyName)) : nullptr;
    if (!Blackboard || !World || !IsValid(Enemy)) { ResetObservation(Blackboard); return; }
    if (ObservedEnemy.Get() != Enemy) { ResetObservation(Blackboard); ObservedEnemy = Enemy; }
    FAimReversalState State{PreviousVelocity, LastValidTime, PenaltyUntil};
    const bool bPenalty = State.Update(Enemy->GetVelocity(), World->GetTimeSeconds(), FMath::Max(0.f, VelocityMemorySeconds), FMath::Max(0.f, PenaltySeconds));
    PreviousVelocity = State.PreviousVelocity;
    LastValidTime = State.LastValidTime;
    PenaltyUntil = State.PenaltyUntil;
    Blackboard->SetValueAsBool(ReversalKey.SelectedKeyName, bPenalty);
}

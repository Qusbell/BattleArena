#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BTService_ObserveAimReversal.generated.h"

/** Tracks the current enemy's horizontal velocity reversals for the aim penalty. */
UCLASS()
class SHOOTINGARENA_API UBTService_ObserveAimReversal : public UBTService
{
    GENERATED_BODY()
public:
    UBTService_ObserveAimReversal();
protected:
    virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, Category="Blackboard") FBlackboardKeySelector EnemyKey;
    UPROPERTY(EditAnywhere, Category="Blackboard") FBlackboardKeySelector ReversalKey;
    UPROPERTY(EditAnywhere, Category="Aim", meta=(ClampMin="0.0")) float VelocityMemorySeconds = 0.3f;
    UPROPERTY(EditAnywhere, Category="Aim", meta=(ClampMin="0.0")) float PenaltySeconds = 0.2f;
    UPROPERTY(EditAnywhere, Category="Aim") TSoftObjectPtr<UObject> AimSettings;

private:
    EBlackboardNotificationResult OnEnemyChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey Key);
    void ResetObservation(UBlackboardComponent* Blackboard);
    TWeakObjectPtr<UBlackboardComponent> ObservedBlackboard;
    TWeakObjectPtr<AActor> ObservedEnemy;
    FVector PreviousVelocity = FVector::ZeroVector;
    double LastValidTime = -DBL_MAX;
    double PenaltyUntil = -DBL_MAX;
};

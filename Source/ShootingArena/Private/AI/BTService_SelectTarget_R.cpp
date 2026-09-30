#include "AI/BTService_SelectTarget_R.h"

#include "AI/QuakeTargetSelectionAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTService_SelectTarget_R::UBTService_SelectTarget_R()
{
	NodeName = TEXT("Select Target R");
	Interval = 0.2f;
	RandomDeviation = 0.0f;
	bNotifyTick = true;
}

void UBTService_SelectTarget_R::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (AQuakeTargetSelectionAIController* Controller =
		Cast<AQuakeTargetSelectionAIController>(OwnerComp.GetAIOwner()))
	{
		Controller->SelectBestTarget();
	}
}

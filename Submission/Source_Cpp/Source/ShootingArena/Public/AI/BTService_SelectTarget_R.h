#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SelectTarget_R.generated.h"

UCLASS()
class SHOOTINGARENA_API UBTService_SelectTarget_R : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SelectTarget_R();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaSeconds) override;
};

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "Weapon/AnimNode_WeaponAimAlignment.h"
#include "AnimGraphNode_WeaponAimAlignment.generated.h"

UCLASS()
class SHOOTINGARENAEDITORTOOLS_API UAnimGraphNode_WeaponAimAlignment : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Settings")
	FAnimNode_WeaponAimAlignment Node;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
protected:
	virtual FText GetControllerDescription() const override;
	virtual const FAnimNode_SkeletalControlBase* GetNode() const override { return &Node; }
};

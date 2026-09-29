#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttackerMemorySettingsDataAsset.generated.h"

/** Settings for how long the most recent other-player attacker remains eligible for credit. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UAttackerMemorySettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Credit", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackerMemoryTime = 5.0f;
};

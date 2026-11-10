#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AimAssistSettings.generated.h"

/** Local assistance tuning. Enabling is session-only and never saved here. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Aim Assist"))
class SHOOTINGARENA_API UAimAssistSettings : public UDeveloperSettings
{

	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Time to remove 90 percent of the angular error against a stationary target. */
	UPROPERTY(Config, EditAnywhere, Category="Aim Assist", meta=(ClampMin="0.01", ClampMax="5.0", Units="s"))
	float ConvergenceTime = 0.3f;
};

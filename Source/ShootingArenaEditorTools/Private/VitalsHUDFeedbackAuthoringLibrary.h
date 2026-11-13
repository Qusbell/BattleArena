#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "VitalsHUDFeedbackAuthoringLibrary.generated.h"

UCLASS()
class UVitalsHUDFeedbackAuthoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Editor|Vitals HUD")
	static FString InstallVitalsHUDFeedback();
};

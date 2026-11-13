#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VitalsHUDFeedbackSettings.generated.h"

class UUserWidget;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FVitalsHUDFeedbackStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Feedback", meta = (ClampMin = "0", ClampMax = "1"))
	float NumberOpacity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Feedback", meta = (ClampMin = "0", ClampMax = "1"))
	float NumberBlur = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Feedback", meta = (ClampMin = "0", ClampMax = "1"))
	float EffectOpacity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Feedback", meta = (ClampMin = "0", ClampMax = "1"))
	float EffectBlur = 0.0f;
};

UCLASS(BlueprintType)
class SHOOTINGARENA_API UVitalsHUDFeedbackSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Feedback")
	FVitalsHUDFeedbackStyle NonDamage{0.45f, 0.0f, 0.30f, 0.0f};

	UPROPERTY(EditAnywhere, Category = "Feedback")
	FVitalsHUDFeedbackStyle Damage{0.65f, 0.0f, 0.50f, 0.0f};
};

UCLASS()
class SHOOTINGARENA_API UVitalsHUDFeedbackLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals", meta = (DefaultToSelf = "HUDWidget"))
	static void ApplyNumberFeedback(UUserWidget* HUDWidget, const UVitalsHUDFeedbackSettings* Settings, bool bNonDamage);

	UFUNCTION(BlueprintCallable, Category = "HUD|Vitals", meta = (DefaultToSelf = "EffectWidget"))
	static void ApplyColorFeedback(UUserWidget* EffectWidget, const UVitalsHUDFeedbackSettings* Settings,
		UMaterialInterface* BlurMaterial);
};

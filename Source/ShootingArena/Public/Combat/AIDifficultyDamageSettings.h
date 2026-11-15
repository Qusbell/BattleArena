#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AIDifficultyDamageSettings.generated.h"

class AActor;
class AController;

USTRUCT(BlueprintType)
struct FAIDifficultyDamageRates
{
	GENERATED_BODY()

	/** Percent change to damage dealt to a human player. Zero preserves the original damage. */
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMin = "-99.0", Units = "Percent"))
	double DamageDealtPercent = 0.0;

	/** Percent change to damage received from a human player. Zero preserves the original damage. */
	UPROPERTY(EditAnywhere, Category = "Damage", meta = (ClampMin = "-99.0", Units = "Percent"))
	double DamageTakenPercent = 0.0;
};

UCLASS(BlueprintType)
class SHOOTINGARENA_API UAIDifficultyDamageSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Difficulty")
	FAIDifficultyDamageRates Easy;

	UPROPERTY(EditAnywhere, Category = "Difficulty")
	FAIDifficultyDamageRates Normal;

	UPROPERTY(EditAnywhere, Category = "Difficulty")
	FAIDifficultyDamageRates Hard;
};

UCLASS()
class SHOOTINGARENA_API UAIDifficultyDamageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Scales post-buff damage before the Life component distributes it to health and armor. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Combat|AI Difficulty")
	static double ScaleAIDifficultyDamage(AActor* Victim, AController* InstigatedBy, double Damage);
};

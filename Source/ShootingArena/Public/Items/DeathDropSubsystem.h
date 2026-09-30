#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Subsystems/WorldSubsystem.h"
#include "DeathDropSubsystem.generated.h"

/** Collects death-state candidates before LifeInfo notifies listeners. */
UCLASS()
class SHOOTINGARENA_API UDeathDropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Call immediately before applying NewHealth to LifeInfo. Alive/client calls are ignored. */
	UFUNCTION(BlueprintCallable, Category="Items|Death Drop")
	static void HandleFatalDamage(AActor* Character, double NewHealth, FDataTableRowHandle Settings);

private:
	TSet<TWeakObjectPtr<AActor>> ProcessedDeaths;
};

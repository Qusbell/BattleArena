#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShooterDisplayLibrary.generated.h"

class APawn;

/** Read-only adapters for the existing Blueprint life and display-name data. */
UCLASS()
class SHOOTINGARENA_API UShooterDisplayLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Shooter Display")
	static bool IsShooterAlive(APawn* Shooter);
	UFUNCTION(BlueprintCallable, Category = "Shooter Display")
	static FText GetShooterDisplayName(APawn* Shooter);
};

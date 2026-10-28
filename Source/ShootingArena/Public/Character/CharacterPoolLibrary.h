#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CharacterPoolLibrary.generated.h"

class APawn;
class APlayerController;
class UActorComponent;

/** Narrow bridge preserving the existing Blueprint lifecycle entry points. */
UCLASS()
class SHOOTINGARENA_API UCharacterPoolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="Character Pool")
	static bool IsDedicatedPawn(APawn* Pawn);
	UFUNCTION(BlueprintPure, Category="Character Pool")
	static bool ShouldApplyPossessionAppearance(APawn* Pawn);
	UFUNCTION(BlueprintPure, Category="Character Pool")
	static bool CanPlayMovementEffects(APawn* Pawn);
	/** True when the native pawn-owned timer handled this death. Legacy client UI keeps its existing timer. */
	UFUNCTION(BlueprintCallable, Category="Character Pool")
	static bool StartPooledRespawn(UActorComponent* RespawnHelper);
	UFUNCTION(BlueprintCallable, Category="Character Pool")
	static void ReserveWaitingPawn(APlayerController* PlayerController, float WaitingTime);
	UFUNCTION(BlueprintCallable, Category="Character Pool", meta=(WorldContext="WorldContextObject"))
	static APawn* GetWaitingPawn(const UObject* WorldContextObject, const FString& UID);
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DebugSpectatorProtectionComponent.generated.h"

class APawn;

/** Server-side parking for the player's body while the debug camera observes an AI. */
UCLASS()
class SHOOTINGARENA_API UDebugSpectatorProtectionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UDebugSpectatorProtectionComponent();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Debug|Spectator", meta=(DefaultToSelf="DebugComponent"))
	static bool BeginDebugSpectating(UActorComponent* DebugComponent);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Debug|Spectator", meta=(DefaultToSelf="DebugComponent"))
	static void EndDebugSpectating(UActorComponent* DebugComponent);

	/** Keep this inside world bounds and above Kill Z, away from the playable arena. */
	UPROPERTY(EditAnywhere, Category="Debug|Spectator")
	FVector ParkingLocation = FVector(0.0, 0.0, 500000.0);

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool ParkPawn(APawn* Pawn);
	void RestorePawn();
	TWeakObjectPtr<APawn> ParkedPawn;
	FTransform SavedTransform;
	FVector SavedVelocity = FVector::ZeroVector;
	EMovementMode SavedMovementMode = MOVE_None;
	uint8 SavedCustomMovementMode = 0;
	bool bSavedCanBeDamaged = true;
	bool bSavedCollisionEnabled = true;
	bool bSpectating = false;
};

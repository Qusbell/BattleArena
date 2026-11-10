#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AimAssistComponent.generated.h"

class SWidget;
class APawn;

/** Invoked by the local controller before its normal camera rotation update. */
UCLASS()
class SHOOTINGARENA_API UAimAssistComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UAimAssistComponent();
	void ProcessToggle();
	bool TryGetRotation(float DeltaTime, FRotator& OutRotation);

	UFUNCTION(BlueprintPure, Category="Aim Assist")
	bool IsAimAssistEnabled() const { return bEnabled; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	bool CanAssist() const;
	APawn* SelectTarget(FVector& OutHeart) const;
	APawn* SelectTargetInView(APawn* Self, FVector Origin, const FIntRect& Rect, const FMatrix& Matrix, FVector& OutHeart) const;
	void ShowStatus();
	void ClearStatus();
	bool bEnabled = false;
	TSharedPtr<SWidget> StatusWidget;
	FTimerHandle StatusTimer;
	friend class FAimAssistIntegrationTest;
};

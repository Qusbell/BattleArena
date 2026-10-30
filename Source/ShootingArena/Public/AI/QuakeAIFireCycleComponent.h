#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "QuakeAIFireCycleComponent.generated.h"

class AAIController;
class APawn;

/** Owns one fire/rest cycle per combat, rather than per BT service activation. */
UCLASS(ClassGroup = AI, meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UQuakeAIFireCycleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuakeAIFireCycleComponent();

	// Called by the existing service's activation/deactivation events. The service is a lease token.
	UFUNCTION(BlueprintCallable, Category = "AI|Fire Cycle")
	static void SetFireGateForController(AAIController* Controller, UObject* Service, bool bAllowed);

	// BP supplies its fixed characteristic values when the fire service activates.
	UFUNCTION(BlueprintCallable, Category = "AI|Fire Cycle")
	static void ActivateFireGateForController(AAIController* Controller, UObject* Service,
		double AimSkill, double FireThrottle);

	void InitializeForPawn(APawn* Pawn);
	void ObserveBlackboard();
	void ResetForLife();

	UFUNCTION(BlueprintPure, Category = "AI|Fire Cycle")
	bool IsCombatCycleActive() const { return bCombatActive; }

	UFUNCTION(BlueprintPure, Category = "AI|Fire Cycle")
	bool IsFirePhase() const { return bFirePhase; }

	UFUNCTION(BlueprintPure, Category = "AI|Fire Cycle")
	double GetPhaseEndTime() const { return PhaseEndTime; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Deactivate() override;

private:
	friend class FQuakeAIFireCycleLifecycleTest;

	void SetFireGate(UObject* Service, bool bAllowed);
	void RefreshCombat();
	void ResetCombat();
	void UpdateCycle();
	void BeginNextPhase(double Now);
	void SendWeaponInput(bool bUse);
	void UnbindLife();
	EBlackboardNotificationResult OnBlackboardChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey Key);

	UFUNCTION()
	void OnPawnDeath(AController* InstigatedBy, AActor* DamageCauser);

	TWeakObjectPtr<APawn> LifePawn;
	TWeakObjectPtr<UBlackboardComponent> ObservedBlackboard;
	TWeakObjectPtr<UActorComponent> LifeComponent;
	TWeakObjectPtr<UObject> ActiveService;
	FTimerHandle UpdateTimer;
	bool bLifeEnded = true;
	bool bCombatActive = false;
	bool bFirePhase = false;
	double PhaseEndTime = 0.;
	bool bHasCharacteristic = false;
	double CachedAimSkill = 0.;
	double CachedFireThrottle = 0.;

	UPROPERTY(Transient)
	TObjectPtr<UFunction> UsingItemFunction;
};

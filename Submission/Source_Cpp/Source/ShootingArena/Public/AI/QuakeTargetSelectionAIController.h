#pragma once

#include "CoreMinimal.h"
#include "../../MyAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "QuakeTargetSelectionAIController.generated.h"

class ACharacter;

UCLASS()
class SHOOTINGARENA_API AQuakeTargetSelectionAIController : public AMyAIController
{
	GENERATED_BODY()

public:
	AQuakeTargetSelectionAIController();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "AI|Target Selection")
	void GetCharacterTargets(TArray<ACharacter*>& Characters) const;
	virtual void GetCharacterTargets_Implementation(TArray<ACharacter*>& Characters) const;

	UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
	void SelectBestTarget();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Target Selection")
	TArray<TObjectPtr<ACharacter>> CharacterTargets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Target Selection", meta = (ClampMin = "0.0"))
	float RecentDamageWindow = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Target Selection", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float LookDotThreshold = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Target Selection")
	FVector RecentDamageDirection = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Target Selection")
	float RecentDamageExpireTime = 0.0f;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void HandlePossessedPawnDamaged(AActor* DamagedActor, float Damage,
		const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

	static bool IsCharacterDead(const ACharacter* Character);
	ACharacter* ResolveAttacker(AController* InstigatedBy, AActor* DamageCauser) const;
	bool TryGetAggression(float& OutScore, float& OutThreshold);
	UClass* GetAcceptedTargetClass() const;
	void ApplySelectedTarget(ACharacter* Target);
};

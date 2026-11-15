#pragma once

#include "CoreMinimal.h"
#include "../../MyAIController.h"
#include "LobbyTypes.h"
#include "Perception/AIPerceptionTypes.h"
#include "QuakeTargetSelectionAIController.generated.h"

class ACharacter;
class UQuakeAIFireCycleComponent;

UCLASS()
class SHOOTINGARENA_API AQuakeTargetSelectionAIController : public AMyAIController
{
	GENERATED_BODY()

public:
	AQuakeTargetSelectionAIController();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Fire Cycle")
	TObjectPtr<UQuakeAIFireCycleComponent> FireCycle;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "AI|Target Selection")
	void GetCharacterTargets(TArray<ACharacter*>& Characters) const;
	virtual void GetCharacterTargets_Implementation(TArray<ACharacter*>& Characters) const;

	UFUNCTION(BlueprintCallable, Category = "AI|Target Selection")
	void SelectBestTarget();

	/** The spawn path supplies the same difficulty index it uses for character traits. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|Difficulty")
	void SetDamageDifficultyIndex(int32 DifficultyIndex);

	TOptional<ELobbyDifficulty> GetDamageDifficulty() const { return DamageDifficulty; }

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
	TOptional<ELobbyDifficulty> DamageDifficulty;

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

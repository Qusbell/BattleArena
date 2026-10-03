#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterRevengeComponent.generated.h"

class APawn;
class APlayerState;
class URecentAttackerComponent;
struct FKillCreditResolution;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShooterRevengeTargetChanged, APlayerState*, TargetPlayerState);

/** Remembers a direct killer until revenge is completed; also accepts external targets. */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UShooterRevengeComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UShooterRevengeComponent();
	/** Supply the target on authority. Its PlayerState is replicated only to this controller's owner. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void SetRevengeTargetPlayerState(APlayerState* Target);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void SetRevengeTarget(APawn* Target);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void ClearRevengeTarget();
	UFUNCTION(BlueprintPure, Category = "Shooter Revenge")
	APlayerState* GetRevengeTargetPlayerState() const;
	UFUNCTION(BlueprintPure, Category = "Shooter Revenge")
	bool IsRevengeTarget(const APawn* Shooter) const;
	UPROPERTY(BlueprintAssignable, Category = "Shooter Revenge")
	FShooterRevengeTargetChanged OnRevengeTargetChanged;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void BindPawnDeathCredit(APawn* NewPawn);
	UFUNCTION()
	void HandleKillCreditResolved(const FKillCreditResolution& Resolution);
	TWeakObjectPtr<URecentAttackerComponent> BoundDeathCredit;
	FDelegateHandle PawnChangedHandle;
	bool bEndingPlay = false;

	UPROPERTY(ReplicatedUsing = OnRep_TargetPlayerState)
	TObjectPtr<APlayerState> TargetPlayerState;
	UFUNCTION()
	void OnRep_TargetPlayerState();
};

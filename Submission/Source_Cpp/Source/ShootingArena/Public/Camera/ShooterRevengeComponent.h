#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterRevengeComponent.generated.h"

class APawn;
class APlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShooterRevengeTargetChanged, APlayerState*, TargetPlayerState);

/** Presents BP_QuakePlayerState.RevengeTarget and transports it to the owning client. */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UShooterRevengeComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UShooterRevengeComponent();
	/** Writes the authoritative PlayerState's RevengeTarget. No separate gameplay rules are applied here. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void SetRevengeTargetPlayerState(APlayerState* Target);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void SetRevengeTarget(APawn* Target);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void ClearRevengeTarget();
	/** Optional immediate refresh; server polling also discovers existing Blueprint changes automatically. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Shooter Revenge")
	void RefreshRevengeTargetFromPlayerState();
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	APlayerState* ReadPlayerStateRevengeTarget() const;
	bool bEndingPlay = false;

	UPROPERTY(ReplicatedUsing = OnRep_TargetPlayerState)
	TObjectPtr<APlayerState> TargetPlayerState;
	UFUNCTION()
	void OnRep_TargetPlayerState();
};

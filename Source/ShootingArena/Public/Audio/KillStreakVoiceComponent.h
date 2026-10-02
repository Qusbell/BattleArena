#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KillStreakVoiceComponent.generated.h"

class UAudioComponent;
class UKillStreakSettingsDataAsset;
class USoundBase;

/** Batches credited kill events, tracks the current streak, and plays its voice for the owning player. */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UKillStreakVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKillStreakVoiceComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Streak Voice")
	TObjectPtr<UKillStreakSettingsDataAsset> Settings = nullptr;

	/** Call on the server once for each kill credited to this PlayerState. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kill Streak Voice")
	void QueueKillEvent(bool bIsRevenge = false);

	/** Call on the server when this PlayerState's pawn dies. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kill Streak Voice")
	void NotifyOwnerDeath();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ProcessKillBatch();
	USoundBase* SelectVoiceForCurrentStreak() const;
	void DispatchVoiceToOwner(USoundBase* Sound);
	void PlayVoiceLocally(USoundBase* Sound);
	void ClearPendingVoiceLocally();

	UFUNCTION(Client, Reliable)
	void ClientPlayKillStreakVoice(USoundBase* Sound);

	UFUNCTION(Client, Reliable)
	void ClientClearPendingKillStreakVoice();

	UFUNCTION()
	void HandleActiveVoiceFinished();

	FTimerHandle KillBatchTimerHandle;
	int32 PendingKillEventCount = 0;
	bool bPendingRevengeKill = false;
	int32 CurrentStreakCount = 0;
	float KillBatchStartTime = 0.0f;
	float NextVoiceAllowedTime = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveVoiceComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> PendingKillVoice = nullptr;
};

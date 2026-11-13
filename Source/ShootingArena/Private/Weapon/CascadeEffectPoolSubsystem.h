#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "CascadeEffectPoolSubsystem.generated.h"

class UNiagaraComponent;

/** Fills the existing Cascade component pool once, before combat in configured worlds. */
UCLASS()
class UCascadeEffectPoolSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	void LimitPlasmaMuzzleLifetime(UNiagaraComponent* Effect);

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	bool bPrewarmed = false;
	TMap<TWeakObjectPtr<UNiagaraComponent>, FTimerHandle> MuzzleTimers;
	UFUNCTION()
	void OnMuzzleFinished(UNiagaraComponent* Effect);
};

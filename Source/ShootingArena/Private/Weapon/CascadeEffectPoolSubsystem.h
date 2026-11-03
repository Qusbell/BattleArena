#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "CascadeEffectPoolSubsystem.generated.h"

/** Fills the existing Cascade component pool once, before combat in configured worlds. */
UCLASS()
class UCascadeEffectPoolSubsystem final : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	bool bPrewarmed = false;
};

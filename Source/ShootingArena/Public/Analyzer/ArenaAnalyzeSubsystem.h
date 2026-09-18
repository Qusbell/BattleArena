#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"

#include "ArenaAnalyzeSubsystem.generated.h"

class AController;

USTRUCT()
struct FArenaInfoSample
{
    GENERATED_BODY()

    UPROPERTY()
    double TimeSeconds = 0.0;

    UPROPERTY()
    FVector Location = FVector::ZeroVector;
};


UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

private:
	TArray<TWeakObjectPtr<AController>> RegisteredControllers;

    TArray<FArenaInfoSample> InfoSamples;

    FTimerHandle LoopTimerHandle;

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    void RegisterController(AController* Controller);

private:
	void AnalyzeControllers();
};
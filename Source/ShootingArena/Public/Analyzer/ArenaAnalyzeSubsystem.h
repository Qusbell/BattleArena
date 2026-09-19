#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"

#include "ArenaAnalyzeSubsystem.generated.h"

class AController;
class APawn;
class UDamageType;

USTRUCT(BlueprintType)
struct FArenaInfoSample
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    double TimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly)
    FVector Location = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FArenaDamageSample
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    double TimeSeconds = 0.0;

    UPROPERTY(BlueprintReadOnly)
    FVector DamagedLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector InstigatorLocation = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FArenaAnalyzeSession
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TArray<FArenaInfoSample> AIMovementSamples;

    UPROPERTY(BlueprintReadOnly)
    TArray<FArenaDamageSample> DamageSamples;
};


UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

private:
	TArray<TWeakObjectPtr<AController>> RegisteredControllers;

    TArray<FArenaInfoSample> AIMovementSamples;
    TArray<FArenaDamageSample> DamageSamples;

    FTimerHandle LoopTimerHandle;

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    void RegisterController(AController* Controller);

private:
	void AnalyzeControllers();
    void SaveSamplesToJson();

    UFUNCTION()
    void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    UFUNCTION()
    void OnPawnTakeAnyDamage(
        AActor* DamagedActor,
        float Damage,
        const UDamageType* DamageType,
        AController* InstigatedBy,
        AActor* DamageCauser
    );
};
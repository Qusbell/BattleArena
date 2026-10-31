// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "MyAIController.generated.h"

UCLASS()
class SHOOTINGARENA_API AMyAIController : public AAIController
{
    GENERATED_BODY()

public:
    virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;

    void RequestDamageLook(const FVector& WorldDirection);
    void ResetDamageLook();

    // Pawn-based entry points keep the Blueprint interface independent of the controller class.
    UFUNCTION(BlueprintCallable, Category = "AI|Orbital Look")
    static void ConfigureOrbitalLook(APawn* Observer, float RotationThreshold, float InterpSpeed, float MaxChange);

    UFUNCTION(BlueprintCallable, Category = "AI|Orbital Look")
    static void ActivateOrbitalLook(APawn* Observer);

    UFUNCTION(BlueprintCallable, Category = "AI|Orbital Look")
    static void SetOrbitalLook(APawn* Observer, AActor* LookTarget, FVector Offset);

    // The caller supplies the existing BPI_GetAim-derived, XY-only world point.
    UFUNCTION(BlueprintCallable, Category = "AI|Orbital Look")
    static void ClearOrbitalLook(APawn* Observer, FVector WorldTarget);

    UFUNCTION(BlueprintPure, Category = "AI|Orbital Look")
    static bool GetOrbitalLookDebug(APawn* Observer, FRotator& CurrentRotation, FVector& NextAimTarget);

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

private:
    TWeakObjectPtr<AActor> OrbitalLookTarget;
    FVector OrbitalAimOffset = FVector::ZeroVector;
    FVector OrbitalNextAimTarget = FVector::ZeroVector;
    FRotator OrbitalCurrentRotation = FRotator::ZeroRotator;
    // Cached when configured: speed selection needs a dot threshold, not an angle.
    double OrbitalRotationThresholdCos = 0.9996573249755573; // cos(1.5 degrees)
    float OrbitalInterpSpeed = 5.0f;
    float OrbitalMaxChange = 180.0f;
    bool bOrbitalLookActive = false;
    FVector DamageLookDirection = FVector::ZeroVector;
    FVector PendingDamageLookDirection = FVector::ZeroVector;
    bool bDamageLookActive = false;
    bool bPendingDamageLook = false;
};

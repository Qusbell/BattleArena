#include "MyAIController.h"
#include "AISystem.h"

void AMyAIController::OnPossess(APawn* InPawn)
{
    // Initialize before Super can dispatch the Blueprint possession event or start the BT.
    OrbitalLookTarget.Reset();
    OrbitalAimOffset = FVector::ZeroVector;
    OrbitalNextAimTarget = FVector::ZeroVector;
    OrbitalCurrentRotation = InPawn ? InPawn->GetActorRotation() : FRotator::ZeroRotator;
    OrbitalRotationThresholdCos = FMath::Cos(FMath::DegreesToRadians(1.5));
    OrbitalInterpSpeed = 5.0f;
    OrbitalMaxChange = 180.0f;
    bOrbitalLookActive = false;
    Super::OnPossess(InPawn);
}

void AMyAIController::OnUnPossess()
{
    bOrbitalLookActive = false;
    OrbitalLookTarget.Reset();
    Super::OnUnPossess();
}

void AMyAIController::ConfigureOrbitalLook(APawn* Observer, float RotationThreshold, float InterpSpeed, float MaxChange)
{
    if (AMyAIController* Controller = Observer ? Cast<AMyAIController>(Observer->GetController()) : nullptr;
        Controller && Controller->HasAuthority())
    {
        // Match angle > threshold even for thresholds outside the [0, 180] range.
        // NaN, like the original comparison, selects the inner speed.
        Controller->OrbitalRotationThresholdCos = RotationThreshold < 0.0f ? 2.0
            : RotationThreshold < 180.0f ? FMath::Cos(FMath::DegreesToRadians(static_cast<double>(RotationThreshold)))
            : -1.0;
        Controller->OrbitalInterpSpeed = InterpSpeed;
        Controller->OrbitalMaxChange = MaxChange;
    }
}

void AMyAIController::ActivateOrbitalLook(APawn* Observer)
{
    if (AMyAIController* Controller = Observer ? Cast<AMyAIController>(Observer->GetController()) : nullptr;
        Controller && Controller->HasAuthority())
    {
        Controller->bOrbitalLookActive = true;
    }
}

void AMyAIController::SetOrbitalLook(APawn* Observer, AActor* LookTarget, FVector Offset)
{
    if (AMyAIController* Controller = Observer ? Cast<AMyAIController>(Observer->GetController()) : nullptr;
        Controller && Controller->HasAuthority() && IsValid(LookTarget))
    {
        Controller->OrbitalLookTarget = LookTarget;
        Controller->OrbitalAimOffset = Offset;
        Controller->OrbitalNextAimTarget = LookTarget->GetActorLocation() + Offset;
    }
}

void AMyAIController::ClearOrbitalLook(APawn* Observer, FVector WorldTarget)
{
    if (AMyAIController* Controller = Observer ? Cast<AMyAIController>(Observer->GetController()) : nullptr;
        Controller && Controller->HasAuthority())
    {
        Controller->OrbitalLookTarget.Reset();
        Controller->OrbitalAimOffset = FVector::ZeroVector;
        Controller->OrbitalNextAimTarget = WorldTarget;
    }
}

bool AMyAIController::GetOrbitalLookDebug(APawn* Observer, FRotator& CurrentRotation, FVector& NextAimTarget)
{
    CurrentRotation = FRotator::ZeroRotator;
    NextAimTarget = FVector::ZeroVector;
    if (const AMyAIController* Controller = Observer ? Cast<AMyAIController>(Observer->GetController()) : nullptr)
    {
        CurrentRotation = Controller->OrbitalCurrentRotation;
        NextAimTarget = Controller->OrbitalNextAimTarget;
        return true;
    }
    return false;
}

void AMyAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    APawn* const MyPawn = GetPawn();
    if (MyPawn)
    {
        FRotator NewControlRotation = GetControlRotation();

        if (bOrbitalLookActive && HasAuthority())
        {
            if (const AActor* Target = OrbitalLookTarget.Get())
            {
                OrbitalNextAimTarget = Target->GetActorLocation() + OrbitalAimOffset;
            }

            FVector ViewLocation;
            FRotator ViewRotation;
            MyPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
            const FVector TargetDelta = OrbitalNextAimTarget - ViewLocation;
            const FRotator TargetRotation = TargetDelta.Rotation();
            // A zero delta previously produced zero rotation, whose forward vector is +X.
            const FVector TargetDirection = TargetDelta.GetSafeNormal(0.0, FVector::ForwardVector);
            const double Dot = FMath::Clamp(FVector::DotProduct(
                OrbitalCurrentRotation.Vector(), TargetDirection), -1.0, 1.0);
            const float Speed = Dot < OrbitalRotationThresholdCos ? OrbitalMaxChange : OrbitalInterpSpeed;
            OrbitalCurrentRotation = FMath::RInterpConstantTo(OrbitalCurrentRotation, TargetRotation, DeltaTime, Speed);

            // Preserve the old drone's EyesViewPoint origin and the controller's PawnViewLocation origin.
            // No Actor transform update is needed to represent this virtual focus point.
            const FVector PawnViewLocation = MyPawn->GetPawnViewLocation();
            if (ViewLocation == PawnViewLocation &&
                OrbitalCurrentRotation.Pitch > -90.0 && OrbitalCurrentRotation.Pitch < 90.0)
            {
                // With coincident origins the vector round trip only normalizes yaw and drops roll.
                NewControlRotation = FRotator(OrbitalCurrentRotation.Pitch,
                    FRotator::NormalizeAxis(OrbitalCurrentRotation.Yaw), 0.0);
            }
            else
            {
                NewControlRotation = (ViewLocation + OrbitalCurrentRotation.Vector() * 100.0
                    - PawnViewLocation).Rotation();
            }
        }
        else
        {
            const FVector FocalPoint = GetFocalPoint();
            if (FAISystem::IsValidLocation(FocalPoint))
            {
                NewControlRotation = (FocalPoint - MyPawn->GetPawnViewLocation()).Rotation();
            }
            else if (bSetControlRotationFromPawnOrientation)
            {
                NewControlRotation = MyPawn->GetActorRotation();
            }
        }


        SetControlRotation(NewControlRotation);

        if (bUpdatePawn)
        {
            const FRotator CurrentPawnRotation = MyPawn->GetActorRotation();
            if (CurrentPawnRotation.Equals(NewControlRotation, 1e-3f) == false)
            {
                MyPawn->FaceRotation(NewControlRotation, DeltaTime);
            }
        }
    }
}

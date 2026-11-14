#include "AI/PathLink/Internal/PathLinkJumpCapability.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"

FPathLinkJumpCapability FPathLinkJumpCapability::FromAgent(const AActor* Agent)
{
    const ACharacter* Character = Cast<ACharacter>(Agent);
    const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
    if (!Movement)
    {
        return {};
    }

    // The Blueprint updates MaxWalkSpeed during the run-up. Its cached limit is the bot's
    // configured maximum, independent of how fast it happens to be moving when asked.
    double Speed = Movement->MaxWalkSpeed;
    const FNumericProperty* Limit = FindFProperty<FNumericProperty>(
        Character->GetClass(), TEXT("NPC Limited Speed Cached"));
    if (Limit && Limit->IsFloatingPoint())
    {
        const double CachedSpeed = Limit->GetFloatingPointPropertyValue(Limit->ContainerPtrToValuePtr<void>(Character));
        if (FMath::IsFinite(CachedSpeed) && CachedSpeed > 0)
        {
            Speed = CachedSpeed;
        }
    }

    return {-Movement->GetGravityDirection(), -Movement->GetGravityZ(), Movement->JumpZVelocity, Speed};
}

bool FPathLinkJumpCapability::Allows(EPathLinkType Type, const FVector& Entry, const FVector& Exit) const
{
    if (Type != EPathLinkType::Jump || Gravity <= UE_SMALL_NUMBER)
    {
        return true;
    }

    const FVector Displacement = Exit - Entry;
    const double Rise = Displacement.Dot(Up);
    const double LaunchSpeed = FMath::Max(0.0, JumpSpeed);
    const double Discriminant = FMath::Square(LaunchSpeed) - 2 * Gravity * Rise;
    if (Discriminant < 0)
    {
        return false;
    }

    const double FlightTime = (LaunchSpeed + FMath::Sqrt(Discriminant)) / Gravity;
    const double Reach = FMath::Max(0.0, HorizontalSpeed) * FlightTime;
    return (Displacement - Up * Rise).SizeSquared() <= FMath::Square(Reach);
}

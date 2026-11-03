#include "Debug/DebugSpectatorProtectionComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PawnMovementComponent.h"

UDebugSpectatorProtectionComponent::UDebugSpectatorProtectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

bool UDebugSpectatorProtectionComponent::BeginDebugSpectating(UActorComponent* DebugComponent)
{
	auto* Controller = IsValid(DebugComponent) ? Cast<APlayerController>(DebugComponent->GetOwner()) : nullptr;
	if (!IsValid(Controller) || !Controller->HasAuthority() || !IsValid(Controller->GetPawn())) return false;
	auto* Protection = Controller->FindComponentByClass<UDebugSpectatorProtectionComponent>();
	if (!Protection)
	{
		Protection = NewObject<UDebugSpectatorProtectionComponent>(Controller);
		Protection->RegisterComponent();
	}
	if (!Protection->ParkPawn(Controller->GetPawn())) return false;
	Protection->bSpectating = true;
	Protection->SetComponentTickEnabled(true);
	return true;
}

void UDebugSpectatorProtectionComponent::EndDebugSpectating(UActorComponent* DebugComponent)
{
	AActor* Owner = IsValid(DebugComponent) ? DebugComponent->GetOwner() : nullptr;
	if (!IsValid(Owner) || !Owner->HasAuthority()) return;
	if (auto* Protection = Owner->FindComponentByClass<UDebugSpectatorProtectionComponent>())
	{
		Protection->bSpectating = false;
		Protection->SetComponentTickEnabled(false);
		Protection->RestorePawn();
	}
}

bool UDebugSpectatorProtectionComponent::ParkPawn(APawn* Pawn)
{
	if (ParkedPawn.Get() == Pawn) return true; // Switching viewed AI must not overwrite the return position.
	RestorePawn();
	if (!IsValid(Pawn)) return false;
	ParkedPawn = Pawn;
	SavedTransform = Pawn->GetActorTransform();
	bSavedCanBeDamaged = Pawn->CanBeDamaged();
	bSavedCollisionEnabled = Pawn->GetActorEnableCollision();
	SavedVelocity = Pawn->GetVelocity();
	Pawn->SetCanBeDamaged(false);
	// Prevent the parked body from blocking shots or generating gameplay overlaps.
	Pawn->SetActorEnableCollision(false);
	if (auto* Character = Cast<ACharacter>(Pawn))
	{
		auto* Movement = Character->GetCharacterMovement();
		SavedMovementMode = Movement->MovementMode;
		SavedCustomMovementMode = Movement->CustomMovementMode;
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	else if (auto* Movement = Pawn->GetMovementComponent()) Movement->StopMovementImmediately();
	if (!Pawn->SetActorLocation(ParkingLocation, false, nullptr, ETeleportType::TeleportPhysics))
	{
		RestorePawn();
		return false;
	}
	Pawn->ForceNetUpdate();
	return true;
}

void UDebugSpectatorProtectionComponent::RestorePawn()
{
	APawn* Pawn = ParkedPawn.Get();
	ParkedPawn.Reset();
	if (!IsValid(Pawn)) return;
	Pawn->SetActorTransform(SavedTransform, false, nullptr, ETeleportType::TeleportPhysics);
	Pawn->SetActorEnableCollision(bSavedCollisionEnabled);
	Pawn->SetCanBeDamaged(bSavedCanBeDamaged);
	if (auto* Character = Cast<ACharacter>(Pawn))
	{
		auto* Movement = Character->GetCharacterMovement();
		Movement->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
		Movement->Velocity = SavedVelocity;
	}
	Pawn->ForceNetUpdate();
}

void UDebugSpectatorProtectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bSpectating || !GetOwner()->HasAuthority()) return;
	auto* Controller = Cast<APlayerController>(GetOwner());
	APawn* CurrentPawn = Controller ? Controller->GetPawn() : nullptr;
	// A respawn or possession change releases the old body and protects the new one.
	if (CurrentPawn != ParkedPawn.Get()) ParkPawn(CurrentPawn);
	if (APawn* Pawn = ParkedPawn.Get())
	{
		if (auto* Movement = Pawn->GetMovementComponent()) Movement->StopMovementImmediately();
		Pawn->SetActorLocation(ParkingLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UDebugSpectatorProtectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestorePawn();
	Super::EndPlay(EndPlayReason);
}

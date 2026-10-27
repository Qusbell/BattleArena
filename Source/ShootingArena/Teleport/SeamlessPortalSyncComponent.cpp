#include "SeamlessPortalSyncComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

USeamlessPortalSyncComponent::USeamlessPortalSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

USeamlessPortalSyncComponent* USeamlessPortalSyncComponent::FindOrCreate(ACharacter* character)
{
	if (!IsValid(character) || !character->HasAuthority() || !IsValid(character->GetWorld())
		|| character->GetWorld()->bIsTearingDown || !Cast<APlayerController>(character->GetController())) return nullptr;
	if (auto* existing = character->FindComponentByClass<USeamlessPortalSyncComponent>()) return existing;
	auto* component = NewObject<USeamlessPortalSyncComponent>(character, NAME_None, RF_Transient);
	character->AddInstanceComponent(component);
	component->RegisterComponent();
	return component->IsRegistered() ? component : nullptr;
}

void USeamlessPortalSyncComponent::SendArrival()
{
	ACharacter* character = Cast<ACharacter>(GetOwner());
	UWorld* world = GetWorld();
	if (!IsValid(character) || !character->HasAuthority() || !IsValid(world) || world->bIsTearingDown) return;
	UCharacterMovementComponent* movement = character->GetCharacterMovement();
	APlayerController* player = Cast<APlayerController>(character->GetController());
	if (!IsValid(movement) || !IsValid(player) || player->GetPawn() != character) return;
	FSeamlessPortalArrival arrival;
	arrival.location = character->GetActorLocation();
	arrival.velocity = movement->Velocity;
	arrival.facing = character->GetActorRotation();
	arrival.controlRotation = player->GetControlRotation();
	if (arrival.location.ContainsNaN() || arrival.velocity.ContainsNaN()
		|| arrival.facing.ContainsNaN() || arrival.controlRotation.ContainsNaN()) return;
	arrival.bValid = true;
	// Never send a queued pre-teleport correction after the new arrival. The next ordinary
	// client move will generate a fresh CMC response; keep its timestamp clock unchanged.
	if (movement->HasPredictionData_Server())
	{
		auto* server = movement->GetPredictionData_Server_Character();
		server->PendingAdjustment.TimeStamp = 0;
		server->bForceClientUpdate = true;
		movement->ForceClientAdjustment();
	}
	ClientArrive(arrival);
}

void USeamlessPortalSyncComponent::ClientArrive_Implementation(const FSeamlessPortalArrival& arrival)
{
	ApplyArrival(arrival);
}

bool USeamlessPortalSyncComponent::ApplyArrival(const FSeamlessPortalArrival& arrival)
{
	ACharacter* character = Cast<ACharacter>(GetOwner());
	UWorld* world = GetWorld();
	if (!arrival.bValid || arrival.location.ContainsNaN() || arrival.velocity.ContainsNaN()
		|| arrival.facing.ContainsNaN() || arrival.controlRotation.ContainsNaN()
		|| !IsValid(character) || !IsValid(world) || world->bIsTearingDown) return false;
	APlayerController* player = Cast<APlayerController>(character->GetController());
	UCharacterMovementComponent* movement = character->GetCharacterMovement();
	if (!IsValid(player) || !player->IsLocalController() || player->GetPawn() != character || !IsValid(movement)) return false;
	if (!character->HasAuthority())
	{
		// Inputs saved before receipt were simulated at the entrance. Replaying them as a
		// correction after this arrival can rewind the camera or combine a move across portals.
		// Do NOT ResetPredictionData_Client(): resetting its clock makes the server reject moves.
		if (movement->HasPredictionData_Client())
		{
			auto* prediction = movement->GetPredictionData_Client_Character();
			prediction->SavedMoves.Reset();
			prediction->PendingMove.Reset();
			prediction->LastAckedMove.Reset();
			prediction->bUpdatePosition = false;
			prediction->MeshTranslationOffset = FVector::ZeroVector;
			prediction->OriginalMeshTranslationOffset = FVector::ZeroVector;
			prediction->MeshRotationOffset = FQuat::Identity;
			prediction->OriginalMeshRotationOffset = FQuat::Identity;
			prediction->MeshRotationTarget = FQuat::Identity;
		}
		// Collision/eligibility was already validated by the server. Do not choose a different
		// client-side fallback location (or a default origin) for the same accepted teleport.
		if (!character->TeleportTo(arrival.location, arrival.facing, false, true)) return false;
		if (!IsValid(character) || !IsValid(movement) || world->bIsTearingDown) return false;
		character->SetBase(nullptr);
		if (!IsValid(character) || !IsValid(movement) || world->bIsTearingDown) return false;
		movement->Velocity = arrival.velocity;
		movement->bJustTeleported = true;
		movement->SetMovementMode(MOVE_Falling);
		if (!IsValid(character) || !IsValid(movement) || !IsValid(player) || world->bIsTearingDown) return false;
		movement->bNetworkSmoothingComplete = true;
		player->SetControlRotation(arrival.controlRotation);
	}
	// A listen-server player was already moved by TryTraverse; do not teleport them twice.
	if (!IsValid(character) || !IsValid(player) || world->bIsTearingDown) return false;
	if (IsValid(player->PlayerCameraManager)) player->PlayerCameraManager->SetGameCameraCutThisFrame();
	return true;
}

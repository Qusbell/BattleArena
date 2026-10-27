#include "SeamlessPortalTraversalSubsystem.h"

#include "SeamlessPortalActor.h"
#include "SeamlessPortalSyncComponent.h"
#include "PortalTraversalGuard.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

bool USeamlessPortalTraversalSubsystem::DoesSupportWorldType(EWorldType::Type type) const
{
	return type == EWorldType::Game || type == EWorldType::PIE;
}

void USeamlessPortalTraversalSubsystem::RegisterPortal(ASeamlessPortalActor* portal)
{
	if (!bDeinitializing && IsValid(portal) && portal->GetWorld() == GetWorld()) portals.Add(portal);
}

void USeamlessPortalTraversalSubsystem::UnregisterPortal(ASeamlessPortalActor* portal)
{
	portals.Remove(portal);
	// The active tick recomputes exclusions after movement. Do not mutate its state from a callback.
	UWorld* world = GetWorld();
	if (bUpdatingTravellers || bDeinitializing || !IsValid(world) || world->bIsTearingDown) return;
	TGuardValue<bool> updating(bUpdatingTravellers, true);
	// Any owned collision exclusions must not survive removal of their portal.
	TArray<TWeakObjectPtr<ACharacter>> characters;
	travellers.GenerateKeyArray(characters);
	for (const auto& key : characters)
	{
		const FTraveller* existing = travellers.Find(key);
		if (!existing) continue;
		FTraveller state = *existing;
		ACharacter* character = key.Get();
		if (IsValid(character) && character->HasAuthority() && IsBlockedWithoutOwnedIgnores(character, state))
			character->SetActorLocation(state.safeLocation, false, nullptr, ETeleportType::TeleportPhysics);
		RestoreIgnores(character, state);
		if (bDeinitializing || world->bIsTearingDown) return;
		state.exitGrace.Reset();
		if (IsValid(character)) { state.previous = character->GetActorLocation(); travellers.Add(key, MoveTemp(state)); }
		else travellers.Remove(key);
	}
}

void USeamlessPortalTraversalSubsystem::RestoreIgnores(ACharacter* character, FTraveller& state)
{
	if (IsValid(character) && IsValid(character->GetCapsuleComponent()))
		for (const auto& wall : state.ownedIgnores)
			if (wall.IsValid()) character->GetCapsuleComponent()->IgnoreActorWhenMoving(wall.Get(), false);
	state.ownedIgnores.Empty();
}

bool USeamlessPortalTraversalSubsystem::IsBlockedWithoutOwnedIgnores(ACharacter* character, const FTraveller& state) const
{
	if (state.ownedIgnores.IsEmpty() || !IsValid(GetWorld()) || GetWorld()->bIsTearingDown
		|| !IsValid(character) || !IsValid(character->GetCapsuleComponent())) return false;
	const UCapsuleComponent* capsule = character->GetCapsuleComponent();
	FCollisionQueryParams query(SCENE_QUERY_STAT(SpatialPortalRestore), false, character);
	for (AActor* ignored : capsule->GetMoveIgnoreActors())
		if (!state.ownedIgnores.Contains(ignored)) query.AddIgnoredActor(ignored);
	return GetWorld()->OverlapBlockingTestByChannel(character->GetActorLocation(), character->GetActorQuat(),
		capsule->GetCollisionObjectType(), FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(),
		capsule->GetScaledCapsuleHalfHeight()), query, FCollisionResponseParams(capsule->GetCollisionResponseToChannels()));
}

void USeamlessPortalTraversalSubsystem::UpdatePassageIgnores(ACharacter* character, FTraveller& state, float deltaTime)
{
	UCapsuleComponent* capsule = character->GetCapsuleComponent();
	if (!IsValid(capsule)) return;
	TSet<TWeakObjectPtr<AActor>> desired;
	bool bRecover = false;
	const FVector location = character->GetActorLocation();
	if (ASeamlessPortalActor* exit = state.exitGrace.Get())
	{
		const double x = exit->GetPortalFrameTransform().InverseTransformPositionNoScale(location).X;
		if (x < -0.01 || !exit->FitsAt(character, location))
		{ bRecover = true; state.exitGrace.Reset(); }
		else if (x <= exit->GetCapsuleNormalExtent(character) + exit->GetPassageFrontExtent() + 10)
		{
			for (AActor* wall : exit->passageWalls) if (IsValid(wall)) desired.Add(wall);
		}
		else state.exitGrace.Reset();
	}
	if (!bRecover && !PortalTraversalGuard::IsLocked(character, GetWorld()->GetTimeSeconds()))
	{
		const FVector predicted = location + character->GetVelocity() * FMath::Clamp(double(deltaTime) * 2, 1.0 / 60, 0.2);
		for (const auto& weakPortal : portals)
		{
			ASeamlessPortalActor* portal = weakPortal.Get();
			if (!IsValid(portal) || !portal->CanTraverse()) continue;
			const FTransform frame = portal->GetPortalFrameTransform();
			const FVector local = frame.InverseTransformPositionNoScale(location);
			const double approachRange = portal->GetCapsuleNormalExtent(character) + portal->GetPassageFrontExtent()
				+ 10 + (predicted - location).Length();
			// An owning client may already have crossed while awaiting the server's position update.
			const double minimumX = character->HasAuthority() ? 0 : -approachRange;
			if (local.X < minimumX || local.X > approachRange || !portal->FitsAt(character, location)
				|| !portal->FitsAt(character, predicted)) continue;
			for (AActor* wall : portal->passageWalls)
				if (IsValid(wall) && wall != character) desired.Add(wall);
		}
	}
	if (character->HasAuthority() && (bRecover || desired.IsEmpty()) && IsBlockedWithoutOwnedIgnores(character, state))
	{
		// A blocked exit, sideways movement out of the opening, or disabling a portal cannot strand a capsule in its wall.
		character->SetActorLocation(state.safeLocation, false, nullptr, ETeleportType::TeleportPhysics);
		if (!IsValid(character) || !IsValid(capsule) || bDeinitializing || GetWorld()->bIsTearingDown) return;
		if (auto* movement = character->GetCharacterMovement()) movement->StopMovementImmediately();
		character->ForceNetUpdate();
		desired.Empty();
	}
	if (state.ownedIgnores.IsEmpty() && !desired.IsEmpty() && !state.exitGrace.IsValid())
		state.safeLocation = character->GetActorLocation();
	for (auto it = state.ownedIgnores.CreateIterator(); it; ++it)
	{
		if (!desired.Contains(*it))
		{
			if (it->IsValid()) capsule->IgnoreActorWhenMoving(it->Get(), false);
			it.RemoveCurrent();
		}
	}
	for (const auto& wall : desired)
	{
		// Preserve exclusions owned by the game/another system; only remove exclusions we added ourselves.
		if (!capsule->GetMoveIgnoreActors().Contains(wall.Get()))
		{
			capsule->IgnoreActorWhenMoving(wall.Get(), true);
			state.ownedIgnores.Add(wall);
		}
	}
}

void USeamlessPortalTraversalSubsystem::Tick(float deltaTime)
{
	UWorld* world = GetWorld();
	if (bUpdatingTravellers || bDeinitializing || !IsValid(world) || world->bIsTearingDown || !FMath::IsFinite(deltaTime)) return;
	TGuardValue<bool> updating(bUpdatingTravellers, true);
	for (auto it = portals.CreateIterator(); it; ++it) if (!it->IsValid()) it.RemoveCurrent();
	if (portals.IsEmpty())
	{
		for (auto& pair : travellers) RestoreIgnores(pair.Key.Get(), pair.Value);
		travellers.Empty();
		return;
	}
	PortalTraversalGuard::Cleanup(world, world->GetTimeSeconds());
	const TArray<TWeakObjectPtr<ASeamlessPortalActor>> portalSnapshot = portals.Array();
	for (auto it = travellers.CreateIterator(); it; ++it)
	{
		ACharacter* character = it.Key().Get();
		if (!IsValid(character) || (!character->HasAuthority() && !character->IsLocallyControlled()))
		{ RestoreIgnores(character, it.Value()); it.RemoveCurrent(); }
	}
	for (TActorIterator<ACharacter> it(world); it; ++it)
	{
		ACharacter* character = *it;
		if (!IsValid(character) || (!character->HasAuthority() && !character->IsLocallyControlled())) continue;
		// Prepare the owner endpoint before the first accepted crossing.
		if (character->HasAuthority()) USeamlessPortalSyncComponent::FindOrCreate(character);
		const FVector current = character->GetActorLocation();
		if (current.ContainsNaN()) continue;
		const FTraveller* existing = travellers.Find(character);
		// Never retain a map element across TeleportTo/SetActorLocation or movement delegates.
		FTraveller localState;
		if (!existing)
		{
			localState.previous = localState.safeLocation = current;
			UpdatePassageIgnores(character, localState, deltaTime);
			if (bDeinitializing || world->bIsTearingDown) return;
			if (IsValid(character)) travellers.Add(character, MoveTemp(localState));
			continue;
		}
		localState = *existing;
		FTraveller* state = &localState;
		if (!character->HasAuthority())
		{
			// A replicated teleport can land inside the destination wall before the capsule has cleared it.
			// Infer that clearance locally without executing or authorizing any client-side teleport.
			const double ordinaryStep = FMath::Max(100.0, character->GetVelocity().Size() * FMath::Max(deltaTime,0.0f) * 4);
			if (FVector::DistSquared(current,state->previous) > FMath::Square(ordinaryStep))
			{
				for (const auto& weakPortal : portalSnapshot)
				{
					ASeamlessPortalActor* portal = weakPortal.Get();
					if (!IsValid(portal) || !portals.Contains(weakPortal)) continue;
					const double x = portal->GetPortalFrameTransform().InverseTransformPositionNoScale(current).X;
					if (x >= -0.01 && x <= portal->GetCapsuleNormalExtent(character) + portal->GetPassageFrontExtent() + 10
						&& portal->FitsAt(character,current))
					{ state->exitGrace = portal; break; }
				}
			}
		}
		else if (!PortalTraversalGuard::IsLocked(character, world->GetTimeSeconds()))
		{
			ASeamlessPortalActor* first = nullptr;
			double firstFraction = DBL_MAX;
			for (const auto& portal : portalSnapshot)
			{
				double fraction;
				if (portal.IsValid() && portals.Contains(portal)
					&& portal->FindCrossing(character, state->previous, fraction) && fraction < firstFraction)
				{ first = portal.Get(); firstFraction = fraction; }
			}
			if (first)
			{
				FVector safeExit;
				const bool bTraversed = first->TryTraverse(character, state->previous, safeExit);
				if (bDeinitializing || world->bIsTearingDown) { RestoreIgnores(character, *state); return; }
				if (!IsValid(character)) { RestoreIgnores(character, *state); travellers.Remove(character); continue; }
				if (bTraversed)
				{
					RestoreIgnores(character, *state);
					state->safeLocation = safeExit;
					state->exitGrace = IsValid(first) ? Cast<ASeamlessPortalActor>(first->GetExitTarget()) : nullptr;
				}
				else if (!state->ownedIgnores.IsEmpty())
				{
					character->SetActorLocation(state->safeLocation, false, nullptr, ETeleportType::TeleportPhysics);
					if (bDeinitializing || world->bIsTearingDown) { RestoreIgnores(character, *state); return; }
					if (!IsValid(character)) { RestoreIgnores(character, *state); travellers.Remove(character); continue; }
					if (auto* movement = character->GetCharacterMovement()) movement->StopMovementImmediately();
				}
			}
		}
		UpdatePassageIgnores(character, *state, deltaTime);
		if (bDeinitializing || world->bIsTearingDown) { RestoreIgnores(character, *state); return; }
		if (IsValid(character))
		{
			state->previous = character->GetActorLocation();
			travellers.Add(character, MoveTemp(localState));
		}
		else { RestoreIgnores(character, *state); travellers.Remove(character); }
	}
}

TStatId USeamlessPortalTraversalSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USeamlessPortalTraversalSubsystem, STATGROUP_Tickables);
}

void USeamlessPortalTraversalSubsystem::Deinitialize()
{
	bDeinitializing = true;
	for (auto& pair : travellers) RestoreIgnores(pair.Key.Get(), pair.Value);
	travellers.Empty();
	portals.Empty();
	Super::Deinitialize();
}

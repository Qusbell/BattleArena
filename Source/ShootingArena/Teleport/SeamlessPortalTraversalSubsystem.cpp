#include "SeamlessPortalTraversalSubsystem.h"
#include "Character/CharacterPoolComponent.h"
#include "SeamlessPortalActor.h"
#include "SeamlessPortalSyncComponent.h"
#include "PortalTraversalGuard.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

bool USeamlessPortalTraversalSubsystem::DoesSupportWorldType(EWorldType::Type type) const
{ return type==EWorldType::Game || type==EWorldType::PIE; }

bool USeamlessPortalTraversalSubsystem::CanTrack(ACharacter* character,bool bAllowResetting) const
{
	const auto* pool=IsValid(character) ? character->FindComponentByClass<UCharacterPoolComponent>() : nullptr;
	const auto* capsule=IsValid(character) ? character->GetCapsuleComponent() : nullptr;
	return !bDeinitializing && IsValid(GetWorld()) && !GetWorld()->bIsTearingDown
		&& IsValid(character) && character->GetWorld()==GetWorld()
		&& ((!UCharacterPoolComponent::IsInactive(character) && character->GetActorEnableCollision()
			&& IsValid(capsule) && capsule->IsQueryCollisionEnabled())
			|| (bAllowResetting && pool && pool->IsResetting()))
		&& ((GetWorld()->GetNetMode()!=NM_Client && character->HasAuthority())
			|| (character->IsLocallyControlled() && IsValid(Cast<APlayerController>(character->GetController()))));
}

void USeamlessPortalTraversalSubsystem::RegisterPortal(ASeamlessPortalActor* portal)
{
	if (!bDeinitializing && IsValid(portal) && portal->GetWorld()==GetWorld()) portals.Add(portal);
}

void USeamlessPortalTraversalSubsystem::NotifyNearby(ASeamlessPortalActor* portal,ACharacter* character,bool bBegin)
{
	if (!CanTrack(character,true) || !IsValid(portal) || !portals.Contains(portal)) return;
	nearbyEvents.Add({portal,character,bBegin});
	if (!bUpdatingTravellers && CanTrack(character)) DrainNearbyEvents();
}

void USeamlessPortalTraversalSubsystem::StoreState(TWeakObjectPtr<ACharacter> key,FTraveller&& state)
{
	const FTraveller* current=travellers.Find(key);
	// Pool reset/EndPlay may replace this life synchronously during a movement callback.
	if (!current || current->revision!=state.revision) return;
	if (!CanTrack(key.Get())) { RestoreIgnores(key.Get(),state); travellers.Remove(key); return; }
	if (state.nearbyPortals.IsEmpty() && !state.exitGrace.IsValid() && state.ownedIgnores.IsEmpty()) travellers.Remove(key);
	else travellers.Add(key,MoveTemp(state));
}

void USeamlessPortalTraversalSubsystem::DrainNearbyEvents()
{
	if (bUpdatingTravellers || bDeinitializing) return;
	TGuardValue<bool> updating(bUpdatingTravellers,true);
	const int32 count=nearbyEvents.Num();
	for (int32 i=0; i<count && !bDeinitializing; ++i)
	{
		const FNearbyEvent event=nearbyEvents[i];
		auto* character=event.character.Get(); auto* portal=event.portal.Get();
		if (!CanTrack(character) && CanTrack(character,true)) { nearbyEvents.Add(event); continue; }
		if (!CanTrack(character) || !IsValid(portal) || !portals.Contains(portal)) continue;
		if (event.bBegin && !travellers.Contains(character))
		{
			FTraveller fresh; fresh.safeLocation=character->GetActorLocation(); fresh.revision=++nextRevision;
			travellers.Add(character,MoveTemp(fresh));
		}
		const auto* found=travellers.Find(character); if (!found) continue;
		FTraveller state=*found;
		if (event.bBegin)
		{
			state.nearbyPortals.Add(portal);
			if (character->HasAuthority()) USeamlessPortalSyncComponent::FindOrCreate(character);
		}
		else state.nearbyPortals.Remove(portal);
		if (!CanTrack(character)) continue;
		UpdatePassageIgnores(character,state,0);
		if (bDeinitializing || GetWorld()->bIsTearingDown) return;
		StoreState(event.character,MoveTemp(state));
	}
	if (!bDeinitializing) nearbyEvents.RemoveAt(0,count,EAllowShrinking::No);
}

void USeamlessPortalTraversalSubsystem::NotifyEntry(ASeamlessPortalActor* portal,ACharacter* character)
{
	if (!CanTrack(character,true) || !character->HasAuthority() || !IsValid(portal)
		|| !portal->HasAuthority() || !portals.Contains(portal) || !portal->CanTraverse()) return;
	// Event-driven entry still needs to prune the shared weak lock table (including old PIE lives).
	PortalTraversalGuard::Cleanup(GetWorld(),GetWorld()->GetTimeSeconds());
	if (PortalTraversalGuard::IsLocked(character,GetWorld()->GetTimeSeconds())) return;
	NotifyNearby(portal,character,true);
	const auto* state=travellers.Find(character);
	entryEvents.Add({portal,character,state ? state->revision : 0});
}

void USeamlessPortalTraversalSubsystem::RestoreIgnores(ACharacter* character,FTraveller& state)
{
	if (IsValid(character) && IsValid(character->GetCapsuleComponent()))
		for (const auto& wall:state.ownedIgnores)
			if (wall.IsValid()) character->GetCapsuleComponent()->IgnoreActorWhenMoving(wall.Get(),false);
	state.ownedIgnores.Empty();
}

bool USeamlessPortalTraversalSubsystem::IsBlockedWithoutOwnedIgnores(ACharacter* character,const FTraveller& state,
	const TSet<TWeakObjectPtr<AActor>>& retained) const
{
	if (state.ownedIgnores.IsEmpty() || !IsValid(GetWorld()) || GetWorld()->bIsTearingDown
		|| !IsValid(character) || !IsValid(character->GetCapsuleComponent())) return false;
	const auto* capsule=character->GetCapsuleComponent();
	FCollisionQueryParams query(SCENE_QUERY_STAT(SpatialPortalRestore),false,character);
	for (AActor* ignored:capsule->GetMoveIgnoreActors()) if (!state.ownedIgnores.Contains(ignored)) query.AddIgnoredActor(ignored);
	for (const auto& wall:retained) if (wall.IsValid()) query.AddIgnoredActor(wall.Get());
	return GetWorld()->OverlapBlockingTestByChannel(character->GetActorLocation(),character->GetActorQuat(),
		capsule->GetCollisionObjectType(),FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(),capsule->GetScaledCapsuleHalfHeight()),
		query,FCollisionResponseParams(capsule->GetCollisionResponseToChannels()));
}

void USeamlessPortalTraversalSubsystem::UpdatePassageIgnores(ACharacter* character,FTraveller& state,float)
{
	if (!CanTrack(character) || !IsValid(character->GetCapsuleComponent())) return;
	auto* capsule=character->GetCapsuleComponent();
	TSet<TWeakObjectPtr<AActor>> desired;
	for (auto it=state.nearbyPortals.CreateIterator(); it; ++it)
	{
		auto* portal=it->Get();
		if (!IsValid(portal) || !portals.Contains(*it)) { it.RemoveCurrent(); continue; }
		if (!portal->CanTraverse() || PortalTraversalGuard::IsLocked(character,GetWorld()->GetTimeSeconds())) continue;
		for (AActor* wall:portal->passageWalls) if (IsValid(wall) && wall!=character) desired.Add(wall);
	}
	if (auto* exit=state.exitGrace.Get())
	{
		const double x=exit->GetPortalFrameTransform().InverseTransformPositionNoScale(character->GetActorLocation()).X;
		if (!portals.Contains(exit) || !IsValid(exit->GetNearbyCollision())
			|| !exit->GetNearbyCollision()->IsOverlappingActor(character)
			|| x<0 || x>exit->GetCapsuleNormalExtent(character)+exit->GetPassageFrontExtent()+10)
			state.exitGrace.Reset();
		else for (AActor* wall:exit->passageWalls) if (IsValid(wall)) desired.Add(wall);
	}
	bool dropping=false;
	for (const auto& wall:state.ownedIgnores) if (!desired.Contains(wall)) { dropping=true; break; }
	// Check every removed exclusion while retaining exclusions still owned by another nearby portal.
	if (character->HasAuthority() && dropping && IsBlockedWithoutOwnedIgnores(character,state,desired))
	{
		character->SetActorLocation(state.safeLocation,false,nullptr,ETeleportType::TeleportPhysics);
		if (!CanTrack(character) || !IsValid(capsule)) return;
		const auto* current=travellers.Find(character);
		if (!current || current->revision!=state.revision) return;
		if (auto* movement=character->GetCharacterMovement()) movement->StopMovementImmediately();
		if (!CanTrack(character)) return;
		current=travellers.Find(character);
		if (!current || current->revision!=state.revision) return;
		character->ForceNetUpdate();
	}
	for (auto it=state.ownedIgnores.CreateIterator(); it; ++it)
	{
		if (desired.Contains(*it)) continue;
		if (it->IsValid()) capsule->IgnoreActorWhenMoving(it->Get(),false);
		it.RemoveCurrent();
	}
	for (const auto& wall:desired)
		if (!capsule->GetMoveIgnoreActors().Contains(wall.Get()))
		{ capsule->IgnoreActorWhenMoving(wall.Get(),true); state.ownedIgnores.Add(wall); }
}

void USeamlessPortalTraversalSubsystem::NotifyLocalArrival(ACharacter* character)
{
	if (bUpdatingTravellers || !CanTrack(character) || character->HasAuthority() || !IsValid(character->GetCapsuleComponent())) return;
	TArray<UPrimitiveComponent*> overlaps;
	character->GetCapsuleComponent()->GetOverlappingComponents(overlaps);
	for (auto* component:overlaps)
	{
		auto* portal=IsValid(component) ? Cast<ASeamlessPortalActor>(component->GetOwner()) : nullptr;
		if (!IsValid(portal) || !portals.Contains(portal) || component!=portal->GetNearbyCollision()) continue;
		const double x=portal->GetPortalFrameTransform().InverseTransformPositionNoScale(character->GetActorLocation()).X;
		if (x<0 || x>portal->GetCapsuleNormalExtent(character)+portal->GetPassageFrontExtent()+10) continue;
		NotifyNearby(portal,character,true);
		if (auto* found=travellers.Find(character))
		{
			FTraveller state=*found; state.exitGrace=portal; state.safeLocation=character->GetActorLocation();
			TGuardValue<bool> updating(bUpdatingTravellers,true);
			UpdatePassageIgnores(character,state,0);
			StoreState(character,MoveTemp(state));
		}
		break;
	}
}

void USeamlessPortalTraversalSubsystem::ForgetCharacter(ACharacter* character)
{
	if (auto* state=travellers.Find(character)) RestoreIgnores(character,*state);
	travellers.Remove(character);
	entryEvents.RemoveAll([character](const auto& e){return e.character==character;});
	// Invalidate queued events in place: an array may currently be read across a pool callback.
	for (auto& event:nearbyEvents) if (event.character==character) event.character.Reset();
	for (auto& event:entryWork) if (event.character==character) event.character.Reset();
}

void USeamlessPortalTraversalSubsystem::UnregisterPortal(ASeamlessPortalActor* portal)
{
	portals.Remove(portal);
	if (bUpdatingTravellers || bDeinitializing || !IsValid(GetWorld()) || GetWorld()->bIsTearingDown) return;
	TGuardValue<bool> updating(bUpdatingTravellers,true);
	travellers.GenerateKeyArray(travellerKeys);
	for (const auto& key:travellerKeys)
	{
		const auto* found=travellers.Find(key); if (!found) continue;
		FTraveller state=*found; state.nearbyPortals.Remove(portal);
		if (state.exitGrace==portal) state.exitGrace.Reset();
		if (CanTrack(key.Get())) UpdatePassageIgnores(key.Get(),state,0);
		if (bDeinitializing || GetWorld()->bIsTearingDown) return;
		StoreState(key,MoveTemp(state));
	}
}

void USeamlessPortalTraversalSubsystem::Tick(float deltaTime)
{
	if (bUpdatingTravellers || bDeinitializing || !IsValid(GetWorld()) || GetWorld()->bIsTearingDown || !FMath::IsFinite(deltaTime)) return;
	// Possession may arrive after initial overlap notifications on a replicated client.
	if (GetWorld()->GetNetMode()==NM_Client)
	{
		auto* pc=GetWorld()->GetFirstPlayerController();
		auto* pawn=IsValid(pc) && pc->IsLocalController() ? Cast<ACharacter>(pc->GetPawn()) : nullptr;
		const auto* pool=IsValid(pawn) ? pawn->FindComponentByClass<UCharacterPoolComponent>() : nullptr;
		const int32 generation=pool ? pool->Life.Generation : 0;
		if (lastLocalPawn!=pawn || lastLocalGeneration!=generation)
		{
			lastLocalPawn=pawn;
			lastLocalGeneration=generation;
			if (CanTrack(pawn) && IsValid(pawn->GetCapsuleComponent()))
			{
				TArray<UPrimitiveComponent*> overlaps; pawn->GetCapsuleComponent()->GetOverlappingComponents(overlaps);
				for (auto* component:overlaps)
					if (IsValid(component)) if (auto* portal=Cast<ASeamlessPortalActor>(component->GetOwner()))
						if (component==portal->GetNearbyCollision()) NotifyNearby(portal,pawn,true);
			}
		}
	}
	DrainNearbyEvents();
	if (bDeinitializing || !IsValid(GetWorld()) || GetWorld()->bIsTearingDown) return;
	{
		TGuardValue<bool> updating(bUpdatingTravellers,true);
		Swap(entryEvents,entryWork); entryEvents.Reset();
		for (const FEntryEvent event:entryWork)
		{
			auto* character=event.character.Get(); auto* portal=event.portal.Get();
			if (!CanTrack(character) && CanTrack(character,true)) { entryEvents.Add(event); continue; }
			const auto* found=travellers.Find(event.character);
			if (!CanTrack(character) || !IsValid(portal) || !portals.Contains(portal)
				|| !found || (event.revision && found->revision!=event.revision)) continue;
			FTraveller state=*found;
			FVector safeExit;
			if (portal->TryTraverseFromOverlap(character,safeExit))
			{
				if (!CanTrack(character)) continue;
				const auto* current=travellers.Find(event.character);
				if (!current || current->revision!=state.revision) continue;
				RestoreIgnores(character,state); state.nearbyPortals.Empty(); state.safeLocation=safeExit;
				state.exitGrace=IsValid(portal) ? Cast<ASeamlessPortalActor>(portal->GetExitTarget()) : nullptr;
			}
			else if (CanTrack(character) && !PortalTraversalGuard::IsLocked(character,GetWorld()->GetTimeSeconds()) && !state.ownedIgnores.IsEmpty())
			{
				character->SetActorLocation(state.safeLocation,false,nullptr,ETeleportType::TeleportPhysics);
				if (CanTrack(character))
				{
					const auto* current=travellers.Find(event.character);
					if (current && current->revision==state.revision)
						if (auto* movement=character->GetCharacterMovement()) movement->StopMovementImmediately();
				}
			}
			if (bDeinitializing || GetWorld()->bIsTearingDown) return;
			StoreState(event.character,MoveTemp(state));
		}
		entryWork.Reset();
		travellers.GenerateKeyArray(travellerKeys);
		for (const auto& key:travellerKeys)
		{
			const auto* found=travellers.Find(key); if (!found) continue;
			FTraveller state=*found;
			if (CanTrack(key.Get())) UpdatePassageIgnores(key.Get(),state,deltaTime);
			if (bDeinitializing || GetWorld()->bIsTearingDown) return;
			StoreState(key,MoveTemp(state));
		}
	}
	DrainNearbyEvents();
}

TStatId USeamlessPortalTraversalSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(USeamlessPortalTraversalSubsystem,STATGROUP_Tickables); }

void USeamlessPortalTraversalSubsystem::Deinitialize()
{
	if (bDeinitializing) return;
	bDeinitializing=true;
	// Expire this world's locks only. Other PIE worlds have their own clocks and participants.
	PortalTraversalGuard::Cleanup(GetWorld(),TNumericLimits<double>::Max());
	for (auto& pair:travellers) RestoreIgnores(pair.Key.Get(),pair.Value);
	travellers.Empty(); portals.Empty(); nearbyEvents.Empty(); entryEvents.Empty();
	// entryWork/travellerKeys may be read by a synchronous movement callback; Tick will unwind.
	Super::Deinitialize();
}

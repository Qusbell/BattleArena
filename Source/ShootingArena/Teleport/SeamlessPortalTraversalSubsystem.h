#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SeamlessPortalTraversalSubsystem.generated.h"

class ASeamlessPortalActor;
class ACharacter;

/** Overlap-driven authority traversal and nearby local-owner collision prediction. No world character scan. */
UCLASS()
class SHOOTINGARENA_API USeamlessPortalTraversalSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterPortal(ASeamlessPortalActor* portal);
	void UnregisterPortal(ASeamlessPortalActor* portal);
	void ForgetCharacter(ACharacter* character);
	void NotifyNearby(ASeamlessPortalActor* portal, ACharacter* character, bool bBegin);
	void NotifyEntry(ASeamlessPortalActor* portal, ACharacter* character);
	void NotifyLocalArrival(ACharacter* character);
	virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type type) const override;

private:
	struct FTraveller
	{
		FVector safeLocation = FVector::ZeroVector;
		TWeakObjectPtr<ASeamlessPortalActor> exitGrace;
		TSet<TWeakObjectPtr<AActor>> ownedIgnores;
		TSet<TWeakObjectPtr<ASeamlessPortalActor>> nearbyPortals;
		uint64 revision = 0;
	};
	struct FNearbyEvent { TWeakObjectPtr<ASeamlessPortalActor> portal; TWeakObjectPtr<ACharacter> character; bool bBegin; };
	struct FEntryEvent { TWeakObjectPtr<ASeamlessPortalActor> portal; TWeakObjectPtr<ACharacter> character; uint64 revision; };
	bool CanTrack(ACharacter* character, bool bAllowResetting = false) const;
	void DrainNearbyEvents();
	void StoreState(TWeakObjectPtr<ACharacter> key, FTraveller&& state);
	void RestoreIgnores(ACharacter* character, FTraveller& state);
	bool IsBlockedWithoutOwnedIgnores(ACharacter* character, const FTraveller& state,
		const TSet<TWeakObjectPtr<AActor>>& retained) const;
	void UpdatePassageIgnores(ACharacter* character, FTraveller& state, float deltaTime);
	TSet<TWeakObjectPtr<ASeamlessPortalActor>> portals;
	TMap<TWeakObjectPtr<ACharacter>, FTraveller> travellers;
	TArray<FNearbyEvent> nearbyEvents;
	TArray<FEntryEvent> entryEvents;
	TArray<FEntryEvent> entryWork;
	TArray<TWeakObjectPtr<ACharacter>> travellerKeys;
	TWeakObjectPtr<ACharacter> lastLocalPawn;
	int32 lastLocalGeneration = -1;
	uint64 nextRevision = 0;
	// Teleport/overlap callbacks may unregister a portal or tear down the world synchronously.
	bool bUpdatingTravellers = false;
	bool bDeinitializing = false;
};

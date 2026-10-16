#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "BuffSpawnCycleSubsystem.generated.h"

class AActor;
class ULevel;

/**
 * Keeps buff item spawn points alternating between their ATK and DEF rows.
 *
 * BP_ItemSpawnPoint owns the actual spawn/respawn flow. This subsystem only
 * changes its SpawnItemTable row after the current item has been removed, so
 * the existing timer, VFX, sound, and replication behavior remain untouched.
 */
UCLASS()
class SHOOTINGARENA_API UBuffSpawnCycleSubsystem final
	: public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

private:
	struct FSpawnPointState
	{
		bool bWasInitialized = false;
		bool bWasHoldingItem = false;
	};

	void RegisterActor(AActor* Actor);
	void RegisterLevelActors(ULevel* Level, UWorld* World);
	void UnregisterLevelActors(ULevel* Level, UWorld* World);
	void UpdateSpawnPoint(AActor* SpawnPoint);
	bool UpdateReplicatedBuffItem(AActor* BuffItem);
	static FName GetNextBuffRow(FName CurrentRow);

	TMap<TWeakObjectPtr<AActor>, FSpawnPointState> SpawnPointStates;
	TSet<TWeakObjectPtr<AActor>> TrackedSpawnPoints;
	TSet<TWeakObjectPtr<AActor>> PendingBuffItems;
	FDelegateHandle ActorSpawnedHandle;
	FDelegateHandle LevelAddedHandle;
	FDelegateHandle LevelRemovedHandle;
	float TimeUntilNextUpdate = 0.0f;
	bool bInitialActorRegistrationPending = true;
};

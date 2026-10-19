#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "CharacterSpawnSoundComponent.generated.h"

class USoundAttenuation;
class USoundBase;
class AController;
class APawn;
class UNetConnection;

UENUM(BlueprintType)
enum class ECharacterSpawnSoundPhase : uint8
{
	Initial UMETA(DisplayName = "Initial Spawn"),
	Respawn UMETA(DisplayName = "Respawn")
};

UENUM(BlueprintType)
enum class ECharacterSpawnSoundSpace : uint8
{
	TwoDimensional UMETA(DisplayName = "2D"),
	ThreeDimensional UMETA(DisplayName = "3D")
};

/** A single one-shot sound scheduled relative to a successful spawn. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FCharacterSpawnSoundEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Sound")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Sound", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Sound", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DelaySeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Sound")
	ECharacterSpawnSoundSpace Space = ECharacterSpawnSoundSpace::ThreeDimensional;

	/** True: every client. False: only the player who owns this character. Independent of 2D/3D. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Sound", meta = (DisplayName = "Audible To Others"))
	bool bAudibleToOthers = true;

	/** Optional 3D attenuation override. When empty, a spatialized default is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Sound", meta = (EditCondition = "Space == ECharacterSpawnSoundSpace::ThreeDimensional"))
	TObjectPtr<USoundAttenuation> AttenuationSettings = nullptr;
};

/** Character-specific spawn sounds. Add to the base character Blueprint once. */
UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UCharacterSpawnSoundComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCharacterSpawnSoundComponent();

	/** Every valid entry is played once; an empty array leaves spawning unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Sounds|Spawn")
	TArray<FCharacterSpawnSoundEntry> InitialSpawnSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character Sounds|Spawn")
	TArray<FCharacterSpawnSoundEntry> RespawnSounds;

	/** Call on the server after possession. Each entry controls its own audience. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Character Sounds|Spawn")
	void PlaySpawnSounds(ECharacterSpawnSoundPhase Phase);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class FCharacterSpawnSoundInitialDeliveryTest;
	friend class FCharacterSpawnSoundInitialQueueTest;
	friend class FCharacterSpawnSoundDelayedLifetimeTest;

	void TryDispatchInitialSpawnSounds();
	static bool HasReadyInitialConnection(const UNetConnection* Connection);
	void DispatchSpawnSounds(ECharacterSpawnSoundPhase Phase, const FVector& SpawnLocation);
	void QueueInitialSpawnSounds(const FVector& SpawnLocation, bool bAudibleToOthers);
	void TryPlayPendingInitialSpawnSounds();
	bool IsLocalInitialPlaybackReady();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlaySpawnSounds(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation);

	UFUNCTION(Client, Reliable)
	void ClientPlaySpawnSounds(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation);

	void PlayLocalSpawnSounds(ECharacterSpawnSoundPhase Phase, const FVector& SpawnLocation, bool bAudibleToOthers);
	void PlayLocalEntry(const FCharacterSpawnSoundEntry& Entry, const FVector& SpawnLocation);

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> Default3DAttenuation = nullptr;

	// Initial events are sent once after the new owning connection can receive multicast RPCs.
	bool bInitialSpawnRequested = false;
	bool bInitialServerDispatchPending = false;
	TWeakObjectPtr<AController> InitialSpawnController;
	FVector InitialSpawnLocation = FVector::ZeroVector;
	FTimerHandle InitialServerRetryTimer;

	// Each client independently waits for its loading overlay, possession and camera update.
	uint8 PendingInitialAudience = 0;
	uint8 PlayedInitialAudience = 0;
	FVector PendingInitialLocation = FVector::ZeroVector;
	TWeakObjectPtr<APawn> ObservedLocalPawn;
	float LocalPawnObservedAt = -1.0f;
	FTimerHandle InitialLocalRetryTimer;
	TArray<FTimerHandle> DelayedSoundTimers;
};

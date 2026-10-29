#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SeamlessPortalSyncComponent.generated.h"

class ACharacter;

/** Created only after the server has accepted an arrival. An uninitialized state is never a teleport. */
USTRUCT()
struct FSeamlessPortalArrival
{
	GENERATED_BODY()

	UPROPERTY() bool bValid = false;
	UPROPERTY() FVector location = FVector::ZeroVector;
	UPROPERTY() FVector velocity = FVector::ZeroVector;
	UPROPERTY() FRotator facing = FRotator::ZeroRotator;
	UPROPERTY() FRotator controlRotation = FRotator::ZeroRotator;
};

/** Owner-only arrival RPC. No server RPC, client traversal authorization, or movement tick is added. */
UCLASS()
class SHOOTINGARENA_API USeamlessPortalSyncComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USeamlessPortalSyncComponent();
	static USeamlessPortalSyncComponent* FindOrCreate(ACharacter* character);
	void SendArrival();
	bool ApplyArrival(const FSeamlessPortalArrival& arrival);

private:
	UFUNCTION(Client, Reliable)
	void ClientArrive(const FSeamlessPortalArrival& arrival);
};

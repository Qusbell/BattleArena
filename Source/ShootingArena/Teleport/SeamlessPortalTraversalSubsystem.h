#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SeamlessPortalTraversalSubsystem.generated.h"

class ASeamlessPortalActor;
class ACharacter;

/** Authority traversal plus local-owner collision prediction, with one character scan per world. */
UCLASS()
class SHOOTINGARENA_API USeamlessPortalTraversalSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterPortal(ASeamlessPortalActor* portal);
	void UnregisterPortal(ASeamlessPortalActor* portal);
	virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type type) const override;

private:
	struct FTraveller
	{
		FVector previous = FVector::ZeroVector;
		FVector safeLocation = FVector::ZeroVector;
		TWeakObjectPtr<ASeamlessPortalActor> exitGrace;
		TSet<TWeakObjectPtr<AActor>> ownedIgnores;
	};
	void RestoreIgnores(ACharacter* character, FTraveller& state);
	bool IsBlockedWithoutOwnedIgnores(ACharacter* character, const FTraveller& state) const;
	void UpdatePassageIgnores(ACharacter* character, FTraveller& state, float deltaTime);
	TSet<TWeakObjectPtr<ASeamlessPortalActor>> portals;
	TMap<TWeakObjectPtr<ACharacter>, FTraveller> travellers;
};

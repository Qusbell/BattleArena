#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "CharacterPoolComponent.generated.h"

class APawn;
class AController;
class USceneComponent;

USTRUCT(BlueprintType)
struct FCharacterPoolLife
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Generation = 1;
	UPROPERTY(BlueprintReadOnly) bool bDead = false;
	UPROPERTY(BlueprintReadOnly) bool bManaged = false;
	UPROPERTY() FTransform SpawnTransform = FTransform::Identity;
};

/** Reusable life state. The pawn is never lent to a different participant. */
UCLASS(ClassGroup=(Character), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UCharacterPoolComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCharacterPoolComponent();
	UPROPERTY(ReplicatedUsing=OnRep_Life, VisibleAnywhere, BlueprintReadOnly, Category="Character Pool")
	FCharacterPoolLife Life;
	bool BeginDeath(AController* Controller, float RespawnDelay);
	bool PrepareRespawn(const FTransform& Transform);
	void CapturePresentation();
	void MarkReconnectClaim();
	bool HasBeenPossessed() const { return bHasBeenPossessed; }
	bool IsResetting() const { return bResetting; }
	bool IsDead() const { return Life.bDead; }
	bool IsManaged() const { return Life.bManaged; }
	static bool IsInactive(const AActor* Actor);
	static void DetachControllerLife(AController* Controller, APawn* Pawn);
	static void ResetAIForLife(AController* Controller, APawn* Pawn);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Props) const override;

private:
	struct FPresentation
	{
		TWeakObjectPtr<UActorComponent> Component;
		TWeakObjectPtr<USceneComponent> Parent;
		FName Socket;
		FTransform Transform;
		FName CollisionProfile;
		FCollisionResponseContainer Responses;
		ECollisionChannel ObjectType = ECC_WorldDynamic;
		uint8 CollisionEnabled = 0;
		bool bTick = false, bVisible = false, bHidden = false, bActive = false, bOverlap = false;
	};
	TArray<FPresentation> Presentation;
	TWeakObjectPtr<AController> AssignedController;
	FTimerHandle RespawnTimer, ReconnectTimer, CaptureTimer;
	double RespawnDeadline = 0.;
	bool bHasBeenPossessed = false, bResetting = false, bReconnectClaim = false, bReceivedLife = false;
	int32 PresentedGeneration = 1;
	float InitialGravity = 1.f, InitialWalkSpeed = 600.f;
	float RespawnDelaySeconds = 5.f;
	bool bInitialActorTick = true, bDroneTick = true, bAIMovementTick = true, bControllerBaselineCaptured = false;
	void TryRespawn();
	void RestorePresentation(bool bEnableCollision = true);
	void StopDeathPhysics();
	void ResetGameplay();
	void ResetAnimationForLife();
	void CompleteReconnect();
	UFUNCTION() void OnRep_Life();
	UFUNCTION() void OnPawnDeath(AController* InstigatedBy, AActor* DamageCauser);
	UFUNCTION() void OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);
};

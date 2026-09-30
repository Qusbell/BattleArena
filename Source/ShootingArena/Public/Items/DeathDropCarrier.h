#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/DeathDropSettings.h"
#include "DeathDropCarrier.generated.h"

class USphereComponent;

/** Replicated physical body for a death pickup. Map pickups never use this actor. */
UCLASS()
class SHOOTINGARENA_API ADeathDropCarrier : public AActor
{
	GENERATED_BODY()

public:
	ADeathDropCarrier();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Set before FinishSpawning. Pickup initialization must precede its Blueprint BeginPlay.
	void Initialize(const FDeathDropSettings& Settings, UClass* PickupClass,
		const FDataTableRowHandle& Item, double BuffEndTime, bool bIsBuff);
	void Launch(const FVector& Impulse);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> PhysicsBody;
	UPROPERTY(ReplicatedUsing=OnRep_Pickup) TObjectPtr<AActor> PickupActor;
	UPROPERTY(ReplicatedUsing=OnRep_Pickup) FDataTableRowHandle PickupItem;
	UPROPERTY(ReplicatedUsing=OnRep_Pickup) double BuffEndServerTime = 0.0;
	UPROPERTY(ReplicatedUsing=OnRep_Pickup) bool bBuffPickup = false;
	UPROPERTY(ReplicatedUsing=OnRep_Settled) bool bSettled = false;
	UPROPERTY() TSubclassOf<AActor> PickupActorClass;

	FDeathDropSettings DropSettings;
	FVector InitialHorizontalVelocity = FVector::ZeroVector;
	double LaunchTime = 0.0;
	bool bLaunched = false;
	bool bPickupConfigured = false;
	bool bWasPickedUp = false;
	bool bHorizontalBlocked = false;

	UFUNCTION() void OnRep_Pickup();
	UFUNCTION() void OnRep_Settled();
	UFUNCTION() void OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	bool ConfigurePickup(AActor* Item, bool bUpdateMesh);
	void SpawnPickup();
	void FinishPickup();
};

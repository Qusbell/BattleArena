#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "ProjectileWorldCollisionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FProjectileResolvedImpact,
	UPrimitiveComponent*, HitComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp,
	FVector, NormalImpulse, const FHitResult&, Hit);

/** Separate environment sweep. The existing root sphere remains the player hit shape. */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UProjectileWorldCollisionComponent : public USphereComponent
{
	GENERATED_BODY()
public:
	UProjectileWorldCollisionComponent();
	/** Reset only between flight callbacks, when taking an idle pooled projectile. */
	void ResetForPooledShot();
	void EndPooledShot();
	/** Connect the projectile's existing hit/damage/explosion graph to this event. */
	UPROPERTY(BlueprintAssignable, Category="Projectile|Collision")
	FProjectileResolvedImpact OnResolvedImpact;
	/** Snapshot before damage/stop callbacks can destroy the character. Reset for each pooled shot. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Projectile|Collision")
	bool bLastImpactWasCharacter = false;
	virtual void BeginPlay() override;
	virtual void OnRegister() override;
	virtual void Activate(bool bReset = false) override;
	virtual void Deactivate() override;
	virtual void SetComponentTickEnabled(bool bEnabled) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;

private:
	friend class FProjectileSplitCollisionTest;
	struct FTravelSegment { FVector Start, End; };
	TArray<FTravelSegment> Travel;
	TWeakObjectPtr<USphereComponent> PlayerCollision;
	FVector LastPosition = FVector::ZeroVector;
	bool bResolved = false;
	bool bResolving = false;
	bool bHasLastPosition = false;
	bool HasLiveContext() const;
	bool BindPlayerCollision();
	void Unbind();
	void RecordTravel(USceneComponent* Updated, EUpdateTransformFlags Flags, ETeleportType Teleport);
	bool ResolveTravel();
	bool FindImpact(const FVector& Start, const FVector& End, FHitResult& OutHit) const;
	void ResolveImpact(const FHitResult& Hit);
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/DeveloperSettings.h"
#include "PickupContactComponent.generated.h"

class USphereComponent;

/** Shared pickup distance for map items and death drops. Changes apply to newly spawned items. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Item Pickup"))
class SHOOTINGARENA_API UPickupContactSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	/** World-space pickup sphere radius in cm. */
	UPROPERTY(Config, EditAnywhere, Category="Pickup", meta=(ClampMin="1", Units="cm"))
	float Radius = 135.f;
};

/** Buffers contacts until the owning Blueprint and its spawn caller have finished initialization. */
UCLASS(ClassGroup=(Items), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UPickupContactComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPickupContactComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
private:
	struct FContact { TWeakObjectPtr<AActor> Actor; TWeakObjectPtr<UPrimitiveComponent> Component; double Time; };
	UPROPERTY(Transient) TObjectPtr<USphereComponent> Sphere;
	TSharedPtr<FComponentBeginOverlapSignature_MCSignature> OriginalOverlap;
	TArray<FContact> Pending;
	FVector PreviousLocation = FVector::ZeroVector;
	bool bReady = false;
	bool bProcessing = false;
	void Enqueue(AActor* Other, UPrimitiveComponent* OtherComponent);
	void ProcessContacts();
	bool IsPickedUp() const;
	UFUNCTION() void OnContact(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

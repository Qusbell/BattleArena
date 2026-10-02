#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BuffPresentationComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UTextBlock;

/** Attachment settings only. Niagara and Cascade assets come from the acquired Buff DT row. */
USTRUCT(BlueprintType)
struct FBuffVisualEffects
{
	GENERATED_BODY()

	/** Socket on the character mesh; None attaches to the mesh origin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effects")
	FName AttachSocket;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effects")
	FVector RelativeOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effects")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effects")
	FVector Scale = FVector::OneVector;
};

USTRUCT()
struct FBuffPresentationState
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag Tag;

	UPROPERTY()
	double EndServerTime = 0.0;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NiagaraVFX;

	UPROPERTY()
	TObjectPtr<UParticleSystem> ParticleVFX;
};

/** Presentation-only parent of BPC_Buff; gameplay duration/damage/drop remain in Blueprint. */
UCLASS(Blueprintable, ClassGroup = (Buff), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UBuffPresentationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuffPresentationComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effects")
	FBuffVisualEffects QuadEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buff|Effects")
	FBuffVisualEffects ProtectionEffects;

	/** Reads currentBuffRowName from the existing grant path and publishes that DT row's VFX and deadline. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff|Presentation")
	void SetBuffPresentation(FGameplayTag BuffTag, double EndServerTime);

	/** Called by the existing buff removal/death path. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff|Presentation")
	void ClearBuffPresentation();

	UFUNCTION(BlueprintPure, Category = "Buff|Presentation")
	double GetPresentationRemainingTime() const;

	/** Safe for missing/unpossessed pawns. Uses the replicated server deadline, not a HUD timer. */
	UFUNCTION(BlueprintCallable, Category = "Buff|Presentation")
	static void UpdateBuffRemainingTimeText(AActor* BuffOwner, UTextBlock* TimeText);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Presentation)
	FBuffPresentationState Presentation;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(Transient)
	TObjectPtr<UParticleSystemComponent> CascadeComponent;

	FGameplayTag PresentedTag;
	TWeakObjectPtr<UActorComponent> LifeComponent;

	UFUNCTION()
	void OnOwnerDeath(AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_Presentation();

	void StopEffects();
	const FBuffVisualEffects* GetEffects() const;
};

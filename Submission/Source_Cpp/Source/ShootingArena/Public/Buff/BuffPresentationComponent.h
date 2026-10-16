#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "BuffPresentationComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USoundAttenuation;
class USoundBase;
class UTextBlock;
class UDataTable;
class UAudioComponent;

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

	UPROPERTY()
	FName RowName;

	UPROPERTY()
	uint32 Revision = 0;

	UPROPERTY()
	int32 WarningStartSecond = 6;

	UPROPERTY()
	TObjectPtr<USoundBase> WarningSound;

	UPROPERTY()
	bool bWarningLocalEnabled = false;

	UPROPERTY()
	bool bWarningWorldEnabled = false;

	UPROPERTY()
	float WarningLocalVolume = 1.0f;

	UPROPERTY()
	float WarningWorldVolume = 1.0f;

	UPROPERTY()
	TObjectPtr<USoundAttenuation> WarningAttenuation;
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

	/** Hard reference assigned in BPC_Buff defaults so pickup never loads the table synchronously. */
	UPROPERTY(EditDefaultsOnly, Category = "Buff|Presentation")
	TObjectPtr<UDataTable> BuffDataTable;

	/** Reads currentBuffRowName from the existing grant path and publishes that DT row's VFX and deadline. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff|Presentation")
	void SetBuffPresentation(FGameplayTag BuffTag, double EndServerTime);

	/** Called by the existing buff removal/death path. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff|Presentation")
	void ClearBuffPresentation();

	/** A/B-tests Buff DT pickup audio. Console: Buff.PickupSound.Mode 0=owner 2D, 1=world 3D. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff|Audio")
	static bool TryPlayBuffPickupSound(AActor* PickupItem, AActor* BuffOwner, USoundBase* Sound, float Volume);

	/** Processes the current warning interval; repeated calls never replay a consumed interval. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Buff|Audio")
	void PlayBuffWarningSoundFromDataTable();

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

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> DefaultPickup3DAttenuation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> DefaultWarning3DAttenuation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> WarningLocalAudio;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> WarningWorldAudio;

	uint32 ObservedWarningRevision = MAX_uint32;
	int32 NextWarningSecond = 6;
	int32 LastWarningPlayedSecond = 0;
	double LastWarningPlayedTime = -1.0;

	FGameplayTag PresentedTag;
	TWeakObjectPtr<UActorComponent> LifeComponent;

	UFUNCTION()
	void OnOwnerDeath(AController* InstigatedBy, AActor* DamageCauser);

	UFUNCTION()
	void OnRep_Presentation();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayBuffPickupSound(USoundBase* Sound, float Volume, FVector Location);

	UFUNCTION(Client, Reliable)
	void ClientPlayBuffPickupSound(USoundBase* Sound, float Volume);

	void StopEffects();
	void UpdateWarningPlayback();
	int32 ConsumeWarningSecond(double Remaining);
	int32 GetWarningOutputMask() const;
	void StopWarningSounds();
	static void StopWarningAudio(TObjectPtr<UAudioComponent>& Audio);
	void PlayBuffPickupSound2D(USoundBase* Sound, float Volume);
	void PlayBuffPickupSound3D(USoundBase* Sound, float Volume, const FVector& Location);
	void PlayBuffWarningSound2D(USoundBase* Sound, float Volume);
	void PlayBuffWarningSound3D(USoundBase* Sound, float Volume, USoundAttenuation* Attenuation);
	const FBuffVisualEffects* GetEffects() const;

	friend class FBuffWarningSoundTest;
};

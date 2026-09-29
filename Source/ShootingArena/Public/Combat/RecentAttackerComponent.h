#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RecentAttackerComponent.generated.h"

class AController;
class APlayerState;
class UKillStreakSettingsDataAsset;
class UDamageType;

UENUM(BlueprintType)
enum class EKillDeathCause : uint8
{
	Player,
	Suicide,
	Environment
};

UENUM(BlueprintType)
enum class EKillCreditSource : uint8
{
	None,
	DirectPlayerKill,
	RecentAttacker
};

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FKillCreditResolution
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Kill Credit")
	TObjectPtr<APlayerState> VictimPlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Kill Credit")
	TObjectPtr<APlayerState> CreditedPlayerState = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Kill Credit")
	EKillDeathCause DeathCause = EKillDeathCause::Environment;

	UPROPERTY(BlueprintReadOnly, Category = "Kill Credit")
	EKillCreditSource CreditSource = EKillCreditSource::None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnKillCreditResolved,
	const FKillCreditResolution&,
	Resolution);

/** Tracks the latest valid player attacker and resolves a victim's death credit. */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API URecentAttackerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URecentAttackerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Credit")
	TObjectPtr<UKillStreakSettingsDataAsset> Settings = nullptr;

	/** Fired once on authority when this owner's death credit is resolved. */
	UPROPERTY(BlueprintAssignable, Category = "Kill Credit")
	FOnKillCreditResolved OnKillCreditResolved;

	/** Resolve this owner's death. The result event is broadcast once per component lifetime. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Kill Credit")
	FKillCreditResolution ResolveDeathCredit(AController* InstigatedBy, AActor* DamageCauser);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleOwnerTakeAnyDamage(
		AActor* DamagedActor,
		float Damage,
		const UDamageType* DamageType,
		AController* InstigatedBy,
		AActor* DamageCauser);

	APlayerState* ResolvePlayerState(AController* InstigatedBy, AActor* DamageCauser) const;
	bool IsSelfCaused(AController* InstigatedBy, AActor* DamageCauser) const;
	APlayerState* GetVictimPlayerState() const;
	float GetAttackerMemoryTime() const;

	TWeakObjectPtr<APlayerState> LastAttackerPlayerState;
	float LastAttackerTime = 0.0f;
	bool bHasLastAttackerTime = false;
	bool bDeathCreditResolved = false;
};

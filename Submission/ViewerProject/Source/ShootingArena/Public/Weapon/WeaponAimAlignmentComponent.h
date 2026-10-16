#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "Engine/NetSerialization.h"
#include "WeaponAimAlignmentComponent.generated.h"

class APawn;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;

/** Procedural TP alignment settings. Does not replace or edit source animation sequences. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UWeaponAimAlignmentDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alignment")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alignment", meta=(ClampMin="0", ClampMax="1"))
	float Strength = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alignment", meta=(ClampMin="0"))
	float InterpSpeed = 15.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alignment", meta=(ClampMin="0", ClampMax="89", Units="deg"))
	float MaxCorrectionAngle = 55.f;
	/** Limits only the animation aim; the actual projectile target is unchanged. Disable for the old pose resolver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Near Target")
	bool bStabilizeNearTargets = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Near Target", meta=(ClampMin="1", Units="cm"))
	float MinimumPoseForwardDistance = 200.f;
	/** Maximum deviation of the visual aim from the shooter's view direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Near Target", meta=(ClampMin="1", ClampMax="89", Units="deg"))
	float MaximumPoseViewDeviation = 15.f;
	/** Applied by the duplicated ABP's spine-angle clamp; arms cover the remaining aim. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alignment", meta=(ClampMin="0", ClampMax="89", Units="deg"))
	float MaximumSpineAimAngle = 45.f;
	/** Solve elevation by rotating the chest subtree about spine_02 before the arms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spine")
	bool bUseSpinePitch = false;
	/** Residual arm elevation after the chest rotation; yaw still uses MaxCorrectionAngle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spine", meta=(ClampMin="0", ClampMax="89", Units="deg"))
	float MaximumArmPitchCorrection = 89.f;
	/** Local barrel axis correction. Default socket +X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FRotator MuzzleDirectionOffset = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
	FName MuzzleSocket = TEXT("Muzzle");
	/** Optional weapon socket for the support hand. None preserves the incoming pose's grip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	FName LeftGripSocket = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	FTransform LeftGripOffset = FTransform::Identity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	bool bAlignLeftHand = true;
	/** Downward aim blends the free arm toward the duplicate ABP's authored neutral pose. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	bool bKeepFreeHandOnLookDown = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands", meta=(ClampMin="1", ClampMax="89", Units="deg"))
	float FreeHandDownBlendAngle = 20.f;
	/** Transport the authored elbow bend with the arm direction, rather than gun roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	bool bStabilizeSupportArm = false;
	/** Share axial wrist rotation with the forearm while retaining the hand's grip orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands", meta=(ClampMin="0", ClampMax="1"))
	float SupportForearmTwistShare = 0.65f;
	/** Keep the incoming pose's elbow bend plane, with these optional mesh-space biases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	FVector RightElbowOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hands")
	FVector LeftElbowOffset = FVector::ZeroVector;
	/** Equipment/reload montages may opt out; firing montages are not suppressed by default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Montages")
	TArray<TObjectPtr<UAnimMontage>> SuppressedMontages;
};

USTRUCT()
struct FWeaponAimAlignmentTarget
{
	GENERATED_BODY()
	UPROPERTY()
	FVector_NetQuantize10 Location = FVector::ZeroVector;
	UPROPERTY()
	FVector_NetQuantizeNormal ViewForward = FVector::ForwardVector;
	UPROPERTY()
	bool bValid = false;
};

/** Attached only to weapons opting into the duplicated TP animation blueprint. */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UWeaponAimAlignmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWeaponAimAlignmentComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Aim Alignment")
	TObjectPtr<UWeaponAimAlignmentDataAsset> Settings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Aim Alignment")
	bool bEnabled = true;
	/** Network target updates are throttled; pose correction itself evaluates every animation frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Aim Alignment", meta=(ClampMin="1", ClampMax="60"))
	float TargetReplicationRate = 20.f;
	/** Server muzzle sockets need evaluated TP bones even when no server viewport renders them. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon Aim Alignment")
	bool bRefreshAuthorityPose = true;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	APawn* GetShooter() const;
	USkeletalMeshComponent* GetThirdPersonWeaponMesh() const;
	bool GetAlignmentTarget(FVector& OutTarget) const;
	bool GetAlignmentAim(FVector& OutTarget, FVector& OutViewForward) const;
	/** Pure geometry, usable during worker evaluation. Never traces or changes the shot. */
	UFUNCTION(BlueprintPure, Category="Weapon Aim Alignment")
	static FVector ResolvePoseAimTarget(FVector MuzzleLocation, FVector FlightTarget, FVector ViewForward,
		float MinimumForwardDistance, float MaximumViewDeviation, bool& bLimited);
	UFUNCTION(BlueprintPure, Category="Weapon Aim Alignment")
	static float GetSpineAimLimit(UAnimInstance* AnimInstance);
	UFUNCTION(BlueprintPure, Category="Weapon Aim Alignment")
	static float ClampSpineAimAngle(UAnimInstance* AnimInstance, float Angle);
	UFUNCTION(BlueprintPure, Category="Weapon Aim Alignment")
	static float GetFreeHandPoseWeight(UAnimInstance* AnimInstance);
	/** Called on the animation game thread. Never invokes Blueprint functions on a worker thread. */
	bool RefreshLocalTarget();
private:
	UPROPERTY(Replicated)
	FWeaponAimAlignmentTarget ReplicatedTarget;
	FVector LocalTarget = FVector::ZeroVector;
	FVector LocalViewForward = FVector::ForwardVector;
	bool bLocalTargetValid = false;
	float PublishElapsed = 0.f;
	TWeakObjectPtr<APawn> TickShooter;
	TWeakObjectPtr<USkeletalMeshComponent> ForcedPoseMesh;
	uint8 PreviousPoseTickOption = 0;
	bool bPreviousUpdateRateOptimizations = false;
	void RestorePoseTick();
};

#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_WeaponAimAlignment.generated.h"

class UWeaponAimAlignmentComponent;

/** Resolves both arms from the incoming (unmodified) pose, avoiding socket-feedback accumulation. */
USTRUCT(BlueprintInternalUseOnly)
struct SHOOTINGARENA_API FAnimNode_WeaponAimAlignment : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()
public:
	FAnimNode_WeaponAimAlignment();
	UPROPERTY(EditAnywhere, Category="Bones")
	FBoneReference RightHand;
	UPROPERTY(EditAnywhere, Category="Bones")
	FBoneReference LeftHand;
	UPROPERTY(EditAnywhere, Category="Bones")
	FBoneReference Spine;
	virtual bool HasPreUpdate() const override { return true; }
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
protected:
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
private:
	FCompactPoseBoneIndex RightLower = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex RightUpper = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex LeftLower = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex LeftUpper = FCompactPoseBoneIndex(INDEX_NONE);
	FCompactPoseBoneIndex SpineIndex = FCompactPoseBoneIndex(INDEX_NONE);
	TWeakObjectPtr<UWeaponAimAlignmentComponent> Source;
	bool bSnapshotValid = false;
	bool bResetSmoothing = true;
	bool bHasLeftGrip = false;
	bool bAlignLeft = true;
	bool bUseSpinePitch = false;
	bool bStabilizeSupportArm = false;
	float SpinePitchLimit = 45.f;
	float ArmPitchLimit = 89.f;
	float SupportForearmTwistShare = 0.65f;
	FVector ShooterForwardWorld = FVector::ForwardVector;
	FVector TargetWorld = FVector::ZeroVector;
	FVector ViewForwardWorld = FVector::ForwardVector;
	bool bStabilizeNearTargets = true;
	float MinimumPoseForwardDistance = 200.f;
	float MaximumPoseViewDeviation = 15.f;
	FTransform MuzzleInHand = FTransform::Identity;
	FTransform LeftGripInHand = FTransform::Identity;
	FVector MuzzleAxis = FVector::ForwardVector;
	FVector RightElbowBias = FVector::ZeroVector;
	FVector LeftElbowBias = FVector::ZeroVector;
	float DesiredStrength = 0.f;
	float SmoothedStrength = 0.f;
	float SmoothingSpeed = 15.f;
	float MaxAngle = 55.f;
	float DeltaSeconds = 0.f;
	FQuat SmoothedCorrection = FQuat::Identity;
	float SmoothedSpinePitch = 0.f;
};

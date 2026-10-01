#include "Weapon/AnimNode_WeaponAimAlignment.h"

#include "Weapon/WeaponAimAlignmentComponent.h"
#include "Weapon/WeaponAimAlignmentMath.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "TwoBoneIK.h"

namespace
{
	FQuat LimitCorrection(FQuat Rotation, float Degrees)
	{
		Rotation.Normalize();
		if (Rotation.W < 0.0) Rotation = Rotation * -1.0;
		const double Angle = Rotation.GetAngle();
		const double Limit = FMath::DegreesToRadians(FMath::Clamp(Degrees, 0.f, 89.f));
		return Angle > Limit && Angle > UE_SMALL_NUMBER
			? FQuat::Slerp(FQuat::Identity, Rotation, Limit / Angle).GetNormalized() : Rotation;
	}

	void SolveArm(const FTransform& InputUpper, const FTransform& InputLower, const FTransform& InputHand, FCompactPoseBoneIndex UpperIndex,
		FCompactPoseBoneIndex LowerIndex, FCompactPoseBoneIndex HandIndex,
		const FTransform& DesiredHand, const FVector& ElbowBias, const FVector& Pivot,
		const FQuat& Correction, bool bStableElbow, float TwistShare, TArray<FBoneTransform>& Result, FTransform& OutHand)
	{
		FTransform Upper = InputUpper;
		FTransform Lower = InputLower;
		FTransform Hand = InputHand;
		const FVector OriginalElbow = Lower.GetLocation();
		const FVector RotatedElbow = Pivot + Correction.RotateVector(OriginalElbow - Pivot);
		// Preserve the authored bend plane instead of imposing one elbow direction at every pitch.
		const FVector JointTarget = bStableElbow
			? WeaponAimAlignmentMath::StableJointTarget(Upper.GetLocation(), Lower.GetLocation(), Hand.GetLocation(), DesiredHand.GetLocation(), ElbowBias)
			: Upper.GetLocation() + (RotatedElbow - Upper.GetLocation()) * 2.0 + ElbowBias;
		AnimationCore::SolveTwoBoneIK(Upper, Lower, Hand, JointTarget, DesiredHand.GetLocation(), false, 1.0, 1.0);
		if (bStableElbow)
		{
			Lower.SetRotation(WeaponAimAlignmentMath::ShareForearmTwist(InputLower, InputHand,
				Lower, Hand, DesiredHand.GetRotation(), TwistShare));
		}
		Hand.SetRotation(DesiredHand.GetRotation());
		Hand.NormalizeRotation();
		Result.Emplace(UpperIndex, Upper);
		Result.Emplace(LowerIndex, Lower);
		Result.Emplace(HandIndex, Hand);
		OutHand = Hand;
	}
}

FAnimNode_WeaponAimAlignment::FAnimNode_WeaponAimAlignment()
{
	RightHand.BoneName = TEXT("hand_r");
	LeftHand.BoneName = TEXT("hand_l");
	Spine.BoneName = TEXT("spine_02");
}

void FAnimNode_WeaponAimAlignment::PreUpdate(const UAnimInstance* AnimInstance)
{
	bSnapshotValid = false;
	DesiredStrength = 0.f;
	USkeletalMeshComponent* Body = AnimInstance ? AnimInstance->GetSkelMeshComponent() : nullptr;
	APawn* Pawn = Body ? Cast<APawn>(Body->GetOwner()) : nullptr;
	if (!IsValid(Body) || !IsValid(Pawn) || Body->IsSimulatingPhysics()) return;
	UWeaponAimAlignmentComponent* Alignment = Source.Get();
	auto MatchesBody = [Body, Pawn](UWeaponAimAlignmentComponent* Candidate)
	{
		USkeletalMeshComponent* WeaponMesh = Candidate ? Candidate->GetThirdPersonWeaponMesh() : nullptr;
		return IsValid(Candidate) && Candidate->GetShooter() == Pawn && IsValid(WeaponMesh)
			&& WeaponMesh->GetAttachParent() == Body && !WeaponMesh->bHiddenInGame
			&& Candidate->GetOwner() && !Candidate->GetOwner()->IsHidden();
	};
	if (!MatchesBody(Alignment))
	{
		Alignment = nullptr;
		TArray<USceneComponent*> Children;
		Body->GetChildrenComponents(false, Children);
		for (USceneComponent* Child : Children)
		{
			AActor* Weapon = IsValid(Child) ? Child->GetOwner() : nullptr;
			UWeaponAimAlignmentComponent* Candidate = Weapon ? Weapon->FindComponentByClass<UWeaponAimAlignmentComponent>() : nullptr;
			if (MatchesBody(Candidate)) { Alignment = Candidate; break; }
		}
	}
	if (Source.Get() != Alignment)
	{
		Source = Alignment;
		bResetSmoothing = true;
	}
	if (!Alignment || !IsValid(Alignment->Settings)) return;
	// Locally controlled clients preview their own aim without network latency. Observers only use replicated shooter data.
	if (!Pawn->HasAuthority() && Pawn->IsLocallyControlled()) Alignment->RefreshLocalTarget();
	const UWeaponAimAlignmentDataAsset* Settings = Alignment->Settings;
	USkeletalMeshComponent* WeaponMesh = Alignment->GetThirdPersonWeaponMesh();
	if (!Alignment->GetAlignmentAim(TargetWorld, ViewForwardWorld) || !Body->DoesSocketExist(RightHand.BoneName)
		|| !WeaponMesh->DoesSocketExist(Settings->MuzzleSocket)) return;
	const FTransform HandWorld = Body->GetSocketTransform(RightHand.BoneName);
	// The relative weapon/socket geometry cancels the last frame's hand rotation. Worker evaluation uses this frame's input pose.
	MuzzleInHand = WeaponMesh->GetSocketTransform(Settings->MuzzleSocket).GetRelativeTransform(HandWorld);
	MuzzleAxis = Settings->MuzzleDirectionOffset.Vector();
	bHasLeftGrip = !Settings->LeftGripSocket.IsNone() && WeaponMesh->DoesSocketExist(Settings->LeftGripSocket);
	if (bHasLeftGrip)
	{
		LeftGripInHand = (Settings->LeftGripOffset * WeaponMesh->GetSocketTransform(Settings->LeftGripSocket)).GetRelativeTransform(HandWorld);
	}
	bAlignLeft = Settings->bAlignLeftHand;
	bUseSpinePitch = Settings->bUseSpinePitch;
	bStabilizeSupportArm = Settings->bStabilizeSupportArm;
	SpinePitchLimit = FMath::Clamp(Settings->MaximumSpineAimAngle, 0.f, 89.f);
	ArmPitchLimit = FMath::Clamp(Settings->MaximumArmPitchCorrection, 0.f, 89.f);
	SupportForearmTwistShare = FMath::Clamp(Settings->SupportForearmTwistShare, 0.f, 1.f);
	ShooterForwardWorld = Pawn->GetActorForwardVector();
	RightElbowBias = Settings->RightElbowOffset;
	LeftElbowBias = Settings->LeftElbowOffset;
	SmoothingSpeed = FMath::Max(0.f, Settings->InterpSpeed);
	MaxAngle = FMath::Clamp(Settings->MaxCorrectionAngle, 0.f, 89.f);
	bStabilizeNearTargets = Settings->bStabilizeNearTargets;
	MinimumPoseForwardDistance = Settings->MinimumPoseForwardDistance;
	MaximumPoseViewDeviation = Settings->MaximumPoseViewDeviation;
	DesiredStrength = FMath::Clamp(Settings->Strength, 0.f, 1.f);
	for (const UAnimMontage* Montage : Settings->SuppressedMontages)
	{
		if (Montage && AnimInstance->Montage_IsActive(Montage)) { DesiredStrength = 0.f; break; }
	}
	bSnapshotValid = !MuzzleInHand.ContainsNaN() && !TargetWorld.ContainsNaN();
	if (!bSnapshotValid) DesiredStrength = 0.f;
}

void FAnimNode_WeaponAimAlignment::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
	SmoothedCorrection = FQuat::Identity;
	SmoothedStrength = 0.f;
	SmoothedSpinePitch = 0.f;
	bResetSmoothing = true;
}

void FAnimNode_WeaponAimAlignment::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DeltaSeconds = FMath::Max(0.f, Context.GetDeltaTime());
	const float Weight = SmoothingSpeed <= 0.f ? 1.f : 1.f - FMath::Exp(-SmoothingSpeed * DeltaSeconds);
	SmoothedStrength = FMath::Lerp(SmoothedStrength, DesiredStrength, Weight);
	if (bResetSmoothing) { SmoothedCorrection = FQuat::Identity; SmoothedSpinePitch = 0.f; SmoothedStrength = 0.f; bResetSmoothing = false; }
}

void FAnimNode_WeaponAimAlignment::InitializeBoneReferences(const FBoneContainer& Bones)
{
	RightHand.Initialize(Bones);
	LeftHand.Initialize(Bones);
	Spine.Initialize(Bones);
	SpineIndex = Spine.IsValidToEvaluate(Bones) ? Spine.GetCompactPoseIndex(Bones) : FCompactPoseBoneIndex(INDEX_NONE);
	auto ResolveArm = [&Bones](const FBoneReference& Hand, FCompactPoseBoneIndex& Lower, FCompactPoseBoneIndex& Upper)
	{
		Lower = Upper = FCompactPoseBoneIndex(INDEX_NONE);
		if (!Hand.IsValidToEvaluate(Bones)) return;
		Lower = Bones.GetParentBoneIndex(Hand.GetCompactPoseIndex(Bones));
		if (Lower != INDEX_NONE) Upper = Bones.GetParentBoneIndex(Lower);
	};
	ResolveArm(RightHand, RightLower, RightUpper);
	ResolveArm(LeftHand, LeftLower, LeftUpper);
}

bool FAnimNode_WeaponAimAlignment::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& Bones)
{
	return RightHand.IsValidToEvaluate(Bones) && LeftHand.IsValidToEvaluate(Bones)
		&& RightUpper != INDEX_NONE && RightLower != INDEX_NONE && LeftUpper != INDEX_NONE && LeftLower != INDEX_NONE;
}

void FAnimNode_WeaponAimAlignment::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& Result)
{
	if (!bSnapshotValid || SmoothedStrength <= UE_KINDA_SMALL_NUMBER) return;
	const FBoneContainer& Bones = Output.Pose.GetPose().GetBoneContainer();
	const FCompactPoseBoneIndex RHandIndex = RightHand.GetCompactPoseIndex(Bones);
	const FCompactPoseBoneIndex LHandIndex = LeftHand.GetCompactPoseIndex(Bones);
	const FTransform InputRHand = Output.Pose.GetComponentSpaceTransform(RHandIndex);
	const FTransform InputMuzzle = MuzzleInHand * InputRHand;
	const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	FVector Target = ComponentTransform.InverseTransformPosition(TargetWorld);
	if (bStabilizeNearTargets)
	{
		bool bLimited;
		// Distances in the data asset are world centimetres, independent of character scale.
		const FVector SafeWorldTarget = UWeaponAimAlignmentComponent::ResolvePoseAimTarget(
			ComponentTransform.TransformPosition(InputMuzzle.GetLocation()), TargetWorld,
			ViewForwardWorld, MinimumPoseForwardDistance, MaximumPoseViewDeviation, bLimited);
		Target = ComponentTransform.InverseTransformPosition(SafeWorldTarget);
	}
	const float Weight = SmoothingSpeed <= 0.f ? 1.f : 1.f - FMath::Exp(-SmoothingSpeed * DeltaSeconds);
	const FVector Up = ComponentTransform.InverseTransformVectorNoScale(FVector::UpVector).GetSafeNormal();
	FVector Forward = ComponentTransform.InverseTransformVectorNoScale(ShooterForwardWorld).GetSafeNormal();
	Forward = (Forward - Up * FVector::DotProduct(Forward, Up)).GetSafeNormal();
	const FVector PitchAxis = FVector::CrossProduct(Forward, Up).GetSafeNormal();
	FQuat ChestRotation = FQuat::Identity;
	FVector ChestPivot = FVector::ZeroVector;
	const bool bChestValid = bUseSpinePitch && SpineIndex != INDEX_NONE && !PitchAxis.IsNearlyZero();
	if (bChestValid)
	{
		ChestPivot = Output.Pose.GetComponentSpaceTransform(SpineIndex).GetLocation();
		const FVector InputBore = InputMuzzle.TransformVectorNoScale(MuzzleAxis).GetSafeNormal();
		double Pitch = 0.0;
		for (int32 Iteration = 0; Iteration < 4; ++Iteration)
		{
			const FQuat Rotation(PitchAxis, Pitch);
			const FVector Origin = ChestPivot + Rotation.RotateVector(InputMuzzle.GetLocation() - ChestPivot);
			const FVector Desired = (Target - Origin).GetSafeNormal();
			const FVector RotatedBore = Rotation.RotateVector(InputBore);
			const double CurrentElevation = FMath::Atan2(FVector::DotProduct(RotatedBore, Up), FVector::DotProduct(RotatedBore, Forward));
			const double TargetElevation = FMath::Atan2(FVector::DotProduct(Desired, Up), FVector::DotProduct(Desired, Forward));
			Pitch = FMath::Clamp(Pitch + FMath::UnwindRadians(TargetElevation - CurrentElevation),
				-FMath::DegreesToRadians(double(SpinePitchLimit)), FMath::DegreesToRadians(double(SpinePitchLimit)));
		}
		SmoothedSpinePitch = FMath::Lerp(SmoothedSpinePitch, float(Pitch), Weight);
		ChestRotation = FQuat(PitchAxis, SmoothedSpinePitch * SmoothedStrength);
	}
	auto WorkingTransform = [&](FCompactPoseBoneIndex Index)
	{
		FTransform Transform = Output.Pose.GetComponentSpaceTransform(Index);
		if (!bChestValid) return Transform;
		for (FCompactPoseBoneIndex Ancestor = Index; Ancestor != INDEX_NONE; Ancestor = Bones.GetParentBoneIndex(Ancestor))
		{
			if (Ancestor == SpineIndex) return WeaponAimAlignmentMath::RotateAround(Transform, ChestPivot, ChestRotation);
		}
		return Transform;
	};
	const FTransform RHand = WorkingTransform(RHandIndex);
	const FTransform LHand = WorkingTransform(LHandIndex);
	const FTransform Muzzle = MuzzleInHand * RHand;
	const FVector Bore = Muzzle.TransformVectorNoScale(MuzzleAxis).GetSafeNormal();
	const FVector Pivot = WorkingTransform(RightUpper).GetLocation();
	if (Bore.IsNearlyZero() || Target.Equals(Muzzle.GetLocation(), 1.0)) return;
	// A join behind the muzzle cannot define a sensible gun pose; leave the existing pose in place.
	if (!bStabilizeNearTargets && FVector::DotProduct(Target - Muzzle.GetLocation(), Bore) <= 0.0) return;
	FQuat Correction = FQuat::Identity;
	for (int32 Iteration = 0; Iteration < 4; ++Iteration)
	{
		const FVector Origin = Pivot + Correction.RotateVector(Muzzle.GetLocation() - Pivot);
		const FVector Direction = (Target - Origin).GetSafeNormal();
		if (Direction.IsNearlyZero()) return;
		Correction = LimitCorrection(FQuat::FindBetweenNormals(Correction.RotateVector(Bore), Direction) * Correction, MaxAngle);
		if (bChestValid && ArmPitchLimit < 89.f)
		{
			const FVector RotatedBore = Correction.RotateVector(Bore);
			const double Elevation = FMath::Atan2(FVector::DotProduct(Bore, Up), FVector::DotProduct(Bore, Forward));
			const double NewElevation = FMath::Atan2(FVector::DotProduct(RotatedBore, Up), FVector::DotProduct(RotatedBore, Forward));
			const double Delta = FMath::UnwindRadians(NewElevation - Elevation);
			const double Limit = FMath::DegreesToRadians(double(ArmPitchLimit));
			Correction = LimitCorrection(FQuat(PitchAxis, FMath::Clamp(Delta, -Limit, Limit) - Delta) * Correction, MaxAngle);
		}
	}
	SmoothedCorrection = FQuat::Slerp(SmoothedCorrection, Correction, Weight).GetNormalized();
	const FQuat Applied = FQuat::Slerp(FQuat::Identity, SmoothedCorrection, SmoothedStrength).GetNormalized();
	const FTransform DesiredRHand = WeaponAimAlignmentMath::RotateAround(RHand, Pivot, Applied);
	if (bChestValid) Result.Emplace(SpineIndex, WorkingTransform(SpineIndex));
	FTransform SolvedRHand;
	SolveArm(WorkingTransform(RightUpper), WorkingTransform(RightLower), RHand, RightUpper, RightLower,
		RHandIndex, DesiredRHand, RightElbowBias, Pivot, Applied, false, 0.f, Result, SolvedRHand);
	if (bAlignLeft)
	{
		// With no authored socket, preserve the support hand's relationship from the incoming animation frame.
		const FTransform Grip = bHasLeftGrip ? LeftGripInHand : LHand.GetRelativeTransform(RHand);
		const FTransform DesiredLHand = Grip * SolvedRHand;
		FTransform SolvedLHand;
		SolveArm(WorkingTransform(LeftUpper), WorkingTransform(LeftLower), LHand, LeftUpper, LeftLower,
			LHandIndex, DesiredLHand, LeftElbowBias, Pivot, Applied, bStabilizeSupportArm, SupportForearmTwistShare, Result, SolvedLHand);
	}
	Result.Sort(FCompareBoneTransformIndex());
}

void FAnimNode_WeaponAimAlignment::GatherDebugData(FNodeDebugData& DebugData)
{
	DebugData.AddDebugItem(DebugData.GetNodeName(this) + FString::Printf(TEXT(" (Alignment %.2f)"), SmoothedStrength));
	ComponentPose.GatherDebugData(DebugData);
}

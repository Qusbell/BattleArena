#include "WeaponAimAlignmentMigrationLibrary.h"

#include "AnimGraphNode_WeaponAimAlignment.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_Self.h"
#include "K2Node_CallFunction.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "Weapon/WeaponAimAlignmentComponent.h"
#include "Weapon/WeaponAimAlignmentMath.h"

namespace
{
	template<typename NodeType>
	NodeType* AddNode(UEdGraph* Graph, int32 X, int32 Y)
	{
		NodeType* Node = NewObject<NodeType>(Graph, NAME_None, RF_Transactional);
		Graph->AddNode(Node, false, false);
		Node->CreateNewGuid();
		Node->PostPlacedNewNode();
		Node->AllocateDefaultPins();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		return Node;
	}
}

bool UWeaponAimAlignmentMigrationLibrary::AddAlignmentToDuplicate(UAnimBlueprint* Blueprint)
{
	if (!Blueprint || !Blueprint->GetName().EndsWith(TEXT("_AimAligned"))) return false;
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	UEdGraph* Graph = nullptr;
	for (UEdGraph* Candidate : Graphs) if (Candidate->GetFName() == TEXT("AnimGraph")) { Graph = Candidate; break; }
	if (!Graph) return false;
	UAnimGraphNode_Root* Root = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Cast<UAnimGraphNode_WeaponAimAlignment>(Node)) return false;
		if (UAnimGraphNode_Root* Candidate = Cast<UAnimGraphNode_Root>(Node)) Root = Candidate;
	}
	UEdGraphPin* Result = Root ? Root->FindPin(TEXT("Result")) : nullptr;
	if (!Result || Result->LinkedTo.Num() != 1) return false;
	UEdGraphPin* OriginalPose = Result->LinkedTo[0];
	Blueprint->Modify();
	Graph->Modify();
	Root->Modify();
	const int32 X = Root->NodePosX;
	const int32 Y = Root->NodePosY;
	auto* ToComponent = AddNode<UAnimGraphNode_LocalToComponentSpace>(Graph, X, Y);
	auto* Alignment = AddNode<UAnimGraphNode_WeaponAimAlignment>(Graph, X + 220, Y);
	auto* ToLocal = AddNode<UAnimGraphNode_ComponentToLocalSpace>(Graph, X + 530, Y);
	Root->NodePosX = X + 750;
	Alignment->NodeComment = TEXT("Duplicated TP ABP only: incoming pose -> muzzle alignment + both-arm IK. Original stance, spine correction and locomotion remain upstream.");
	Result->BreakAllPinLinks();
	const UEdGraphSchema* Schema = Graph->GetSchema();
	const bool bConnected = Schema->TryCreateConnection(OriginalPose, ToComponent->FindPinChecked(TEXT("LocalPose")))
		&& Schema->TryCreateConnection(ToComponent->FindPinChecked(TEXT("ComponentPose")), Alignment->FindPinChecked(TEXT("ComponentPose")))
		&& Schema->TryCreateConnection(Alignment->FindPinChecked(TEXT("Pose")), ToLocal->FindPinChecked(TEXT("ComponentPose")))
		&& Schema->TryCreateConnection(ToLocal->FindPinChecked(TEXT("Pose")), Result);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return bConnected;
}

FString UWeaponAimAlignmentMigrationLibrary::AddAnimationSelfNode(UAnimBlueprint* Blueprint, int32 X, int32 Y)
{
	if (!Blueprint || !Blueprint->GetName().EndsWith(TEXT("_AimAligned"))) return FString();
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs)
	{
		if (Graph->GetFName() != TEXT("EventGraph")) continue;
		Blueprint->Modify();
		Graph->Modify();
		UK2Node_Self* Node = AddNode<UK2Node_Self>(Graph, X, Y);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		return Node->GetName();
	}
	return FString();
}

bool UWeaponAimAlignmentMigrationLibrary::AddPlasmaFreeHandLayer(UAnimBlueprint* Blueprint)
{
	if (!Blueprint || Blueprint->GetName() != TEXT("Steel_ABP_TP_BGH_PlasmaGun_AimAligned")) return false;
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	UEdGraph* Graph = nullptr;
	for (UEdGraph* Candidate : Graphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
	if (!Graph || !Blueprint->TargetSkeleton || Blueprint->TargetSkeleton->GetReferenceSkeleton().FindBoneIndex(TEXT("clavicle_l")) == INDEX_NONE) return false;
	UAnimGraphNode_Root* Root = nullptr;
	UAnimGraphNode_LayeredBoneBlend* UpperBody = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (Node->NodeComment == TEXT("Plasma free left arm: downward view restores cached locomotion.")) return false;
		if (auto* Candidate = Cast<UAnimGraphNode_Root>(Node)) Root = Candidate;
		if (Node->GetFName() == TEXT("AnimGraphNode_LayeredBoneBlend_1")) UpperBody = Cast<UAnimGraphNode_LayeredBoneBlend>(Node);
	}
	UEdGraphPin* Result = Root ? Root->FindPin(TEXT("Result")) : nullptr;
	UEdGraphPin* Base = UpperBody ? UpperBody->FindPin(TEXT("BasePose")) : nullptr;
	if (!Result || Result->LinkedTo.Num() != 1 || !Base || Base->LinkedTo.Num() != 1) return false;
	UEdGraphPin* AimedPose = Result->LinkedTo[0];
	UEdGraphPin* Locomotion = Base->LinkedTo[0];
	Blueprint->Modify();
	Graph->Modify();
	const int32 X = Root->NodePosX;
	const int32 Y = Root->NodePosY;
	auto* Save = AddNode<UAnimGraphNode_SaveCachedPose>(Graph, -800, -320);
	Save->CacheName = TEXT("PlasmaFreeHandLocomotion");
	Save->Node.CachePoseName = FName(*Save->CacheName);
	auto* UseBase = AddNode<UAnimGraphNode_UseCachedPose>(Graph, -550, -180);
	auto* UseHand = AddNode<UAnimGraphNode_UseCachedPose>(Graph, X, Y + 220);
	for (auto* Use : { UseBase, UseHand })
	{
		Use->SaveCachedPoseNode = Save;
		Use->Node.CachePoseName = FName(*Save->CacheName);
	}
	auto* Blend = AddNode<UAnimGraphNode_LayeredBoneBlend>(Graph, X + 240, Y);
	Blend->NodeComment = TEXT("Plasma free left arm: downward view restores cached locomotion.");
	Blend->Node.bMeshSpaceRotationBlend = true;
	Blend->Node.LayerSetup[0].BranchFilters.Reset();
	FBranchFilter Filter;
	Filter.BoneName = TEXT("clavicle_l");
	Filter.BlendDepth = 1;
	Blend->Node.LayerSetup[0].BranchFilters.Add(Filter);
	auto* Weight = NewObject<UK2Node_CallFunction>(Graph, NAME_None, RF_Transactional);
	Weight->SetFromFunction(UWeaponAimAlignmentComponent::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(UWeaponAimAlignmentComponent, GetFreeHandPoseWeight)));
	Graph->AddNode(Weight, false, false);
	Weight->CreateNewGuid();
	Weight->PostPlacedNewNode();
	Weight->AllocateDefaultPins();
	Weight->NodePosX = X - 80;
	Weight->NodePosY = Y + 440;
	auto* Self = AddNode<UK2Node_Self>(Graph, X - 260, Y + 510);
	Base->BreakAllPinLinks();
	Result->BreakAllPinLinks();
	Root->NodePosX = X + 650;
	const UEdGraphSchema* Schema = Graph->GetSchema();
	const bool bConnected = Schema->TryCreateConnection(Locomotion, Save->FindPinChecked(TEXT("Pose")))
		&& Schema->TryCreateConnection(UseBase->FindPinChecked(TEXT("Pose")), Base)
		&& Schema->TryCreateConnection(AimedPose, Blend->FindPinChecked(TEXT("BasePose")))
		&& Schema->TryCreateConnection(UseHand->FindPinChecked(TEXT("Pose")), Blend->FindPinChecked(TEXT("BlendPoses_0")))
		&& Schema->TryCreateConnection(Self->FindPinChecked(TEXT("self")), Weight->FindPinChecked(TEXT("AnimInstance")))
		&& Schema->TryCreateConnection(Weight->GetReturnValuePin(), Blend->FindPinChecked(TEXT("BlendWeights_0")))
		&& Schema->TryCreateConnection(Blend->FindPinChecked(TEXT("Pose")), Result);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return bConnected;
}

bool UWeaponAimAlignmentMigrationLibrary::RetargetPlasmaFreeHandStance(UAnimBlueprint* Blueprint)
{
	if (!Blueprint || Blueprint->GetName() != TEXT("Steel_ABP_TP_BGH_PlasmaGun_AimAligned")) return false;
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	UEdGraph* Graph = nullptr;
	for (UEdGraph* Candidate : Graphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
	if (!Graph) return false;
	auto Find = [Graph](const TCHAR* Name) -> UEdGraphNode*
	{
		for (UEdGraphNode* Node : Graph->Nodes) if (Node->GetFName() == Name) return Node;
		return nullptr;
	};
	UEdGraphNode* Locomotion = Find(TEXT("AnimGraphNode_StateMachine_58"));
	UEdGraphNode* Stance = Find(TEXT("AnimGraphNode_SequencePlayer_2"));
	UEdGraphNode* UseBase = Find(TEXT("AnimGraphNode_UseCachedPose_0"));
	UEdGraphNode* UpperBody = Find(TEXT("AnimGraphNode_LayeredBoneBlend_1"));
	UEdGraphNode* ToComponent = Find(TEXT("AnimGraphNode_LocalToComponentSpace_0"));
	auto* Save = Cast<UAnimGraphNode_SaveCachedPose>(Find(TEXT("AnimGraphNode_SaveCachedPose_0")));
	UEdGraphNode* FreeHand = Find(TEXT("AnimGraphNode_LayeredBoneBlend_0"));
	if (!Locomotion || !Stance || !UseBase || !UpperBody || !ToComponent || !Save || !FreeHand) return false;
	UEdGraphPin* LocomotionPose = Locomotion->FindPin(TEXT("Pose"));
	UEdGraphPin* StancePose = Stance->FindPin(TEXT("Pose"));
	UEdGraphPin* CachedPose = UseBase->FindPin(TEXT("Pose"));
	UEdGraphPin* Base = UpperBody->FindPin(TEXT("BasePose"));
	UEdGraphPin* SavedPose = Save->FindPin(TEXT("Pose"));
	UEdGraphPin* LocalPose = ToComponent->FindPin(TEXT("LocalPose"));
	if (!LocomotionPose || !StancePose || !CachedPose || !Base || !SavedPose || !LocalPose) return false;
	auto HasOnlyExpectedLinks = [](const UEdGraphPin* Pin, const UEdGraphPin* A, const UEdGraphPin* B)
	{
		for (const UEdGraphPin* Linked : Pin->LinkedTo) if (Linked != A && Linked != B) return false;
		return true;
	};
	if (!HasOnlyExpectedLinks(LocomotionPose, Base, SavedPose) || !HasOnlyExpectedLinks(StancePose, SavedPose, LocalPose)
		|| !HasOnlyExpectedLinks(CachedPose, Base, LocalPose) || !HasOnlyExpectedLinks(Base, LocomotionPose, CachedPose)
		|| !HasOnlyExpectedLinks(SavedPose, StancePose, LocomotionPose) || !HasOnlyExpectedLinks(LocalPose, StancePose, CachedPose)) return false;
	Blueprint->Modify();
	Graph->Modify();
	for (UEdGraphNode* Node : { Locomotion, Stance, UseBase, UpperBody, ToComponent, static_cast<UEdGraphNode*>(Save), FreeHand }) Node->Modify();
	// The generic connector does not handle BREAK_OTHERS_AB for pose links.
	// Explicitly clear both ends before creating each replacement connection.
	for (UEdGraphPin* Pin : { LocomotionPose, StancePose, CachedPose, Base, SavedPose, LocalPose }) Pin->BreakAllPinLinks();
	const UEdGraphSchema* Schema = Graph->GetSchema();
	const bool bConnected = Schema->TryCreateConnection(LocomotionPose, Base)
		&& Schema->TryCreateConnection(StancePose, SavedPose)
		&& Schema->TryCreateConnection(CachedPose, LocalPose);
	Save->CacheName = TEXT("PlasmaFreeHandNeutralStance");
	Save->Node.CachePoseName = FName(*Save->CacheName);
	FreeHand->NodeComment = TEXT("Plasma free left arm: downward view preserves the neutral weapon stance elbow bend.");
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return bConnected;
}

FString UWeaponAimAlignmentMigrationLibrary::InspectPitchPose(UAnimBlueprint* Blueprint)
{
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	int32 FreeHandLayers = 0;
	TArray<UEdGraph*> Graphs;
	if (Blueprint) Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (const auto* Alignment = Cast<UAnimGraphNode_WeaponAimAlignment>(Node))
			Result->SetStringField(TEXT("spine_bone"), Alignment->Node.Spine.BoneName.ToString());
		if (!Node->NodeComment.StartsWith(TEXT("Plasma free left arm:"))) continue;
		if (const auto* Blend = Cast<UAnimGraphNode_LayeredBoneBlend>(Node))
		{
			++FreeHandLayers;
			Result->SetBoolField(TEXT("mesh_space_rotation_blend"), Blend->Node.bMeshSpaceRotationBlend);
			Result->SetStringField(TEXT("free_hand_branch"), Blend->Node.LayerSetup[0].BranchFilters[0].BoneName.ToString());
		}
	}
	Result->SetNumberField(TEXT("free_hand_layers"), FreeHandLayers);
	if (Blueprint && Blueprint->TargetSkeleton)
	{
		const FReferenceSkeleton& Skeleton = Blueprint->TargetSkeleton->GetReferenceSkeleton();
		Result->SetNumberField(TEXT("spine_index"), Skeleton.FindBoneIndex(TEXT("spine_02")));
		Result->SetNumberField(TEXT("clavicle_l_index"), Skeleton.FindBoneIndex(TEXT("clavicle_l")));
	}
	FString Text;
	FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Text));
	return Text;
}

FString UWeaponAimAlignmentMigrationLibrary::VerifyPitchPoseMath()
{
	using namespace WeaponAimAlignmentMath;
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	bool bPassed = true;
	TArray<TSharedPtr<FJsonValue>> Cases;
	for (double Pitch : { -89.0, -60.0, -20.0, -10.0, 0.0, 60.0, 89.0 })
	{
		const float Weight = FreeHandWeight(FRotator(Pitch, 31.0, 0.0).Vector(), 20.f);
		const bool bValidWeight = Pitch <= -20.0 ? FMath::IsNearlyEqual(Weight, 1.f)
			: Pitch >= 0.0 ? FMath::IsNearlyZero(Weight) : FMath::IsNearlyEqual(Weight, 0.5f);
		const FVector Pivot(0.0, 0.0, 100.0);
		const FQuat Rotation(FVector(0.0, -1.0, 0.0), FMath::DegreesToRadians(FMath::Clamp(Pitch, -45.0, 45.0)));
		const FTransform Shoulder(FQuat::Identity, FVector(0.0, 20.0, 125.0));
		const FTransform Elbow(FQuat::Identity, FVector(25.0, 35.0, 115.0));
		const FTransform Hand(FQuat::Identity, FVector(45.0, 15.0, 135.0));
		const FTransform RotatedShoulder = RotateAround(Shoulder, Pivot, Rotation);
		const FTransform RotatedElbow = RotateAround(Elbow, Pivot, Rotation);
		const FTransform RotatedHand = RotateAround(Hand, Pivot, Rotation);
		const FVector DesiredHand = RotatedShoulder.GetLocation() + FRotator(Pitch, 10.0, 0.0).Vector() * 40.0;
		const FVector Pole = StableJointTarget(RotatedShoulder.GetLocation(), RotatedElbow.GetLocation(), RotatedHand.GetLocation(), DesiredHand, FVector::ZeroVector);
		const FVector ArmDirection = (DesiredHand - RotatedShoulder.GetLocation()).GetSafeNormal();
		const bool bValid = bValidWeight && !Pole.ContainsNaN()
			&& FVector::CrossProduct(ArmDirection, Pole - RotatedShoulder.GetLocation()).Size() > 1.0
			&& FMath::IsNearlyEqual(FVector::Distance(Shoulder.GetLocation(), Hand.GetLocation()), FVector::Distance(RotatedShoulder.GetLocation(), RotatedHand.GetLocation()), 0.001)
			&& RotateAround(FTransform(FQuat::Identity, Pivot), Pivot, Rotation).GetLocation().Equals(Pivot);
		bPassed &= bValid;
		TSharedRef<FJsonObject> Case = MakeShared<FJsonObject>();
		Case->SetNumberField(TEXT("view_pitch"), Pitch);
		Case->SetNumberField(TEXT("free_hand_weight"), Weight);
		Case->SetBoolField(TEXT("valid"), bValid);
		Cases.Add(MakeShared<FJsonValueObject>(Case));
	}
	const FTransform Lower(FQuat::Identity, FVector::ZeroVector);
	const FTransform Hand(FQuat::Identity, FVector(30.0, 0.0, 0.0));
	const FQuat Shared = ShareForearmTwist(Lower, Hand, Lower, Hand, FQuat(FVector::ForwardVector, FMath::DegreesToRadians(90.0)), 0.65f);
	const double SharedAngle = FMath::RadiansToDegrees(Shared.GetAngle());
	const bool bTwistValid = !Shared.ContainsNaN() && FMath::IsNearlyEqual(SharedAngle, 39.0, 0.001);
	Result->SetBoolField(TEXT("forearm_twist_valid"), bTwistValid);
	Result->SetNumberField(TEXT("shared_forearm_twist_deg"), SharedAngle);
	Result->SetArrayField(TEXT("pitch_cases"), Cases);
	Result->SetBoolField(TEXT("success"), bPassed && bTwistValid);
	Result->SetBoolField(TEXT("play_tests_run"), false);
	FString Text;
	FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Text));
	return Text;
}

bool UWeaponAimAlignmentMigrationLibrary::AddWeaponAlignmentComponent(UBlueprint* Blueprint, UWeaponAimAlignmentDataAsset* Settings)
{
	if (!Blueprint || !Blueprint->SimpleConstructionScript || !Settings) return false;
	USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
	for (USCS_Node* Node : SCS->GetAllNodes()) if (Node->ComponentClass->IsChildOf(UWeaponAimAlignmentComponent::StaticClass())) return false;
	Blueprint->Modify();
	SCS->Modify();
	USCS_Node* Node = SCS->CreateNode(UWeaponAimAlignmentComponent::StaticClass(), TEXT("WeaponAimAlignment"));
	auto* Component = Cast<UWeaponAimAlignmentComponent>(Node->ComponentTemplate);
	if (!Component) return false;
	Component->Settings = Settings;
	Component->bEnabled = true;
	SCS->AddNode(Node);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	return true;
}

bool UWeaponAimAlignmentMigrationLibrary::SetThirdPersonAnimation(UObject* WeaponData, UAnimBlueprint* Blueprint)
{
	if (!WeaponData || !Blueprint || !Blueprint->GeneratedClass || !Blueprint->GetName().EndsWith(TEXT("_AimAligned"))) return false;
	const FClassProperty* Property = FindFProperty<FClassProperty>(WeaponData->GetClass(), TEXT("tpAnim"));
	if (!Property) return false;
	WeaponData->Modify();
	Property->SetObjectPropertyValue_InContainer(WeaponData, Blueprint->GeneratedClass);
	WeaponData->PostEditChange();
	WeaponData->MarkPackageDirty();
	return true;
}

FString UWeaponAimAlignmentMigrationLibrary::InspectAlignment(UAnimBlueprint* Blueprint, UBlueprint* WeaponBlueprint, UObject* WeaponData)
{
	TSharedRef<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("animation"), GetPathNameSafe(Blueprint));
	Result->SetStringField(TEXT("settings_scope"), TEXT("RocketLauncher TP only"));
	int32 AlignmentNodes = 0;
	TArray<UEdGraph*> Graphs;
	if (Blueprint) Blueprint->GetAllGraphs(Graphs);
	for (UEdGraph* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes) if (Cast<UAnimGraphNode_WeaponAimAlignment>(Node)) ++AlignmentNodes;
	Result->SetNumberField(TEXT("alignment_nodes"), AlignmentNodes);
	const FClassProperty* AnimationProperty = WeaponData ? FindFProperty<FClassProperty>(WeaponData->GetClass(), TEXT("tpAnim")) : nullptr;
	Result->SetStringField(TEXT("tpAnim"), AnimationProperty ? GetPathNameSafe(AnimationProperty->GetObjectPropertyValue_InContainer(WeaponData)) : TEXT("None"));
	int32 AlignmentComponents = 0;
	if (WeaponBlueprint && WeaponBlueprint->SimpleConstructionScript)
	{
		for (USCS_Node* Node : WeaponBlueprint->SimpleConstructionScript->GetAllNodes())
		{
			const auto* Component = Cast<UWeaponAimAlignmentComponent>(Node->ComponentTemplate);
			if (!Component) continue;
			++AlignmentComponents;
			Result->SetStringField(TEXT("settings"), GetPathNameSafe(Component->Settings));
		}
	}
	Result->SetNumberField(TEXT("alignment_components"), AlignmentComponents);
	if (Blueprint && Blueprint->TargetSkeleton)
	{
		TArray<TSharedPtr<FJsonValue>> Bones;
		for (FName Name : { FName(TEXT("hand_r")), FName(TEXT("lowerarm_r")), FName(TEXT("upperarm_r")), FName(TEXT("hand_l")), FName(TEXT("lowerarm_l")), FName(TEXT("upperarm_l")) })
		{
			TSharedRef<FJsonObject> Bone = MakeShared<FJsonObject>();
			Bone->SetStringField(TEXT("name"), Name.ToString());
			Bone->SetNumberField(TEXT("index"), Blueprint->TargetSkeleton->GetReferenceSkeleton().FindBoneIndex(Name));
			Bones.Add(MakeShared<FJsonValueObject>(Bone));
		}
		Result->SetArrayField(TEXT("bones"), Bones);
	}
	FString Text;
	FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Text));
	return Text;
}

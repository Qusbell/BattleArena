#include "Camera/DebugCameraOcclusionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

UDebugCameraOcclusionComponent::UDebugCameraOcclusionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> FadeMaterial(
		TEXT("/Game/QuakeLike_1_0/Test/M_DebugCameraOccluder.M_DebugCameraOccluder"));
	if (FadeMaterial.Succeeded())
	{
		OcclusionMaterial = FadeMaterial.Object;
	}
}

void UDebugCameraOcclusionComponent::UpdateOcclusion(UCameraComponent* Camera, APawn* TargetPawn)
{
	if (PreviousTarget.Get() != TargetPawn)
	{
		RestoreOcclusion();
		PreviousTarget = TargetPawn;
	}

	if (!IsValid(Camera) || !IsValid(TargetPawn) || !IsValid(OcclusionMaterial))
	{
		RestoreOcclusion();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		RestoreOcclusion();
		return;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DebugCameraOcclusion), false);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(TargetPawn);

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = TargetPawn->GetActorLocation() + TargetOffset;
	TArray<UPrimitiveComponent*> CurrentOccluders;
	CurrentOccluders.Reserve(MaxOccluders);

	// Re-trace after each hit: a multi trace stops at the first blocking wall.
	for (int32 HitIndex = 0; HitIndex < MaxOccluders; ++HitIndex)
	{
		FHitResult Hit;
		if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
		{
			break;
		}

		UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (!IsValid(HitComponent))
		{
			break;
		}

		QueryParams.AddIgnoredComponent(HitComponent);
		if (Cast<UMeshComponent>(HitComponent) && HitComponent->GetNumMaterials() > 0)
		{
			CurrentOccluders.AddUnique(HitComponent);
		}
	}

	for (int32 Index = OccludedMeshes.Num() - 1; Index >= 0; --Index)
	{
		FDebugCameraOccludedMesh& Saved = OccludedMeshes[Index];
		UPrimitiveComponent* Component = Saved.Component.Get();
		if (IsValid(Component) && CurrentOccluders.Contains(Component))
		{
			continue;
		}
		if (IsValid(Component))
		{
			for (int32 Slot = 0; Slot < Saved.OriginalMaterials.Num(); ++Slot)
			{
				Component->SetMaterial(Slot, Saved.OriginalMaterials[Slot]);
			}
		}
		OccludedMeshes.RemoveAtSwap(Index);
	}

	for (UPrimitiveComponent* Component : CurrentOccluders)
	{
		if (OccludedMeshes.ContainsByPredicate([Component](const FDebugCameraOccludedMesh& Saved)
			{ return Saved.Component.Get() == Component; }))
		{
			continue;
		}

		FDebugCameraOccludedMesh& Saved = OccludedMeshes.AddDefaulted_GetRef();
		Saved.Component = Component;
		for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
		{
			Saved.OriginalMaterials.Add(Component->GetMaterial(Slot));
			Component->SetMaterial(Slot, OcclusionMaterial);
		}
	}
}

void UDebugCameraOcclusionComponent::RestoreOcclusion()
{
	for (FDebugCameraOccludedMesh& Saved : OccludedMeshes)
	{
		if (UPrimitiveComponent* Component = Saved.Component.Get(); IsValid(Component))
		{
			for (int32 Slot = 0; Slot < Saved.OriginalMaterials.Num(); ++Slot)
			{
				Component->SetMaterial(Slot, Saved.OriginalMaterials[Slot]);
			}
		}
	}
	OccludedMeshes.Reset();
	PreviousTarget.Reset();
}

void UDebugCameraOcclusionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestoreOcclusion();
	Super::EndPlay(EndPlayReason);
}

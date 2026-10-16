#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DebugCameraOcclusionComponent.generated.h"

class APawn;
class UCameraComponent;
class UMaterialInterface;
class UPrimitiveComponent;

USTRUCT()
struct FDebugCameraOccludedMesh
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> Component;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
};

/** Local-only visibility aid for BP_DebugFollowCamera. The camera drives this from its Tick. */
UCLASS(ClassGroup=(Camera), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UDebugCameraOcclusionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDebugCameraOcclusionComponent();

	/** Replaces every occluding mesh material between the camera and pawn, restoring stale hits. */
	UFUNCTION(BlueprintCallable, Category="Debug Camera")
	void UpdateOcclusion(UCameraComponent* Camera, APawn* TargetPawn);

	/** Call before disabling the camera; EndPlay also calls this. */
	UFUNCTION(BlueprintCallable, Category="Debug Camera")
	void RestoreOcclusion();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="Debug Camera")
	TObjectPtr<UMaterialInterface> OcclusionMaterial;

	UPROPERTY(EditDefaultsOnly, Category="Debug Camera")
	FVector TargetOffset = FVector(0.0, 0.0, 50.0);

	UPROPERTY(EditDefaultsOnly, Category="Debug Camera", meta=(ClampMin="1", ClampMax="64"))
	int32 MaxOccluders = 32;

private:
	UPROPERTY(Transient)
	TArray<FDebugCameraOccludedMesh> OccludedMeshes;

	TWeakObjectPtr<APawn> PreviousTarget;
};

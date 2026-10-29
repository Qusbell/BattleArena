#pragma once

#include "CoreMinimal.h"
#include "OneWayTeleportActor.h"
#include "SeamlessPortalActor.generated.h"

class ACharacter;

/** A directed spatial doorway. Reuses legacy presentation, settings and capture scheduling, not its fixed camera/launch. */
UCLASS(Blueprintable)
class SHOOTINGARENA_API ASeamlessPortalActor : public AOneWayTeleportActor
{
	GENERATED_BODY()

public:
	ASeamlessPortalActor();
	virtual bool CanDisplayPortalView(const APlayerController* controller, const FVector& location,
		const FVector& aim, float& distance, float& score, float& coverage) const override;
	virtual bool BuildPortalCaptureView(const FTransform& camera, FPortalCaptureView& outView) const override;
	virtual FTransform GetPortalViewCameraTransform(const FTransform& camera) const override;
	virtual void ApplyPortalView(UTextureRenderTarget2D* target, float blur, float opacity) override;
	virtual const UTeleportDataAsset* GetTeleportDataAsset() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outProps) const override;

	/** Copies references and numeric presentation settings; never edits the source actor or its assets. */
	UFUNCTION(CallInEditor, Category = "Spatial Portal|Setup")
	void CopyPresentationFromSource();

	/** One-shot editor move: places the opening's lower edge on a walkable floor. Never attaches or ticks. */
	UFUNCTION(CallInEditor, Category = "Spatial Portal|Setup", meta = (DisplayName = "Snap Portal To Floor"))
	void SnapPortalToFloor();

	/** Downward search distance for the editor button, in cm. No ongoing floor following is performed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Setup", meta = (ClampMin = "1.0", ClampMax = "20000.0"))
	float floorSnapSearchDistance = 2000.0f;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spatial Portal|Setup")
	TObjectPtr<AOneWayTeleportActor> presentationSource;

	UFUNCTION(BlueprintCallable, Category = "Spatial Portal")
	void SetDestination(AActor* destination);

	/** Use a dedicated, portal-sized wall actor. Only these actors may be ignored while inside the opening. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Spatial Portal|Traversal")
	TArray<TObjectPtr<AActor>> passageWalls;

	/** Also changes the default screen geometry. Equal Portal Shape Scale X/Y makes a circle; unequal makes an oval.
	 * An explicitly assigned custom mesh is always retained. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Setup", meta = (DisplayName = "Elliptical Opening"))
	bool bEllipticalOpening = true;

	/** Enter at the first front-side capsule contact with the opening, without waiting for its centre.
	 * Does not expand the opening, change the real capsule, or bypass destination collision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Traversal", meta = (DisplayName = "Allow Partial Capsule Entry"))
	bool bAllowPartialCapsuleEntry = true;

	/** Arrival only: discard the entrance's sideways offset, so both edges use the same exit centre.
	 * Keep height and momentum; arrival faces the exit yaw. Disabling restores relative sideways mapping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Traversal", meta = (DisplayName = "Center Exit Horizontally"))
	bool bCenterExitHorizontally = true;

	/** Strict mode only: allow a little head/foot overhang while the torso crosses the plane.
	 * Does not shrink the real capsule or relax destination collision. 0 restores full-height eligibility. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Traversal", meta = (DisplayName = "Head And Foot Tolerance", EditCondition = "!bAllowPartialCapsuleEntry", ClampMin = "0.0", ClampMax = "50.0", Units = "cm"))
	float headAndFootTolerance = 20.0f;

	/** Non-spatial targets (any Actor, including legacy portals) are exit anchors, not matching doorway meshes.
	 * Align their virtual exit floor to the entrance floor. Rendering and traversal use the same frame.
	 * Disable for an intentionally airborne/exact-transform exit. Spatial portal pairs keep their exact frames. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Traversal")
	bool bAlignActorExitToFloor = true;

	/** Maximum downward floor search around an ordinary exit anchor, in cm. Never searches through walls. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Traversal", meta = (ClampMin = "1.0", ClampMax = "5000.0", EditCondition = "bAlignActorExitToFloor"))
	float actorExitFloorSearchDistance = 500.0f;

	/** Position discontinuities larger than this are treated as another system's teleport, not a crossing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Traversal", meta = (ClampMin = "1.0"))
	float maximumTraversalStep = 5000.0f;

	/** Minimum camera-to-portal-plane distance used only for the spatial capture, in cm.
	 * Inside this distance, stop advancing the capture eye while retaining lateral/vertical parallax.
	 * 0 restores unrestricted perspective. Never changes entry collision or the player's camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spatial Portal|Rendering",
		meta = (DisplayName = "Near View Lock Distance", ClampMin = "0.0", UIMin = "0.0", UIMax = "500.0", Units = "cm"))
	float nearViewLockDistance = 100.0f;

	bool CanTraverse() const;
	bool GetDestinationFrame(FTransform& frame) const;
	FVector2D GetOpeningHalfSize() const;
	bool FindCrossing(const ACharacter* character, const FVector& previous, double& fraction) const;
	bool TryTraverse(ACharacter* character, const FVector& previous, FVector& outSafeExit) const;
	bool FitsAt(const ACharacter* character, const FVector& position) const;
	double GetCapsuleNormalExtent(const ACharacter* character) const;
	double GetPassageFrontExtent() const;

protected:
	virtual void OnConstruction(const FTransform& transform) override;
	virtual void PostRegisterAllComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;
	virtual void OnEntryBeginOverlap(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32,
		bool, const FHitResult&) override;

private:
	/** Shared project settings when this portal has no explicitly assigned data asset. */
	UPROPERTY()
	TObjectPtr<UTeleportDataAsset> defaultPortalInfo;

	void AlignSpatialScreen();
	void UpdateVFXFrontOffset();
	/** Applies a small screen separation without overwriting the designer's VFX local transform. */
	UPROPERTY(VisibleAnywhere, Category = "Teleport|Components")
	TObjectPtr<USceneComponent> vfxFrontOffsetRoot;
	void UpdateSpatialOpening();
	UPROPERTY(Transient, DuplicateTransient)
	TObjectPtr<UStaticMesh> generatedEllipseMesh;
	void AlignActorExitFrame(FTransform& frame) const;
	// Reuse the two floor queries across characters/capture candidates; no extra per-frame capture work.
	mutable TWeakObjectPtr<AActor> cachedActorExit;
	mutable FTransform cachedActorExitRawFrame = FTransform::Identity;
	mutable FTransform cachedActorExitEntryFrame = FTransform::Identity;
	mutable FTransform cachedActorExitFrame = FTransform::Identity;
	mutable FVector2D cachedActorExitHalfSize = FVector2D::ZeroVector;
	mutable float cachedActorExitSearchDistance = 0;
	mutable double actorExitCacheTime = -DBL_MAX;
	UPROPERTY(Transient)
	FTransform screenRestRelative = FTransform::Identity;
	UPROPERTY(ReplicatedUsing = OnRep_Destination)
	TObjectPtr<AActor> replicatedDestination;
	UFUNCTION()
	void OnRep_Destination();
};

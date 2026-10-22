#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "ProjectileFirstPersonRenderingComponent.generated.h"

class UCameraComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class APlayerController;
class APawn;
class FSceneViewExtensionBase;
struct FMinimalViewInfo;

/** One per pawn; owns camera/arm rendering state so weapon switches cannot accumulate overrides. */
UCLASS(ClassGroup=(Weapon), meta=(BlueprintSpawnableComponent))
class SHOOTINGARENA_API UProjectileFirstPersonRenderingComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UProjectileFirstPersonRenderingComponent();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering", meta=(ClampMin="0.001", ClampMax="1"))
	float FirstPersonScale = 0.25f;
	/** Reduce the configured maximum scale for long weapons so their rendered bound fits inside the pawn capsule. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering")
	bool bFitInsideCollisionCapsule = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering", meta=(ClampMin="0", Units="cm"))
	float CapsuleSafetyMargin = 2.f;
	/** Applied only to the local active FP view; does not change camera position or FOV. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering", meta=(ClampMin="0.01", Units="cm"))
	float FirstPersonNearClipPlane = 0.5f;
	/** Prepare only the local pawn's owned FP weapon textures, with bounded residency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering")
	bool bPrestreamOwnedWeaponTextures = true;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;
	virtual void Deactivate() override;
	/** Game-thread view hook: apply current-frame scale and near clip to this local FP view only. */
	bool ApplyFirstPersonView(const APlayerController* Controller, FMinimalViewInfo& View) const;
	/** Only when world geometry occludes the real muzzle, convert its cosmetic
	 * origin to the rendered FP position. An unobstructed muzzle stays unchanged.
	 */
	static bool TryTransformEffectPosition(const AActor* Weapon, FVector Position, FVector& OutPosition);
	/** Read the active local view and physical FP socket only when world geometry occludes it. */
	static bool TryGetOccludedEffectView(const AActor* Weapon, FMinimalViewInfo& OutView, FVector& OutMuzzle);
	/** Narrow camera-to-muzzle visibility check; ignores pawns, projectiles and overlaps. */
	static bool IsEffectMuzzleOccluded(UWorld* World, FVector Eye, FVector Muzzle,
		const AActor* Weapon, const AActor* Shooter);
	/** Called immediately after the existing Cascade/Niagara spawn. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Projectile")
	static void ApplyFirstPersonMuzzleEffect(AActor* Weapon, UPrimitiveComponent* Effect);
private:
	bool CanUpdateRendering() const;
	float CalculateRenderScale(const APawn* Pawn, const USkeletalMeshComponent* Arms,
		const UPrimitiveComponent* Gun, const FMinimalViewInfo& View) const;
	void Track(UPrimitiveComponent* Primitive);
	void Restore();
	void RestoreView();
	void PrepareWeapons(USkeletalMeshComponent* Arms);
	TWeakObjectPtr<UCameraComponent> Camera;
	TWeakObjectPtr<AActor> ActiveWeapon;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, EFirstPersonPrimitiveType> PreviousTypes;
	TMap<TWeakObjectPtr<UPrimitiveComponent>, EFirstPersonPrimitiveType> PreparedGunTypes;
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TWeakObjectPtr<USkeletalMesh>> PreparedMeshes;
	float NextTextureRefreshTime = 0.f;
	float PreviousScale = 1.f;
	bool bPreviousEnableScale = false;
	float ActiveRenderScale = 1.f;
	bool bAcceptMuzzleEffects = false;
	TSharedPtr<FSceneViewExtensionBase, ESPMode::ThreadSafe> ViewExtension;
};

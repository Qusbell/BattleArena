#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "ProjectileFirstPersonRenderingComponent.generated.h"

class UCameraComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

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
	/** Prepare only the local pawn's owned FP weapon textures, with bounded residency. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="First Person Rendering")
	bool bPrestreamOwnedWeaponTextures = true;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** Called immediately after the existing Cascade/Niagara spawn. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Projectile")
	static void ApplyFirstPersonMuzzleEffect(AActor* Weapon, UPrimitiveComponent* Effect);
private:
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
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OneWayTeleportActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTeleportDataAsset;
class UTextureRenderTarget2D;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMesh;
class ACharacter;
class APlayerController;

/**
 * 기획서 기준 단방향 텔레포트 Actor입니다.
 *
 * - EntryCollision에 Character가 진입하면 발동
 * - 지정된 모든 Actor를 exitTarget으로 사용할 수 있음
 * - exitTarget 위치로 즉시 이동
 * - exitTarget Yaw를 출구 정면으로 사용
 * - DataAsset의 launchAngle / launchPower로 Launch
 * - DataAsset의 moveLockTime 동안 입력 제한
 * - PortalVisual로 충돌 영역을 반투명 표시
 * - EditorSelectionHandle로 에디터에서 쉽게 선택 가능
 */
UCLASS(Blueprintable)
class SHOOTINGARENA_API AOneWayTeleportActor : public AActor
{
	GENERATED_BODY()

public:
	AOneWayTeleportActor();

	/** PortalViewSubsystem이 로컬 플레이어의 주시 대상인지 판정할 때 사용합니다. */
	bool CanDisplayPortalView(
		const APlayerController* playerController,
		const FVector& cameraLocation,
		const FVector& aimDirection,
		float& outDistance,
		float& outScore,
		float& outScreenCoverage) const;

	/** 입구 카메라 시점을 출구 Launch 방향 기준의 카메라 시점으로 변환합니다. */
	FTransform GetPortalViewCameraTransform(const FTransform& cameraTransform) const;

	/** 로컬 클라이언트의 Render Target과 거리 기반 화면 값을 포탈 화면에 적용합니다. */
	void ApplyPortalView(UTextureRenderTarget2D* renderTarget, float blurStrength, float screenOpacity);
	void ClearPortalView();
	float GetPortalViewOpacity(float distance) const;

	AActor* GetExitTarget() const { return exitTarget.Get(); }
	const UTeleportDataAsset* GetTeleportDataAsset() const { return teleportDA.Get(); }
	FVector GetExitLaunchForward() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// ---------------------------------------------------------------------
	// Components
	// ---------------------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<USceneComponent> root;

	/** 실제 텔레포트 진입 판정용 Collision입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<UBoxComponent> entryCollision;

	/**
	 * EntryCollision 영역을 보여주는 반투명 Cube입니다.
	 * Collision / Navigation에는 영향을 주지 않습니다.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<UStaticMeshComponent> portalVisual;

	/** 출구 화면을 표시하는 입구 전면의 평면입니다. Collision에는 영향을 주지 않습니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<UStaticMeshComponent> portalScreen;

	/** 포탈 전면에 붙는 Niagara 테두리/에너지 효과입니다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Components")
	TObjectPtr<UNiagaraComponent> portalVFX;

#if WITH_EDITORONLY_DATA
	/**
	 * 에디터에서 Portal Actor를 쉽게 클릭하기 위한 선택용 구체입니다.
	 * 실제 게임/패키징에는 포함되지 않습니다.
	 */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> editorSelectionHandle;
#endif

	// ---------------------------------------------------------------------
	// Teleport Settings
	// ---------------------------------------------------------------------

	/**
	 * 레벨에 배치된 임의의 Actor를 출구로 지정합니다.
	 * TargetPoint뿐 아니라 다른 Portal Actor, Marker Actor 등도 사용할 수 있습니다.
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<AActor> exitTarget;

	/** BP 자식 Class Defaults에서 한 번 지정합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	TObjectPtr<UTeleportDataAsset> teleportDA;

	/** false이면 진입해도 텔레포트하지 않습니다. 런타임에는 서버에서 변경하세요. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	bool bTeleportEnabled = true;

	/** false이면 이 포탈만 출구 화면을 표시하지 않습니다. 텔레포트 이동에는 영향을 주지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teleport")
	bool bEnablePortalView = true;

	/**
	 * 텔레포트 직후 다른 Portal의 Collision에 겹쳐도 다시 이동하지 않는 보호 시간입니다.
	 * 두 Portal을 서로의 출구로 지정한 경우의 무한 왕복을 방지합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float reentryLockDuration = 0.25f;

	// ---------------------------------------------------------------------
	// Visual Settings
	// ---------------------------------------------------------------------

	/**
	 * 체크: 에디터 + 실제 인게임에서 PortalVisual 표시
	 * 체크 해제: 에디터에서만 표시하고 실제 인게임에서는 숨김
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual")
	bool bShowPortalInGame = true;

	/** PortalVisual의 표시 색상입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual")
	FLinearColor portalColor = FLinearColor(0.0f, 0.5f, 1.0f, 1.0f);

	/** 0 = 완전 투명, 1 = 완전 불투명 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalOpacity = 0.25f;

	/**
	 * Color(Vector) / Opacity(Scalar) 파라미터를 가진 Translucent Material을 지정합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport|Visual")
	TObjectPtr<UMaterialInterface> portalVisualMaterial;

	/**
	 * PortalTexture(Texture), BlurStrength(Scalar), PortalOpacity(Scalar),
	 * FrameColor(Vector), FrameGlowIntensity(Scalar), FrameThickness(Scalar) 파라미터를 가진 포탈 화면용 Material입니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport|Visual")
	TObjectPtr<UMaterialInterface> portalScreenMaterial;

	/** 포탈 화면 및 VFX 크기의 기준이 되는 Mesh입니다. 비워두면 기존 PortalScreen Mesh를 그대로 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual")
	TObjectPtr<UStaticMesh> portalShapeMesh;

	/** true이면 portalShapeMesh의 가로·세로 Bounds를 읽어 EntryCollision 크기를 자동으로 맞춥니다. 위치·회전은 건드리지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual")
	bool bAutoFitCollisionToPortalMesh = true;

	/** 포탈 Mesh의 가로(X)·세로(Y) 배율입니다. PortalScreen, Collision, VFX가 함께 같은 비율로 커집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual",
		meta = (ClampMin = "0.01", UIMin = "0.01"))
	FVector2D portalShapeScale = FVector2D(1.0f, 1.0f);

	/** 자동 맞춤 Collision의 앞뒤 전체 두께입니다. 포탈 Mesh가 평면이어도 안정적으로 Overlap되도록 별도로 지정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Visual",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float portalCollisionDepth = 100.0f;

	/** false이면 화면 머티리얼의 사각 테두리 발광을 끕니다. 원형/타원형 포탈에는 Niagara VFX 사용을 권장합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX")
	bool bEnableMaterialFrame = false;

	/** 이 포탈에서 Niagara 테두리/에너지 효과를 표시할지 결정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX")
	bool bEnablePortalVFX = true;

	/**
	 * AdvancedPortalsSystemVFX의 NS_Portal_Vortex_1~10 등 원하는 Niagara System을 지정합니다.
	 * 완성형 BP_Portal 프리셋은 자체 화면 Mesh를 포함하므로 여기에는 직접 지정하지 않습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX")
	TObjectPtr<UNiagaraSystem> portalVFXSystem;

	/** 자동 맞춤 이후 이펙트별 여백을 보정하는 배율입니다. 기본값 (1,1,1)을 기준으로 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "VFX Scale", ClampMin = "0.01", UIMin = "0.01"))
	FVector portalVFXScale = FVector(1.0f, 1.0f, 1.0f);

	/** Niagara User.Background Alpha 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Background Alpha", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXBackgroundAlpha = 0.0f;

	/** Niagara User.Background Control 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Background Control", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXBackgroundControl = 0.0f;

	/** Niagara User.Vortex Alpha 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Vortex Alpha", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXVortexAlpha = 0.0f;

	/** Niagara User.Circle Alpha 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Circle Alpha", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXCircleAlpha = 0.0f;

	/** Niagara User.Distortion Control 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Distortion Control", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXDistortionControl = 0.0f;

	/** Niagara User.Ring Alpha 값입니다. Ring 이미터가 있는 Vortex에서 테두리 투명도를 제어합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Ring Alpha", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXRingAlpha = 1.0f;

	/** Niagara User.Energy Alpha 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal VFX",
		meta = (DisplayName = "Energy Alpha", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalVFXEnergyAlpha = 1.0f;

	/** 포탈 화면 가장자리 띠의 발광 색입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Appearance")
	FLinearColor portalFrameColor = FLinearColor(0.0f, 0.65f, 1.0f, 1.0f);

	/** 포탈 화면 가장자리 띠의 Emissive 세기입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Appearance",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float portalFrameGlowIntensity = 12.0f;

	/** 화면 UV 기준 테두리 띠의 두께입니다. 0.025는 각 가장자리 약 2.5%입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Appearance",
		meta = (ClampMin = "0.001", ClampMax = "0.25", UIMin = "0.001", UIMax = "0.25"))
	float portalFrameThickness = 0.025f;

	// ---------------------------------------------------------------------
	// Overlap
	// ---------------------------------------------------------------------

	UFUNCTION()
	void OnEntryBeginOverlap(
		UPrimitiveComponent* overlappedComponent,
		AActor* otherActor,
		UPrimitiveComponent* otherComp,
		int32 otherBodyIndex,
		bool bFromSweep,
		const FHitResult& sweepResult);

	// ---------------------------------------------------------------------
	// Core
	// ---------------------------------------------------------------------

	void TeleportCharacter(ACharacter* character);
	FRotator GetExitFacingRotation() const;
	FVector GetLaunchVelocity() const;

	/**
	 * 프로젝트 기존 입력 잠금 시스템이 있으면 BP 자식에서 Override할 수 있습니다.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Teleport")
	void ApplyMoveLock(ACharacter* character, float duration);

	virtual void ApplyMoveLock_Implementation(ACharacter* character, float duration);

private:
	/** PortalVisual 색/투명도 갱신용 MID */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> portalVisualMID;

	/** 각 클라이언트가 독립적으로 생성하는 포탈 화면용 MID입니다. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> portalScreenMID;

	void UpdatePortalVisual();
	void UpdatePortalVFX();
};

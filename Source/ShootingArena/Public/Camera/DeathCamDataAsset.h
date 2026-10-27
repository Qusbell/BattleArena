#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInterface.h"
#include "DeathCamDataAsset.generated.h"

/**
 * DeathCam에서 Killer를 강조하는 표현 방식입니다.
 *
 * Outline : 장애물에 가려진 Killer의 외곽선만 표시합니다.
 * Fill    : 장애물에 가려진 Killer 실루엣 전체를 표시합니다.
 */
UENUM(BlueprintType)
enum class EDeathCamKillerHighlightType : uint8
{
	Outline UMETA(DisplayName = "Outline"),
	Fill UMETA(DisplayName = "Fill")
};

/**
 * DeathCam에서 디자이너가 조절할 값만 보관하는 DataAsset입니다.
 * 게임 로직 / 네트워크 로직은 이 클래스에 넣지 않습니다.
 *
 * Killer Highlight는 실제 Material을 직접 교체하는 대신
 * killerHighlightType에서 Outline / Fill 중 하나를 선택해 사용합니다.
 */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UDeathCamDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---------------------------------------------------------------------
	// Camera
	// ---------------------------------------------------------------------

	/**
	 * 후보 카메라 위치의 높이 기준입니다.
	 * 사망 위치에 World Z 방향으로 더하며, 카메라의 시선은 이 위치로 고정되지 않습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Camera")
	float centerOffset = 70.0f;

	/**
	 * 카메라가 시체와 킬러를 담기 위해 선호하는 거리입니다.
	 * 실제 위치는 이동 반경과 지형 충돌에 따라 더 가까워질 수 있습니다.
	 *
	 * 기존 DataAsset이 이미 0으로 저장되어 있는 경우를 대비해,
	 * 런타임에서는 0 이하일 때 안전 기본값 600을 사용합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Camera", meta = (ClampMin = "0.0"))
	float initialCameraDistance = 600.0f;

	/** 사망 위치에서 카메라가 이동할 수 있는 최대 거리(cm)입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Camera", meta = (ClampMin = "50.0"))
	float cameraMovementRadius = 600.0f;

	/** Killer 이동으로 기본 위치가 바뀔 때 카메라가 해당 위치로 보간되는 속도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Camera", meta = (ClampMin = "0.0"))
	float cameraMoveInterpSpeed = 5.0f;

	// ---------------------------------------------------------------------
	// Optimization
	// ---------------------------------------------------------------------

	/** 후보 탐색을 다시 시작할 수 있는 최소 간격(초)입니다. 값을 높이면 탐색 빈도가 줄어듭니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Optimization", meta = (ClampMin = "0.05", ClampMax = "5.0", Units = "s"))
	float cameraSearchInterval = 0.35f;

	/** 한 번의 탐색에서 확인할 방향 수입니다. 프레임당 검사량과 별도로 적용됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Optimization", meta = (ClampMin = "4", ClampMax = "32"))
	int32 cameraSearchDirectionCount = 12;

	/** 한 프레임에 수행할 탐색 작업 수입니다. 기본값 1은 후보 검사를 여러 프레임에 분산합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Optimization", meta = (ClampMin = "1", ClampMax = "4"))
	int32 cameraSearchCandidatesPerFrame = 1;

	/** 마지막 탐색 이후 Killer가 이 거리(cm) 이상 움직이면 새 탐색을 요청합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Optimization", meta = (ClampMin = "0.0", Units = "cm"))
	float cameraSearchKillerMoveThreshold = 50.0f;

	/** Killer가 정지해 있어도 움직이는 문/장애물을 다시 확인할 최대 대기 시간(초)입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Optimization", meta = (ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
	float cameraSearchForceRefreshInterval = 1.0f;

	/** 새 후보가 현재 목표보다 이 점수 이상 좋을 때만 교체합니다. 높을수록 잦은 목표 변경이 줄어듭니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Optimization", meta = (ClampMin = "0.0"))
	float cameraTargetSwitchMargin = 40.0f;

	// ---------------------------------------------------------------------
	// Killer Highlight
	// ---------------------------------------------------------------------

	/** Killer CustomDepth / Stencil 강조 사용 여부입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|KillerHighlight")
	bool bEnableKillerHighlight = true;

	/**
	 * Killer Highlight 표현 방식을 선택합니다.
	 * 기획자는 실제 Post Process Material을 직접 교체하지 않고
	 * Outline / Fill 두 가지 중 하나만 선택하면 됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|KillerHighlight")
	EDeathCamKillerHighlightType killerHighlightType = EDeathCamKillerHighlightType::Outline;

	/** Killer Highlight에 사용할 Custom Stencil 값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|KillerHighlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 killerHighlightStencilValue = 1;

	/**
	 * 현재 선택된 Highlight Type에 대응하는 Post Process Material을 반환합니다.
	 * DeathCamActor는 Material 종류를 직접 판단하지 않고 이 함수를 통해 가져옵니다.
	 */
	UMaterialInterface* GetKillerHighlightMaterial() const;

	// ---------------------------------------------------------------------
	// Killer Highlight - Setup
	// ---------------------------------------------------------------------

	/**
	 * Outline 모드에서 사용할 Post Process Material입니다.
	 * 시스템 초기 세팅용 값이므로 기획자는 일반적으로 수정할 필요가 없습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|KillerHighlight|Setup")
	TObjectPtr<UMaterialInterface> killerHighlightOutlineMaterial = nullptr;

	/**
	 * Fill 모드에서 사용할 Post Process Material입니다.
	 * 시스템 초기 세팅용 값이므로 기획자는 일반적으로 수정할 필요가 없습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|KillerHighlight|Setup")
	TObjectPtr<UMaterialInterface> killerHighlightFillMaterial = nullptr;

	// ---------------------------------------------------------------------
	// Transition
	// ---------------------------------------------------------------------

	/** 부활 후 DeathCam에서 새 Pawn 카메라로 돌아갈 때의 ViewTarget Blend 시간입니다. DeathCam 진입은 즉시 전환됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DeathCam|Transition", meta = (ClampMin = "0.0"))
	float viewTargetBlendTime = 0.2f;
};

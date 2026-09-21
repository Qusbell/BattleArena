#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TeleportDataAsset.generated.h"

class USoundBase;

/**
 * 단방향 텔레포트의 기획 수치만 보관하는 DataAsset입니다.
 * 실제 텔레포트 로직은 AOneWayTeleportActor가 담당합니다.
 */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UTeleportDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 출구 정면을 0도로 보고 위쪽으로 올리는 발사 각도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport",
		meta = (ClampMin = "0", ClampMax = "360", UIMin = "0", UIMax = "360"))
	int32 launchAngle = 15;

	/** 출구에서 캐릭터를 발사하는 세기입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport",
		meta = (ClampMin = "0.0"))
	float launchPower = 600.0f;

	/** 발사 직후 이동/점프 입력을 제한하는 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport",
		meta = (ClampMin = "0.0"))
	float moveLockTime = 0.2f;

	/** 포탈 이용자 본인에게만 재생할 사운드입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Sound")
	TObjectPtr<USoundBase> teleportSound;

	/** 사운드 에셋 기본 볼륨에 곱할 값입니다. 0이면 무음, 1이면 원본 볼륨입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Sound",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float soundVolumeMultiplier = 1.0f;

	/** 이 거리 안에서만 포탈 출구 화면을 갱신합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float viewDistance = 2000.0f;

	/**
	 * 이 거리 안으로 들어오면 화면이 Far Blur에서 Near Blur로 점차 선명해지기 시작합니다.
	 * View Distance보다 크게 설정해도 실제 값은 View Distance까지로 제한됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float clarityStartDistance = 1200.0f;

	/** 포탈에 도달했을 때의 화면 불투명도입니다. 0보다 크게 두면 도달 시에도 화면이 남습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalScreenNearOpacity = 0.35f;

	/** View Distance 끝에서의 화면 불투명도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalScreenFarOpacity = 1.0f;

	/** 포탈에 가까울 때 적용할 흐림 강도입니다. 0이면 흐림이 없습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float blurAtNearDistance = 0.0f;

	/** 주시 가능 거리 끝에서 적용할 흐림 강도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float blurAtFarDistance = 8.0f;

	/** 클라이언트별 로컬 Render Target 한 변의 픽셀 크기입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "128", UIMin = "128"))
	int32 portalViewRenderTargetSize = 1024;

	/** 화면 점유율에 따라 Render Target 크기를 낮춥니다. 위 크기는 최종 최대 해상도로 사용됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Dynamic Resolution")
	bool bUseDynamicPortalResolution = true;

	/** 포탈의 화면 점유율이 Low 기준보다 작을 때 사용할 해상도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Dynamic Resolution",
		meta = (EditCondition = "bUseDynamicPortalResolution", ClampMin = "128", UIMin = "128"))
	int32 portalViewLowResolution = 512;

	/** Low와 High 기준 사이에서 사용할 해상도입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Dynamic Resolution",
		meta = (EditCondition = "bUseDynamicPortalResolution", ClampMin = "128", UIMin = "128"))
	int32 portalViewMediumResolution = 1024;

	/** 이 화면 면적 비율 아래에서는 Low 해상도를 사용합니다. 0.1은 화면의 10%입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Dynamic Resolution",
		meta = (EditCondition = "bUseDynamicPortalResolution", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalViewLowCoverageThreshold = 0.1f;

	/** 이 화면 면적 비율 이상에서는 기존 최대 해상도를 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Dynamic Resolution",
		meta = (EditCondition = "bUseDynamicPortalResolution", ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float portalViewHighCoverageThreshold = 0.35f;

	/** 경계 부근에서 Render Target이 반복 생성되지 않게 하는 전환 여유값입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View|Dynamic Resolution",
		meta = (EditCondition = "bUseDynamicPortalResolution", ClampMin = "0.0", ClampMax = "0.2", UIMin = "0.0", UIMax = "0.1"))
	float portalViewResolutionHysteresis = 0.03f;

	/**
	 * 한 로컬 클라이언트가 동시에 갱신할 최대 포탈 수입니다. 0이면 화면에 보이는 포탈을 모두 갱신합니다.
	 * 성능 문제가 확인된 플랫폼에서만 양수로 제한하세요. 점수가 높은(더 가깝고 정면인) 포탈부터 유지됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "16"))
	int32 maxSimultaneousPortalViews = 0;

	/** 포탈 화면 갱신 빈도입니다. 0이면 매 프레임 갱신합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float portalViewUpdateRate = 30.0f;

	/**
	 * 한 프레임에 새로 CaptureScene을 실행할 최대 포탈 수입니다.
	 * 0이면 기존처럼 갱신 대상 전체를 같은 프레임에 캡처합니다.
	 * 고해상도 Render Target 사용 시에는 2~3을 권장합니다. 예산을 넘긴 포탈은
	 * 마지막 화면을 유지하고 다음 프레임에 우선 갱신됩니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0", UIMin = "0", UIMax = "8"))
	int32 maxPortalCapturesPerFrame = 2;

	/**
	 * 켜면 포탈 캡처도 HDR, 동적 그림자, Lumen, 고비용 후처리를 사용합니다.
	 * 고해상도에서는 기본값(false)을 권장합니다. 메인 카메라 품질에는 영향이 없습니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View")
	bool bUseHighQualityPortalCapture = false;
};

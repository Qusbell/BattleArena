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
		meta = (ClampMin = "128", ClampMax = "2048", UIMin = "128", UIMax = "2048"))
	int32 portalViewRenderTargetSize = 1024;

	/** 포탈 화면 갱신 빈도입니다. 0이면 매 프레임 갱신합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleport|Portal View",
		meta = (ClampMin = "0.0", UIMin = "0.0"))
	float portalViewUpdateRate = 30.0f;
};

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "DeathCamActor.generated.h"

class APlayerController;
class UDeathCamDataAsset;
class UPrimitiveComponent;
class UMaterialInterface;

/**
 * 실제 DeathCam 시점을 담당하는 CameraActor입니다.
 *
 * 사망 위치 주변의 제한된 범위에서 벽을 따라 이동하며 사망 위치와 Killer를 함께 담습니다.
 *
 * 네트워크 / ViewTarget / Spawn / Destroy는 DeathCamComponent가 담당합니다.
 */
UCLASS(Blueprintable)
class SHOOTINGARENA_API ADeathCamActor : public ACameraActor
{
	GENERATED_BODY()

public:
	ADeathCamActor();

	virtual void Tick(float deltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

	/**
	 * 기존 DeathCamComponent에서 사용하던 시그니처를 그대로 유지합니다.
	 * PlayerController Blueprint를 수정하지 않아도 기존 StartDeathCam 흐름이 깨지지 않게 하기 위함입니다.
	 */
	bool InitializeDeathCam(
		const FVector& inDeathLocation,
		AActor* inOtherActor,
		APlayerController* inPlayerController,
		UDeathCamDataAsset* inDeathCamData,
		AActor* inDeadActorToIgnore);

	/** 카메라 갱신과 Killer Highlight를 중지합니다. ViewTarget 복귀/Destroy는 DeathCamComponent가 담당합니다. */
	void StopDeathCam();

	UFUNCTION(BlueprintPure, Category = "DeathCam")
	bool IsDeathCamActive() const { return bDeathCamActive; }

	/**
	 * TopView는 최종 기획에서 제거되었습니다.
	 * 기존 Blueprint가 이 함수를 참조하고 있어도 컴파일 오류가 나지 않도록 호환용으로 남겨두며 항상 false를 반환합니다.
	 */
	UFUNCTION(BlueprintPure, Category = "DeathCam", meta = (DeprecatedFunction, DeprecationMessage = "TopView was removed from the final DeathCam design."))
	bool IsTopView() const { return false; }

	UFUNCTION(BlueprintPure, Category = "DeathCam")
	FVector GetDeathLocation() const { return deathLocation; }

	/** 후보 위치의 높이 기준: 사망 위치 + Offset(Z). */
	UFUNCTION(BlueprintPure, Category = "DeathCam")
	FVector GetCenterLocation() const { return centerLocation; }

	/** 사망 위치를 중심으로 하는 최대 카메라 이동 반경. */
	UFUNCTION(BlueprintPure, Category = "DeathCam")
	float GetMaxDistance() const { return maxDistance; }

private:
	/** 최종 기획에서 말하는 Killer. 기존 코드/호환을 위해 입력 이름은 OtherActor를 유지합니다. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> otherActor;

	UPROPERTY(Transient)
	TObjectPtr<AActor> deadActorToIgnore;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> ownerPlayerController;

	/** 실제 사망 위치. */
	FVector deathLocation = FVector::ZeroVector;

	/** 후보 높이 기준 = deathLocation + FVector::UpVector * centerOffset. */
	FVector centerLocation = FVector::ZeroVector;

	/** 사망 위치를 중심으로 한 카메라 이동 반경. */
	float maxDistance = 600.0f;

	bool bDeathCamActive = false;

	/** 후보 위치를 자주 바꾸지 않아 장애물 경계에서 시점이 흔들리는 것을 방지합니다. */
	FVector cameraTargetLocation = FVector::ZeroVector;
	float baseCameraFov = 90.0f;

	/** -2: 공통 출발점 확인, -1: 현재 목표 평가, 0 이상: 방향별 후보 평가입니다. */
	int32 nextCameraSearchDirection = -2;
	bool bCameraSearchInProgress = false;
	bool bSearchHasKiller = false;
	bool bCameraMovementBlocked = false;
	float timeSinceCameraSearch = 0.0f;
	FVector searchKillerDirection = FVector::ForwardVector;
	FVector searchKillerLocation = FVector::ZeroVector;
	FVector searchSweepStart = FVector::ZeroVector;
	FVector searchBestLocation = FVector::ZeroVector;
	float searchBestScore = -1.0e9f;
	float searchCurrentTargetScore = -1.0e9f;

	// DataAsset 값을 런타임에 복사합니다.
	// DataAsset이 비어 있어도 아래 기본값으로 안전하게 동작합니다.
	float centerOffset = 0.0f;
	float initialCameraDistance = 600.0f;
	float cameraMoveInterpSpeed = 5.0f;
	float cameraSearchInterval = 0.35f;
	int32 cameraSearchDirectionCount = 12;
	int32 cameraSearchCandidatesPerFrame = 1;
	float cameraSearchKillerMoveThreshold = 50.0f;
	float cameraSearchForceRefreshInterval = 1.0f;
	float cameraTargetSwitchMargin = 40.0f;
	bool bEnableKillerHighlight = true;
	int32 killerHighlightStencilValue = 1;

	/** DeathCam 카메라에만 적용할 Killer Highlight Post Process Material. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> killerHighlightMaterial;

	// 기존 SpringArm Camera Collision과 같은 역할을 하는 Probe 반경입니다.
	static constexpr float cameraProbeRadius = 12.0f;

	// 벽과의 여유 공간을 Sweep 반경에 포함합니다.
	// 충돌 후 위치를 법선 방향으로 밀면 좁은 모서리에서 다른 벽/이동 제한과 충돌할 수 있습니다.
	static constexpr float cameraCollisionSkin = 2.0f;

	// 접촉점보다 이동 경로를 따라 조금 앞에서 멈춰 다음 Sweep의 초기 겹침을 방지합니다.
	static constexpr float cameraSweepPullback = 0.1f;

	// 모서리처럼 한 프레임에 두 면 이상을 만나는 경우에도
	// 남은 이동량을 다시 투영할 수 있도록 제한된 횟수만 반복합니다.
	static constexpr int32 maxSlideIterations = 3;

	static constexpr float maxFramingFov = 150.0f;
	static constexpr float minDeathLocationCameraDistance = 60.0f;

	/** Killer Highlight 종료 시 원래 CustomDepth 상태로 되돌리기 위한 백업입니다. */
	struct FHighlightComponentState
	{
		TWeakObjectPtr<UPrimitiveComponent> component;
		bool bRenderCustomDepth = false;
		int32 customDepthStencilValue = 0;
	};

	TArray<FHighlightComponentState> killerHighlightStates;

	void CacheSettings(UDeathCamDataAsset* inDeathCamData);
	void UpdateDeathCam(float deltaSeconds);

	/** Center -> Killer 방향. Killer가 유효하지 않으면 안전한 fallback 방향을 반환합니다. */
	FVector CalculateKillerDirection() const;

	/** Killer 반대편을 선호 위치로 삼되, 충돌에 따라 다른 위치도 탐색합니다. */
	FVector CalculateBaseCameraLocation(const FVector& killerDirection) const;

	/** 후보들의 공통 출발점을 한 번 검사해 같은 Overlap 검사를 반복하지 않습니다. */
	FVector FindCameraSweepStart() const;

	/** 안전한 공통 출발점에서 후보 위치까지 Sweep합니다. */
	FVector ResolveCameraCandidateLocation(const FVector& desiredCameraLocation, const FVector& sweepStart) const;

	/** 방향/위치를 저장한 뒤 프레임별 예산 안에서 탐색을 진행합니다. */
	void BeginCameraSearch(const FVector& killerDirection);
	void AdvanceCameraSearch();
	bool ShouldBeginCameraSearch() const;
	float ScoreCameraTarget(const FVector& candidate) const;

	/** 두 대상의 방향을 기준으로 회전과 필요한 시야각을 계산합니다. */
	FRotator CalculateFramingRotation(const FVector& cameraLocation, float& outFov) const;

	/** 현재 위치에서 원하는 이동량만큼 움직이되, 충돌하면 Hit Normal 기준으로 벽면 Slide를 적용합니다. */
	FVector MoveCameraWithCollision(const FVector& cameraMove) const;

	/** 고정된 사망 위치를 중심으로 한 이동 반경을 Sweep 목표에 적용합니다. */
	FVector ClampCameraLocation(const FVector& candidateLocation) const;

	/** DataAsset의 Highlight Type에 따라 선택된 Post Process Material을 이 DeathCam 카메라에만 적용합니다. */
	void ApplyKillerHighlightPostProcess();

	void ApplyKillerHighlight();
	void RemoveKillerHighlight();
};

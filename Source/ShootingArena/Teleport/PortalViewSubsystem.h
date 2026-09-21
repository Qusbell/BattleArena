#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PortalViewSubsystem.generated.h"

class AOneWayTeleportActor;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

USTRUCT()
struct FPortalViewInstance
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<USceneCaptureComponent2D> sceneCapture;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> renderTarget;

	UPROPERTY(Transient)
	float captureAccumulator = 0.0f;

	UPROPERTY(Transient)
	int32 currentResolution = 0;

	UPROPERTY(Transient)
	FTransform lastCaptureTransform = FTransform::Identity;

	UPROPERTY(Transient)
	bool bHasCaptured = false;
};

/**
 * 각 클라이언트의 로컬 카메라 기준으로만 포탈 화면을 계산하고 렌더링합니다.
 * 서버와 다른 클라이언트에는 Render Target, Material 상태를 복제하지 않습니다.
 */
UCLASS()
class SHOOTINGARENA_API UPortalViewSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	FPortalViewInstance& FindOrCreateView(AOneWayTeleportActor* portal);
	int32 ResolvePortalRenderTargetSize(
		const FPortalViewInstance& view,
		const class UTeleportDataAsset& settings,
		float screenCoverage) const;
	float ResolvePortalUpdateRate(
		const FPortalViewInstance& view,
		const class UTeleportDataAsset& settings,
		const FTransform& captureTransform,
		float screenCoverage) const;
	bool EnsureRenderTarget(FPortalViewInstance& view, int32 size, bool bUseHighQualityCapture);
	void ClearPortalView(AOneWayTeleportActor* portal, FPortalViewInstance& view);
	void ClearAllPortalViews();

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AOneWayTeleportActor>, FPortalViewInstance> portalViews;
};

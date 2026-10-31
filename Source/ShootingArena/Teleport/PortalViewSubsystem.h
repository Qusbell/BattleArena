#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PortalViewSubsystem.generated.h"

class AOneWayTeleportActor;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
struct FPortalViewTickBuffers;
struct FPortalViewTickBuffersDeleter
{
	void operator()(FPortalViewTickBuffers* buffers) const;
};

/** Snapshot of the reference frames used by an actual submitted capture, not a skipped update. */
struct FPortalCaptureHistory
{
	TWeakObjectPtr<AActor> exitTarget;
	FTransform entryFrame = FTransform::Identity;
	FTransform exitFrame = FTransform::Identity;
	uint64 observerCutRevision = 0;
	bool bSpatialView = false;
	bool bOcclusionEnabled = false;
	bool bPersistentState = false;
};

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

	// Coverage hysteresis must track the ordinary tier, not the temporary moving tier.
	UPROPERTY(Transient)
	int32 normalResolution = 0;

	UPROPERTY(Transient)
	float smoothCaptureRemaining = 0.0f;

	UPROPERTY(Transient)
	FTransform lastCaptureTransform = FTransform::Identity;

	UPROPERTY(Transient)
	bool bHasCaptured = false;

	FPortalCaptureHistory lastCaptureHistory;
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
	UPortalViewSubsystem();
	virtual ~UPortalViewSubsystem() override;
	void RegisterPortal(AOneWayTeleportActor* portal);
	void UnregisterPortal(AOneWayTeleportActor* portal);
	int32 GetRegisteredPortalCount() const;
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float deltaTime) override;
	virtual TStatId GetStatId() const override;

	/** Spatial surfaces feed linear scene light into the player's own exposure/tonemapper exactly once. */
	static void ConfigureCaptureAppearance(USceneCaptureComponent2D& capture, bool bSpatialView, bool bHighQuality,
		bool bEnableOcclusionCulling = true);
	/** A camera cut discards old visibility results without reallocating the ViewState or changing capture cadence. */
	static bool ShouldResetCaptureHistory(const FPortalCaptureHistory& previous,
		const FPortalCaptureHistory& current, bool bHasCaptured);
	/** Preserve fractional capture time without queuing catch-up captures after a hitch. */
	static float CalculateCaptureRemainder(float accumulatedSeconds, float intervalSeconds, bool bForced);
	/** Main-view camera motion, independent of portal-eye clamping and capture intervals. */
	static bool IsObserverMoving(const FTransform& previous, const FTransform& current, float deltaTime,
		float speedThreshold, float angularSpeedThreshold);
	static float ApplyMovementCaptureLimit(float normalRate, float movingRate, bool bEnabled, bool bMoving);
	/** Short hold avoids capture-rate/texture-size oscillation during brief pauses in motion. */
	static bool UpdateSmoothCaptureState(FPortalViewInstance& view, const class UTeleportDataAsset& settings,
		bool bSpatialView, bool bObserverMoving, float deltaTime);

private:
	TUniquePtr<FPortalViewTickBuffers,FPortalViewTickBuffersDeleter> tickBuffers;
	friend class FSpatialPortalSmoothCaptureTest;
	friend class FSpatialPortalRenderShutdownTest;
	void FinishViewUpdate();
	bool bUpdatingViews = false;
	bool bDeinitializing = false;
	bool bClearingViews = false;
	static int32 ResolveSmoothRenderTargetSize(int32 normalResolution, const class UTeleportDataAsset& settings);
	FPortalViewInstance& FindOrCreateView(AOneWayTeleportActor* portal);
	int32 ResolvePortalRenderTargetSize(
		const FPortalViewInstance& view,
		const class UTeleportDataAsset& settings,
		float screenCoverage) const;
	float ResolvePortalUpdateRate(
		const FPortalViewInstance& view,
		const class UTeleportDataAsset& settings,
		bool bObserverMoving,
		float screenCoverage) const;
	bool EnsureRenderTarget(FPortalViewInstance& view, int32 size, bool bUseHDRCapture);
	void ClearPortalView(AOneWayTeleportActor* portal, FPortalViewInstance& view);
	void ClearAllPortalViews();
	FTransform lastObserverTransform = FTransform::Identity;
	bool bHasObserverTransform = false;
	TWeakObjectPtr<AActor> lastObserverPawn;
	TWeakObjectPtr<AActor> lastObserverViewTarget;
	uint64 observerCutRevision = 0;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<AOneWayTeleportActor>, FPortalViewInstance> portalViews;
};

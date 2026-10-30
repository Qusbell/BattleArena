#include "PortalViewSubsystem.h"

#include "OneWayTeleportActor.h"
#include "TeleportDataAsset.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Engine.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "RHIGlobals.h"
#include "Misc/ScopeExit.h"

namespace PortalViewPrivate
{
	struct FPortalCandidate
	{
		TWeakObjectPtr<AOneWayTeleportActor> portal;
		float distance = 0.0f;
		float score = 0.0f;
		float screenCoverage = 0.0f;
	};

	struct FReadyPortalCapture
	{
		TWeakObjectPtr<AOneWayTeleportActor> portal;
		float score = 0.0f;
		float accumulatedWaitTime = 0.0f;
		FPortalCaptureView captureView;
		float captureInterval = 0.0f;
		bool bForced = false;
		FPortalCaptureHistory history;
	};
}

struct FPortalViewTickBuffers
{
	TSet<TWeakObjectPtr<AOneWayTeleportActor>> registeredPortals;
	TArray<TWeakObjectPtr<AActor>> portalSnapshot;
	TArray<PortalViewPrivate::FPortalCandidate> candidates;
	TArray<PortalViewPrivate::FReadyPortalCapture> readyCaptures;
	TSet<TWeakObjectPtr<AOneWayTeleportActor>> visiblePortals;
	bool bRegistryDirty = false;
};

void FPortalViewTickBuffersDeleter::operator()(FPortalViewTickBuffers* buffers) const { delete buffers; }
UPortalViewSubsystem::UPortalViewSubsystem() : tickBuffers(new FPortalViewTickBuffers()) {}
UPortalViewSubsystem::~UPortalViewSubsystem() = default;

int32 UPortalViewSubsystem::GetRegisteredPortalCount() const
{ return tickBuffers->registeredPortals.Num(); }

void UPortalViewSubsystem::RegisterPortal(AOneWayTeleportActor* portal)
{
	if (bDeinitializing || !IsValid(portal) || portal->GetWorld()!=GetWorld() || !IsValid(GetWorld())
		|| GetWorld()->bIsTearingDown || GetWorld()->GetNetMode()==NM_DedicatedServer) return;
	tickBuffers->registeredPortals.Add(portal);
	tickBuffers->bRegistryDirty = true;
}

void UPortalViewSubsystem::UnregisterPortal(AOneWayTeleportActor* portal)
{
	tickBuffers->registeredPortals.Remove(portal);
	// Keep this tick's snapshot and view-map entries stable across actor callbacks.
	tickBuffers->bRegistryDirty = true;
}

void UPortalViewSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	UWorld* world = GetWorld();
	if (!IsValid(world) || world->bIsTearingDown || world->GetNetMode() == NM_DedicatedServer)
	{
		ClearAllPortalViews();
		return;
	}

}

void UPortalViewSubsystem::Deinitialize()
{
	if (bDeinitializing) return;
	bDeinitializing = true;
	// Capture/portal transform callbacks can end the world inside Tick. Its references must
	// remain alive until the active update unwinds, then all resources are released together.
	if (!bUpdatingViews && !bClearingViews) FinishViewUpdate();
	Super::Deinitialize();
}

void UPortalViewSubsystem::FinishViewUpdate()
{
	bUpdatingViews = false;
	if (bDeinitializing)
	{
		ClearAllPortalViews();
		*tickBuffers = FPortalViewTickBuffers();
	}
}

void UPortalViewSubsystem::Tick(float deltaTime)
{
	UWorld* world = GetWorld();
	if (bUpdatingViews || bDeinitializing || !IsValid(world) || world->bIsTearingDown || !FMath::IsFinite(deltaTime)
		|| world->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	bUpdatingViews = true;
	ON_SCOPE_EXIT { FinishViewUpdate(); };

	APlayerController* playerController = world->GetFirstPlayerController();
	if (!IsValid(playerController) || !playerController->IsLocalController()
		|| !IsValid(playerController->PlayerCameraManager))
	{
		ClearAllPortalViews();
		return;
	}

	const FVector cameraLocation = playerController->PlayerCameraManager->GetCameraLocation();
	const FRotator cameraRotation = playerController->PlayerCameraManager->GetCameraRotation();
	if (cameraLocation.ContainsNaN() || cameraRotation.ContainsNaN())
	{
		ClearAllPortalViews();
		return;
	}
	const FVector aimDirection = cameraRotation.Vector();
	const FTransform observerTransform(cameraRotation, cameraLocation);
	const FTransform previousObserverTransform = lastObserverTransform;
	const bool bCanCompareObserver = bHasObserverTransform;
	// Latch cuts even on frames where the DA interval or shared budget postpones the capture.
	const float translationThreshold = GEngine ? GEngine->CameraTranslationThreshold : 10000.0f;
	const float rotationThreshold = GEngine ? GEngine->CameraRotationThreshold : 45.0f;
	const bool bObserverJump = bCanCompareObserver
		&& (FVector::DistSquared(previousObserverTransform.GetLocation(), cameraLocation) > FMath::Square(translationThreshold)
			|| previousObserverTransform.GetRotation().AngularDistance(observerTransform.GetRotation())
				> FMath::DegreesToRadians(rotationThreshold));
	if (playerController->PlayerCameraManager->bGameCameraCutThisFrame || bObserverJump
		|| (bCanCompareObserver && (lastObserverPawn != playerController->GetPawn()
			|| lastObserverViewTarget != playerController->GetViewTarget()))) ++observerCutRevision;
	lastObserverPawn = playerController->GetPawn();
	lastObserverViewTarget = playerController->GetViewTarget();
	lastObserverTransform = observerTransform;
	bHasObserverTransform = true;

	if (tickBuffers->bRegistryDirty)
	{
		tickBuffers->portalSnapshot.Reset();
		for (auto it=tickBuffers->registeredPortals.CreateIterator(); it; ++it)
		{
			if (!it->IsValid()) { it.RemoveCurrent(); continue; }
			tickBuffers->portalSnapshot.Add(it->Get());
		}
		tickBuffers->bRegistryDirty = false;
	}
	auto& candidates = tickBuffers->candidates;
	auto& readyCaptures = tickBuffers->readyCaptures;
	auto& visiblePortals = tickBuffers->visiblePortals;
	const auto& allPortals = tickBuffers->portalSnapshot;
	candidates.Reset(); readyCaptures.Reset(); visiblePortals.Reset();
	for (const auto& weakPortal : allPortals)
	{
		auto* portal = Cast<AOneWayTeleportActor>(weakPortal.Get());
		if (!IsValid(portal) || !tickBuffers->registeredPortals.Contains(portal))
		{ tickBuffers->bRegistryDirty = true; continue; }
		float distance = 0.0f;
		float score = 0.0f;
		float screenCoverage = 0.0f;
		const bool bCanDisplay = portal->CanDisplayPortalView(
				playerController,
				cameraLocation,
				aimDirection,
				distance,
				score,
				screenCoverage);
		if (bDeinitializing || world->bIsTearingDown) return;
		if (bCanDisplay && IsValid(portal) && tickBuffers->registeredPortals.Contains(portal)
			&& FMath::IsFinite(distance) && FMath::IsFinite(score) && FMath::IsFinite(screenCoverage))
		{
			candidates.Add({ portal, distance, score, screenCoverage });
		}
	}

	// A later portal's callback may have removed an earlier candidate in this same update.
	candidates.RemoveAll([&](const auto& candidate)
	{
		return !candidate.portal.IsValid() || !tickBuffers->registeredPortals.Contains(candidate.portal);
	});
	if (candidates.IsEmpty())
	{
		ClearAllPortalViews();
		return;
	}

	// Only selecting a limited subset needs a full sort. The global DA still comes from the top score.
	int32 bestCandidate = 0;
	for (int32 i=1; i<candidates.Num(); ++i)
		if (candidates[i].score>candidates[bestCandidate].score) bestCandidate=i;
	candidates.Swap(0,bestCandidate);

	const UTeleportDataAsset* firstSettings = candidates[0].portal->GetTeleportDataAsset();
	const int32 maxViews = IsValid(firstSettings) ? firstSettings->maxSimultaneousPortalViews : 0;
	if (maxViews > 0 && candidates.Num() > maxViews)
	{
		candidates.Sort([](const auto& left,const auto& right) { return left.score>right.score; });
		candidates.SetNum(maxViews,EAllowShrinking::No);
	}
	for (const PortalViewPrivate::FPortalCandidate& candidate : candidates)
	{
		AOneWayTeleportActor* portal = candidate.portal.Get();
		if (!IsValid(portal))
		{
			continue;
		}

		const UTeleportDataAsset* settings = portal->GetTeleportDataAsset();
		if (!IsValid(settings))
		{
			continue;
		}

		visiblePortals.Add(portal);
		FPortalCaptureView captureView;
		const FTransform playerCameraTransform(cameraRotation, cameraLocation);
		const bool bBuiltView=portal->BuildPortalCaptureView(playerCameraTransform, captureView);
		if (bDeinitializing || world->bIsTearingDown) return;
		if (!bBuiltView || !IsValid(portal) || !tickBuffers->registeredPortals.Contains(portal)
			|| captureView.transform.ContainsNaN() || !captureView.transform.GetRotation().IsNormalized()
			|| (captureView.bCustomProjection && captureView.projection.ContainsNaN())
			|| (captureView.bEnableClipPlane && (captureView.clipBase.ContainsNaN()
				|| captureView.clipNormal.ContainsNaN() || captureView.clipNormal.IsNearlyZero())))
		{
			visiblePortals.Remove(portal);
			continue;
		}
		FPortalViewInstance& view = FindOrCreateView(portal);
		if (bDeinitializing || world->bIsTearingDown) return;
		const bool bObserverMoving = bCanCompareObserver
			&& IsObserverMoving(previousObserverTransform, observerTransform, deltaTime,
				settings->portalViewStillLocationTolerance, settings->portalViewStillRotationTolerance);
		const bool bSmoothCapture = UpdateSmoothCaptureState(view, *settings,
			captureView.bSpatialView, bObserverMoving, deltaTime);
		view.normalResolution = ResolvePortalRenderTargetSize(
			view,
			*settings,
			candidate.screenCoverage);
		const int32 desiredRenderTargetSize = bSmoothCapture
			? ResolveSmoothRenderTargetSize(view.normalResolution, *settings) : view.normalResolution;
		const bool bRenderTargetRecreated = EnsureRenderTarget(
			view,
			desiredRenderTargetSize,
			captureView.bSpatialView || settings->bUseHighQualityPortalCapture);
		if (bDeinitializing || world->bIsTearingDown) return;
		if (!IsValid(view.renderTarget) || !IsValid(view.sceneCapture))
		{
			continue;
		}
		if (bRenderTargetRecreated)
		{
			// 새 Render Target의 검은 화면이 남지 않도록 이번 프레임 캡처 대상으로 올립니다.
			view.captureAccumulator = 60.0f;
		}
		// View Distance 바깥은 CanDisplayPortalView에서 이미 제외됩니다. 이 안에서도
		// Clarity Start Distance보다 멀면 Far Blur를 유지하고, 그 거리부터 입구까지
		// 점차 Near Blur로 바꿉니다.
		const float clarityStartDistance = FMath::Clamp(
			settings->clarityStartDistance,
			KINDA_SMALL_NUMBER,
			settings->viewDistance);
		const float blurAlpha = FMath::Clamp(candidate.distance / clarityStartDistance, 0.0f, 1.0f);
		const float blurStrength = FMath::Lerp(
			settings->blurAtNearDistance,
			settings->blurAtFarDistance,
			blurAlpha);
		portal->ApplyPortalView(view.renderTarget, captureView.bSpatialView ? 0.0f : blurStrength,
			captureView.bSpatialView ? 1.0f : portal->GetPortalViewOpacity(candidate.distance));
		if (bDeinitializing || world->bIsTearingDown) return;
		if (!IsValid(portal) || !tickBuffers->registeredPortals.Contains(portal))
		{ visiblePortals.Remove(candidate.portal); continue; }

		view.captureAccumulator = FMath::Min(
			view.captureAccumulator + FMath::Max(deltaTime, 0.0f),
			60.0f);
		const float updateRate = portal->NeedsFullRatePortalView(candidate.distance) ? 0.0f : ResolvePortalUpdateRate(
			view,
			*settings,
			bObserverMoving,
			candidate.screenCoverage);
		// 음수는 이동 중 캡처 중단을 의미합니다. 첫 캡처와 해상도 변경은 항상 우선합니다.
		if (updateRate < 0.0f && !bRenderTargetRecreated)
		{
			continue;
		}
		const float captureInterval = updateRate > 0.0f
			? 1.0f / updateRate
			: 0.0f;
		if (captureInterval > 0.0f && view.captureAccumulator < captureInterval)
		{
			continue;
		}

		FPortalCaptureHistory history;
		AActor* exitTarget = portal->GetExitTarget();
		if (!IsValid(exitTarget)) { visiblePortals.Remove(portal); continue; }
		history.exitTarget = exitTarget;
		history.entryFrame = portal->GetPortalFrameTransform();
		if (const auto* exitPortal = Cast<AOneWayTeleportActor>(exitTarget))
			history.exitFrame = exitPortal->GetPortalFrameTransform();
		else history.exitFrame = exitTarget->GetActorTransform();
		history.observerCutRevision = observerCutRevision;
		history.bSpatialView = captureView.bSpatialView;
		history.bOcclusionEnabled = settings->bEnablePortalCaptureOcclusionCulling;
		history.bPersistentState = captureView.bSpatialView
			&& (settings->bUseHighQualityPortalCapture || settings->bEnablePortalCaptureOcclusionCulling);
		readyCaptures.Add({ portal, candidate.score, view.captureAccumulator, captureView,
			captureInterval, bRenderTargetRecreated || !view.bHasCaptured, history });
	}

	// 예산에서 밀린 포탈은 누적 대기 시간이 계속 증가합니다. 다음 프레임에는 가장 오래
	// 기다린 포탈부터 처리하므로, 가까운 포탈만 계속 갱신되고 나머지가 멈추는 현상을 막습니다.
	const int32 captureBudget = IsValid(firstSettings)
		? FMath::Max(0, firstSettings->maxPortalCapturesPerFrame) : 0;
	if (captureBudget>0 && readyCaptures.Num()>captureBudget) readyCaptures.Sort([](
		const PortalViewPrivate::FReadyPortalCapture& left,
		const PortalViewPrivate::FReadyPortalCapture& right)
	{
		if (!FMath::IsNearlyEqual(left.accumulatedWaitTime, right.accumulatedWaitTime))
		{
			return left.accumulatedWaitTime > right.accumulatedWaitTime;
		}

		return left.score > right.score;
	});

	const int32 captureCount = captureBudget > 0
		? FMath::Min(captureBudget, readyCaptures.Num())
		: readyCaptures.Num();

	for (int32 captureIndex = 0; captureIndex < captureCount; ++captureIndex)
	{
		AOneWayTeleportActor* portal = readyCaptures[captureIndex].portal.Get();
		if (!IsValid(portal))
		{
			continue;
		}

		FPortalViewInstance* view = portalViews.Find(TWeakObjectPtr<AOneWayTeleportActor>(portal));
		if (view == nullptr || !IsValid(view->renderTarget) || !IsValid(view->sceneCapture)
			|| !view->sceneCapture->IsRegistered() || world->bIsTearingDown
			|| !IsValid(portal->GetExitTarget()))
		{
			continue;
		}

		const UTeleportDataAsset* settings = portal->GetTeleportDataAsset();
		if (!IsValid(settings))
		{
			continue;
		}

		const FPortalCaptureView& captureView = readyCaptures[captureIndex].captureView;
		view->captureAccumulator = CalculateCaptureRemainder(view->captureAccumulator,
			readyCaptures[captureIndex].captureInterval, readyCaptures[captureIndex].bForced);
		ConfigureCaptureAppearance(*view->sceneCapture, captureView.bSpatialView, settings->bUseHighQualityPortalCapture,
			settings->bEnablePortalCaptureOcclusionCulling);
		view->sceneCapture->bCameraCutThisFrame |= readyCaptures[captureIndex].bForced
			|| ShouldResetCaptureHistory(view->lastCaptureHistory, readyCaptures[captureIndex].history, view->bHasCaptured);
		view->sceneCapture->MaxViewDistanceOverride = FMath::IsFinite(settings->portalCaptureMaxViewDistance)
			&& settings->portalCaptureMaxViewDistance > 0.0f
			? settings->portalCaptureMaxViewDistance
			: -1.0f;
		view->sceneCapture->LODDistanceFactor = FMath::IsFinite(settings->portalCaptureLODDistanceFactor)
			? FMath::Max(settings->portalCaptureLODDistanceFactor, 0.01f) : 1.0f;

		view->sceneCapture->SetWorldTransform(captureView.transform);
		if (bDeinitializing || world->bIsTearingDown) return;
		if (!IsValid(portal) || !tickBuffers->registeredPortals.Contains(portal)
			|| !IsValid(view->sceneCapture) || !view->sceneCapture->IsRegistered()) continue;
		view->sceneCapture->bUseCustomProjectionMatrix = captureView.bCustomProjection;
		view->sceneCapture->CustomProjectionMatrix = captureView.projection;
		view->sceneCapture->bEnableClipPlane = captureView.bEnableClipPlane;
		view->sceneCapture->TextureTarget = view->renderTarget;
		view->sceneCapture->ClipPlaneBase = captureView.clipBase;
		view->sceneCapture->ClipPlaneNormal = captureView.clipNormal;
		view->sceneCapture->HiddenActors.Reset();
		if (captureView.bSpatialView)
			for (const auto& hidden:allPortals)
				if (AActor* actor=hidden.Get()) view->sceneCapture->HiddenActors.Add(actor);
		view->sceneCapture->HiddenActors.AddUnique(portal);
		if (AOneWayTeleportActor* exitPortal = Cast<AOneWayTeleportActor>(portal->GetExitTarget()))
		{
			view->sceneCapture->HiddenActors.AddUnique(exitPortal);
		}
		if (captureView.bSpatialView)
		{
			// No recursive feedback or a separate Niagara render inside each portal view.
			view->sceneCapture->ShowFlags.SetMotionBlur(false);
			view->sceneCapture->ShowFlags.SetDepthOfField(false);
		}
		if (captureView.bSpatialView)
		{
			// UE supplies the current player's resolved PP (including volumes/camera blends) here.
			// Do not copy its camera/projection/resolution or keep a raw main-view pointer ourselves.
			view->sceneCapture->CaptureSceneDeferred();
		}
		else view->sceneCapture->CaptureScene();
		if (bDeinitializing || world->bIsTearingDown) return;
		view->lastCaptureTransform = captureView.transform;
		view->lastCaptureHistory = readyCaptures[captureIndex].history;
		view->bHasCaptured = true;
	}

	for (auto iterator = portalViews.CreateIterator(); iterator; ++iterator)
	{
		if (!visiblePortals.Contains(iterator.Key()))
		{
			ClearPortalView(iterator.Key().Get(), iterator.Value());
			iterator.RemoveCurrent();
		}
	}
}

bool UPortalViewSubsystem::IsObserverMoving(const FTransform& previous, const FTransform& current, float deltaTime,
	float speedThreshold, float angularSpeedThreshold)
{
	if (previous.ContainsNaN() || current.ContainsNaN() || !FMath::IsFinite(deltaTime) || deltaTime <= SMALL_NUMBER
		|| !FMath::IsFinite(speedThreshold) || !FMath::IsFinite(angularSpeedThreshold)) return false;
	const double distance = FMath::Max(0.0f,speedThreshold)*double(deltaTime);
	const double angle = FMath::DegreesToRadians(FMath::Max(0.0f,angularSpeedThreshold))*double(deltaTime);
	return FVector::DistSquared(previous.GetLocation(),current.GetLocation()) > distance*distance
		|| previous.GetRotation().AngularDistance(current.GetRotation()) > angle;
}

float UPortalViewSubsystem::ApplyMovementCaptureLimit(float normalRate, float movingRate, bool bEnabled, bool bMoving)
{
	if (!FMath::IsFinite(normalRate)) return -1.0f;
	normalRate = FMath::Max(0.0f,normalRate);
	if (!bEnabled || !bMoving) return normalRate;
	if (!FMath::IsFinite(movingRate)) return -1.0f;
	if (movingRate <= 0.0f) return -1.0f;
	// An optimization cap may only reduce the chosen high/medium/low rate, never raise it.
	return normalRate > 0.0f ? FMath::Min(normalRate,movingRate) : movingRate;
}

float UPortalViewSubsystem::CalculateCaptureRemainder(float accumulatedSeconds, float intervalSeconds, bool bForced)
{
	if (bForced || !FMath::IsFinite(accumulatedSeconds) || !FMath::IsFinite(intervalSeconds)
		|| accumulatedSeconds <= 0.0f || intervalSeconds <= 0.0f) return 0.0f;
	// Resetting to zero rounds every interval up to a whole number of game frames:
	// e.g. 30fps captures become 20fps at 40fps gameplay. Keep the fractional time instead.
	// Drop whole missed intervals so a budget delay/hitch never causes a capture burst.
	return FMath::Max(0.0f, FMath::Fmod(accumulatedSeconds, intervalSeconds));
}

bool UPortalViewSubsystem::UpdateSmoothCaptureState(FPortalViewInstance& view, const UTeleportDataAsset& settings,
	bool bSpatialView, bool bObserverMoving, float deltaTime)
{
	if (!bSpatialView || !settings.bPrioritizeSpatialPortalSmoothness || settings.bReducePortalUpdateRateWhenStill
		|| !FMath::IsFinite(deltaTime) || deltaTime <= 0.0f)
	{
		view.smoothCaptureRemaining = 0.0f;
		return false;
	}
	view.smoothCaptureRemaining = bObserverMoving ? 0.25f
		: FMath::Max(0.0f, view.smoothCaptureRemaining - deltaTime);
	return view.smoothCaptureRemaining > 0.0f;
}

int32 UPortalViewSubsystem::ResolveSmoothRenderTargetSize(int32 normalResolution, const UTeleportDataAsset& settings)
{
	if (!settings.bUseDynamicPortalResolution) return normalResolution;
	const int32 maximumResolution = FMath::Max(128, settings.portalViewRenderTargetSize);
	const int32 lowResolution = FMath::Clamp(settings.portalViewLowResolution, 128, maximumResolution);
	const int32 mediumResolution = FMath::Clamp(settings.portalViewMediumResolution, lowResolution, maximumResolution);
	// Skip duplicate tiers (e.g. Max=Medium=1024), without inventing a new resolution or lowering Low.
	if (normalResolution > mediumResolution) return mediumResolution;
	return FMath::Min(normalResolution, lowResolution);
}

bool UPortalViewSubsystem::ShouldResetCaptureHistory(const FPortalCaptureHistory& previous,
	const FPortalCaptureHistory& current, bool bHasCaptured)
{
	// Reference-frame edits are discontinuities even when swapping equal-position exits.
	// Ordinary observer motion retains query history; UE also rejects history on large capture-camera movement.
	return !bHasCaptured || previous.exitTarget != current.exitTarget
		|| previous.observerCutRevision != current.observerCutRevision
		|| previous.bSpatialView != current.bSpatialView
		|| previous.bOcclusionEnabled != current.bOcclusionEnabled
		|| previous.bPersistentState != current.bPersistentState
		|| !previous.entryFrame.Equals(current.entryFrame, 0.01f)
		|| !previous.exitFrame.Equals(current.exitFrame, 0.01f);
}

void UPortalViewSubsystem::ConfigureCaptureAppearance(USceneCaptureComponent2D& capture, bool bSpatialView, bool bHighQuality,
	bool bEnableOcclusionCulling)
{
	// FinalColorLDR would bake exposure + filmic tone mapping, then the opaque portal's emissive
	// would be exposed/tone mapped AGAIN by the player view. Keep spatial textures scene-linear.
	capture.CaptureSource = bSpatialView || bHighQuality ? SCS_SceneColorHDR : SCS_FinalColorLDR;
	capture.bMainViewFamily = bSpatialView;
	capture.bInheritMainViewCameraPostProcessSettings = bSpatialView;
	capture.bMainViewCamera = false;
	capture.bMainViewResolution = false;
	// Main-view PP is inherited by the renderer only for deferred spatial captures. No second set of
	// camera blendables/color grades/exposure is baked into the scene-color texture.
	capture.PostProcessBlendWeight = bSpatialView ? 0.0f : 1.0f;
	// Manual GPU occlusion needs a ViewState even with all expensive lighting disabled.
	// Keep the existing high-quality temporal state, independently of the occlusion switch.
	capture.bAlwaysPersistRenderingState = bSpatialView && (bHighQuality || bEnableOcclusionCulling);
	capture.ShowFlags.SetDisableOcclusionQueries(!bEnableOcclusionCulling);
	// Preserve the existing lighting-cost switches, independently of the HDR texture format.
	capture.ShowFlags.SetDynamicShadows(bHighQuality);
	capture.ShowFlags.SetLumenGlobalIllumination(bHighQuality);
	capture.ShowFlags.SetLumenReflections(bHighQuality);
	capture.ShowFlags.SetScreenSpaceReflections(bHighQuality);
	capture.ShowFlags.SetAmbientOcclusion(bHighQuality);
	capture.ShowFlags.SetContactShadows(bHighQuality);
	// Fog directly changes scene brightness/color, so spatial windows cannot silently drop it.
	capture.ShowFlags.SetFog(true);
	capture.ShowFlags.SetVolumetricFog(bSpatialView || bHighQuality);
	capture.ShowFlags.SetMotionBlur(!bSpatialView && bHighQuality);
	capture.ShowFlags.SetDepthOfField(!bSpatialView && bHighQuality);
	capture.ShowFlags.SetLensFlares(!bSpatialView && bHighQuality);
	capture.ShowFlags.SetBloom(!bSpatialView && bHighQuality);
	capture.ShowFlags.SetEyeAdaptation(!bSpatialView && bHighQuality);
}

TStatId UPortalViewSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPortalViewSubsystem, STATGROUP_Tickables);
}

FPortalViewInstance& UPortalViewSubsystem::FindOrCreateView(AOneWayTeleportActor* portal)
{
	const TWeakObjectPtr<AOneWayTeleportActor> portalKey(portal);
	if (FPortalViewInstance* existing = portalViews.Find(portalKey))
	{
		if (IsValid(existing->sceneCapture) && existing->sceneCapture->IsRegistered())
		{
			return *existing;
		}

		ClearPortalView(portal, *existing);
		portalViews.Remove(portalKey);
	}

	FPortalViewInstance& newView = portalViews.Add(portalKey);
	if (!IsValid(portal) || !IsValid(portal->GetWorld()) || portal->GetWorld()->bIsTearingDown) return newView;
	// SceneCapture를 Subsystem 소유로 등록하면 레벨 전환/GC 중 Subsystem은 이미
	// unreachable인데 렌더 업데이트가 남을 수 있다. 포탈 Actor의 런타임 컴포넌트로
	// 소유시켜 Actor의 종료 순서에 맞춰 안전하게 unregister 되도록 한다.
	newView.sceneCapture = NewObject<USceneCaptureComponent2D>(portal, NAME_None, RF_Transient);
	if (!IsValid(newView.sceneCapture) || !IsValid(portal->GetWorld()) || portal->GetWorld()->bIsTearingDown)
	{
		newView.sceneCapture = nullptr;
		return newView;
	}
	portal->AddInstanceComponent(newView.sceneCapture);
	newView.sceneCapture->bCaptureEveryFrame = false;
	newView.sceneCapture->bCaptureOnMovement = false;
	newView.sceneCapture->bEnableClipPlane = true;
	newView.sceneCapture->RegisterComponent();
	if (!IsValid(newView.sceneCapture) || !newView.sceneCapture->IsRegistered())
	{
		if (IsValid(newView.sceneCapture)) newView.sceneCapture->DestroyComponent();
		newView.sceneCapture = nullptr;
	}
	return newView;
}

int32 UPortalViewSubsystem::ResolvePortalRenderTargetSize(
	const FPortalViewInstance& view,
	const UTeleportDataAsset& settings,
	float screenCoverage) const
{
	const int32 maximumResolution = FMath::Max(settings.portalViewRenderTargetSize, 128);
	if (!settings.bUseDynamicPortalResolution)
	{
		return maximumResolution;
	}

	const int32 lowResolution = FMath::Min(
		FMath::Max(settings.portalViewLowResolution, 128),
		maximumResolution);
	const int32 mediumResolution = FMath::Clamp(
		settings.portalViewMediumResolution,
		lowResolution,
		maximumResolution);
	const float lowThreshold = FMath::Clamp(settings.portalViewLowCoverageThreshold, 0.0f, 1.0f);
	const float highThreshold = FMath::Clamp(
		settings.portalViewHighCoverageThreshold,
		lowThreshold,
		1.0f);
	const float hysteresis = FMath::Max(0.0f, settings.portalViewResolutionHysteresis);

	// Equal configured sizes collapse to two (or one) tiers; do not get stuck at Max=Medium.
	if (lowResolution == maximumResolution) return maximumResolution;
	if (mediumResolution == maximumResolution || mediumResolution == lowResolution)
	{
		const float threshold = mediumResolution == maximumResolution ? lowThreshold : highThreshold;
		if (view.normalResolution <= 0)
			return screenCoverage < threshold ? lowResolution : maximumResolution;
		return view.normalResolution <= lowResolution
			? (screenCoverage >= threshold + hysteresis ? maximumResolution : lowResolution)
			: (screenCoverage < threshold - hysteresis ? lowResolution : maximumResolution);
	}

	// 현재 단계에 따라 서로 다른 진입/이탈 경계를 사용해 경계 근처의 리소스 재생성을 막습니다.
	if (view.normalResolution <= 0)
	{
		return screenCoverage < lowThreshold
			? lowResolution
			: (screenCoverage < highThreshold ? mediumResolution : maximumResolution);
	}
	if (view.normalResolution <= lowResolution)
	{
		return screenCoverage >= lowThreshold + hysteresis ? mediumResolution : lowResolution;
	}
	if (view.normalResolution < maximumResolution)
	{
		if (screenCoverage < lowThreshold - hysteresis)
		{
			return lowResolution;
		}
		return screenCoverage >= highThreshold + hysteresis ? maximumResolution : mediumResolution;
	}

	return screenCoverage < highThreshold - hysteresis ? mediumResolution : maximumResolution;
}

float UPortalViewSubsystem::ResolvePortalUpdateRate(
	const FPortalViewInstance& view,
	const UTeleportDataAsset& settings,
	bool bObserverMoving,
	float screenCoverage) const
{
	if (view.smoothCaptureRemaining > 0.0f && settings.bPrioritizeSpatialPortalSmoothness
		&& !settings.bReducePortalUpdateRateWhenStill) return 0.0f;
	float updateRate = settings.portalViewUpdateRate;
	if (settings.bUseDynamicPortalResolution)
	{
		const float lowThreshold = FMath::Clamp(settings.portalViewLowCoverageThreshold, 0.0f, 1.0f);
		const float highThreshold = FMath::Clamp(
			settings.portalViewHighCoverageThreshold,
			lowThreshold,
			1.0f);
		if (screenCoverage < lowThreshold)
		{
			updateRate = settings.portalViewLowUpdateRate;
		}
		else if (screenCoverage < highThreshold)
		{
			updateRate = settings.portalViewMediumUpdateRate;
		}
	}

	if (!view.bHasCaptured)
	{
		return FMath::Max(updateRate, 0.0f);
	}
	return ApplyMovementCaptureLimit(updateRate, settings.portalViewStillUpdateRate,
		settings.bReducePortalUpdateRateWhenStill, bObserverMoving);
}

bool UPortalViewSubsystem::EnsureRenderTarget(
	FPortalViewInstance& view,
	int32 size,
	bool bUseHDRCapture)
{
	if (!IsValid(view.sceneCapture) || !view.sceneCapture->IsRegistered()
		|| !IsValid(view.sceneCapture->GetOwner()) || !IsValid(GetWorld()) || GetWorld()->bIsTearingDown)
	{
		return false;
	}

	const int32 maximumTextureDimension = FMath::Max(
		static_cast<int32>(GetMax2DTextureDimension()),
		128);
	const int32 clampedSize = FMath::Clamp(size, 128, maximumTextureDimension);
	if (clampedSize != size)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PortalView] Render Target 크기 %d를 장치 지원 범위인 %d로 제한했습니다."),
			size,
			clampedSize);
	}
	const ETextureRenderTargetFormat desiredFormat = bUseHDRCapture
		? ETextureRenderTargetFormat::RTF_RGBA16f
		: ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
	if (IsValid(view.renderTarget)
		&& view.renderTarget->SizeX == clampedSize
		&& view.renderTarget->SizeY == clampedSize
		&& view.renderTarget->RenderTargetFormat == desiredFormat)
	{
		return false;
	}

	// RenderTarget도 포탈 수명에 묶인 임시 런타임 리소스입니다.
	UTextureRenderTarget2D* newRenderTarget = NewObject<UTextureRenderTarget2D>(
		view.sceneCapture->GetOwner(),
		NAME_None,
		RF_Transient);
	if (!IsValid(newRenderTarget))
	{
		return false;
	}

	newRenderTarget->ClearColor = FLinearColor::Black;
	newRenderTarget->RenderTargetFormat = desiredFormat;
	// HDR is scene radiance, not display-referred sRGB. Let the player's camera do the final conversion.
	if (bUseHDRCapture)
	{
		newRenderTarget->bForceLinearGamma = true;
		newRenderTarget->TargetGamma = 1.0f;
	}
	newRenderTarget->InitAutoFormat(clampedSize, clampedSize);
	newRenderTarget->UpdateResourceImmediate(true);
	view.renderTarget = newRenderTarget;
	view.currentResolution = clampedSize;
	return true;
}

void UPortalViewSubsystem::ClearPortalView(AOneWayTeleportActor* portal, FPortalViewInstance& view)
{
	// GC owns unreachable/destroying objects. Calling component/render setters on them asserts.
	const EObjectFlags destroyingFlags = RF_BeginDestroyed | RF_FinishDestroyed;
	if (IsValid(portal) && !portal->IsUnreachable() && !portal->HasAnyFlags(destroyingFlags))
	{
		portal->ClearPortalView();
	}
	if (IsValid(view.sceneCapture) && !view.sceneCapture->IsUnreachable()
		&& !view.sceneCapture->HasAnyFlags(destroyingFlags))
	{
		AActor* owner = view.sceneCapture->GetOwner();
		if (!owner || (IsValid(owner) && !owner->IsUnreachable() && !owner->HasAnyFlags(destroyingFlags)))
		{
			view.sceneCapture->TextureTarget = nullptr;
			view.sceneCapture->HiddenActors.Empty();
			view.sceneCapture->DestroyComponent();
		}
	}
	view.sceneCapture = nullptr;
	view.renderTarget = nullptr;
	view.currentResolution = 0;
	view.normalResolution = 0;
	view.smoothCaptureRemaining = 0.0f;
	view.captureAccumulator = 0.0f;
	view.bHasCaptured = false;
	view.lastCaptureHistory = FPortalCaptureHistory();
}

void UPortalViewSubsystem::ClearAllPortalViews()
{
	if (bClearingViews) return;
	TGuardValue<bool> clearing(bClearingViews,true);
	bHasObserverTransform = false;
	lastObserverPawn.Reset();
	lastObserverViewTarget.Reset();
	for (auto& [portal, view] : portalViews)
	{
		ClearPortalView(portal.Get(), view);
	}
	portalViews.Empty();
}

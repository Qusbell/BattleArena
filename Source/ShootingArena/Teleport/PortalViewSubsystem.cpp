#include "PortalViewSubsystem.h"

#include "OneWayTeleportActor.h"
#include "TeleportDataAsset.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

namespace PortalViewPrivate
{
	struct FPortalCandidate
	{
		TObjectPtr<AOneWayTeleportActor> portal;
		float distance = 0.0f;
		float score = 0.0f;
	};

	struct FReadyPortalCapture
	{
		TObjectPtr<AOneWayTeleportActor> portal;
		float score = 0.0f;
		float accumulatedWaitTime = 0.0f;
	};
}

void UPortalViewSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	UWorld* world = GetWorld();
	if (!IsValid(world) || world->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

}

void UPortalViewSubsystem::Deinitialize()
{
	ClearAllPortalViews();
	Super::Deinitialize();
}

void UPortalViewSubsystem::Tick(float deltaTime)
{
	UWorld* world = GetWorld();
	if (!IsValid(world) || world->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	APlayerController* playerController = world->GetFirstPlayerController();
	if (!IsValid(playerController) || !playerController->IsLocalController()
		|| !IsValid(playerController->PlayerCameraManager))
	{
		ClearAllPortalViews();
		return;
	}

	const FVector cameraLocation = playerController->PlayerCameraManager->GetCameraLocation();
	const FRotator cameraRotation = playerController->PlayerCameraManager->GetCameraRotation();
	const FVector aimDirection = cameraRotation.Vector();

	TArray<PortalViewPrivate::FPortalCandidate> candidates;

	for (TActorIterator<AOneWayTeleportActor> iterator(world); iterator; ++iterator)
	{
		AOneWayTeleportActor* portal = *iterator;
		float distance = 0.0f;
		float score = 0.0f;
		if (IsValid(portal)
			&& portal->CanDisplayPortalView(
				playerController,
				cameraLocation,
				aimDirection,
				distance,
				score)
			)
		{
			candidates.Add({ portal, distance, score });
		}
	}

	if (candidates.IsEmpty())
	{
		ClearAllPortalViews();
		return;
	}

	candidates.Sort([](const PortalViewPrivate::FPortalCandidate& left, const PortalViewPrivate::FPortalCandidate& right)
	{
		return left.score > right.score;
	});

	const UTeleportDataAsset* firstSettings = candidates[0].portal->GetTeleportDataAsset();
	const int32 maxViews = IsValid(firstSettings) ? firstSettings->maxSimultaneousPortalViews : 0;
	if (maxViews > 0 && candidates.Num() > maxViews)
	{
		candidates.SetNum(maxViews);
	}

	TSet<TWeakObjectPtr<AOneWayTeleportActor>> visiblePortals;
	TArray<PortalViewPrivate::FReadyPortalCapture> readyCaptures;
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
		FPortalViewInstance& view = FindOrCreateView(portal);
		EnsureRenderTarget(
			view,
			settings->portalViewRenderTargetSize,
			settings->bUseHighQualityPortalCapture);
		if (!IsValid(view.renderTarget) || !IsValid(view.sceneCapture))
		{
			continue;
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
		portal->ApplyPortalView(view.renderTarget, blurStrength, portal->GetPortalViewOpacity(candidate.distance));

		view.captureAccumulator += deltaTime;
		const float captureInterval = settings->portalViewUpdateRate > 0.0f
			? 1.0f / settings->portalViewUpdateRate
			: 0.0f;
		if (captureInterval > 0.0f && view.captureAccumulator < captureInterval)
		{
			continue;
		}

		readyCaptures.Add({ portal, candidate.score, view.captureAccumulator });
	}

	// 예산에서 밀린 포탈은 누적 대기 시간이 계속 증가합니다. 다음 프레임에는 가장 오래
	// 기다린 포탈부터 처리하므로, 가까운 포탈만 계속 갱신되고 나머지가 멈추는 현상을 막습니다.
	readyCaptures.Sort([](
		const PortalViewPrivate::FReadyPortalCapture& left,
		const PortalViewPrivate::FReadyPortalCapture& right)
	{
		if (!FMath::IsNearlyEqual(left.accumulatedWaitTime, right.accumulatedWaitTime))
		{
			return left.accumulatedWaitTime > right.accumulatedWaitTime;
		}

		return left.score > right.score;
	});

	const int32 captureBudget = IsValid(firstSettings)
		? FMath::Max(0, firstSettings->maxPortalCapturesPerFrame)
		: 0;
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
			|| !IsValid(portal->GetExitTarget()))
		{
			continue;
		}

		view->captureAccumulator = 0.0f;
		const UTeleportDataAsset* settings = portal->GetTeleportDataAsset();
		if (!IsValid(settings))
		{
			continue;
		}

		const bool bHighQuality = settings->bUseHighQualityPortalCapture;
		view->sceneCapture->CaptureSource = bHighQuality
			? ESceneCaptureSource::SCS_SceneColorHDR
			: ESceneCaptureSource::SCS_FinalColorLDR;

		// 포탈 전용 캡처에만 적용됩니다. 메인 카메라의 ShowFlags는 변경하지 않습니다.
		// 저품질 모드에서도 기본 조명과 Translucency는 유지해 출구 판독성을 보존합니다.
		view->sceneCapture->ShowFlags.SetDynamicShadows(bHighQuality);
		view->sceneCapture->ShowFlags.SetLumenGlobalIllumination(bHighQuality);
		view->sceneCapture->ShowFlags.SetLumenReflections(bHighQuality);
		view->sceneCapture->ShowFlags.SetScreenSpaceReflections(bHighQuality);
		view->sceneCapture->ShowFlags.SetAmbientOcclusion(bHighQuality);
		view->sceneCapture->ShowFlags.SetContactShadows(bHighQuality);
		view->sceneCapture->ShowFlags.SetVolumetricFog(bHighQuality);
		view->sceneCapture->ShowFlags.SetMotionBlur(bHighQuality);
		view->sceneCapture->ShowFlags.SetDepthOfField(bHighQuality);
		view->sceneCapture->ShowFlags.SetLensFlares(bHighQuality);
		view->sceneCapture->ShowFlags.SetBloom(bHighQuality);
		view->sceneCapture->ShowFlags.SetEyeAdaptation(bHighQuality);

		const FTransform cameraTransform(cameraRotation, cameraLocation);
		view->sceneCapture->SetWorldTransform(portal->GetPortalViewCameraTransform(cameraTransform));
		view->sceneCapture->TextureTarget = view->renderTarget;
		view->sceneCapture->ClipPlaneBase = portal->GetExitTarget()->GetActorLocation();
		view->sceneCapture->ClipPlaneNormal = portal->GetExitLaunchForward();
		view->sceneCapture->HiddenActors.Empty();
		view->sceneCapture->HiddenActors.Add(portal);
		if (AOneWayTeleportActor* exitPortal = Cast<AOneWayTeleportActor>(portal->GetExitTarget()))
		{
			view->sceneCapture->HiddenActors.Add(exitPortal);
		}
		view->sceneCapture->CaptureScene();
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

TStatId UPortalViewSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPortalViewSubsystem, STATGROUP_Tickables);
}

FPortalViewInstance& UPortalViewSubsystem::FindOrCreateView(AOneWayTeleportActor* portal)
{
	const TWeakObjectPtr<AOneWayTeleportActor> portalKey(portal);
	if (FPortalViewInstance* existing = portalViews.Find(portalKey))
	{
		return *existing;
	}

	FPortalViewInstance& newView = portalViews.Add(portalKey);
	// SceneCapture를 Subsystem 소유로 등록하면 레벨 전환/GC 중 Subsystem은 이미
	// unreachable인데 렌더 업데이트가 남을 수 있다. 포탈 Actor의 런타임 컴포넌트로
	// 소유시켜 Actor의 종료 순서에 맞춰 안전하게 unregister 되도록 한다.
	newView.sceneCapture = NewObject<USceneCaptureComponent2D>(portal, NAME_None, RF_Transient);
	portal->AddInstanceComponent(newView.sceneCapture);
	newView.sceneCapture->bCaptureEveryFrame = false;
	newView.sceneCapture->bCaptureOnMovement = false;
	newView.sceneCapture->bEnableClipPlane = true;
	newView.sceneCapture->RegisterComponent();
	return newView;
}

void UPortalViewSubsystem::EnsureRenderTarget(
	FPortalViewInstance& view,
	int32 size,
	bool bUseHighQualityCapture)
{
	const int32 clampedSize = FMath::Clamp(size, 128, 2048);
	const ETextureRenderTargetFormat desiredFormat = bUseHighQualityCapture
		? ETextureRenderTargetFormat::RTF_RGBA16f
		: ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
	if (IsValid(view.renderTarget)
		&& view.renderTarget->SizeX == clampedSize
		&& view.renderTarget->SizeY == clampedSize
		&& view.renderTarget->RenderTargetFormat == desiredFormat)
	{
		return;
	}

	// RenderTarget도 포탈 수명에 묶인 임시 런타임 리소스입니다.
	view.renderTarget = NewObject<UTextureRenderTarget2D>(view.sceneCapture->GetOwner(), NAME_None, RF_Transient);
	view.renderTarget->ClearColor = FLinearColor::Black;
	view.renderTarget->RenderTargetFormat = desiredFormat;
	view.renderTarget->InitAutoFormat(clampedSize, clampedSize);
	view.renderTarget->UpdateResourceImmediate(true);
}

void UPortalViewSubsystem::ClearPortalView(AOneWayTeleportActor* portal, FPortalViewInstance& view)
{
	if (IsValid(portal))
	{
		portal->ClearPortalView();
	}
	if (IsValid(view.sceneCapture))
	{
		view.sceneCapture->DestroyComponent();
	}
	view.sceneCapture = nullptr;
	view.renderTarget = nullptr;
}

void UPortalViewSubsystem::ClearAllPortalViews()
{
	for (auto& [portal, view] : portalViews)
	{
		ClearPortalView(portal.Get(), view);
	}
	portalViews.Empty();
}

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
	for (const PortalViewPrivate::FPortalCandidate& candidate : candidates)
	{
		AOneWayTeleportActor* portal = candidate.portal.Get();
		const UTeleportDataAsset* settings = portal->GetTeleportDataAsset();
		if (!IsValid(portal) || !IsValid(settings))
		{
			continue;
		}

		visiblePortals.Add(portal);
		FPortalViewInstance& view = FindOrCreateView(portal);
		EnsureRenderTarget(view, settings->portalViewRenderTargetSize);
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

		view.captureAccumulator = 0.0f;
		const FTransform cameraTransform(cameraRotation, cameraLocation);
		view.sceneCapture->SetWorldTransform(portal->GetPortalViewCameraTransform(cameraTransform));
		view.sceneCapture->TextureTarget = view.renderTarget;
		view.sceneCapture->ClipPlaneBase = portal->GetExitTarget()->GetActorLocation();
		view.sceneCapture->ClipPlaneNormal = portal->GetExitLaunchForward();
		view.sceneCapture->HiddenActors.Empty();
		view.sceneCapture->HiddenActors.Add(portal);
		if (AOneWayTeleportActor* exitPortal = Cast<AOneWayTeleportActor>(portal->GetExitTarget()))
		{
			view.sceneCapture->HiddenActors.Add(exitPortal);
		}
		view.sceneCapture->CaptureScene();
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
	newView.sceneCapture = NewObject<USceneCaptureComponent2D>(this);
	newView.sceneCapture->bCaptureEveryFrame = false;
	newView.sceneCapture->bCaptureOnMovement = false;
	newView.sceneCapture->bEnableClipPlane = true;
	newView.sceneCapture->RegisterComponentWithWorld(GetWorld());
	return newView;
}

void UPortalViewSubsystem::EnsureRenderTarget(FPortalViewInstance& view, int32 size)
{
	const int32 clampedSize = FMath::Clamp(size, 128, 2048);
	if (IsValid(view.renderTarget) && view.renderTarget->SizeX == clampedSize && view.renderTarget->SizeY == clampedSize)
	{
		return;
	}

	view.renderTarget = NewObject<UTextureRenderTarget2D>(this);
	view.renderTarget->ClearColor = FLinearColor::Black;
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

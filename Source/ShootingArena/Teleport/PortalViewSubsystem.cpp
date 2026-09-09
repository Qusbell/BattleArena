#include "PortalViewSubsystem.h"

#include "OneWayTeleportActor.h"
#include "TeleportDataAsset.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"

void UPortalViewSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	UWorld* world = GetWorld();
	if (!IsValid(world) || world->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	sceneCapture = NewObject<USceneCaptureComponent2D>(this, TEXT("PortalViewSceneCapture"));
	sceneCapture->bCaptureEveryFrame = false;
	sceneCapture->bCaptureOnMovement = false;
	sceneCapture->bEnableClipPlane = true;
	sceneCapture->RegisterComponentWithWorld(world);
}

void UPortalViewSubsystem::Deinitialize()
{
	ClearActivePortal();

	if (IsValid(sceneCapture))
	{
		sceneCapture->DestroyComponent();
	}

	sceneCapture = nullptr;
	renderTarget = nullptr;
	Super::Deinitialize();
}

void UPortalViewSubsystem::Tick(float deltaTime)
{
	UWorld* world = GetWorld();
	if (!IsValid(world) || !IsValid(sceneCapture) || world->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	APlayerController* playerController = world->GetFirstPlayerController();
	if (!IsValid(playerController) || !playerController->IsLocalController()
		|| !IsValid(playerController->PlayerCameraManager))
	{
		ClearActivePortal();
		return;
	}

	const FVector cameraLocation = playerController->PlayerCameraManager->GetCameraLocation();
	const FRotator cameraRotation = playerController->PlayerCameraManager->GetCameraRotation();
	const FVector aimDirection = cameraRotation.Vector();

	AOneWayTeleportActor* bestPortal = nullptr;
	float bestDistance = 0.0f;
	float bestScore = -FLT_MAX;

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
			&& score > bestScore)
		{
			bestPortal = portal;
			bestDistance = distance;
			bestScore = score;
		}
	}

	if (!IsValid(bestPortal))
	{
		ClearActivePortal();
		return;
	}

	if (activePortal.Get() != bestPortal)
	{
		ClearActivePortal();
		activePortal = bestPortal;
		captureAccumulator = 0.0f;
	}

	const UTeleportDataAsset* settings = bestPortal->GetTeleportDataAsset();
	if (!IsValid(settings))
	{
		ClearActivePortal();
		return;
	}

	EnsureRenderTarget(settings->portalViewRenderTargetSize);
	if (!IsValid(renderTarget))
	{
		return;
	}

	// View Distance 바깥은 CanDisplayPortalView에서 이미 제외됩니다. 이 안에서도
	// Clarity Start Distance보다 멀면 Far Blur를 유지하고, 그 거리부터 입구까지
	// 점차 Near Blur로 바꿉니다.
	const float clarityStartDistance = FMath::Clamp(
		settings->clarityStartDistance,
		KINDA_SMALL_NUMBER,
		settings->viewDistance);
	const float blurAlpha = FMath::Clamp(
		bestDistance / clarityStartDistance,
		0.0f,
		1.0f);
	const float blurStrength = FMath::Lerp(
		settings->blurAtNearDistance,
		settings->blurAtFarDistance,
		blurAlpha);
	const float screenOpacity = bestPortal->GetPortalViewOpacity(bestDistance);
	bestPortal->ApplyPortalView(renderTarget, blurStrength, screenOpacity);

	captureAccumulator += deltaTime;
	const float captureInterval = settings->portalViewUpdateRate > 0.0f
		? 1.0f / settings->portalViewUpdateRate
		: 0.0f;
	if (captureInterval > 0.0f && captureAccumulator < captureInterval)
	{
		return;
	}

	captureAccumulator = 0.0f;
	const FTransform cameraTransform(cameraRotation, cameraLocation);
	const FTransform portalCameraTransform = bestPortal->GetPortalViewCameraTransform(cameraTransform);
	sceneCapture->SetWorldTransform(portalCameraTransform);
	sceneCapture->TextureTarget = renderTarget;
	sceneCapture->ClipPlaneBase = bestPortal->GetExitTarget()->GetActorLocation();
	sceneCapture->ClipPlaneNormal = bestPortal->GetExitLaunchForward();
	sceneCapture->HiddenActors.Empty();
	sceneCapture->HiddenActors.Add(bestPortal);
	if (AOneWayTeleportActor* exitPortal = Cast<AOneWayTeleportActor>(bestPortal->GetExitTarget()))
	{
		sceneCapture->HiddenActors.Add(exitPortal);
	}
	sceneCapture->CaptureScene();
}

TStatId UPortalViewSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPortalViewSubsystem, STATGROUP_Tickables);
}

void UPortalViewSubsystem::EnsureRenderTarget(int32 size)
{
	const int32 clampedSize = FMath::Clamp(size, 128, 2048);
	if (IsValid(renderTarget) && renderTarget->SizeX == clampedSize && renderTarget->SizeY == clampedSize)
	{
		return;
	}

	renderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("PortalViewRenderTarget"));
	renderTarget->ClearColor = FLinearColor::Black;
	renderTarget->InitAutoFormat(clampedSize, clampedSize);
	renderTarget->UpdateResourceImmediate(true);
}

void UPortalViewSubsystem::ClearActivePortal()
{
	if (AOneWayTeleportActor* portal = activePortal.Get())
	{
		portal->ClearPortalView();
	}

	activePortal.Reset();
	captureAccumulator = 0.0f;
}

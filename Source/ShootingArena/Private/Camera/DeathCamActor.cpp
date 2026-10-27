#include "Camera/DeathCamActor.h"

#include "Camera/DeathCamDataAsset.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

DEFINE_LOG_CATEGORY_STATIC(LogDeathCamActor, Log, All);

ADeathCamActor::ADeathCamActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// DeathCam Actor는 owning client에만 로컬 생성합니다.
	SetReplicates(false);
}

void ADeathCamActor::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (!bDeathCamActive)
	{
		return;
	}

	UpdateDeathCam(deltaSeconds);
}

void ADeathCamActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	// StopDeathCam을 거치지 않고 World 종료/강제 Destroy가 발생해도
	// Killer의 CustomDepth 상태가 남지 않도록 항상 원복합니다.
	RemoveKillerHighlight();

	Super::EndPlay(endPlayReason);
}

bool ADeathCamActor::InitializeDeathCam(
	const FVector& inDeathLocation,
	AActor* inOtherActor,
	APlayerController* inPlayerController,
	UDeathCamDataAsset* inDeathCamData,
	AActor* inDeadActorToIgnore)
{
	if (!IsValid(inPlayerController) || !GetWorld())
	{
		UE_LOG(LogDeathCamActor, Warning, TEXT("InitializeDeathCam failed: PlayerController or World is invalid."));
		return false;
	}

	ownerPlayerController = inPlayerController;
	otherActor = inOtherActor;
	deadActorToIgnore = inDeadActorToIgnore;
	deathLocation = inDeathLocation;

	CacheSettings(inDeathCamData);

	// 사망 당시 저장한 위치를 기준으로 사용하며 Ragdoll 이동을 따라가지 않습니다.
	centerLocation = deathLocation + (FVector::UpVector * centerOffset);

	const FVector killerDirection = CalculateKillerDirection();

	if (UCameraComponent* cameraComponent = GetCameraComponent())
	{
		baseCameraFov = FMath::Clamp(cameraComponent->FieldOfView, 5.0f, maxFramingFov);
		// 시야각 계산과 실제 투영이 같은 수평 FOV 기준을 사용하도록 맞춥니다.
		cameraComponent->bOverrideAspectRatioAxisConstraint = true;
		cameraComponent->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
	}
	// 진입 프레임에는 기본 위치 하나만 검사하고, 전체 후보 탐색은 이후 프레임에 분산합니다.
	cameraTargetLocation = ResolveCameraCandidateLocation(
		CalculateBaseCameraLocation(killerDirection), FindCameraSweepStart());
	bCameraMovementBlocked = false;
	BeginCameraSearch(killerDirection);
	float initialFov = baseCameraFov;
	const FVector initialCameraLocation = cameraTargetLocation;
	const FRotator initialRotation = CalculateFramingRotation(initialCameraLocation, initialFov);
	if (UCameraComponent* cameraComponent = GetCameraComponent())
	{
		cameraComponent->SetFieldOfView(initialFov);
	}

	SetActorLocationAndRotation(
		initialCameraLocation,
		initialRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	// Post Process Material은 이 로컬 DeathCam 카메라에만 적용합니다.
	// 따라서 다른 플레이어의 일반 카메라에는 Killer Highlight가 보이지 않습니다.
	ApplyKillerHighlightPostProcess();
	ApplyKillerHighlight();

	bDeathCamActive = true;
	SetActorTickEnabled(true);

	UE_LOG(LogDeathCamActor, Log,
		TEXT("DeathCam initialized. Center=%s Killer=%s MaxDistance=%.1f"),
		*centerLocation.ToString(),
		*GetNameSafe(otherActor),
		maxDistance);

	return true;
}

void ADeathCamActor::StopDeathCam()
{
	if (!bDeathCamActive && killerHighlightStates.IsEmpty())
	{
		return;
	}

	bDeathCamActive = false;
	bCameraSearchInProgress = false;
	SetActorTickEnabled(false);
	RemoveKillerHighlight();
}

void ADeathCamActor::CacheSettings(UDeathCamDataAsset* inDeathCamData)
{
	if (!IsValid(inDeathCamData))
	{
		UE_LOG(LogDeathCamActor, Warning, TEXT("DeathCamDataAsset is not assigned. Using built-in default values."));
		return;
	}

	centerOffset = FMath::IsFinite(inDeathCamData->centerOffset) ? inDeathCamData->centerOffset : 70.0f;
	cameraMoveInterpSpeed = FMath::IsFinite(inDeathCamData->cameraMoveInterpSpeed) ? inDeathCamData->cameraMoveInterpSpeed : 5.0f;
	bEnableKillerHighlight = inDeathCamData->bEnableKillerHighlight;
	killerHighlightMaterial = inDeathCamData->GetKillerHighlightMaterial();
	killerHighlightStencilValue = inDeathCamData->killerHighlightStencilValue;

	// Legacy OrbitDistance는 최종 기획에서 제거했습니다.
	// 기존 DataAsset에 initialCameraDistance가 0으로 저장되어 있어도 카메라가 Center에 붙지 않도록
	// 안전 기본값 600을 사용합니다. DataAsset을 한 번 열어 원하는 값을 명시적으로 저장하는 것을 권장합니다.
	initialCameraDistance = FMath::IsFinite(inDeathCamData->initialCameraDistance)
		&& inDeathCamData->initialCameraDistance > KINDA_SMALL_NUMBER
		? inDeathCamData->initialCameraDistance
		: 600.0f;

	initialCameraDistance = FMath::Max(initialCameraDistance, 0.0f);
	maxDistance = FMath::IsFinite(inDeathCamData->cameraMovementRadius)
		? FMath::Max(inDeathCamData->cameraMovementRadius, 50.0f) : 600.0f;
	cameraMoveInterpSpeed = FMath::Max(cameraMoveInterpSpeed, 0.0f);
	killerHighlightStencilValue = FMath::Clamp(killerHighlightStencilValue, 0, 255);

	// 에디터 외의 경로로 잘못된 값이 저장되어도 탐색 횟수가 무제한 늘어나지 않게 제한합니다.
	cameraSearchInterval = FMath::IsFinite(inDeathCamData->cameraSearchInterval)
		? FMath::Clamp(inDeathCamData->cameraSearchInterval, 0.05f, 5.0f) : 0.35f;
	cameraSearchDirectionCount = FMath::Clamp(inDeathCamData->cameraSearchDirectionCount, 4, 32);
	cameraSearchCandidatesPerFrame = FMath::Clamp(inDeathCamData->cameraSearchCandidatesPerFrame, 1, 4);
	cameraSearchKillerMoveThreshold = FMath::IsFinite(inDeathCamData->cameraSearchKillerMoveThreshold)
		? FMath::Clamp(inDeathCamData->cameraSearchKillerMoveThreshold, 0.0f, 1000000.0f) : 50.0f;
	cameraSearchForceRefreshInterval = FMath::IsFinite(inDeathCamData->cameraSearchForceRefreshInterval)
		? FMath::Clamp(inDeathCamData->cameraSearchForceRefreshInterval, cameraSearchInterval, 10.0f) : FMath::Max(1.0f, cameraSearchInterval);
	cameraTargetSwitchMargin = FMath::IsFinite(inDeathCamData->cameraTargetSwitchMargin)
		? FMath::Clamp(inDeathCamData->cameraTargetSwitchMargin, 0.0f, 1000000.0f) : 40.0f;
}

void ADeathCamActor::UpdateDeathCam(float deltaSeconds)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(DeathCam_Update);
	if (!IsValid(ownerPlayerController))
	{
		StopDeathCam();
		return;
	}

	timeSinceCameraSearch += FMath::Max(deltaSeconds, 0.0f);
	if (!bCameraSearchInProgress && ShouldBeginCameraSearch())
	{
		BeginCameraSearch(CalculateKillerDirection());
	}
	AdvanceCameraSearch();

	// 기획의 "즉시 이동하지 않고 부드럽게(Lerp) 이동"을 보간으로 처리합니다.
	const FVector interpolatedLocation = FMath::VInterpTo(
		GetActorLocation(),
		cameraTargetLocation,
		deltaSeconds,
		cameraMoveInterpSpeed);

	const FVector cameraMove = interpolatedLocation - GetActorLocation();
	const FVector newCameraLocation = MoveCameraWithCollision(cameraMove);
	bCameraMovementBlocked = !cameraMove.IsNearlyZero(0.01f)
		&& !newCameraLocation.Equals(interpolatedLocation, 1.0f);
	float framingFov = baseCameraFov;
	const FRotator newCameraRotation = CalculateFramingRotation(newCameraLocation, framingFov);
	if (UCameraComponent* cameraComponent = GetCameraComponent())
	{
		// 보간 중에도 화면 밖으로 나가지 않도록 필요한 시야각 확대는 즉시 적용합니다.
		const float currentFov = cameraComponent->FieldOfView;
		cameraComponent->SetFieldOfView(framingFov >= currentFov
			? framingFov : FMath::FInterpTo(currentFov, framingFov, deltaSeconds, 3.0f));
	}

	SetActorLocationAndRotation(
		newCameraLocation,
		newCameraRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
}

FVector ADeathCamActor::CalculateKillerDirection() const
{
	if (IsValid(otherActor))
	{
		const FVector toKiller = (otherActor->GetActorLocation() - deathLocation).GetSafeNormal2D();
		if (!toKiller.IsNearlyZero())
		{
			return toKiller.GetSafeNormal();
		}
	}

	// Killer가 없는 사망의 연출은 기획서에서 정의되지 않았습니다.
	// 다만 0 Vector로 인한 NaN/잘못된 Transform을 막기 위한 기술적 fallback만 둡니다.
	if (IsValid(deadActorToIgnore))
	{
		const FVector fallbackDirection = deadActorToIgnore->GetActorForwardVector().GetSafeNormal2D();
		if (!fallbackDirection.IsNearlyZero())
		{
			return fallbackDirection.GetSafeNormal();
		}
	}

	return FVector::ForwardVector;
}

FVector ADeathCamActor::CalculateBaseCameraLocation(const FVector& killerDirection) const
{
	return ClampCameraLocation(centerLocation - (killerDirection * initialCameraDistance));
}

FVector ADeathCamActor::FindCameraSweepStart() const
{
	FVector sweepStart = ClampCameraLocation(centerLocation);
	if (!GetWorld())
	{
		return sweepStart;
	}

	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(DeathCamSearchOrigin), false, this);
	queryParams.AddIgnoredActor(this);
	if (IsValid(deadActorToIgnore)) queryParams.AddIgnoredActor(deadActorToIgnore);
	if (IsValid(otherActor)) queryParams.AddIgnoredActor(otherActor);
	const FCollisionShape probeShape = FCollisionShape::MakeSphere(cameraProbeRadius + cameraCollisionSkin);
	// 낮은 천장에 Center가 겹쳐도 사망 위치가 안전하면 그곳에서 후보를 찾습니다.
	if (GetWorld()->OverlapBlockingTestByChannel(sweepStart, FQuat::Identity, ECC_Camera, probeShape, queryParams)
		&& !GetWorld()->OverlapBlockingTestByChannel(deathLocation, FQuat::Identity, ECC_Camera, probeShape, queryParams))
	{
		sweepStart = deathLocation;
	}
	return sweepStart;
}

FVector ADeathCamActor::ResolveCameraCandidateLocation(
	const FVector& desiredCameraLocation, const FVector& sweepStart) const
{
	const FVector constrainedLocation = ClampCameraLocation(desiredCameraLocation);
	if (!GetWorld())
	{
		return constrainedLocation;
	}

	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(DeathCamCandidateSweep), false, this);
	queryParams.AddIgnoredActor(this);
	if (IsValid(deadActorToIgnore)) queryParams.AddIgnoredActor(deadActorToIgnore);
	if (IsValid(otherActor)) queryParams.AddIgnoredActor(otherActor);
	const FCollisionShape probeShape = FCollisionShape::MakeSphere(cameraProbeRadius + cameraCollisionSkin);
	FHitResult hitResult;
	const bool bHit = GetWorld()->SweepSingleByChannel(
		hitResult,
		sweepStart,
		constrainedLocation,
		FQuat::Identity,
		ECC_Camera,
		probeShape,
		queryParams);

	if (!bHit)
	{
		return constrainedLocation;
	}

	if (hitResult.bStartPenetrating)
	{
		// Center 자체가 벽에 겹친 경우에만 밀어내기를 시도합니다.
		// 이동 제한 안이면서 다른 벽에도 겹치지 않는 위치만 채택합니다.
		const FVector safeLocation = ClampCameraLocation(
			sweepStart + hitResult.Normal.GetSafeNormal() * (hitResult.PenetrationDepth + cameraSweepPullback));
		return GetWorld()->OverlapBlockingTestByChannel(
			safeLocation, FQuat::Identity, ECC_Camera, probeShape, queryParams)
			? sweepStart : safeLocation;
	}

	// 법선 방향으로 밀지 않고, 검사한 이동 경로에서 접촉점보다 조금 앞에 멈춥니다.
	const FVector initialMove = constrainedLocation - sweepStart;
	const double safeTime = FMath::Max(0.0,
		static_cast<double>(hitResult.Time) - cameraSweepPullback / FMath::Max(initialMove.Size(), UE_DOUBLE_SMALL_NUMBER));
	return sweepStart + initialMove * safeTime;
}

float ADeathCamActor::ScoreCameraTarget(const FVector& candidate) const
{
	if (!GetWorld())
	{
		return -1.0e9f;
	}

	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(DeathCamCandidate), false, this);
	queryParams.AddIgnoredActor(this);
	if (IsValid(deadActorToIgnore)) queryParams.AddIgnoredActor(deadActorToIgnore);
	if (IsValid(otherActor)) queryParams.AddIgnoredActor(otherActor);
	if (GetWorld()->OverlapBlockingTestByChannel(candidate, FQuat::Identity, ECC_Camera,
		FCollisionShape::MakeSphere(cameraProbeRadius + cameraCollisionSkin), queryParams))
	{
		return -1.0e9f;
	}

	const FVector toDeathLocation = deathLocation - candidate;
	const float deathLocationDistance = static_cast<float>(toDeathLocation.Size());
	if (deathLocationDistance < minDeathLocationCameraDistance)
	{
		// 다른 선택지가 없으면 조금이라도 멀리 있는 후보를 선택합니다.
		return -1000000.0f + deathLocationDistance;
	}

	FHitResult visibilityHit;
	const bool bDeathLocationBlocked = GetWorld()->LineTraceSingleByChannel(
		visibilityHit, candidate, deathLocation, ECC_Visibility, queryParams);
	float score = bDeathLocationBlocked ? -400.0f : 400.0f;
	score += FMath::Min(deathLocationDistance, 300.0f) * 0.3f;

	if (bSearchHasKiller)
	{
		// 여러 프레임의 후보를 같은 조건에서 비교하도록 탐색 시작 시점의 위치를 사용합니다.
		const FVector toKiller = (searchKillerLocation - candidate).GetSafeNormal();
		// 둘 사이의 화면상 각도가 작을수록 같은 화면에 담기 쉽습니다.
		score += static_cast<float>(FVector::DotProduct(toDeathLocation.GetSafeNormal(), toKiller)) * 180.0f;
		score += static_cast<float>(FVector::DotProduct((deathLocation - candidate).GetSafeNormal2D(), searchKillerDirection)) * 30.0f;
	}
	return score;
}

void ADeathCamActor::BeginCameraSearch(const FVector& killerDirection)
{
	bCameraSearchInProgress = true;
	nextCameraSearchDirection = -2;
	timeSinceCameraSearch = 0.0f;
	searchKillerDirection = killerDirection;
	bSearchHasKiller = IsValid(otherActor);
	searchKillerLocation = bSearchHasKiller ? otherActor->GetActorLocation() : deathLocation;
	searchBestLocation = cameraTargetLocation;
	searchBestScore = -1.0e9f;
	searchCurrentTargetScore = -1.0e9f;
}

bool ADeathCamActor::ShouldBeginCameraSearch() const
{
	if (timeSinceCameraSearch < cameraSearchInterval)
	{
		return false;
	}
	if (bCameraMovementBlocked || timeSinceCameraSearch >= cameraSearchForceRefreshInterval)
	{
		return true;
	}
	const bool bHasKiller = IsValid(otherActor);
	return bHasKiller != bSearchHasKiller
		|| (bHasKiller && FVector::DistSquared(otherActor->GetActorLocation(), searchKillerLocation)
			> FMath::Square(cameraSearchKillerMoveThreshold));
}

void ADeathCamActor::AdvanceCameraSearch()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(DeathCam_CameraSearch);
	if (!bCameraSearchInProgress || !GetWorld())
	{
		return;
	}

	// 공통 검사와 현재 목표 평가도 예산에 포함해 탐색 시작/종료 프레임에 몰리지 않게 합니다.
	for (int32 workIndex = 0; workIndex < cameraSearchCandidatesPerFrame; ++workIndex)
	{
		if (nextCameraSearchDirection == -2)
		{
			searchSweepStart = FindCameraSweepStart();
			nextCameraSearchDirection = -1;
			continue;
		}
		if (nextCameraSearchDirection == -1)
		{
			searchCurrentTargetScore = ScoreCameraTarget(cameraTargetLocation);
			searchBestScore = searchCurrentTargetScore;
			nextCameraSearchDirection = 0;
			continue;
		}

		const FVector direction = searchKillerDirection.RotateAngleAxis(
			360.0f * nextCameraSearchDirection / cameraSearchDirectionCount, FVector::UpVector);
		const FVector candidate = ResolveCameraCandidateLocation(
			CalculateBaseCameraLocation(direction), searchSweepStart);
		const float score = ScoreCameraTarget(candidate);
		if (score > searchBestScore)
		{
			searchBestScore = score;
			searchBestLocation = candidate;
		}

		++nextCameraSearchDirection;
		if (nextCameraSearchDirection >= cameraSearchDirectionCount)
		{
			// 완료 시점에 추가 충돌 검사를 몰아서 하지 않습니다. 실제 이동은 매 Tick의 Sweep으로 보호합니다.
			if (searchBestScore > searchCurrentTargetScore + cameraTargetSwitchMargin)
			{
				cameraTargetLocation = searchBestLocation;
			}
			bCameraSearchInProgress = false;
			return;
		}
	}
}

FRotator ADeathCamActor::CalculateFramingRotation(
	const FVector& cameraLocation, float& outFov) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(DeathCam_Framing);
	outFov = baseCameraFov;
	const FVector toDeathLocation = (deathLocation - cameraLocation).GetSafeNormal();
	const FVector toKiller = IsValid(otherActor)
		? (otherActor->GetActorLocation() - cameraLocation).GetSafeNormal() : FVector::ZeroVector;

	FVector viewDirection = (toDeathLocation + toKiller).GetSafeNormal();
	if (viewDirection.IsNearlyZero())
	{
		viewDirection = !toDeathLocation.IsNearlyZero() ? toDeathLocation : toKiller;
	}
	if (viewDirection.IsNearlyZero())
	{
		return GetActorRotation();
	}

	const FRotator rotation = viewDirection.Rotation();
	const FRotationMatrix rotationMatrix(rotation);
	const FVector right = rotationMatrix.GetUnitAxis(EAxis::Y);
	const FVector up = rotationMatrix.GetUnitAxis(EAxis::Z);
	float aspectRatio = 16.0f / 9.0f;
	if (const UCameraComponent* cameraComponent = GetCameraComponent())
	{
		if (cameraComponent->bConstrainAspectRatio)
		{
			aspectRatio = cameraComponent->AspectRatio;
		}
		else if (IsValid(ownerPlayerController))
		{
			int32 width = 0, height = 0;
			ownerPlayerController->GetViewportSize(width, height);
			if (width > 0 && height > 0)
			{
				aspectRatio = static_cast<float>(width) / height;
				if (const ULocalPlayer* localPlayer = ownerPlayerController->GetLocalPlayer())
				{
					if (localPlayer->Size.X > 0.0 && localPlayer->Size.Y > 0.0)
					{
						aspectRatio *= static_cast<float>(localPlayer->Size.X / localPlayer->Size.Y);
					}
				}
			}
		}
	}
	aspectRatio = FMath::Clamp(aspectRatio, 0.25f, 4.0f);

	// 수평/수직 시야각을 모두 고려하며 화면 가장자리에 작은 여유를 둡니다.
	float requiredTangent = 0.0f;
	for (const FVector& direction : { toDeathLocation, toKiller })
	{
		if (direction.IsNearlyZero()) continue;
		const double forward = FVector::DotProduct(direction, viewDirection);
		if (forward <= KINDA_SMALL_NUMBER)
		{
			outFov = maxFramingFov;
			return toDeathLocation.IsNearlyZero() ? rotation : toDeathLocation.Rotation();
		}
		const float horizontal = static_cast<float>(FMath::Abs(FVector::DotProduct(direction, right) / forward));
		const float vertical = static_cast<float>(FMath::Abs(FVector::DotProduct(direction, up) / forward));
		requiredTangent = FMath::Max(requiredTangent, FMath::Max(horizontal, vertical * aspectRatio));
	}
	const float requiredFov = FMath::RadiansToDegrees(2.0f * FMath::Atan(requiredTangent / 0.85f));
	outFov = FMath::Clamp(FMath::Max(baseCameraFov, requiredFov), baseCameraFov, maxFramingFov);
	if (requiredFov > maxFramingFov && !toDeathLocation.IsNearlyZero())
	{
		// 두 대상을 최대 시야각으로도 담을 수 없다면 사망 위치가 프레임 밖으로 밀리지 않게
		// 필요한 만큼만 시선을 조정합니다. 이후 후보 탐색이 구도를 개선합니다.
		const float maxTangent = FMath::Tan(FMath::DegreesToRadians(maxFramingFov * 0.5f)) * 0.85f;
		float low = 0.0f, high = 1.0f;
		for (int32 iteration = 0; iteration < 12; ++iteration)
		{
			const float weight = (low + high) * 0.5f;
			const FVector adjustedDirection = FMath::Lerp(viewDirection, toDeathLocation, weight).GetSafeNormal();
			const FRotationMatrix adjustedAxes(adjustedDirection.Rotation());
			const double depth = FVector::DotProduct(toDeathLocation, adjustedDirection);
			const bool bDeathLocationFits = depth > KINDA_SMALL_NUMBER
				&& FMath::Abs(FVector::DotProduct(toDeathLocation, adjustedAxes.GetUnitAxis(EAxis::Y))) <= depth * maxTangent
				&& FMath::Abs(FVector::DotProduct(toDeathLocation, adjustedAxes.GetUnitAxis(EAxis::Z))) <= depth * maxTangent / aspectRatio;
			if (bDeathLocationFits) high = weight;
			else low = weight;
		}
		return FMath::Lerp(viewDirection, toDeathLocation, high).GetSafeNormal().Rotation();
	}
	return rotation;
}

FVector ADeathCamActor::MoveCameraWithCollision(const FVector& cameraMove) const
{
	if (cameraMove.IsNearlyZero() || !GetWorld())
	{
		return GetActorLocation();
	}

	FCollisionQueryParams queryParams(SCENE_QUERY_STAT(DeathCamSlideCollision), false, this);
	queryParams.AddIgnoredActor(this);

	if (IsValid(deadActorToIgnore))
	{
		queryParams.AddIgnoredActor(deadActorToIgnore);
	}

	if (IsValid(otherActor))
	{
		queryParams.AddIgnoredActor(otherActor);
	}

	FVector resolvedLocation = GetActorLocation();
	FVector remainingMove = cameraMove;
	const FCollisionShape probeShape = FCollisionShape::MakeSphere(cameraProbeRadius + cameraCollisionSkin);

	// 한 번만 Slide하면 모서리에서 두 번째 면에 걸린 순간 정지하기 쉽습니다.
	// 제한된 횟수만 반복하면서 매 충돌마다 남은 이동량의 벽 안쪽 성분을 제거합니다.
	for (int32 iteration = 0; iteration < maxSlideIterations; ++iteration)
	{
		if (remainingMove.IsNearlyZero(0.01f))
		{
			break;
		}

		// Slide 후에도 이동 제한을 먼저 적용한 위치까지 Sweep합니다.
		// 충돌 해결 후 Clamp하면 카메라가 다시 벽 안으로 들어갈 수 있습니다.
		const FVector desiredLocation = ClampCameraLocation(resolvedLocation + remainingMove);
		remainingMove = desiredLocation - resolvedLocation;
		if (remainingMove.IsNearlyZero(0.01f))
		{
			break;
		}

		FHitResult hitResult;
		const bool bHit = GetWorld()->SweepSingleByChannel(
			hitResult,
			resolvedLocation,
			desiredLocation,
			FQuat::Identity,
			ECC_Camera,
			probeShape,
			queryParams);

		// 카메라가 사망 위치를 가로질러 지나가면 해당 위치가 화면 뒤로 사라질 수 있습니다.
		// 간단한 구 영역을 두고, 지형과 마찬가지로 표면을 따라 이동시킵니다.
		const FVector fromDeathLocation = resolvedLocation - deathLocation;
		const double moveSizeSquared = remainingMove.SizeSquared();
		const double alongMove = FVector::DotProduct(fromDeathLocation, remainingMove);
		const double outsideDistance = fromDeathLocation.SizeSquared() - FMath::Square(static_cast<double>(minDeathLocationCameraDistance));
		const double discriminant = alongMove * alongMove - moveSizeSquared * outsideDistance;
		if (outsideDistance > 0.0 && alongMove < 0.0 && discriminant >= 0.0)
		{
			const double bodyHitTime = (-alongMove - FMath::Sqrt(discriminant)) / moveSizeSquared;
			if (bodyHitTime >= 0.0 && bodyHitTime <= 1.0 && (!bHit || bodyHitTime < hitResult.Time))
			{
				const FVector bodyNormal = (fromDeathLocation + remainingMove * bodyHitTime).GetSafeNormal();
				const double safeTime = FMath::Max(0.0, bodyHitTime - cameraSweepPullback / remainingMove.Size());
				resolvedLocation += remainingMove * safeTime;
				const FVector afterImpact = remainingMove * (1.0 - bodyHitTime);
				remainingMove = FVector::VectorPlaneProject(afterImpact, bodyNormal);
				if (remainingMove.IsNearlyZero(0.01f) && !afterImpact.IsNearlyZero(0.01f))
				{
					// 목표가 사망 위치 정반대에 있어 접선 이동량이 0이면 한 방향으로 우회합니다.
					FVector tangent = FVector::CrossProduct(FVector::UpVector, bodyNormal).GetSafeNormal();
					if (tangent.IsNearlyZero()) tangent = FVector::CrossProduct(FVector::ForwardVector, bodyNormal).GetSafeNormal();
					if (FVector::DotProduct(tangent, cameraTargetLocation - resolvedLocation) < -0.01) tangent = -tangent;
					remainingMove = tangent * afterImpact.Size();
				}
				continue;
			}
		}

		if (!bHit)
		{
			resolvedLocation = desiredLocation;
			remainingMove = FVector::ZeroVector;
			break;
		}

		// 모서리에서는 표면의 ImpactNormal과 다를 수 있는 Sphere의 접촉 법선을 사용합니다.
		const FVector hitNormal = hitResult.Normal.GetSafeNormal();
		if (hitResult.bStartPenetrating)
		{
			const FVector safeLocation = ClampCameraLocation(
				resolvedLocation + hitNormal * (hitResult.PenetrationDepth + cameraSweepPullback));

			// 다른 벽에 겹치거나 이동 제한 때문에 원위치가 되면 무리하게 밀지 않습니다.
			if (hitNormal.IsNearlyZero() || safeLocation.Equals(resolvedLocation, 0.01f)
				|| GetWorld()->OverlapBlockingTestByChannel(
					safeLocation, FQuat::Identity, ECC_Camera, probeShape, queryParams))
			{
				break;
			}

			resolvedLocation = safeLocation;
			remainingMove = desiredLocation - resolvedLocation;
			continue;
		}

		const double safeTime = FMath::Max(0.0,
			static_cast<double>(hitResult.Time) - cameraSweepPullback / remainingMove.Size());
		resolvedLocation += remainingMove * safeTime;

		// 접촉 전 남은 이동 비율을 구하고, 벽 안쪽으로 향하는 성분을 제거합니다.
		const float remainingFraction = 1.0f - FMath::Clamp(hitResult.Time, 0.0f, 1.0f);
		const FVector moveAfterImpact = remainingMove * remainingFraction;

		// 기획 공식:
		// Adjusted_Move = Camera_Move - Dot(Camera_Move, HitNormal) * HitNormal
		// FVector::VectorPlaneProject는 위 식과 동일하게 Normal 방향 성분을 제거합니다.
		remainingMove = hitNormal.IsNearlyZero()
			? FVector::ZeroVector
			: FVector::VectorPlaneProject(moveAfterImpact, hitNormal);
	}

	// 모든 이동은 Sweep을 거쳤으므로 여기서 충돌 검사 없는 위치 보정을 추가하지 않습니다.
	return resolvedLocation;
}

FVector ADeathCamActor::ClampCameraLocation(const FVector& candidateLocation) const
{
	FVector relativeLocation = candidateLocation - deathLocation;

	// Camera는 고정된 사망 위치 기준 이동 반경보다 멀어질 수 없습니다.
	const float distanceFromCenter = relativeLocation.Size();
	if (distanceFromCenter > maxDistance && distanceFromCenter > KINDA_SMALL_NUMBER)
	{
		relativeLocation = relativeLocation.GetSafeNormal() * maxDistance;
	}

	return deathLocation + relativeLocation;
}

void ADeathCamActor::ApplyKillerHighlightPostProcess()
{
	if (!bEnableKillerHighlight || !IsValid(killerHighlightMaterial))
	{
		if (bEnableKillerHighlight && !IsValid(killerHighlightMaterial))
		{
			UE_LOG(LogDeathCamActor, Warning,
				TEXT("Killer Highlight is enabled, but the Material for the selected Highlight Type is not assigned in DeathCamDataAsset."));
		}
		return;
	}

	UCameraComponent* cameraComponent = GetCameraComponent();
	if (!IsValid(cameraComponent))
	{
		UE_LOG(LogDeathCamActor, Warning, TEXT("Killer Highlight Post Process was not applied: CameraComponent is invalid."));
		return;
	}

	// ACameraActor가 소유한 이 카메라에만 Blendable을 추가합니다.
	// 레벨의 PostProcessVolume이나 PlayerController 수정은 필요하지 않습니다.
	cameraComponent->PostProcessSettings.AddBlendable(killerHighlightMaterial, 1.0f);
	cameraComponent->PostProcessBlendWeight = 1.0f;
}

void ADeathCamActor::ApplyKillerHighlight()
{
	// DeathCam이 다시 시작되는 경우를 대비해,
	// 이전 Highlight 대상의 CustomDepth / Stencil 상태를 먼저 원래 값으로 복구합니다.
	RemoveKillerHighlight();

	// Highlight 기능이 비활성화되어 있거나 Killer가 유효하지 않으면 처리하지 않습니다.
	if (!bEnableKillerHighlight || !IsValid(otherActor))
	{
		return;
	}

	/**
	 * 지정한 Actor의 모든 PrimitiveComponent에
	 * DeathCam Highlight용 CustomDepth / Stencil 값을 적용합니다.
	 *
	 * 적용 전에 각 Component의 기존 CustomDepth / Stencil 상태를 저장해 두며,
	 * DeathCam 종료 시 RemoveKillerHighlight()에서 원래 상태로 복구합니다.
	 *
	 * Killer 본체와 장착 무기에는 동일한 Stencil 값을 적용합니다.
	 * 이를 통해 장착 무기가 Killer의 몸을 가리고 있어도
	 * 무기를 별도의 장애물로 잘못 판단하지 않도록 합니다.
	 */
	auto ApplyStencilToActor =
		[this](AActor* targetActor, const int32 stencilValue)
		{
			if (!IsValid(targetActor))
			{
				return;
			}

			// Actor에 포함된 모든 PrimitiveComponent를 가져옵니다.
			TInlineComponentArray<UPrimitiveComponent*> primitiveComponents;
			targetActor->GetComponents(primitiveComponents);

			// 저장 배열의 재할당을 줄이기 위해 필요한 크기를 미리 확보합니다.
			killerHighlightStates.Reserve(
				killerHighlightStates.Num() + primitiveComponents.Num());

			for (UPrimitiveComponent* primitiveComponent : primitiveComponents)
			{
				if (!IsValid(primitiveComponent))
				{
					continue;
				}

				FHighlightComponentState& state =
					killerHighlightStates.AddDefaulted_GetRef();

				// DeathCam 종료 후 원래 상태로 되돌릴 수 있도록
				// 현재 CustomDepth / Stencil 설정을 저장합니다.
				state.component = primitiveComponent;
				state.bRenderCustomDepth = primitiveComponent->bRenderCustomDepth;
				state.customDepthStencilValue = primitiveComponent->CustomDepthStencilValue;

				// DeathCam Post Process Material에서 Killer 그룹으로 인식할 수 있도록
				// CustomDepth와 지정된 Stencil 값을 적용합니다.
				primitiveComponent->SetRenderCustomDepth(true);
				primitiveComponent->SetCustomDepthStencilValue(stencilValue);
			}
		};

	// Killer 본체에 DeathCam Highlight용 Stencil 값을 적용합니다.
	ApplyStencilToActor(otherActor, killerHighlightStencilValue);

	/**
	 * 무기는 Killer와 별도의 Actor로 존재하면서 Character에 Attach되어 있으므로,
	 * Killer의 Component만 조회해서는 Weapon Mesh를 처리할 수 없습니다.
	 *
	 * 따라서 Killer에게 Attach된 Actor를 별도로 탐색한 뒤,
	 * DeathCamWeapon 태그가 지정된 Actor에도 동일한 Stencil 값을 적용합니다.
	 */
	TArray<AActor*> attachedActors;

	otherActor->GetAttachedActors(
		attachedActors,
		true,   // 기존 배열 내용을 초기화한 뒤 결과를 저장합니다.
		true    // 하위에 Attach된 Actor까지 재귀적으로 탐색합니다.
	);

	for (AActor* attachedActor : attachedActors)
	{
		if (!IsValid(attachedActor))
		{
			continue;
		}

		// DeathCam Highlight 처리 대상인 장착 무기만 선택합니다.
		if (!attachedActor->ActorHasTag(TEXT("DeathCamWeapon")))
		{
			continue;
		}

		/**
		 * 장착 무기도 Killer 본체와 동일한 Stencil 그룹으로 처리합니다.
		 *
		 * 무기가 화면에 직접 보이는 경우에는 SceneDepth와 CustomDepth가 거의 동일하므로
		 * Occlusion 조건을 만족하지 않아 Highlight가 표시되지 않습니다.
		 *
		 * 반대로 Killer와 무기가 벽 뒤에 함께 가려진 경우에는
		 * 둘을 하나의 연속된 Killer 실루엣으로 판단할 수 있어,
		 * 무기와 몸이 겹치는 부분에서 Highlight가 끊기는 현상을 방지합니다.
		 */
		ApplyStencilToActor(attachedActor, killerHighlightStencilValue);
	}
}

void ADeathCamActor::RemoveKillerHighlight()
{
	for (const FHighlightComponentState& state : killerHighlightStates)
	{
		UPrimitiveComponent* primitiveComponent = state.component.Get();
		if (!IsValid(primitiveComponent))
		{
			continue;
		}

		primitiveComponent->SetRenderCustomDepth(state.bRenderCustomDepth);
		primitiveComponent->SetCustomDepthStencilValue(state.customDepthStencilValue);
	}

	killerHighlightStates.Reset();
}

#include "OneWayTeleportActor.h"

#include "TeleportDataAsset.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Math/RotationMatrix.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace OneWayTeleportPrivate
{
	// 모든 Portal이 공유합니다. 따라서 A -> B로 이동하면서 B의 Overlap이 즉시
	// 발생해도 B가 같은 Character를 다시 텔레포트하지 않습니다.
	TMap<TWeakObjectPtr<AActor>, double> ReentryUnlockTimes;

	bool IsReentryLocked(AActor* actor, double currentTime)
	{
		const TWeakObjectPtr<AActor> weakActor(actor);
		if (const double* unlockTime = ReentryUnlockTimes.Find(weakActor))
		{
			return *unlockTime > currentTime;
		}

		return false;
	}

	void LockReentry(AActor* actor, double unlockTime)
	{
		ReentryUnlockTimes.Add(TWeakObjectPtr<AActor>(actor), unlockTime);
	}

	void UnlockReentry(AActor* actor)
	{
		ReentryUnlockTimes.Remove(TWeakObjectPtr<AActor>(actor));
	}

	void RemoveExpiredLocks(double currentTime)
	{
		for (auto iterator = ReentryUnlockTimes.CreateIterator(); iterator; ++iterator)
		{
			if (!iterator.Key().IsValid() || iterator.Value() <= currentTime)
			{
				iterator.RemoveCurrent();
			}
		}
	}
}

AOneWayTeleportActor::AOneWayTeleportActor()
{
	PrimaryActorTick.bCanEverTick = false;

	root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(root);

	// ---------------------------------------------------------------------
	// 실제 진입 판정 Collision
	// ---------------------------------------------------------------------

	entryCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("EntryCollision"));
	entryCollision->SetupAttachment(root);
	entryCollision->InitBoxExtent(FVector(100.0f, 100.0f, 100.0f));

	entryCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	entryCollision->SetCollisionObjectType(ECC_WorldDynamic);
	entryCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	entryCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	entryCollision->SetGenerateOverlapEvents(true);
	entryCollision->SetCanEverAffectNavigation(false);

	// ---------------------------------------------------------------------
	// 반투명 시각화 Cube
	// ---------------------------------------------------------------------

	portalVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalVisual"));
	portalVisual->SetupAttachment(root);

	portalVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	portalVisual->SetGenerateOverlapEvents(false);
	portalVisual->SetCanEverAffectNavigation(false);
	portalVisual->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> cubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));

	if (cubeMesh.Succeeded())
	{
		portalVisual->SetStaticMesh(cubeMesh.Object);
	}

	// 실제 출구 화면은 기존 Collision 시각화 Cube와 분리된 전면 Plane에 표시합니다.
	portalScreen = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalScreen"));
	portalScreen->SetupAttachment(root);
	portalScreen->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	portalScreen->SetGenerateOverlapEvents(false);
	portalScreen->SetCanEverAffectNavigation(false);
	portalScreen->SetCastShadow(false);
	portalScreen->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> planeMesh(
		TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (planeMesh.Succeeded())
	{
		portalScreen->SetStaticMesh(planeMesh.Object);
	}

#if WITH_EDITORONLY_DATA
	// ---------------------------------------------------------------------
	// 에디터 선택용 Handle
	//
	// 반투명 PortalVisual은 뷰포트에서 클릭하기 까다로울 수 있으므로
	// 포탈 위쪽에 불투명한 Sphere를 하나 표시합니다.
	// CreateEditorOnlyDefaultSubobject를 사용하므로 실제 게임/패키징에는 없습니다.
	// ---------------------------------------------------------------------

	editorSelectionHandle =
		CreateEditorOnlyDefaultSubobject<UStaticMeshComponent>(
			TEXT("EditorSelectionHandle"));

	if (editorSelectionHandle)
	{
		editorSelectionHandle->SetupAttachment(root);
		editorSelectionHandle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		editorSelectionHandle->SetGenerateOverlapEvents(false);
		editorSelectionHandle->SetCanEverAffectNavigation(false);
		editorSelectionHandle->SetCastShadow(false);
		editorSelectionHandle->SetHiddenInGame(true);

		static ConstructorHelpers::FObjectFinder<UStaticMesh> sphereMesh(
			TEXT("/Engine/BasicShapes/Sphere.Sphere"));

		if (sphereMesh.Succeeded())
		{
			editorSelectionHandle->SetStaticMesh(sphereMesh.Object);
		}

		// 기본 Sphere 지름이 100uu이므로 약 50uu 크기의 선택 Handle로 사용합니다.
		editorSelectionHandle->SetRelativeScale3D(FVector(0.2f));
	}
#endif
}

void AOneWayTeleportActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdatePortalVisual();
}

void AOneWayTeleportActor::BeginPlay()
{
	Super::BeginPlay();

	entryCollision->OnComponentBeginOverlap.AddUniqueDynamic(
		this,
		&AOneWayTeleportActor::OnEntryBeginOverlap);

	if (IsValid(portalVisual))
	{
		portalVisual->SetHiddenInGame(!bShowPortalInGame);
	}
}

void AOneWayTeleportActor::UpdatePortalVisual()
{
	if (!IsValid(entryCollision) || !IsValid(portalVisual))
	{
		return;
	}

	// ---------------------------------------------------------------------
	// PortalVisual을 EntryCollision과 같은 위치/회전/크기로 맞춥니다.
	// Engine 기본 Cube는 100 x 100 x 100입니다.
	// ---------------------------------------------------------------------

	portalVisual->SetRelativeLocation(entryCollision->GetRelativeLocation());
	portalVisual->SetRelativeRotation(entryCollision->GetRelativeRotation());

	const FVector boxExtent = entryCollision->GetUnscaledBoxExtent();
	portalVisual->SetRelativeScale3D(boxExtent / 50.0f);

	// 에디터에서는 항상 보입니다.
	portalVisual->SetVisibility(true);

	// 체크 해제 시 실제 게임에서만 숨깁니다.
	portalVisual->SetHiddenInGame(!bShowPortalInGame);

	if (IsValid(portalScreen))
	{
		// Basic Plane의 법선(+Z)을 포탈 전면(EntryCollision Forward)에 정확히 맞춥니다.
		// Relative Euler 값을 더하면 부모/인스턴스 회전에서 Plane이 바닥으로 눕기 때문에,
		// 전면과 위쪽 벡터를 기준으로 월드 회전을 직접 생성합니다.
		const FVector screenNormal = entryCollision->GetForwardVector();
		const FVector screenUp = entryCollision->GetUpVector();
		const FVector screenBoxExtent = entryCollision->GetScaledBoxExtent();
		const FTransform screenTransform(
			FRotationMatrix::MakeFromZY(screenNormal, screenUp).ToQuat(),
			entryCollision->GetComponentLocation()
				+ screenNormal * (screenBoxExtent.X + 0.5f),
			FVector(screenBoxExtent.Y / 50.0f, screenBoxExtent.Z / 50.0f, 1.0f));
		portalScreen->SetWorldTransform(screenTransform);
	}

#if WITH_EDITORONLY_DATA
	if (IsValid(editorSelectionHandle))
	{
		// 선택용 Handle을 EntryCollision 정중앙에 배치합니다.
		editorSelectionHandle->SetRelativeLocation(
			entryCollision->GetRelativeLocation());
		editorSelectionHandle->SetRelativeRotation(FRotator::ZeroRotator);
		editorSelectionHandle->SetVisibility(true);
	}
#endif

	// ---------------------------------------------------------------------
	// 색 / Alpha 갱신
	// ---------------------------------------------------------------------

	if (!IsValid(portalVisualMaterial))
	{
		return;
	}

	if (!IsValid(portalVisualMID)
		|| portalVisualMID->Parent != portalVisualMaterial)
	{
		portalVisualMID = UMaterialInstanceDynamic::Create(
			portalVisualMaterial,
			this);

		portalVisual->SetMaterial(0, portalVisualMID);
	}

	if (IsValid(portalVisualMID))
	{
		portalVisualMID->SetVectorParameterValue(
			TEXT("Color"),
			portalColor);

		portalVisualMID->SetScalarParameterValue(
			TEXT("Opacity"),
			portalOpacity);
	}
}

bool AOneWayTeleportActor::CanDisplayPortalView(
	const APlayerController* playerController,
	const FVector& cameraLocation,
	const FVector& aimDirection,
	float& outDistance,
	float& outScore) const
{
	outDistance = 0.0f;
	outScore = 0.0f;

	if (!IsValid(playerController)
		|| !IsValid(entryCollision)
		|| !IsValid(exitTarget)
		|| !IsValid(teleportDA)
		|| !bEnablePortalView)
	{
		return false;
	}

	const FVector portalLocation = entryCollision->GetComponentLocation();
	const FVector toPortal = portalLocation - cameraLocation;
	outDistance = toPortal.Length();
	if (outDistance > teleportDA->viewDistance || outDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector directionToPortal = toPortal / outDistance;
	const float aimDot = FVector::DotProduct(aimDirection.GetSafeNormal(), directionToPortal);
	// 기획에는 "포탈을 향해 볼 때"만 정의되어 있으므로, 정면 중앙을 조준했는지까지
	// 강제하지 않습니다. 카메라 앞 반구에 있는 동안에는 화면을 계속 갱신해 포탈이
	// 화면 가장자리에 걸쳐도 빈 화면으로 바뀌지 않게 합니다.
	if (aimDot <= 0.0f)
	{
		return false;
	}

	FCollisionQueryParams traceParams(SCENE_QUERY_STAT(PortalViewOcclusion), false);
	traceParams.AddIgnoredActor(this);
	traceParams.AddIgnoredActor(playerController->GetPawn());
	FHitResult hit;
	if (GetWorld()->LineTraceSingleByChannel(
		hit,
		cameraLocation,
		portalLocation,
		ECC_Visibility,
		traceParams))
	{
		return false;
	}

	// 여러 포탈이 카메라 앞에 있을 때에는 더 정면이고 가까운 Portal을 우선 선택합니다.
	outScore = aimDot * 2.0f - outDistance / FMath::Max(teleportDA->viewDistance, 1.0f);
	return true;
}

FTransform AOneWayTeleportActor::GetPortalViewCameraTransform(
	const FTransform& cameraTransform) const
{
	if (!IsValid(entryCollision) || !IsValid(exitTarget))
	{
		return FTransform::Identity;
	}

	// 출구 카메라 위치는 항상 ExitTarget의 Launch 방향 바로 앞에 고정합니다.
	// 플레이어-입구 사이의 전체 위치 오프셋을 출구로 복사하면, 포탈에서 멀리
	// 주시할 때 카메라가 출구에서 수천 uu 떨어져 맵 밖을 찍을 수 있습니다.
	const FQuat entryRotation = entryCollision->GetComponentQuat();
	const FVector launchForward = GetExitLaunchForward();
	const FVector localAim = entryRotation.UnrotateVector(
		cameraTransform.GetRotation().GetForwardVector());
	// 입구를 향한 -X 방향을 출구의 +X(Launch) 방향으로 뒤집어 대응합니다.
	const FVector mappedLocalAim(-localAim.X, -localAim.Y, localAim.Z);
	FVector exitAim = exitTarget->GetActorQuat().RotateVector(mappedLocalAim);
	if (exitAim.IsNearlyZero())
	{
		exitAim = launchForward;
	}

	const FQuat exitCameraRotation = FRotationMatrix::MakeFromXZ(
		exitAim.GetSafeNormal(),
		FVector::UpVector).ToQuat();
	const FVector exitCameraLocation = exitTarget->GetActorLocation()
		+ launchForward * 5.0f;
	return FTransform(exitCameraRotation, exitCameraLocation);
}

float AOneWayTeleportActor::GetPortalViewOpacity(float distance) const
{
	if (!IsValid(teleportDA))
	{
		return 1.0f;
	}

	const float viewDistance = teleportDA->viewDistance;
	const float distanceAlpha = FMath::Clamp(
		distance / FMath::Max(viewDistance, KINDA_SMALL_NUMBER),
		0.0f,
		1.0f);
	return FMath::Lerp(
		teleportDA->portalScreenNearOpacity,
		teleportDA->portalScreenFarOpacity,
		distanceAlpha);
}

void AOneWayTeleportActor::ApplyPortalView(
	UTextureRenderTarget2D* renderTarget,
	float blurStrength,
	float screenOpacity)
{
	if (!IsValid(portalScreen) || !IsValid(renderTarget))
	{
		return;
	}

	UMaterialInterface* material = portalScreenMaterial;
	if (!IsValid(material))
	{
		material = portalVisualMaterial;
	}

	if (!IsValid(material))
	{
		return;
	}

	if (!IsValid(portalScreenMID) || portalScreenMID->Parent != material)
	{
		portalScreenMID = UMaterialInstanceDynamic::Create(material, this);
		portalScreen->SetMaterial(0, portalScreenMID);
	}

	portalScreenMID->SetTextureParameterValue(TEXT("PortalTexture"), renderTarget);
	portalScreenMID->SetScalarParameterValue(TEXT("BlurStrength"), blurStrength);
	portalScreenMID->SetScalarParameterValue(TEXT("PortalOpacity"), screenOpacity);
	portalScreenMID->SetVectorParameterValue(TEXT("FrameColor"), portalFrameColor);
	portalScreenMID->SetScalarParameterValue(TEXT("FrameGlowIntensity"), portalFrameGlowIntensity);
	portalScreenMID->SetScalarParameterValue(TEXT("FrameThickness"), portalFrameThickness);
	portalScreen->SetHiddenInGame(false);
}

void AOneWayTeleportActor::ClearPortalView()
{
	if (IsValid(portalScreen))
	{
		portalScreen->SetHiddenInGame(true);
	}
}

FVector AOneWayTeleportActor::GetExitLaunchForward() const
{
	return IsValid(exitTarget)
		? exitTarget->GetActorForwardVector()
		: FVector::ForwardVector;
}

void AOneWayTeleportActor::OnEntryBeginOverlap(
	UPrimitiveComponent* overlappedComponent,
	AActor* otherActor,
	UPrimitiveComponent* otherComp,
	int32 otherBodyIndex,
	bool bFromSweep,
	const FHitResult& sweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!bTeleportEnabled)
	{
		return;
	}

	ACharacter* character = Cast<ACharacter>(otherActor);
	if (!IsValid(character))
	{
		return;
	}

	if (otherComp != character->GetCapsuleComponent())
	{
		return;
	}

	const double currentTime = GetWorld()->GetTimeSeconds();
	OneWayTeleportPrivate::RemoveExpiredLocks(currentTime);
	if (OneWayTeleportPrivate::IsReentryLocked(character, currentTime))
	{
		return;
	}

	if (!IsValid(exitTarget))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[OneWayTeleport] %s : exitTarget이 지정되지 않았습니다."),
			*GetName());

		return;
	}

	if (!IsValid(teleportDA))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[OneWayTeleport] %s : teleportDA가 지정되지 않았습니다."),
			*GetName());

		return;
	}

	TeleportCharacter(character);
}

void AOneWayTeleportActor::TeleportCharacter(ACharacter* character)
{
	if (!IsValid(character)
		|| !IsValid(exitTarget)
		|| !IsValid(teleportDA))
	{
		return;
	}

	const FVector exitLocation = exitTarget->GetActorLocation();
	const FRotator exitRotation = GetExitFacingRotation();
	const double currentTime = GetWorld()->GetTimeSeconds();
	OneWayTeleportPrivate::LockReentry(
		character,
		currentTime + FMath::Max(0.0f, reentryLockDuration));

	const bool bTeleported = character->SetActorLocationAndRotation(
		exitLocation,
		exitRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (!bTeleported)
	{
		OneWayTeleportPrivate::UnlockReentry(character);
		return;
	}

	if (AController* controller = character->GetController())
	{
		controller->SetControlRotation(exitRotation);

		if (APlayerController* playerController =
			Cast<APlayerController>(controller))
		{
			playerController->ClientSetRotation(exitRotation, false);

			// PlayerController의 Client RPC이므로 포탈을 실제 이용한 플레이어의
			// 클라이언트에서만 재생됩니다. 다른 플레이어와 서버에는 들리지 않습니다.
			if (IsValid(teleportDA->teleportSound))
			{
				playerController->ClientPlaySound(
					teleportDA->teleportSound,
					teleportDA->soundVolumeMultiplier);
			}
		}
	}

	character->LaunchCharacter(
		GetLaunchVelocity(),
		true,
		true);

	ApplyMoveLock(character, teleportDA->moveLockTime);
	character->ForceNetUpdate();
}

FRotator AOneWayTeleportActor::GetExitFacingRotation() const
{
	if (!IsValid(exitTarget))
	{
		return FRotator::ZeroRotator;
	}

	return FRotator(
		0.0f,
		exitTarget->GetActorRotation().Yaw,
		0.0f);
}

FVector AOneWayTeleportActor::GetLaunchVelocity() const
{
	if (!IsValid(exitTarget) || !IsValid(teleportDA))
	{
		return FVector::ZeroVector;
	}

	const FRotator launchRotation(
		static_cast<float>(teleportDA->launchAngle),
		exitTarget->GetActorRotation().Yaw,
		0.0f);

	return launchRotation.Vector() * teleportDA->launchPower;
}

void AOneWayTeleportActor::ApplyMoveLock_Implementation(
	ACharacter* character,
	float duration)
{
	if (!IsValid(character) || duration <= 0.0f)
	{
		return;
	}

	AController* controller = character->GetController();
	if (!IsValid(controller))
	{
		return;
	}

	controller->SetIgnoreMoveInput(true);
	character->StopJumping();

	TWeakObjectPtr<AController> weakController = controller;

	FTimerDelegate unlockDelegate;
	unlockDelegate.BindLambda(
		[weakController]()
		{
			if (AController* validController = weakController.Get())
			{
				validController->SetIgnoreMoveInput(false);
			}
		});

	FTimerHandle unlockHandle;
	GetWorldTimerManager().SetTimer(
		unlockHandle,
		unlockDelegate,
		duration,
		false);
}

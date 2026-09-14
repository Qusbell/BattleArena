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
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
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

	portalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	portalVFX->SetupAttachment(root);
	portalVFX->SetAutoActivate(true);
	portalVFX->SetCastShadow(false);
	portalVFX->SetHiddenInGame(true);

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
	UpdatePortalVisual();

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

	// 새 Portal Shape Mesh를 지정하지 않은 기존 Blueprint/레벨 인스턴스는 기획에서 설정한
	// PortalVisual의 Mesh를 기준으로 삼습니다. 이전 구현이 PortalScreen의 기본 Plane을
	// 기준으로 삼아 기존 기획 Mesh와 Collision/VFX 크기가 달라지던 문제를 막습니다.
	UStaticMesh* resolvedPortalShapeMesh = portalShapeMesh.Get();
	if (!IsValid(resolvedPortalShapeMesh) && IsValid(portalVisual))
	{
		UStaticMesh* portalVisualMesh = portalVisual->GetStaticMesh();
		// C++ 기본값 Cube는 기획 Mesh가 아니므로, 이 경우에는 기존처럼 PortalScreen을 사용합니다.
		if (IsValid(portalVisualMesh)
			&& portalVisualMesh->GetPathName() != TEXT("/Engine/BasicShapes/Cube.Cube"))
		{
			resolvedPortalShapeMesh = portalVisualMesh;
		}
	}

	if (IsValid(portalScreen) && IsValid(resolvedPortalShapeMesh))
	{
		portalScreen->SetStaticMesh(resolvedPortalShapeMesh);
	}
	else if (IsValid(portalScreen))
	{
		resolvedPortalShapeMesh = portalScreen->GetStaticMesh();
	}

	// Mesh의 local X/Y는 화면 가로/세로입니다. BoxComponent의 local X는 포탈 법선
	// (앞뒤 두께)이므로 축을 맞춰 Bounds만 자동 반영하고, 디자이너가 잡은 위치·회전은 유지합니다.
	if (bAutoFitCollisionToPortalMesh && IsValid(resolvedPortalShapeMesh))
	{
		const FVector meshExtent = resolvedPortalShapeMesh->GetBounds().BoxExtent;
		entryCollision->SetBoxExtent(FVector(
			portalCollisionDepth * 0.5f,
			FMath::Max(meshExtent.X * portalShapeScale.X, 1.0f),
			FMath::Max(meshExtent.Y * portalShapeScale.Y, 1.0f)));
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
		const FVector screenScale = bAutoFitCollisionToPortalMesh
			? FVector(-portalShapeScale.X, portalShapeScale.Y, 1.0f)
			: FVector(-screenBoxExtent.Y / 50.0f, screenBoxExtent.Z / 50.0f, 1.0f);
		const FTransform screenTransform(
			FRotationMatrix::MakeFromZY(screenNormal, screenUp).ToQuat(),
			entryCollision->GetComponentLocation()
				+ screenNormal * (screenBoxExtent.X + 0.5f),
			// local X는 화면 가로(U)입니다. 음수 Scale로 SceneCapture의 좌우 반전을 보정합니다.
			screenScale);
		portalScreen->SetWorldTransform(screenTransform);
	}

	UpdatePortalVFX();

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
	// 입구를 향한 -X 방향만 출구의 +X(Launch) 방향으로 넘깁니다.
	// Y(포탈의 좌/우 축)까지 반전하면 출구 화면이 거울처럼 좌우 반전됩니다.
	const FVector mappedLocalAim(-localAim.X, localAim.Y, localAim.Z);
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
	portalScreenMID->SetVectorParameterValue(
		TEXT("FrameColor"),
		bEnableMaterialFrame ? portalFrameColor : FLinearColor::Transparent);
	portalScreenMID->SetScalarParameterValue(
		TEXT("FrameGlowIntensity"),
		bEnableMaterialFrame ? portalFrameGlowIntensity : 0.0f);
	portalScreenMID->SetScalarParameterValue(TEXT("FrameThickness"), portalFrameThickness);
	portalScreen->SetHiddenInGame(false);
}

void AOneWayTeleportActor::UpdatePortalVFX()
{
	if (!IsValid(portalVFX) || !IsValid(entryCollision))
	{
		return;
	}

	portalVFX->SetAsset(portalVFXSystem);
	// Vortex의 중앙 배경/소용돌이 알파는 아래 User 파라미터로 투명하게 만들어
	// 출구 Render Target을 가리지 않습니다.
	const bool bShowVFX = bEnablePortalVFX && IsValid(portalVFXSystem);
	portalVFX->SetVisibility(bShowVFX, true);
	portalVFX->SetHiddenInGame(!bShowVFX);
	if (!bShowVFX)
	{
		return;
	}

	const FVector portalNormal = entryCollision->GetForwardVector();
	const FVector portalUp = entryCollision->GetUpVector();
	const FVector portalRight = entryCollision->GetRightVector();
	const FVector extent = entryCollision->GetScaledBoxExtent();
	const FVector fullSize = extent * 2.0f;
	FVector portalSurfaceSize(fullSize.Y, fullSize.Z, fullSize.X);

	// Portal Shape Scale은 Screen/Collision/VFX가 공유하는 유일한 크기 기준입니다.
	// Collision의 여유 두께나 Niagara의 현재 파티클 Bounds가 이 기준을 바꾸지 않게,
	// PortalScreen에 적용한 같은 Mesh Bounds와 Shape Scale로 화면 크기를 계산합니다.
	if (IsValid(portalScreen) && IsValid(portalScreen->GetStaticMesh()))
	{
		const FVector screenExtent = portalScreen->GetStaticMesh()->GetBounds().BoxExtent;
		const float screenWidth = 2.0f * screenExtent.X * FMath::Abs(portalShapeScale.X);
		const float screenHeight = 2.0f * screenExtent.Y * FMath::Abs(portalShapeScale.Y);
		if (screenWidth > KINDA_SMALL_NUMBER && screenHeight > KINDA_SMALL_NUMBER)
		{
			portalSurfaceSize.X = screenWidth;
			portalSurfaceSize.Y = screenHeight;
		}
	}

	FVector vfxScale(portalSurfaceSize.X / 100.0f, portalSurfaceSize.Y / 100.0f, 1.0f);

	// Niagara 이펙트는 각각 제작 당시의 기본 Bounds가 다릅니다. Vortex처럼 기본 크기가
	// 큰 시스템은 100cm 기준의 단순 Scale로는 포탈 밖으로 넘치므로, 유효한 고정 Bounds를
	// 포탈의 가로(X)/세로(Y) 크기에 정규화합니다.
	const FBox systemBounds = portalVFXSystem->GetFixedBounds();
	const FVector systemSize = systemBounds.GetSize();
	if (systemBounds.IsValid
		&& FMath::IsFinite(systemSize.X) && FMath::IsFinite(systemSize.Y)
		&& systemSize.X > KINDA_SMALL_NUMBER && systemSize.Y > KINDA_SMALL_NUMBER)
	{
		vfxScale.X = portalSurfaceSize.X / systemSize.X;
		vfxScale.Y = portalSurfaceSize.Y / systemSize.Y;
	}

	// AdvancedPortalsSystemVFX의 시각적인 발광 외곽은 System Fixed Bounds보다 약 1.67배
	// 크게 잡혀 있습니다. 이 보정을 코드에 고정해 Details의 Portal VFX Scale (1,1,1)이
	// 기존 Portal Shape Scale과 정확히 맞는 기본값이 되게 합니다.
	constexpr float PortalVFXVisualSizeCorrection = 0.6f;
	vfxScale.X *= PortalVFXVisualSizeCorrection;
	vfxScale.Y *= PortalVFXVisualSizeCorrection;

	// Niagara 기본 평면(XY)을 포탈 전면에 맞추고, 원형 이펙트는 Collision의 가로/세로에
	// 맞춰 자동으로 타원형이 됩니다. Niagara System에 User.Size가 있으면 같은 크기도 전달합니다.
	const FTransform vfxTransform(
		FRotationMatrix::MakeFromXY(portalRight, portalUp).ToQuat(),
		entryCollision->GetComponentLocation()
			+ portalNormal * (extent.X + 2.0f),
		vfxScale * portalVFXScale);
	portalVFX->SetWorldTransform(vfxTransform);
	portalVFX->SetVariableVec3(TEXT("User.Size"), portalSurfaceSize);
	// AdvancedPortalsSystemVFX의 Vortex에는 중앙을 채우는 이미터가 Background/Vortex/Circle로
	// 나뉘어 있습니다. 모두 포탈 화면 위에 렌더링되므로, 테두리 전용 사용에서는 각각 0으로
	// 설정해야 출구 Render Target을 가리지 않습니다. 에셋에 없는 User 파라미터는 무시됩니다.
	// Vortex 2~10은 User.* 이름이 아니라 각 이미터의 실제 런타임 AlphaScale을 읽습니다.
	// 1번처럼 User.*를 쓰는 구형 변형도 함께 지원합니다.
	portalVFX->SetVariableFloat(TEXT("Background.AlphaScale"), portalVFXBackgroundAlpha);
	portalVFX->SetVariableFloat(TEXT("Vortex.AlphaScale"), portalVFXVortexAlpha);
	portalVFX->SetVariableFloat(TEXT("Circle.AlphaScale"), portalVFXCircleAlpha);
	portalVFX->SetVariableFloat(TEXT("Ring.AlphaScale"), portalVFXRingAlpha);
	portalVFX->SetVariableFloat(TEXT("Energy.AlphaScale"), portalVFXEnergyAlpha);
	portalVFX->SetVariableFloat(TEXT("User.Background Control"), portalVFXBackgroundControl);
	portalVFX->SetVariableFloat(TEXT("User.Background Alpha"), portalVFXBackgroundAlpha);
	portalVFX->SetVariableFloat(TEXT("User.Vortex Alpha"), portalVFXVortexAlpha);
	portalVFX->SetVariableFloat(TEXT("User.Circle Alpha"), portalVFXCircleAlpha);
	portalVFX->SetVariableFloat(TEXT("User.Distortion Control"), portalVFXDistortionControl);
	portalVFX->SetVariableFloat(TEXT("User.Ring Alpha"), portalVFXRingAlpha);
	portalVFX->SetVariableFloat(TEXT("User.Energy Alpha"), portalVFXEnergyAlpha);

	// NS_Portal_Vortex_1은 Energy.AlphaScale을 외부로 노출하지 않은 구형 시스템입니다.
	// 해당 Energy 이미터가 실제로 읽는 User.Color Vortex의 Alpha로 Energy Alpha를 전달합니다.
	if (portalVFXSystem->GetFName() == TEXT("NS_Portal_Vortex_1"))
	{
		portalVFX->SetVariableLinearColor(
			TEXT("User.Color Vortex"),
			FLinearColor(1.0f, 1.0f, 1.0f, portalVFXEnergyAlpha));
	}

	// 이 팩의 AlphaScale은 Niagara 내부 상수라 Component에서 연속값으로 덮어쓸 수 없습니다.
	// 다만 0일 때는 실제 이미터를 꺼서, 모든 Alpha가 0인데 화면에 남는 문제를 방지합니다.
	// 0보다 큰 값은 원본 Niagara의 표현을 그대로 유지합니다.
	auto SetEmitterVisibleForAlpha = [this](const TCHAR* emitterName, float alpha)
	{
		portalVFX->SetEmitterEnable(FName(emitterName), alpha > KINDA_SMALL_NUMBER);
	};
	SetEmitterVisibleForAlpha(TEXT("Background"), portalVFXBackgroundAlpha);
	// 일부 Vortex 에셋은 배경을 Background가 아니라 Texture emitter로 렌더링한다.
	// 따라서 Background Alpha가 두 emitter를 함께 제어해야 0일 때 배경이 남지 않는다.
	SetEmitterVisibleForAlpha(TEXT("Texture"), portalVFXBackgroundAlpha);
	SetEmitterVisibleForAlpha(TEXT("Vortex"), portalVFXVortexAlpha);
	SetEmitterVisibleForAlpha(TEXT("Circle"), portalVFXCircleAlpha);
	SetEmitterVisibleForAlpha(TEXT("Distortion"), portalVFXDistortionControl);
	SetEmitterVisibleForAlpha(TEXT("Ring"), portalVFXRingAlpha);
	SetEmitterVisibleForAlpha(TEXT("Energy"), portalVFXEnergyAlpha);
	// 이 Niagara들은 User Alpha를 Spawn 단계에서 읽습니다. Asset/인스턴스 값 변경 후
	// 다시 초기화해야 Background Alpha가 실제 파티클에 반영됩니다.
	portalVFX->ReinitializeSystem();

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

#include "SeamlessPortalActor.h"

#include "SeamlessPortalMath.h"
#include "SeamlessPortalTraversalSubsystem.h"
#include "PortalTraversalGuard.h"
#include "TeleportDataAsset.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Camera/PlayerCameraManager.h"
#include "SceneView.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#if WITH_EDITOR
#include "ScopedTransaction.h"
#endif

namespace SpatialPortalGeometry
{
	// Shared, transient, code-generated surface. No Content asset or per-portal simulation is required.
	static TWeakObjectPtr<UStaticMesh> EllipseMesh;
	static UStaticMesh* GetEllipseMesh()
	{
		if (EllipseMesh.IsValid()) return EllipseMesh.Get();
		FMeshDescription description;
		FStaticMeshAttributes attributes(description);
		attributes.Register();
		auto positions = attributes.GetVertexPositions();
		auto uvs = attributes.GetVertexInstanceUVs();
		uvs.SetNumChannels(1);
		auto normals = attributes.GetVertexInstanceNormals();
		auto tangents = attributes.GetVertexInstanceTangents();
		auto signs = attributes.GetVertexInstanceBinormalSigns();
		auto colors = attributes.GetVertexInstanceColors();
		const FPolygonGroupID group = description.CreatePolygonGroup();
		attributes.GetPolygonGroupMaterialSlotNames()[group] = TEXT("PortalScreen");
		constexpr int32 segments = 128;
		TArray<FVertexInstanceID, TInlineAllocator<segments+1>> vertices;
		for (int32 i = 0; i <= segments; ++i)
		{
			const double angle = (i-1) * 2.0 * PI / segments;
			const FVector3f p = i == 0 ? FVector3f::ZeroVector : FVector3f(50*FMath::Cos(angle),50*FMath::Sin(angle),0);
			const FVertexID vertex = description.CreateVertex();
			positions[vertex] = p;
			const FVertexInstanceID instance = description.CreateVertexInstance(vertex);
			uvs.Set(instance,0,FVector2f(0.5f+p.X/100,0.5f+p.Y/100));
			normals[instance] = FVector3f(0,0,1);
			tangents[instance] = FVector3f(1,0,0);
			signs[instance] = 1;
			colors[instance] = FVector4f(1,1,1,1);
			vertices.Add(instance);
		}
		for (int32 i = 0; i < segments; ++i)
		{
			const FVertexInstanceID triangle[] = {vertices[0],vertices[(i+1)%segments+1],vertices[i+1]};
			description.CreateTriangle(group,MakeArrayView(triangle));
		}
		UStaticMesh* mesh = NewObject<UStaticMesh>(GetTransientPackage(),NAME_None,RF_Transient);
		mesh->GetStaticMaterials().Add(FStaticMaterial(nullptr,TEXT("PortalScreen")));
		UStaticMesh::FBuildMeshDescriptionsParams params;
		params.bFastBuild = true;
		params.bBuildSimpleCollision = false;
		params.bCommitMeshDescription = false;
		params.bMarkPackageDirty = false;
		params.bAllowCpuAccess = true;
		if (!mesh->BuildFromMeshDescriptions({&description},params)) return nullptr;
		EllipseMesh = mesh;
		return mesh;
	}
}

ASeamlessPortalActor::ASeamlessPortalActor()
{
	bReplicates = true;
	bShowPortalInGame = false;
	portalOpacity = 0;
	portalShapeScale = FVector2D(3.5, 5);
	portalCollisionDepth = 2.0f;
	bEllipticalOpening = true;
	// Code-only default: opaque unlit texture, with ordinary planar UVs and no distance fade.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> material(
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"));
	if (material.Succeeded()) portalScreenMaterial = material.Object;
}

const UTeleportDataAsset* ASeamlessPortalActor::GetTeleportDataAsset() const
{
	return IsValid(teleportDA) ? teleportDA.Get() : GetDefault<UTeleportDataAsset>();
}

void ASeamlessPortalActor::OnConstruction(const FTransform& transform)
{
	cachedActorExit.Reset();
	Super::OnConstruction(transform);
	UpdateSpatialOpening();
	AlignSpatialScreen();
}

void ASeamlessPortalActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	// Transient generated geometry is recreated on map load/component re-registration, not saved as an asset.
	if (IsValid(entryCollision) && IsValid(portalScreen) && IsValid(portalVisual))
	{
		UpdatePortalVisual();
		UpdateSpatialOpening();
		AlignSpatialScreen();
	}
}

void ASeamlessPortalActor::BeginPlay()
{
	cachedActorExit.Reset();
	Super::BeginPlay();
	UpdateSpatialOpening();
	AlignSpatialScreen();
	// Plane crossings are processed once per authority world. Legacy overlap-launch is not used.
	if (IsValid(entryCollision))
	{
		entryCollision->SetGenerateOverlapEvents(false);
		entryCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (HasAuthority()) replicatedDestination = exitTarget;
	if (auto* subsystem = GetWorld()->GetSubsystem<USeamlessPortalTraversalSubsystem>()) subsystem->RegisterPortal(this);
}

void ASeamlessPortalActor::EndPlay(const EEndPlayReason::Type reason)
{
	if (UWorld* world = GetWorld())
		if (auto* subsystem = world->GetSubsystem<USeamlessPortalTraversalSubsystem>()) subsystem->UnregisterPortal(this);
	Super::EndPlay(reason);
}

void ASeamlessPortalActor::OnEntryBeginOverlap(UPrimitiveComponent*, AActor*, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	// Deliberately not the legacy teleport/LaunchCharacter path.
}

void ASeamlessPortalActor::UpdateSpatialOpening()
{
	if (!IsValid(portalScreen) || !IsValid(portalVisual)) return;
	const bool bDefaultVisual = !portalVisual->GetStaticMesh()
		|| portalVisual->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Cube.Cube");
	if (bDefaultVisual) portalVisual->SetVisibility(false);
	// The planning team's custom shape is authoritative. Never replace it with a stock/generated mesh.
	if (IsValid(portalShapeMesh) || !bDefaultVisual) return;
	UStaticMesh* current = portalScreen->GetStaticMesh();
	if (IsValid(current) && current != generatedEllipseMesh && current != SpatialPortalGeometry::EllipseMesh.Get()
		&& current->GetPathName() != TEXT("/Engine/BasicShapes/Plane.Plane")) return;
	if (bEllipticalOpening)
	{
		if (!IsValid(generatedEllipseMesh)) generatedEllipseMesh = SpatialPortalGeometry::GetEllipseMesh();
		if (IsValid(generatedEllipseMesh)) portalScreen->SetStaticMesh(generatedEllipseMesh);
	}
	else if (!IsValid(current) || current == generatedEllipseMesh || current == SpatialPortalGeometry::EllipseMesh.Get())
		portalScreen->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));
}

void ASeamlessPortalActor::SnapPortalToFloor()
{
#if WITH_EDITOR
	UWorld* world = GetWorld();
	if (!IsValid(world) || world->IsGameWorld() || world->bIsTearingDown || !IsValid(entryCollision)
		|| !FMath::IsFinite(floorSnapSearchDistance)) return;
	const FTransform frame = GetPortalFrameTransform();
	const FVector2D halfSize = GetOpeningHalfSize();
	if (!SeamlessPortalMath::IsValidFrame(frame) || halfSize.ContainsNaN() || halfSize.X <= 0 || halfSize.Y <= 0) return;
	const FVector right = frame.GetRotation().GetRightVector();
	const FVector up = frame.GetRotation().GetUpVector();
	const double verticalExtent = bEllipticalOpening
		? FMath::Sqrt(FMath::Square(halfSize.X*right.Z)+FMath::Square(halfSize.Y*up.Z))
		: halfSize.X*FMath::Abs(right.Z)+halfSize.Y*FMath::Abs(up.Z);
	FCollisionQueryParams query(SCENE_QUERY_STAT(SpatialPortalSnapFloor),false,this);
	for (AActor* wall : passageWalls) if (IsValid(wall)) query.AddIgnoredActor(wall);
	FCollisionObjectQueryParams objects;
	objects.AddObjectTypesToQuery(ECC_WorldStatic);
	objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	TArray<FHitResult> hits;
	world->LineTraceMultiByObjectType(hits,frame.GetLocation()+FVector::UpVector*(verticalExtent+10),
		frame.GetLocation()-FVector::UpVector*FMath::Clamp(floorSnapSearchDistance,1.0f,20000.0f),objects,query);
	const FHitResult* floor = nullptr;
	for (const FHitResult& hit : hits)
	{
		const UPrimitiveComponent* component = hit.GetComponent();
		if (hit.bStartPenetrating || hit.ImpactNormal.Z < 0.7 || !IsValid(component)
			|| component->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block) continue;
		const bool bBelow = hit.ImpactPoint.Z <= frame.GetLocation().Z+2;
		const bool bPreviousBelow = floor && floor->ImpactPoint.Z <= frame.GetLocation().Z+2;
		if (!floor || (bBelow && !bPreviousBelow)
			|| (bBelow == bPreviousBelow && FMath::Abs(hit.ImpactPoint.Z-frame.GetLocation().Z)
				< FMath::Abs(floor->ImpactPoint.Z-frame.GetLocation().Z))) floor = &hit;
	}
	if (!floor) return;
	const double y = halfSize.X*FVector::DotProduct(right,floor->ImpactNormal);
	const double z = halfSize.Y*FVector::DotProduct(up,floor->ImpactNormal);
	const double support = bEllipticalOpening ? FMath::Sqrt(y*y+z*z) : FMath::Abs(y)+FMath::Abs(z);
	const double delta = (support-FVector::DotProduct(frame.GetLocation()-floor->ImpactPoint,floor->ImpactNormal))/floor->ImpactNormal.Z;
	if (!FMath::IsFinite(delta) || FMath::Abs(delta) > floorSnapSearchDistance+verticalExtent+10) return;
	if (FMath::Abs(delta) < 0.001) return;
	const FScopedTransaction transaction(NSLOCTEXT("SpatialPortal","SnapFloor","Snap Portal To Floor"));
	Modify();
	SetActorLocation(GetActorLocation()+FVector::UpVector*delta,false,nullptr,ETeleportType::TeleportPhysics);
	cachedActorExit.Reset();
	PostEditMove(true);
	MarkPackageDirty();
#endif
}

void ASeamlessPortalActor::AlignSpatialScreen()
{
	if (!IsValid(portalScreen) || !IsValid(entryCollision)) return;
	// The projection describes this exact plane. Even the legacy 0.5cm offset creates errors up close.
	portalScreen->UpdateBounds();
	// Engine Plane's V increases along local +Y. Turn that axis down for the capture texture's top-left origin.
	FVector scale = portalScreen->GetComponentScale();
	if (const UStaticMesh* mesh = portalScreen->GetStaticMesh())
	{
		const FVector extent = mesh->GetBounds().BoxExtent;
		const FVector2D opening = GetOpeningHalfSize();
		// The base class uses world scale without actor scale. A spatial surface must match the
		// physical opening used by projection, traversal, and floor snapping, including actor scale.
		if (extent.X > UE_SMALL_NUMBER && extent.Y > UE_SMALL_NUMBER)
		{
			scale.X = -opening.X/extent.X;
			scale.Y = -opening.Y/extent.Y;
		}
	}
	scale.Y = -FMath::Abs(scale.Y);
	portalScreen->SetWorldScale3D(scale);
	portalScreen->UpdateBounds();
	portalScreen->AddWorldOffset(entryCollision->GetComponentLocation() - portalScreen->Bounds.Origin);
	screenRestRelative = portalScreen->GetRelativeTransform();
}

void ASeamlessPortalActor::SetDestination(AActor* destination)
{
	if (GetWorld() && GetWorld()->IsGameWorld() && !HasAuthority()) return;
	if (destination && (!IsValid(destination) || destination == this || destination->GetWorld() != GetWorld())) return;
	exitTarget = destination;
	cachedActorExit.Reset();
	replicatedDestination = destination;
	ForceNetUpdate();
}

void ASeamlessPortalActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASeamlessPortalActor, replicatedDestination);
}

void ASeamlessPortalActor::OnRep_Destination()
{
	exitTarget = replicatedDestination;
	cachedActorExit.Reset();
}

bool ASeamlessPortalActor::GetDestinationFrame(FTransform& frame) const
{
	if (!IsValid(exitTarget) || exitTarget == this || exitTarget->GetWorld() != GetWorld()) return false;
	if (const auto* portal = Cast<AOneWayTeleportActor>(exitTarget)) frame = portal->GetPortalFrameTransform();
	else frame = FTransform(exitTarget->GetActorQuat(), exitTarget->GetActorLocation());
	if (!SeamlessPortalMath::IsValidFrame(frame) || !SeamlessPortalMath::IsValidFrame(GetPortalFrameTransform())) return false;
	// An arbitrary actor is an anchor: it need not share the entrance mesh, pivot, size, or floor height.
	// Real spatial portal pairs still preserve their deliberately placed doorway transforms exactly.
	if (bAlignActorExitToFloor && !Cast<ASeamlessPortalActor>(exitTarget)) AlignActorExitFrame(frame);
	return SeamlessPortalMath::IsValidFrame(frame);
}

void ASeamlessPortalActor::AlignActorExitFrame(FTransform& frame) const
{
	UWorld* world = GetWorld();
	if (!IsValid(world) || world->bIsTearingDown || !FMath::IsFinite(actorExitFloorSearchDistance)) return;
	const FTransform entry = GetPortalFrameTransform();
	const FVector2D halfSize = GetOpeningHalfSize();
	const float searchDistance = FMath::Clamp(actorExitFloorSearchDistance, 1.0f, 5000.0f);
	const double now = world->GetTimeSeconds();
	if (cachedActorExit.Get() == exitTarget.Get() && cachedActorExitRawFrame.Equals(frame, 0.001)
		&& cachedActorExitEntryFrame.Equals(entry, 0.001) && cachedActorExitHalfSize.Equals(halfSize, 0.001)
		&& cachedActorExitSearchDistance == searchDistance && now >= actorExitCacheTime && now - actorExitCacheTime < 0.25)
	{
		frame = cachedActorExitFrame;
		return;
	}
	cachedActorExit = exitTarget;
	cachedActorExitRawFrame = frame;
	cachedActorExitEntryFrame = entry;
	cachedActorExitHalfSize = halfSize;
	cachedActorExitSearchDistance = searchDistance;
	actorExitCacheTime = now;
	FCollisionQueryParams query(SCENE_QUERY_STAT(SpatialPortalAnchorFloor), false, this);
	// Portal presentation/overlap volumes are never floors. Ordinary target meshes ARE queried.
	if (Cast<AOneWayTeleportActor>(exitTarget)) query.AddIgnoredActor(exitTarget);
	for (AActor* wall : passageWalls) if (IsValid(wall)) query.AddIgnoredActor(wall);
	FCollisionObjectQueryParams objects;
	objects.AddObjectTypesToQuery(ECC_WorldStatic);
	objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	auto FindFloor = [&](const FVector& origin, bool bTarget, FVector& floor)
	{
		FVector start = origin + FVector::UpVector * 2;
		if (bTarget && !Cast<AOneWayTeleportActor>(exitTarget))
		{
			// A floor/prop actor may have its pivot inside its collision. Start above that actor, not inside it.
			FVector boundsOrigin, boundsExtent;
			exitTarget->GetActorBounds(true, boundsOrigin, boundsExtent);
			const double top = boundsOrigin.Z + boundsExtent.Z + 2;
			if (!boundsExtent.IsNearlyZero() && top > start.Z && top - origin.Z <= searchDistance) start.Z = top;
		}
		TArray<FHitResult> hits;
		world->LineTraceMultiByObjectType(hits, start, origin - FVector::UpVector * searchDistance, objects, query);
		for (const FHitResult& hit : hits)
		{
			const UPrimitiveComponent* component = hit.GetComponent();
			if (!IsValid(component) || component->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Block) continue;
			// Never look through an obstruction. The start is only raised over the target's own bounds.
			// Coincident floor meshes are valid even if the trace returns the neighbour before the target.
			if (hit.bStartPenetrating || hit.ImpactNormal.Z < 0.7) return false;
			floor = hit.ImpactPoint;
			return !floor.ContainsNaN();
		}
		return false;
	};
	FVector exitFloor;
	if (FindFloor(frame.GetLocation(), true, exitFloor))
	{
		FVector entryFloor;
		// An airborne entrance has no floor: use its aperture's bottom as the virtual floor baseline.
		if (!FindFloor(entry.GetLocation(), false, entryFloor))
			entryFloor = entry.TransformPositionNoScale(FVector(0, 0, -halfSize.Y));
		const FVector mappedFloor = SeamlessPortalMath::MapPoint(entry, frame, entryFloor);
		const double correction = exitFloor.Z - mappedFloor.Z;
		if (FMath::IsFinite(correction) && FMath::Abs(correction) <= searchDistance)
			frame.AddToTranslation(FVector::UpVector * correction);
	}
	cachedActorExitFrame = frame;
}

bool ASeamlessPortalActor::CanTraverse() const
{
	FTransform destination;
	return bTeleportEnabled && GetDestinationFrame(destination);
}

FVector2D ASeamlessPortalActor::GetOpeningHalfSize() const
{
	if (!IsValid(entryCollision)) return FVector2D::ZeroVector;
	const FVector extent = entryCollision->GetScaledBoxExtent();
	return FVector2D(extent.Y, extent.Z);
}

bool ASeamlessPortalActor::BuildPortalCaptureView(const FTransform& camera, FPortalCaptureView& view) const
{
	view = FPortalCaptureView();
	FTransform destination;
	if (camera.ContainsNaN() || !GetDestinationFrame(destination)) return false;
	const FVector eye = SeamlessPortalMath::MapPoint(GetPortalFrameTransform(), destination, camera.GetLocation());
	if (!SeamlessPortalMath::BuildWindowProjection(destination.InverseTransformPositionNoScale(eye),
		GetOpeningHalfSize(), 0.0, view.projection)) return false;
	// A fixed orientation plus an asymmetric frustum represents a physical window; the eye still moves in 3D.
	view.transform = FTransform(destination.GetRotation(), eye);
	view.bCustomProjection = true;
	view.bSpatialView = true;
	// Near plane is the exit plane itself; no project-wide Global Clip Plane/shader rebuild is required.
	view.bEnableClipPlane = false;
	return true;
}

FTransform ASeamlessPortalActor::GetPortalViewCameraTransform(const FTransform& camera) const
{
	FPortalCaptureView view;
	return BuildPortalCaptureView(camera, view) ? view.transform : FTransform::Identity;
}

bool ASeamlessPortalActor::NeedsFullRatePortalView(float distance) const
{
	return FMath::IsFinite(fullRateDistance) && distance <= FMath::Max(0.0f, fullRateDistance);
}

bool ASeamlessPortalActor::CanDisplayPortalView(const APlayerController* controller, const FVector& location,
	const FVector& aim, float& distance, float& score, float& coverage) const
{
	distance = score = coverage = 0;
	FTransform destination;
	const UTeleportDataAsset* settings = GetTeleportDataAsset();
	if (!bEnablePortalView || !IsValid(controller) || !IsValid(entryCollision) || !IsValid(settings)
		|| !GetDestinationFrame(destination) || location.ContainsNaN()) return false;
	const FTransform frame = GetPortalFrameTransform();
	if (frame.InverseTransformPositionNoScale(location).X <= 0.001) return false;
	distance = FVector::Distance(location, frame.GetLocation());
	if (!FMath::IsFinite(distance) || distance > settings->viewDistance) return false;

	const ULocalPlayer* localPlayer = controller->GetLocalPlayer();
	if (!localPlayer || !localPlayer->ViewportClient || !localPlayer->ViewportClient->Viewport) return false;
	FSceneViewProjectionData projectionData;
	if (!localPlayer->GetProjectionData(localPlayer->ViewportClient->Viewport, projectionData)) return false;
	const FMatrix viewProjection = projectionData.ComputeViewProjectionMatrix();
	const FVector2D halfSize = GetOpeningHalfSize();
	TArray<FVector4, TInlineAllocator<4>> corners;
	for (const FVector2D uv : {FVector2D(-1,-1), FVector2D(-1,1), FVector2D(1,1), FVector2D(1,-1)})
		corners.Add(viewProjection.TransformFVector4(FVector4(frame.TransformPosition(FVector(0, uv.X * halfSize.X, uv.Y * halfSize.Y)),1)));
	if (!SeamlessPortalMath::ScreenCoverage(corners, coverage)) return false;
	// Test several points. A pillar occluding the centre must not remove a partially visible portal.
	FCollisionQueryParams query(SCENE_QUERY_STAT(SpatialPortalVisibility), false, this);
	query.AddIgnoredActor(controller->GetPawn());
	for (AActor* wall : passageWalls) if (IsValid(wall)) query.AddIgnoredActor(wall);
	bool bVisible = false;
	for (const FVector2D uv : {FVector2D::ZeroVector, FVector2D(-0.8,-0.8), FVector2D(0.8,-0.8), FVector2D(-0.8,0.8), FVector2D(0.8,0.8)})
	{
		FHitResult hit;
		if (!GetWorld()->LineTraceSingleByChannel(hit, location,
			frame.TransformPosition(FVector(0.1, uv.X * halfSize.X, uv.Y * halfSize.Y)), ECC_Visibility, query))
		{ bVisible = true; break; }
	}
	score = coverage * 2.0f - distance / FMath::Max(settings->viewDistance, 1.0f);
	return bVisible;
}

void ASeamlessPortalActor::ApplyPortalView(UTextureRenderTarget2D* target, float, float)
{
	if (!IsValid(portalScreen) || !IsValid(target) || !IsValid(portalScreenMaterial)) return;
	if (!IsValid(portalScreenMID) || portalScreenMID->Parent != portalScreenMaterial)
	{
		portalScreenMID = UMaterialInstanceDynamic::Create(portalScreenMaterial, this);
		portalScreen->SetMaterial(0, portalScreenMID);
	}
	if (!IsValid(portalScreenMID)) return;
	portalScreenMID->SetTextureParameterValue(TEXT("SlateUI"), target);
	portalScreenMID->SetTextureParameterValue(TEXT("PortalTexture"), target);
	portalScreenMID->SetVectorParameterValue(TEXT("TintColorAndOpacity"), FLinearColor::White);
	portalScreenMID->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0);
	portalScreenMID->SetScalarParameterValue(TEXT("PortalOpacity"), 1);
	portalScreenMID->SetScalarParameterValue(TEXT("BlurStrength"), 0);
	// Preserve the projected doorway when the main camera's near plane would otherwise slice it away.
	FTransform rest = IsValid(portalScreen->GetAttachParent())
		? screenRestRelative * portalScreen->GetAttachParent()->GetComponentTransform() : screenRestRelative;
	if (APlayerController* player = GetWorld()->GetFirstPlayerController())
		if (APlayerCameraManager* camera = player->PlayerCameraManager)
		{
			const FVector eye = camera->GetCameraLocation();
			const double depth = FVector::DotProduct(rest.GetLocation() - eye, camera->GetCameraRotation().Vector());
			const double nearPlane = camera->GetCameraCacheView().GetFinalPerspectiveNearClipPlane();
			if (depth > 0.001 && FMath::IsFinite(nearPlane))
			{
				const double factor = FMath::Clamp(nearPlane * 1.05 / depth, 1.0, 1000.0);
				rest.SetLocation(eye + (rest.GetLocation() - eye) * factor);
				rest.SetScale3D(rest.GetScale3D() * factor);
			}
		}
	portalScreen->SetWorldTransform(rest);
	portalScreen->SetHiddenInGame(false);
}

bool ASeamlessPortalActor::FitsAt(const ACharacter* character, const FVector& position) const
{
	const UCapsuleComponent* capsule = IsValid(character) ? character->GetCapsuleComponent() : nullptr;
	if (!IsValid(capsule)) return false;
	const double radius = capsule->GetScaledCapsuleRadius();
	const double halfHeight = capsule->GetScaledCapsuleHalfHeight();
	if (bAllowPartialCapsuleEntry)
		return SeamlessPortalMath::CapsuleTouchesOpening(GetPortalFrameTransform(), position,
			capsule->GetUpVector(), radius, halfHeight, GetOpeningHalfSize(), bEllipticalOpening);
	if (!FMath::IsFinite(headAndFootTolerance)) return false;
	// Only shorten the projected head/foot ends, never the torso radius. Keep at least half of the
	// cylindrical segment even on small/crouched characters or with an excessive tolerance value.
	const double allowance = FMath::Clamp(double(headAndFootTolerance),0.0,FMath::Max(0.0,(halfHeight-radius)*0.5));
	return SeamlessPortalMath::CapsuleFits(GetPortalFrameTransform(), position,
		capsule->GetUpVector(), radius, halfHeight-allowance,
		GetOpeningHalfSize() + FVector2D(1.0, 1.0), bEllipticalOpening);
}

double ASeamlessPortalActor::GetCapsuleNormalExtent(const ACharacter* character) const
{
	const UCapsuleComponent* capsule = IsValid(character) ? character->GetCapsuleComponent() : nullptr;
	if (!IsValid(capsule)) return 0;
	return capsule->GetScaledCapsuleRadius() + (capsule->GetScaledCapsuleHalfHeight() - capsule->GetScaledCapsuleRadius())
		* FMath::Abs(FVector::DotProduct(capsule->GetUpVector(), GetPortalFrameTransform().GetRotation().GetForwardVector()));
}

double ASeamlessPortalActor::GetPassageFrontExtent() const
{
	const FTransform frame = GetPortalFrameTransform();
	const FVector normal = frame.GetRotation().GetForwardVector();
	double extent = 0;
	for (const AActor* wall : passageWalls)
	{
		if (!IsValid(wall)) continue;
		FVector origin, bounds;
		wall->GetActorBounds(true, origin, bounds);
		const double front = FVector::DotProduct(origin - frame.GetLocation(), normal) + FVector::DotProduct(bounds, normal.GetAbs());
		if (FMath::IsFinite(front)) extent = FMath::Max(extent, front);
	}
	return extent;
}

bool ASeamlessPortalActor::FindCrossing(const ACharacter* character, const FVector& previous, double& fraction) const
{
	if (!CanTraverse() || !IsValid(character) || !FMath::IsFinite(maximumTraversalStep)
		|| maximumTraversalStep <= 0 || FVector::DistSquared(previous, character->GetActorLocation()) > FMath::Square(maximumTraversalStep)) return false;
	const FTransform frame = GetPortalFrameTransform();
	if (bAllowPartialCapsuleEntry)
	{
		const UCapsuleComponent* capsule = character->GetCapsuleComponent();
		return IsValid(capsule) && SeamlessPortalMath::FindFrontCapsuleContact(frame,previous,character->GetActorLocation(),
			capsule->GetUpVector(),capsule->GetScaledCapsuleRadius(),capsule->GetScaledCapsuleHalfHeight(),
			GetOpeningHalfSize(),bEllipticalOpening,fraction);
	}
	FVector local;
	return SeamlessPortalMath::FindFrontCrossing(frame.InverseTransformPositionNoScale(previous),
		frame.InverseTransformPositionNoScale(character->GetActorLocation()), local, fraction)
		&& FitsAt(character, frame.TransformPositionNoScale(local));
}

bool ASeamlessPortalActor::TryTraverse(ACharacter* character, const FVector& previous, FVector& safeExit) const
{
	double fraction;
	FTransform destination;
	UWorld* world = GetWorld();
	if (!HasAuthority() || !IsValid(character) || !world || world->bIsTearingDown
		|| !FindCrossing(character, previous, fraction) || !GetDestinationFrame(destination)
		|| PortalTraversalGuard::IsLocked(character, world->GetTimeSeconds())) return false;
	UCharacterMovementComponent* movement = character->GetCharacterMovement();
	UCapsuleComponent* capsule = character->GetCapsuleComponent();
	if (!IsValid(movement) || !IsValid(capsule)) return false;
	const FTransform entry = GetPortalFrameTransform();
	const FVector right = destination.GetRotation().GetRightVector();
	auto MapArrival = [&](const FVector& point)
	{
		FVector mapped = SeamlessPortalMath::MapPoint(entry,destination,point);
		if (bCenterExitHorizontally)
			mapped -= right*FVector::DotProduct(mapped-destination.GetLocation(),right);
		return mapped;
	};
	FVector position = MapArrival(character->GetActorLocation());
	const FVector velocity = SeamlessPortalMath::MapVector(entry, destination, movement->Velocity);
	const FRotator facing(0, SeamlessPortalMath::MapRotation(entry, destination, character->GetActorQuat()).Rotator().Yaw, 0);
	if (position.ContainsNaN() || velocity.ContainsNaN()) return false;
	const auto* exitPortal = Cast<ASeamlessPortalActor>(exitTarget);
	if (exitPortal && !exitPortal->FitsAt(character, position)) return false;
	const FVector normal = destination.GetRotation().GetForwardVector();
	const double extent = capsule->GetScaledCapsuleRadius()
		+ (capsule->GetScaledCapsuleHalfHeight() - capsule->GetScaledCapsuleRadius()) * FMath::Abs(normal.Z);
	safeExit = position + normal * FMath::Max(0.0, extent + (exitPortal ? exitPortal->GetPassageFrontExtent() : 0)
		+ 2.0 - FVector::DotProduct(position - destination.GetLocation(), normal));
	if (safeExit.ContainsNaN()) return false;
	// Early contact maps the source centre behind the exit. Emerge clear of the exit plane instead;
	// retain tangential placement, rotation and velocity, and preserve the former mapping for late steps.
	if (bAllowPartialCapsuleEntry && FVector::DotProduct(position-destination.GetLocation(),normal) < 0)
		position = safeExit;
	FCollisionQueryParams query(SCENE_QUERY_STAT(SpatialPortalDestination), false, character);
	query.AddIgnoredActor(this);
	// Only a portal's own visual/trigger is exempt. A mesh used as an anchor remains solid.
	if (Cast<AOneWayTeleportActor>(exitTarget)) query.AddIgnoredActor(exitTarget);
	// Do not carry the entrance wall exclusions to an unrelated destination.
	if (exitPortal) for (AActor* wall : exitPortal->passageWalls) if (IsValid(wall)) query.AddIgnoredActor(wall);
	const FCollisionResponseParams responses(capsule->GetCollisionResponseToChannels());
	const FCollisionShape shape = FCollisionShape::MakeCapsule(capsule->GetScaledCapsuleRadius(), capsule->GetScaledCapsuleHalfHeight());
	if (world->OverlapBlockingTestByChannel(position, facing.Quaternion(), capsule->GetCollisionObjectType(), shape, query, responses)
		|| world->OverlapBlockingTestByChannel(safeExit, facing.Quaternion(), capsule->GetCollisionObjectType(), shape, query, responses)) return false;
	FHitResult hit;
	const FVector crossing = FMath::Lerp(previous, character->GetActorLocation(), fraction);
	// Validate the actual centred arrival path, not an imaginary path from the old edge position.
	FVector sweepStart = MapArrival(crossing);
	if (bAllowPartialCapsuleEntry)
		sweepStart += normal*FMath::Max(0.0,extent+(exitPortal ? exitPortal->GetPassageFrontExtent() : 0)+2.0
			-FVector::DotProduct(sweepStart-destination.GetLocation(),normal));
	if (world->SweepSingleByChannel(hit, sweepStart, position,
		facing.Quaternion(), capsule->GetCollisionObjectType(), shape, query, responses)) return false;
	PortalTraversalGuard::Lock(character, world->GetTimeSeconds() + FMath::Max(0.01f, reentryLockDuration));
	if (!character->TeleportTo(position, facing, false, true))
	{ PortalTraversalGuard::Unlock(character); return false; }
	if (!IsValid(character) || !IsValid(movement) || world->bIsTearingDown) return false;
	character->SetBase(nullptr);
	movement->Velocity = velocity;
	movement->bJustTeleported = true;
	movement->SetMovementMode(MOVE_Falling);
	if (AController* controller = character->GetController())
	{
		const FRotator control = SeamlessPortalMath::MapRotation(entry, destination, controller->GetControlRotation().Quaternion()).Rotator();
		controller->SetControlRotation(FRotator(control.Pitch, control.Yaw, 0));
		if (auto* player = Cast<APlayerController>(controller))
		{
			player->ClientSetRotation(FRotator(control.Pitch, control.Yaw, 0), false);
			const auto* settings = GetTeleportDataAsset();
			if (IsValid(settings->teleportSound)) player->ClientPlaySound(settings->teleportSound, settings->soundVolumeMultiplier);
		}
	}
	character->ForceNetUpdate();
	return true;
}

void ASeamlessPortalActor::CopyPresentationFromSource()
{
	AOneWayTeleportActor* source = presentationSource;
	if (!IsValid(source) || source == this || !IsValid(source->portalScreen) || !IsValid(source->entryCollision)
		|| !IsValid(source->portalVFX)) return;
	Modify();
	teleportDA = source->teleportDA;
	UStaticMesh* sourceShape = IsValid(source->portalShapeMesh) ? source->portalShapeMesh : source->portalScreen->GetStaticMesh();
	// A default plane is not a designer-authored override of this portal's code-generated opening.
	portalShapeMesh = IsValid(sourceShape) && sourceShape->GetPathName() != TEXT("/Engine/BasicShapes/Plane.Plane")
		&& sourceShape != SpatialPortalGeometry::EllipseMesh.Get() ? sourceShape : nullptr;
	portalShapeScale = source->portalShapeScale;
	bAutoFitCollisionToPortalMesh = source->bAutoFitCollisionToPortalMesh;
	// Plane crossings do not need the legacy portal's thick overlap volume.
	entryCollision->SetRelativeTransform(source->entryCollision->GetRelativeTransform());
	bEnablePortalVFX = source->bEnablePortalVFX;
	portalVFXSystem = source->portalVFXSystem;
	portalVFXScale = source->portalVFXScale;
	portalVFXBackgroundAlpha = source->portalVFXBackgroundAlpha;
	portalVFXBackgroundControl = source->portalVFXBackgroundControl;
	portalVFXVortexAlpha = source->portalVFXVortexAlpha;
	portalVFXCircleAlpha = source->portalVFXCircleAlpha;
	portalVFXDistortionControl = source->portalVFXDistortionControl;
	portalVFXRingAlpha = source->portalVFXRingAlpha;
	portalVFXEnergyAlpha = source->portalVFXEnergyAlpha;
	bOverridePortalVFXColor = source->bOverridePortalVFXColor;
	portalVFXColor = source->portalVFXColor;
	portalVFX->SetRelativeLocationAndRotation(source->portalVFX->GetRelativeLocation(), source->portalVFX->GetRelativeRotation());
	UpdatePortalVisual();
	UpdateSpatialOpening();
	AlignSpatialScreen();
}

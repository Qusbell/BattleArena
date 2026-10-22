#if WITH_DEV_AUTOMATION_TESTS

#include "../../Teleport/SeamlessPortalActor.h"
#include "../../Teleport/SeamlessPortalMath.h"
#include "../../Teleport/SeamlessPortalTraversalSubsystem.h"
#include "../../Teleport/PortalTraversalGuard.h"
#include "../../Teleport/PortalViewSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "Misc/AutomationTest.h"
#include "StaticMeshResources.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalMathTest, "ShootingArena.Portal.Spatial.TransformAndProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalMathTest::RunTest(const FString&)
{
	using namespace SeamlessPortalMath;
	const FTransform entry(FRotator(0, 35, 0), FVector(20, 80, 100), FVector(2, 3, 4));
	const FTransform exit(FRotator(60, -80, 0), FVector(-1000, 400, 300), FVector(5, 6, 7));
	const FVector eye = entry.GetLocation() + entry.GetRotation().RotateVector(FVector(200, 30, 40));
	const FVector mapped = MapPoint(entry, exit, eye);
	TestTrue(TEXT("Eye position uses a rigid half-turn, not a reflection or scale warp"),
		(mapped - exit.GetLocation()).Equals(exit.GetRotation().RotateVector(FVector(-200, -30, 40)), 0.001));
	TestTrue(TEXT("Reverse mapping round-trips the eye"), MapPoint(exit, entry, mapped).Equals(eye, 0.001));
	const FVector velocity(500, -320, 150);
	TestTrue(TEXT("Portal transforms conserve speed"), FMath::IsNearlyEqual(MapVector(entry, exit, velocity).Size(), velocity.Size(), 0.001));
	TestTrue(TEXT("Directions preserve handedness"), FVector::CrossProduct(MapVector(entry, exit, FVector::ForwardVector),
		MapVector(entry, exit, FVector::RightVector)).Equals(MapVector(entry, exit, FVector::UpVector), 0.001));
	TestTrue(TEXT("Rotation and vector transformations agree"),
		MapRotation(entry, exit, FQuat::Identity).GetForwardVector().Equals(MapVector(entry, exit, FVector::ForwardVector), 0.001));
	for (const FVector camera : {FVector(-500, 0, 0), FVector(-80, -150, 40), FVector(-0.02, 300, -200)})
	{
		FMatrix projection;
		TestTrue(TEXT("Off-axis projection handles near and side-on eyes"), BuildWindowProjection(camera, FVector2D(175, 250), 0, projection));
		for (const double y : {-175.0, 175.0}) for (const double z : {-250.0, 250.0})
		{
			const FVector4 clip = projection.TransformFVector4(FVector4(y - camera.Y, z - camera.Z, -camera.X, 1));
			TestTrue(TEXT("Each opening corner stays registered to the same texture corner"),
				FMath::IsNearlyEqual(clip.X / clip.W, y / 175, 0.00001) && FMath::IsNearlyEqual(clip.Y / clip.W, z / 250, 0.00001));
			TestTrue(TEXT("Exit plane is the reversed-Z near plane"), FMath::IsNearlyEqual(clip.Z / clip.W, 1.0, 0.00001));
		}
		const FVector4 before = projection.TransformFVector4(FVector4(0, 0, -camera.X * 0.5, 1));
		const FVector4 after = projection.TransformFVector4(FVector4(0, 0, -camera.X + 100, 1));
		TestTrue(TEXT("Geometry behind the exit plane is clipped without a global clip shader"), before.Z / before.W > 1 && after.Z / after.W < 1);
	}
	FMatrix a, b;
	BuildWindowProjection(FVector(-100, 0, 0), FVector2D(175, 250), 0, a);
	BuildWindowProjection(FVector(-100, 30, 0), FVector2D(175, 250), 0, b);
	auto U = [](const FMatrix& p, double eyeY, double depth)
	{
		const FVector4 clip = p.TransformFVector4(FVector4(-eyeY, 0, 100 + depth, 1));
		return clip.X / clip.W;
	};
	TestTrue(TEXT("Sideways motion produces different parallax at different depths"),
		!FMath::IsNearlyEqual(U(b,30,100) - U(a,0,100), U(b,30,1000) - U(a,0,1000), 0.001));
	TestFalse(TEXT("An eye on the plane cannot create a singular projection"), BuildWindowProjection(FVector::ZeroVector, FVector2D(175,250), 0, a));
	TestFalse(TEXT("An eye behind the entrance does not render a mirrored back view"), BuildWindowProjection(FVector(1,0,0), FVector2D(175,250), 0, a));
	TestFalse(TEXT("Invalid opening is rejected"), BuildWindowProjection(FVector(-100,0,0), FVector2D(0,250), 0, a));
	TestFalse(TEXT("Nonfinite eyes are rejected"), BuildWindowProjection(FVector(-100,std::numeric_limits<double>::infinity(),0), FVector2D(175,250), 0, a));
	FVector crossing; double fraction;
	TestTrue(TEXT("A fast segment crossing cannot skip the plane"), FindFrontCrossing(FVector(2000,40,10), FVector(-2000,80,30), crossing, fraction));
	TestTrue(TEXT("Crossing uses the swept intersection"), crossing.Equals(FVector(0,60,20)) && FMath::IsNearlyEqual(fraction, 0.5));
	TestFalse(TEXT("Reverse traversal is disabled"), FindFrontCrossing(FVector(-10,0,0), FVector(10,0,0), crossing, fraction));
	TestFalse(TEXT("Standing on the plane is not a repeat crossing"), FindFrontCrossing(FVector::ZeroVector, FVector(-10,0,0), crossing, fraction));
	TestTrue(TEXT("A centred upright capsule fits"), CapsuleFits(FTransform::Identity, FVector::ZeroVector, FVector::UpVector, 34, 88, FVector2D(175,250), true));
	TestFalse(TEXT("Oval corners do not admit the capsule"), CapsuleFits(FTransform::Identity, FVector(0,130,160), FVector::UpVector, 34,88,FVector2D(175,250),true));
	TestTrue(TEXT("Rounded capsule fits the bottom centre of an oval without a jump"),CapsuleFits(FTransform::Identity,FVector(0,0,-115.85),FVector::UpVector,51,132,FVector2D(175,250),true));
	TestTrue(TEXT("A small lateral walking offset fits an oval with the existing 1cm boundary tolerance"),CapsuleFits(FTransform::Identity,FVector(0,20,-115.85),FVector::UpVector,51,132,FVector2D(176,251),true));
	TestFalse(TEXT("A capsule too low for an oval is still rejected"),CapsuleFits(FTransform::Identity,FVector(0,0,-120),FVector::UpVector,51,132,FVector2D(175,250),true));
	float coverage = 0;
	const TArray<FVector4> screen = {FVector4(-1,-1,0,1),FVector4(-1,1,0,1),FVector4(1,1,0,1),FVector4(1,-1,0,1)};
	TestTrue(TEXT("A doorway covering the whole view stays visible"), ScreenCoverage(screen,coverage) && FMath::IsNearlyEqual(coverage,1.0f));
	const TArray<FVector4> outside = {FVector4(2,-1,0,1),FVector4(2,1,0,1),FVector4(3,1,0,1),FVector4(3,-1,0,1)};
	TestFalse(TEXT("A doorway outside the view requires no capture"), ScreenCoverage(outside,coverage));
	const TArray<FVector4> above = {FVector4(-1,2,0,1),FVector4(-1,3,0,1),FVector4(1,3,0,1),FVector4(1,2,0,1)};
	TestFalse(TEXT("Clipping at the last viewport edge cannot leave phantom coverage"), ScreenCoverage(above,coverage));
	const TArray<FVector4> behind = {FVector4(-1,-1,0,-1),FVector4(-1,1,0,-1),FVector4(1,1,0,-1),FVector4(1,-1,0,-1)};
	TestFalse(TEXT("A doorway behind the camera requires no capture"), ScreenCoverage(behind,coverage));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalTraversalTest, "ShootingArena.Portal.Spatial.TraversalAndIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalTraversalTest::RunTest(const FString&)
{
	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &values);
	if (!TestNotNull(TEXT("Isolated world"), world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	auto* entry = world->SpawnActor<ASeamlessPortalActor>();
	auto* exit = world->SpawnActor<ASeamlessPortalActor>();
	auto* legacy = world->SpawnActor<AOneWayTeleportActor>();
	exit->SetActorLocation(FVector(1000,0,0));
	entry->SetDestination(exit);
	TestTrue(TEXT("New portal accepts a directed exit"), entry->CanTraverse());
	TestFalse(TEXT("Setting A to B never implicitly links B to A"), exit->CanTraverse());
	TestNotNull(TEXT("Existing DA type is reused with a safe default"), entry->GetTeleportDataAsset());
	FPortalCaptureView view;
	TestTrue(TEXT("Spatial view builds with no DA asset or material authoring"), entry->BuildPortalCaptureView(FTransform(FVector(100,20,30)), view));
	TestTrue(TEXT("Spatial view is custom perspective, not a fixed exit camera"),
		view.bCustomProjection && view.bSpatialView && view.transform.GetLocation().Equals(FVector(900,-20,30)));
	// Project actual engine mesh vertices using the actual actor transform, not assumed UV conventions.
	TArray<UStaticMeshComponent*> components;
	entry->GetComponents(components);
	for (UStaticMeshComponent* component : components)
	{
		if (component->GetFName() != TEXT("PortalScreen") || !component->GetStaticMesh()) continue;
		const auto& lod = component->GetStaticMesh()->GetRenderData()->LODResources[0];
		for (uint32 i = 0; i < lod.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++i)
		{
			const FVector position = component->GetComponentTransform().TransformPosition(FVector(lod.VertexBuffers.PositionVertexBuffer.VertexPosition(i)));
			const FVector destination = SeamlessPortalMath::MapPoint(entry->GetPortalFrameTransform(),exit->GetPortalFrameTransform(),position);
			const FVector local = view.transform.InverseTransformPositionNoScale(destination);
			const FVector4 clip = view.projection.TransformFVector4(FVector4(local.Y,local.Z,local.X,1));
			const FVector2f uv = lod.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i,0);
			TestTrue(TEXT("Actual surface UV matches the doorway ray: no upside-down or mirrored image"),
				FMath::IsNearlyEqual(double(uv.X),0.5+0.5*clip.X/clip.W,0.001)
				&& FMath::IsNearlyEqual(double(uv.Y),0.5-0.5*clip.Y/clip.W,0.001));
		}
	}
	TestFalse(TEXT("Default engine global clip plane is not required"), view.bEnableClipPlane);
	TestFalse(TEXT("Unconfigured legacy portal remains unconfigured"), legacy->BuildPortalCaptureView(FTransform::Identity, view));
	ACharacter* character = world->SpawnActor<ACharacter>();
	character->GetCapsuleComponent()->SetCapsuleSize(20,40);
	character->SetActorLocation(FVector(-25,0,0));
	character->GetCharacterMovement()->Velocity = FVector(-600,50,20);
	FVector safeExit;
	TestTrue(TEXT("Server traverses a fitting capsule"), entry->TryTraverse(character, FVector(100,0,0), safeExit));
	TestTrue(TEXT("Relative position survives traversal"), character->GetActorLocation().Equals(FVector(1025,0,0),0.01));
	TestTrue(TEXT("Velocity is redirected instead of replaced by Launch settings"), character->GetCharacterMovement()->Velocity.Equals(FVector(600,-50,20),0.01));
	TestTrue(TEXT("Reentry lock is shared with the legacy portal path"), PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	PortalTraversalGuard::Unlock(character);
	// A solid exit must reject traversal before moving or starting a lock.
	AActor* wall = world->SpawnActor<AActor>();
	UBoxComponent* block = NewObject<UBoxComponent>(wall);
	wall->SetRootComponent(block);
	block->SetBoxExtent(FVector(100,100,100));
	block->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	block->SetCollisionResponseToAllChannels(ECR_Block);
	block->RegisterComponent();
	wall->SetActorLocation(FVector(1025,0,0));
	character->SetActorLocation(FVector(-25,0,0));
	TestFalse(TEXT("Blocked destination cannot embed a player in geometry"), entry->TryTraverse(character,FVector(100,0,0),safeExit));
	TestTrue(TEXT("Failed traversal retains the original position"), character->GetActorLocation().Equals(FVector(-25,0,0)));
	TestFalse(TEXT("Failed traversal does not leave a reentry lock"), PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	wall->Destroy();
	auto SpawnWall = [world](double x)
	{
		AActor* actor = world->SpawnActor<AActor>();
		UBoxComponent* box = NewObject<UBoxComponent>(actor);
		actor->SetRootComponent(box);
		box->SetBoxExtent(FVector(5,175,250));
		box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		box->SetCollisionResponseToAllChannels(ECR_Block);
		box->RegisterComponent();
		actor->SetActorLocation(FVector(x,0,0));
		return actor;
	};
	AActor* entranceWall = SpawnWall(0);
	AActor* exitWall = SpawnWall(1000);
	entry->passageWalls.Add(entranceWall);
	exit->passageWalls.Add(exitWall);
	AActor* externalIgnore = world->SpawnActor<AActor>();
	character->GetCapsuleComponent()->IgnoreActorWhenMoving(externalIgnore,true);
	character->GetCharacterMovement()->Velocity = FVector::ZeroVector;
	character->SetActorLocation(FVector(30,0,0));
	auto* subsystem = world->GetSubsystem<USeamlessPortalTraversalSubsystem>();
	if (TestNotNull(TEXT("Authority traversal subsystem"),subsystem))
	{
		subsystem->RegisterPortal(entry);
		subsystem->RegisterPortal(exit);
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("Only the explicitly selected wall is temporarily ignored"),character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(entranceWall));
		character->SetActorLocation(FVector(-5,0,0));
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("Subsystem detects a plane crossing"),character->GetActorLocation().Equals(FVector(1005,0,0),0.01));
		TestFalse(TEXT("Entrance wall exclusion is restored after transfer"),character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(entranceWall));
		TestTrue(TEXT("Destination-only portals retain exit wall clearance during transfer"),character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(exitWall));
		subsystem->UnregisterPortal(exit);
		TestFalse(TEXT("Removing a portal releases owned exclusions"),character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(exitWall));
		TestTrue(TEXT("Another system's exclusion survives cleanup"),character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(externalIgnore));
		TestTrue(TEXT("Removing a portal cannot leave a capsule embedded in its wall"),character->GetActorLocation().X>=1027);
		ACharacter* owner = world->SpawnActor<ACharacter>();
		owner->GetCapsuleComponent()->SetCapsuleSize(20,40);
		owner->SetActorLocation(FVector(30,0,0));
		APlayerController* controller = world->SpawnActor<APlayerController>();
		controller->Possess(owner);
		owner->SetRole(ROLE_AutonomousProxy);
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("Owning client predicts only its own entrance wall exclusions"),owner->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(entranceWall));
		owner->SetActorLocation(FVector(-5,0,0));
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("Owning client never executes the authority teleport"),owner->GetActorLocation().Equals(FVector(-5,0,0)));
		TestTrue(TEXT("Client clearance survives while waiting for the server position update"),owner->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(entranceWall));
		subsystem->UnregisterPortal(entry);
		TestFalse(TEXT("Owner collision prediction is cleaned up with the portal"),owner->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(entranceWall));
		// Restore the synthetic network role before destroying this standalone test world.
		owner->SetRole(ROLE_Authority);
		controller->UnPossess();
		controller->Destroy();
	}
	entry->SetDestination(nullptr);
	TestFalse(TEXT("Missing destination is inert"), entry->CanTraverse());
	entry->SetDestination(entry);
	TestFalse(TEXT("Self destinations are rejected"), entry->CanTraverse());
	entry->SetDestination(exit);
	exit->Destroy();
	TestFalse(TEXT("Destroyed exits are handled safely"), entry->CanTraverse());
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalActorDestinationTest, "ShootingArena.Portal.Spatial.ActorDestinations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalActorDestinationTest::RunTest(const FString&)
{
	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &values);
	if (!TestNotNull(TEXT("Actor destination test world"), world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	auto SpawnBox = [world](const FVector& location, const FVector& extent)
	{
		AActor* actor = world->SpawnActor<AActor>();
		UBoxComponent* box = NewObject<UBoxComponent>(actor);
		actor->SetRootComponent(box);
		box->SetBoxExtent(extent);
		box->SetCollisionObjectType(ECC_WorldStatic);
		box->SetCollisionResponseToAllChannels(ECR_Block);
		box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		box->RegisterComponent();
		actor->SetActorLocation(location);
		return actor;
	};
	SpawnBox(FVector(0,0,-50), FVector(500,500,50));
	SpawnBox(FVector(2000,0,-50), FVector(500,500,50));
	ASeamlessPortalActor* entry = world->SpawnActor<ASeamlessPortalActor>();
	// These anchor-height regressions intentionally use a rectangular doorway, independent of oval edge curvature.
	entry->bEllipticalOpening = false;
	entry->bCenterExitHorizontally = false; // Independently cover the optional original relative mapping.
	entry->SetActorLocationAndRotation(FVector(0,0,250), FRotator(0,-90,0));
	ACharacter* character = world->SpawnActor<ACharacter>();
	character->GetCapsuleComponent()->SetCapsuleSize(51,132);
	double walkingHeight = 134.15;
	auto TestAnchor = [&](AActor* target, const TCHAR* label, double expectedFrameHeight)
	{
		PortalTraversalGuard::Unlock(character);
		const FTransform original = target->GetActorTransform();
		entry->SetDestination(target);
		FTransform destination;
		TestTrue(FString::Printf(TEXT("%s is a valid destination without a portal class requirement"),label), entry->GetDestinationFrame(destination));
		TestTrue(FString::Printf(TEXT("%s aligns the virtual floor, not the target actor"),label), FMath::IsNearlyEqual(destination.GetLocation().Z,expectedFrameHeight,0.01));
		const FTransform source = entry->GetPortalFrameTransform();
		const double z = walkingHeight - source.GetLocation().Z;
		const FVector previous = source.TransformPositionNoScale(FVector(100,20,z));
		const FVector current = source.TransformPositionNoScale(FVector(-20,20,z));
		character->SetActorLocation(current);
		const FVector velocity = source.TransformVectorNoScale(FVector(-600,30,0));
		character->GetCharacterMovement()->Velocity = velocity;
		FPortalCaptureView capture;
		const FTransform camera(source.TransformPositionNoScale(FVector(100,20,z+64)));
		TestTrue(FString::Printf(TEXT("%s renders through the resolved exit"),label),entry->BuildPortalCaptureView(camera,capture));
		TestTrue(TEXT("Camera and traversal share the same corrected destination frame"),capture.transform.GetLocation().Equals(
			SeamlessPortalMath::MapPoint(source,destination,camera.GetLocation()),0.01));
		FVector safeExit;
		TestTrue(FString::Printf(TEXT("Walking through the rotated entrance reaches %s without jumping"),label),entry->TryTraverse(character,previous,safeExit));
		TestTrue(TEXT("Mapped feet stay above the destination floor"),character->GetActorLocation().Equals(
			SeamlessPortalMath::MapPoint(source,destination,current),0.01));
		TestTrue(TEXT("Rotation still redirects the original velocity"),character->GetCharacterMovement()->Velocity.Equals(
			SeamlessPortalMath::MapVector(source,destination,velocity),0.01));
		TestTrue(TEXT("Exit actor transform is never rewritten"),target->GetActorTransform().Equals(original));
		PortalTraversalGuard::Unlock(character);
	};
	ATargetPoint* point = world->SpawnActor<ATargetPoint>();
	point->SetActorLocationAndRotation(FVector(2000,0,0),FRotator(0,90,0));
	TestAnchor(point,TEXT("floor-level TargetPoint"),250);
	AActor* plainActor = world->SpawnActor<AActor>();
	USceneComponent* root = NewObject<USceneComponent>(plainActor);
	plainActor->SetRootComponent(root);
	root->RegisterComponent();
	plainActor->SetActorLocationAndRotation(FVector(2000,0,50),FRotator(0,35,0));
	TestAnchor(plainActor,TEXT("ordinary Actor with a low pivot"),250);
	AOneWayTeleportActor* legacy = world->SpawnActor<AOneWayTeleportActor>();
	legacy->SetActorLocationAndRotation(FVector(2000,0,137),FRotator(0,-90,0));
	TestAnchor(legacy,TEXT("legacy portal 113cm shorter than the entrance"),250);
	AStaticMeshActor* meshActor = world->SpawnActor<AStaticMeshActor>();
	meshActor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
	meshActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	meshActor->GetStaticMeshComponent()->SetCollisionResponseToAllChannels(ECR_Block);
	meshActor->SetActorScale3D(FVector(6,6,1));
	meshActor->SetActorLocationAndRotation(FVector(2000,0,-50),FRotator(0,45,0));
	TestAnchor(meshActor,TEXT("colliding StaticMeshActor used as the landing floor"),250);
	// Realistic walking clearance can put the capsule a few millimetres below the visible opening.
	entry->SetActorLocation(FVector(0,0,252.5));
	TestAnchor(point,TEXT("TargetPoint with a 0.35cm aperture boundary discrepancy"),252.5);
	AActor* ceiling = SpawnBox(FVector(2000,0,230),FVector(300,300,30));
	entry->SetDestination(point);
	const FTransform source = entry->GetPortalFrameTransform();
	const FVector previous = source.TransformPositionNoScale(FVector(100,0,walkingHeight-source.GetLocation().Z));
	const FVector current = source.TransformPositionNoScale(FVector(-20,0,walkingHeight-source.GetLocation().Z));
	character->SetActorLocation(current);
	FVector safeExit;
	TestFalse(TEXT("Floor alignment cannot bypass an obstructing ceiling"),entry->TryTraverse(character,previous,safeExit));
	TestTrue(TEXT("An unsafe ordinary exit leaves the player at the entrance"),character->GetActorLocation().Equals(current));
	TestFalse(TEXT("An unsafe ordinary exit never acquires a teleport lock"),PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	ceiling->Destroy();
	entry->bAlignActorExitToFloor = false;
	entry->SetDestination(point);
	FTransform exact;
	TestTrue(TEXT("Exact airborne anchor mode is available"),entry->GetDestinationFrame(exact));
	TestTrue(TEXT("Opting out preserves the literal actor transform"),exact.GetLocation().Equals(point->GetActorLocation()));
	TestFalse(TEXT("Exact mode does not bypass floor collision"),entry->TryTraverse(character,previous,safeExit));
	AActor* solidTarget = SpawnBox(FVector(2000,0,250),FVector(100,100,100));
	entry->SetDestination(solidTarget);
	TestFalse(TEXT("A solid actor target is not silently excluded from destination collision"),entry->TryTraverse(character,previous,safeExit));
	solidTarget->Destroy();
	entry->bAlignActorExitToFloor = true;
	ASeamlessPortalActor* spatialExit = world->SpawnActor<ASeamlessPortalActor>();
	spatialExit->SetActorLocation(FVector(2000,0,137));
	entry->SetDestination(spatialExit);
	TestTrue(TEXT("Spatial portal pairs remain valid"),entry->GetDestinationFrame(exact));
	TestTrue(TEXT("Spatial pairs never auto-move a deliberately placed doorway"),exact.GetLocation().Equals(spatialExit->GetPortalFrameTransform().GetLocation()));
	point->SetActorLocationAndRotation(FVector(4000,0,1000),FRotator(25,70,0));
	entry->SetDestination(point);
	TestTrue(TEXT("A rotated airborne target remains valid"),entry->GetDestinationFrame(exact));
	TestTrue(TEXT("No floor in range preserves an intentionally airborne exit"),exact.GetLocation().Equals(point->GetActorLocation()));
	double fraction;
	TestTrue(TEXT("Crossing detection remains independent of anchor resolution"),entry->FindCrossing(character,previous,fraction));
	TestTrue(TEXT("A non-yaw ordinary target can still be traversed"),entry->TryTraverse(character,previous,safeExit));
	PortalTraversalGuard::Unlock(character);
	// Reproduce the reported layout: entry Z470 over floor Z220.715, legacy exit Z1598 over floor Z1460.715.
	SpawnBox(FVector(0,0,170.715),FVector(500,500,50));
	SpawnBox(FVector(2000,0,1410.715),FVector(500,500,50));
	entry->SetActorLocation(FVector(0,0,470));
	legacy->SetActorLocation(FVector(2000,0,1598));
	walkingHeight = 220.715 + 132 + 2.15;
	TestAnchor(legacy,TEXT("reported rotated legacy exit with a 112cm floor mismatch"),1710);
	entry->SetDestination(point);
	point->Destroy();
	TestFalse(TEXT("Destroyed arbitrary targets safely disable traversal"),entry->CanTraverse());
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalForgivingCrossingTest, "ShootingArena.Portal.Spatial.HeadAndFootTolerance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalForgivingCrossingTest::RunTest(const FString&)
{
	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&values);
	if (!TestNotNull(TEXT("Isolated forgiving traversal world"),world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	ASeamlessPortalActor* entry = world->SpawnActor<ASeamlessPortalActor>();
	ASeamlessPortalActor* exit = world->SpawnActor<ASeamlessPortalActor>();
	entry->bAllowPartialCapsuleEntry = false; // Regression coverage for the optional former strict mode.
	exit->SetActorLocation(FVector(1000,0,0));
	entry->SetDestination(exit);
	ACharacter* character = world->SpawnActor<ACharacter>();
	character->GetCapsuleComponent()->SetCapsuleSize(51,132);
	// At the swept plane intersection the feet are 12cm below the opening, but by the end
	// of this jump frame they are inside. The former full-height test lost this crossing forever.
	const FVector previous(20,0,-145), current(-20,0,-115);
	character->SetActorLocation(current);
	double fraction;
	entry->headAndFootTolerance = 0;
	TestFalse(TEXT("Zero tolerance reproduces the strict jump rejection"),entry->FindCrossing(character,previous,fraction));
	entry->headAndFootTolerance = 20;
	TestTrue(TEXT("A jumping torso crossing is accepted despite slight foot overhang"),entry->FindCrossing(character,previous,fraction));
	TestTrue(TEXT("Swept plane-crossing timing is unchanged"),FMath::IsNearlyEqual(fraction,0.5));
	FVector safeExit;
	TestTrue(TEXT("A valid slightly-overhanging jump really reaches the exit"),entry->TryTraverse(character,previous,safeExit));
	TestTrue(TEXT("Tolerance never adds a teleport position offset"),character->GetActorLocation().Equals(FVector(1020,0,-115),0.01));
	TestTrue(TEXT("Real character capsule height is never changed"),FMath::IsNearlyEqual(character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),132.0));
	PortalTraversalGuard::Unlock(character);
	character->SetActorLocation(FVector(10,0,0));
	TestFalse(TEXT("Approaching/touching without a centre-plane crossing cannot teleport"),entry->FindCrossing(character,FVector(100,0,0),fraction));
	TestFalse(TEXT("Back-to-front crossings remain disabled"),entry->FindCrossing(character,FVector(-10,0,0),fraction));
	character->SetActorLocation(FVector(-10,160,0));
	TestFalse(TEXT("Side grazing is not made wider by head/foot tolerance"),entry->FindCrossing(character,FVector(10,160,0),fraction));
	character->SetActorLocation(FVector(-10,0,-180));
	TestFalse(TEXT("A torso too far below the portal is still rejected"),entry->FindCrossing(character,FVector(10,0,-180),fraction));
	entry->headAndFootTolerance = 100000;
	character->SetActorLocation(FVector(-10,0,-200));
	TestFalse(TEXT("Oversized tolerance values cannot remove the torso requirement"),entry->FindCrossing(character,FVector(10,0,-200),fraction));
	entry->headAndFootTolerance = 20;
	AActor* ground = world->SpawnActor<AActor>();
	UBoxComponent* floor = NewObject<UBoxComponent>(ground);
	ground->SetRootComponent(floor);
	floor->SetBoxExtent(FVector(200,200,50));
	floor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	floor->SetCollisionResponseToAllChannels(ECR_Block);
	floor->RegisterComponent();
	ground->SetActorLocation(FVector(1000,0,-300));
	character->SetActorLocation(FVector(-10,0,-130));
	TestTrue(TEXT("Torso eligibility allows a small foot overhang"),entry->FindCrossing(character,FVector(10,0,-130),fraction));
	TestFalse(TEXT("The FULL real capsule still prevents arrival inside the destination floor"),entry->TryTraverse(character,FVector(10,0,-130),safeExit));
	TestTrue(TEXT("Rejected arrival retains the original location"),character->GetActorLocation().Equals(FVector(-10,0,-130)));
	TestFalse(TEXT("Rejected arrival never starts the reentry lock"),PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalPartialEntryTest, "ShootingArena.Portal.Spatial.PartialCapsuleEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalPartialEntryTest::RunTest(const FString&)
{
	using namespace SeamlessPortalMath;
	const FVector2D opening(175,250);
	TestTrue(TEXT("A side/arm contact is accepted even with the centre outside the aperture"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,215,0),FVector::UpVector,51,132,opening,true));
	TestFalse(TEXT("A capsule entirely outside the side cannot trigger"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,230,0),FVector::UpVector,51,132,opening,true));
	TestTrue(TEXT("A head/foot-only contact is accepted"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,0,-350),FVector::UpVector,51,132,opening,true));
	TestFalse(TEXT("No head/foot contact means no traversal"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,0,-390),FVector::UpVector,51,132,opening,true));
	TestTrue(TEXT("A diagonal contact with a circular aperture uses the curved edge"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,77,77),FVector::UpVector,10,10,FVector2D(100,100),true));
	TestFalse(TEXT("An empty bounding-box corner is not part of the circular aperture"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,80,80),FVector::UpVector,10,10,FVector2D(100,100),true));
	TestTrue(TEXT("A rectangular corner can be touched by a rounded capsule"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,210,280),FVector::UpVector,51,51,opening,false));
	TestFalse(TEXT("A rectangular bounding-box expansion cannot admit a non-contact corner"),
		CapsuleTouchesOpening(FTransform::Identity,FVector(0,225,300),FVector::UpVector,51,51,opening,false));
	const FTransform rotated(FRotator(32,73,-21),FVector(700,-200,100));
	TestTrue(TEXT("Contact is invariant under a rotated portal and capsule"),
		CapsuleTouchesOpening(rotated,rotated.TransformPositionNoScale(FVector(0,215,0)),
			rotated.TransformVectorNoScale(FVector::UpVector),51,132,opening,true));
	const FTransform floorPortal(FRotator(90,0,0));
	TestTrue(TEXT("A floor portal handles a capsule axis normal to its surface"),
		CapsuleTouchesOpening(floorPortal,floorPortal.TransformPositionNoScale(FVector(0,215,0)),FVector::UpVector,51,132,opening,true));
	TestFalse(TEXT("Tilted partial contact still rejects a distant capsule"),
		CapsuleTouchesOpening(floorPortal,floorPortal.TransformPositionNoScale(FVector(0,240,0)),FVector::UpVector,51,132,opening,true));
	const FTransform tiltedPortal(FRotator(45,0,0));
	TestTrue(TEXT("A tilted capsule segment can touch the curved edge while its centre sphere misses"),
		CapsuleTouchesOpening(tiltedPortal,tiltedPortal.TransformPositionNoScale(FVector(0,0,310)),FVector::UpVector,51,132,opening,true));
	TestFalse(TEXT("Tilted capsule distance includes its normal offset, not just its projected silhouette"),
		CapsuleTouchesOpening(tiltedPortal,tiltedPortal.TransformPositionNoScale(FVector(0,0,330)),FVector::UpVector,51,132,opening,true));
	TestFalse(TEXT("Invalid capsule input is rejected"),CapsuleTouchesOpening(FTransform::Identity,FVector::ZeroVector,
		FVector::UpVector,std::numeric_limits<double>::quiet_NaN(),132,opening,true));
	TestFalse(TEXT("Invalid opening input is rejected"),CapsuleTouchesOpening(FTransform::Identity,FVector::ZeroVector,
		FVector::UpVector,51,132,FVector2D(0,250),true));

	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&values);
	if (!TestNotNull(TEXT("Isolated partial-entry world"),world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	auto* entry = world->SpawnActor<ASeamlessPortalActor>();
	entry->bCenterExitHorizontally = false; // This regression suite specifically covers relative side mapping.
	auto* exit = world->SpawnActor<ASeamlessPortalActor>();
	exit->SetActorLocation(FVector(1000,0,0));
	entry->SetDestination(exit);
	ACharacter* character = world->SpawnActor<ACharacter>();
	character->GetCapsuleComponent()->SetCapsuleSize(51,132);
	TestTrue(TEXT("Partial capsule entry is enabled by default"),entry->bAllowPartialCapsuleEntry);
	const FVector previous(10,-137.8,-115.85), current(-10,-137.8,-115.85);
	character->SetActorLocation(current);
	double fraction;
	entry->bAllowPartialCapsuleEntry = false;
	entry->headAndFootTolerance = 50;
	TestFalse(TEXT("The reported side contact reproduces the former strict rejection even at tolerance 50"),entry->FindCrossing(character,previous,fraction));
	entry->bAllowPartialCapsuleEntry = true;
	entry->headAndFootTolerance = 0;
	TestTrue(TEXT("The reported side contact crosses with no full-capsule containment requirement"),entry->FindCrossing(character,previous,fraction));
	FVector safeExit;
	TestTrue(TEXT("The actual reported off-centre walking contact reaches the exit"),entry->TryTraverse(character,previous,safeExit));
	TestTrue(TEXT("Partial entry preserves the mapped position, not a forced centre snap"),
		character->GetActorLocation().Equals(FVector(1010,137.8,-115.85),0.001));
	TestTrue(TEXT("The real collision capsule is not resized"),
		FMath::IsNearlyEqual(character->GetCapsuleComponent()->GetScaledCapsuleRadius(),51.0)
		&& FMath::IsNearlyEqual(character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),132.0));
	PortalTraversalGuard::Unlock(character);
	character->SetActorLocation(FVector(-10,215,0));
	TestTrue(TEXT("An arm-only contact can teleport, even with the centre beyond the side edge"),entry->TryTraverse(character,FVector(10,215,0),safeExit));
	PortalTraversalGuard::Unlock(character);
	character->SetActorLocation(FVector(10,215,0));
	TestTrue(TEXT("Capsule contact no longer waits for the centre-plane crossing"),entry->FindCrossing(character,FVector(100,215,0),fraction));
	TestFalse(TEXT("Reverse travel remains disabled for partial contacts"),entry->FindCrossing(character,FVector(-10,215,0),fraction));
	character->SetActorLocation(FVector(-10,230,0));
	TestFalse(TEXT("A non-contact crossing outside the side does not teleport"),entry->FindCrossing(character,FVector(10,230,0),fraction));
	// A permissive aperture is never permission to arrive inside solid geometry.
	AActor* wall = world->SpawnActor<AActor>();
	UBoxComponent* box = NewObject<UBoxComponent>(wall);
	wall->SetRootComponent(box);
	box->SetBoxExtent(FVector(100,100,200));
	box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	box->SetCollisionResponseToAllChannels(ECR_Block);
	box->RegisterComponent();
	wall->SetActorLocation(FVector(1010,-215,0));
	character->SetActorLocation(FVector(-10,215,0));
	TestFalse(TEXT("Partial entry still rejects a blocked exit using the FULL capsule"),entry->TryTraverse(character,FVector(10,215,0),safeExit));
	TestTrue(TEXT("Blocked partial entry retains the original location"),character->GetActorLocation().Equals(FVector(-10,215,0)));
	TestFalse(TEXT("Blocked partial entry does not acquire a reentry lock"),PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	wall->Destroy();
	// The world scheduler must use the same contact rule, not just direct TryTraverse calls.
	auto* subsystem = world->GetSubsystem<USeamlessPortalTraversalSubsystem>();
	if (TestNotNull(TEXT("Partial entry traversal scheduler"),subsystem))
	{
		subsystem->RegisterPortal(entry);
		character->SetActorLocation(FVector(10,215,0));
		subsystem->Tick(1.f/60);
		character->SetActorLocation(FVector(-10,215,0));
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("Scheduled side contact really teleports"),character->GetActorLocation().Equals(FVector(1010,-215,0),0.001));
		subsystem->UnregisterPortal(entry);
	}
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalContactEntryTest, "ShootingArena.Portal.Spatial.ContactBeforeCentre",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalContactEntryTest::RunTest(const FString&)
{
	using namespace SeamlessPortalMath;
	const FVector2D opening(175,250);
	double fraction;
	TestTrue(TEXT("First capsule contact is found while BOTH centres remain in front"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(100,0,0),FVector(30,0,0),FVector::UpVector,51,132,opening,true,fraction));
	TestTrue(TEXT("Early contact occurs at the actual leading surface, 51cm before the plane"),
		FMath::IsNearlyEqual(FMath::Lerp(100.0,30.0,fraction),51.0,0.001));
	TestTrue(TEXT("Arm-edge contact accounts for normal AND lateral distance"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(100,215,0),FVector(30,215,0),FVector::UpVector,51,132,opening,true,fraction));
	TestTrue(TEXT("Side contact is not falsely triggered at the full radius from the plane"),
		FMath::IsNearlyEqual(FMath::Lerp(100.0,30.0,fraction),FMath::Sqrt(51.0*51-40.0*40),0.001));
	TestFalse(TEXT("Projected arm overlap alone is insufficient before actual plane contact"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(100,215,0),FVector(40,215,0),FVector::UpVector,51,132,opening,true,fraction));
	TestTrue(TEXT("Swept lateral arm contact works with no normal centre movement"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(40,300,0),FVector(40,-300,0),FVector::UpVector,51,132,opening,true,fraction));
	TestTrue(TEXT("A fast grazing step finds its first contact even when both endpoints miss"),
		FMath::IsNearlyEqual(FMath::Lerp(300.0,-300.0,fraction),175+FMath::Sqrt(51.0*51-40.0*40),0.001));
	TestFalse(TEXT("Walking parallel just beyond capsule reach never triggers"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(52,300,0),FVector(52,0,0),FVector::UpVector,51,132,opening,true,fraction));
	TestFalse(TEXT("Standing in contact does not repeatedly fire a movement event"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(30,0,0),FVector(30,0,0),FVector::UpVector,51,132,opening,true,fraction));
	TestFalse(TEXT("Receding from a contacted portal never fires"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(30,0,0),FVector(40,0,0),FVector::UpVector,51,132,opening,true,fraction));
	TestFalse(TEXT("Rear-side contact cannot teleport"),
		FindFrontCapsuleContact(FTransform::Identity,FVector(-30,0,0),FVector(-20,0,0),FVector::UpVector,51,132,opening,true,fraction));
	TestFalse(TEXT("Nonfinite swept input is rejected"),FindFrontCapsuleContact(FTransform::Identity,
		FVector(100,0,0),FVector(std::numeric_limits<double>::infinity(),0,0),FVector::UpVector,51,132,opening,true,fraction));
	const FTransform floorPortal(FRotator(90,0,0));
	TestTrue(TEXT("Floor-portal contact uses the capsule height rather than its radius"),
		FindFrontCapsuleContact(floorPortal,floorPortal.TransformPositionNoScale(FVector(200,0,0)),
			floorPortal.TransformPositionNoScale(FVector(120,0,0)),FVector::UpVector,51,132,opening,true,fraction));
	TestTrue(TEXT("The feet contact a floor portal while the centre is still 132cm away"),
		FMath::IsNearlyEqual(FMath::Lerp(200.0,120.0,fraction),132.0,0.001));
	const FTransform rotated(FRotator(0,-90,0),FVector(-1240,-290,470.715));
	TestTrue(TEXT("The reported rotated portal contacts the walking capsule before its centre crosses"),
		FindFrontCapsuleContact(rotated,rotated.TransformPositionNoScale(FVector(60,-137.8,-115.85)),
			rotated.TransformPositionNoScale(FVector(29,-137.8,-115.85)),FVector::UpVector,51,132,opening,true,fraction));

	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&values);
	if (!TestNotNull(TEXT("Isolated early-contact world"),world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	auto* entry = world->SpawnActor<ASeamlessPortalActor>();
	entry->bCenterExitHorizontally = false; // Contact timing remains independent of the arrival policy.
	auto* exit = world->SpawnActor<ASeamlessPortalActor>();
	exit->SetActorLocation(FVector(1000,0,0));
	entry->SetDestination(exit);
	ACharacter* character = world->SpawnActor<ACharacter>();
	character->GetCapsuleComponent()->SetCapsuleSize(51,132);
	character->GetCharacterMovement()->Velocity = FVector(-600,0,0);
	character->SetActorLocation(FVector(30,215,0));
	FVector safeExit;
	TestTrue(TEXT("Actual early arm contact teleports before the centre reaches the entry"),
		entry->TryTraverse(character,FVector(100,215,0),safeExit));
	TestTrue(TEXT("Early contact places the FULL capsule on the exit FRONT, not behind its wall"),
		character->GetActorLocation().Equals(FVector(1053,-215,0),0.001));
	TestTrue(TEXT("Early arrival retains the redirected velocity"),character->GetVelocity().Equals(FVector(600,0,0),0.001));
	character->SetActorLocation(FVector(30,215,0));
	TestFalse(TEXT("The shared reentry lock prevents repeat early contacts"),entry->TryTraverse(character,FVector(100,215,0),safeExit));
	PortalTraversalGuard::Unlock(character);
	character->SetActorLocation(FVector(30,215,0));
	entry->bAllowPartialCapsuleEntry = false;
	TestFalse(TEXT("Strict-mode rollback keeps the old centre-plane requirement"),entry->FindCrossing(character,FVector(100,215,0),fraction));
	entry->bAllowPartialCapsuleEntry = true;
	AActor* wall = world->SpawnActor<AActor>();
	UBoxComponent* box = NewObject<UBoxComponent>(wall);
	wall->SetRootComponent(box);
	box->SetBoxExtent(FVector(30,100,200));
	box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	box->SetCollisionResponseToAllChannels(ECR_Block);
	box->RegisterComponent();
	wall->SetActorLocation(FVector(1053,-215,0));
	TestFalse(TEXT("An early-arrival exit obstruction is rejected with the FULL real capsule"),
		entry->TryTraverse(character,FVector(100,215,0),safeExit));
	TestTrue(TEXT("Blocked early contact leaves the player in front of the entry"),character->GetActorLocation().Equals(FVector(30,215,0)));
	TestFalse(TEXT("Blocked early contact leaves no lock"),PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	box->SetBoxExtent(FVector(500,500,50));
	wall->SetActorLocation(FVector(1000,0,-300));
	character->SetActorLocation(FVector(30,0,-130));
	TestTrue(TEXT("A head/foot contact can be eligible before the source centre crosses"),
		entry->FindCrossing(character,FVector(100,0,-130),fraction));
	TestFalse(TEXT("Early contact does not bypass a destination floor intersecting the actual feet"),
		entry->TryTraverse(character,FVector(100,0,-130),safeExit));
	wall->Destroy();
	auto* subsystem = world->GetSubsystem<USeamlessPortalTraversalSubsystem>();
	if (TestNotNull(TEXT("Early contact world scheduler"),subsystem))
	{
		subsystem->RegisterPortal(entry);
		character->SetActorLocation(FVector(100,215,0));
		subsystem->Tick(1.f/60);
		character->SetActorLocation(FVector(30,215,0));
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("The real scheduler teleports while the source centre is still in front"),
			character->GetActorLocation().Equals(FVector(1053,-215,0),0.001));
		ACharacter* owner = world->SpawnActor<ACharacter>();
		owner->GetCapsuleComponent()->SetCapsuleSize(51,132);
		owner->SetActorLocation(FVector(100,215,0));
		APlayerController* controller = world->SpawnActor<APlayerController>();
		controller->Possess(owner);
		owner->SetRole(ROLE_AutonomousProxy);
		subsystem->Tick(1.f/60);
		owner->SetActorLocation(FVector(30,215,0));
		subsystem->Tick(1.f/60);
		TestTrue(TEXT("Owning-client early contact never executes an authority teleport"),
			owner->GetActorLocation().Equals(FVector(30,215,0)));
		owner->SetRole(ROLE_Authority);
		controller->UnPossess();
		controller->Destroy();
		subsystem->UnregisterPortal(entry);
	}
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalCenteredExitTest,"ShootingArena.Portal.Spatial.CenteredExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalCenteredExitTest::RunTest(const FString&)
{
	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&values);
	if (!TestNotNull(TEXT("Isolated centred arrival world"),world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	auto SpawnBox = [world](const FVector& point, const FVector& extent)
	{
		AActor* actor = world->SpawnActor<AActor>();
		UBoxComponent* box = NewObject<UBoxComponent>(actor);
		actor->SetRootComponent(box);
		box->SetBoxExtent(extent);
		box->SetCollisionObjectType(ECC_WorldStatic);
		box->SetCollisionResponseToAllChannels(ECR_Block);
		box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		box->RegisterComponent();
		actor->SetActorLocation(point);
		return actor;
	};
	const FVector entrance(-1240,-290,470.715);
	const FVector exitAnchor(-2760.632,80.005,1598);
	SpawnBox(FVector(entrance.X,entrance.Y,170.715),FVector(500,500,50));
	SpawnBox(FVector(exitAnchor.X,exitAnchor.Y,1410.715),FVector(500,500,50));
	ASeamlessPortalActor* entry = world->SpawnActor<ASeamlessPortalActor>();
	entry->SetActorLocationAndRotation(entrance,FRotator(0,-90,0));
	AOneWayTeleportActor* legacyExit = world->SpawnActor<AOneWayTeleportActor>();
	legacyExit->SetActorLocationAndRotation(exitAnchor,FRotator(0,-90,0));
	entry->SetDestination(legacyExit);
	ACharacter* character = world->SpawnActor<ACharacter>();
	character->GetCapsuleComponent()->SetCapsuleSize(51,132);
	TestTrue(TEXT("Exit centring is enabled by default"),entry->bCenterExitHorizontally);
	const FTransform source = entry->GetPortalFrameTransform();
	FTransform destination;
	TestTrue(TEXT("The existing arbitrary-actor/legacy exit resolution is retained"),entry->GetDestinationFrame(destination));
	const FVector expected = destination.TransformPositionNoScale(FVector(53,0,-115.85));
	// Reproduce a one-sided Boolean/frame obstruction at the old right-edge arrival location.
	SpawnBox(expected-destination.GetRotation().GetRightVector()*154,FVector(30,30,200));
	FVector safeExit;
	entry->bCenterExitHorizontally = false;
	const FVector rightCurrent = source.TransformPositionNoScale(FVector(30,154,-115.85));
	character->SetActorLocation(rightCurrent);
	TestFalse(TEXT("Former relative mapping reproduces the blocked right edge"),
		entry->TryTraverse(character,source.TransformPositionNoScale(FVector(100,154,-115.85)),safeExit));
	entry->bCenterExitHorizontally = true;
	const FVector localVelocity(-600,30,10);
	for (double side : {-154.0,0.0,154.0})
	{
		PortalTraversalGuard::Unlock(character);
		character->SetActorLocation(source.TransformPositionNoScale(FVector(30,side,-115.85)));
		character->GetCharacterMovement()->Velocity = source.TransformVectorNoScale(localVelocity);
		TestTrue(TEXT("Left, centre and right contacts all reach the centre despite the side obstruction"),
			entry->TryTraverse(character,source.TransformPositionNoScale(FVector(100,side,-115.85)),safeExit));
		TestTrue(TEXT("All three contacts have exactly the same safe arrival position"),character->GetActorLocation().Equals(expected,0.01));
		TestTrue(TEXT("Walking height remains unchanged above the corrected exit floor"),FMath::IsNearlyEqual(character->GetActorLocation().Z,1594.865,0.01));
		TestTrue(TEXT("Centring does not change mapped velocity"),character->GetVelocity().Equals(
			SeamlessPortalMath::MapVector(source,destination,source.TransformVectorNoScale(localVelocity)),0.001));
	}
	FPortalCaptureView view;
	const FTransform camera(source.TransformPositionNoScale(FVector(100,154,-50)));
	TestTrue(TEXT("Off-centre capture still renders correctly"),entry->BuildPortalCaptureView(camera,view));
	TestTrue(TEXT("Only arrival is centred; render parallax is NOT centred"),view.transform.GetLocation().Equals(
		SeamlessPortalMath::MapPoint(source,destination,camera.GetLocation()),0.001));
	AActor* centreBlock = SpawnBox(expected,FVector(30,30,200));
	for (double side : {-154.0,0.0,154.0})
	{
		PortalTraversalGuard::Unlock(character);
		const FVector current = source.TransformPositionNoScale(FVector(30,side,-115.85));
		character->SetActorLocation(current);
		TestFalse(TEXT("A blocked centre rejects ALL entry sides instead of ignoring the wall"),
			entry->TryTraverse(character,source.TransformPositionNoScale(FVector(100,side,-115.85)),safeExit));
		TestTrue(TEXT("Failed centred arrival never moves the player"),character->GetActorLocation().Equals(current));
		TestFalse(TEXT("Failed centred arrival leaves no reentry lock"),PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	}
	centreBlock->Destroy();
	// Centre placement is shared by arbitrary Actors, not restricted to portals.
	SpawnBox(FVector(2000,0,-50),FVector(500,500,50));
	ATargetPoint* point = world->SpawnActor<ATargetPoint>();
	point->SetActorLocation(FVector(2000,0,0));
	entry->SetDestination(point);
	for (double side : {-154.0,154.0})
	{
		PortalTraversalGuard::Unlock(character);
		character->SetActorLocation(source.TransformPositionNoScale(FVector(30,side,-115.85)));
		TestTrue(TEXT("Both sides also reach an ordinary Actor exit"),
			entry->TryTraverse(character,source.TransformPositionNoScale(FVector(100,side,-115.85)),safeExit));
		TestTrue(TEXT("Ordinary Actor arrival is centred with the original floor alignment"),
			character->GetActorLocation().Equals(FVector(2053,0,134.15),0.01));
	}
	PortalTraversalGuard::Unlock(character);
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalPlacementTest, "ShootingArena.Portal.Spatial.OpeningAndFloorSnap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalPlacementTest::RunTest(const FString&)
{
#if WITH_EDITOR
	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Editor,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&values);
	if (!TestNotNull(TEXT("Isolated editor placement world"),world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Editor).SetCurrentWorld(world);
	AActor* ground = world->SpawnActor<AActor>();
	UBoxComponent* floor = NewObject<UBoxComponent>(ground);
	ground->SetRootComponent(floor);
	floor->SetBoxExtent(FVector(1000,1000,50));
	floor->SetCollisionObjectType(ECC_WorldStatic);
	floor->SetCollisionResponseToAllChannels(ECR_Block);
	floor->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	floor->RegisterComponent();
	ground->SetActorLocation(FVector(0,0,-50));
	ASeamlessPortalActor* portal = world->SpawnActor<ASeamlessPortalActor>();
	ASeamlessPortalActor* other = world->SpawnActor<ASeamlessPortalActor>();
	AOneWayTeleportActor* legacy = world->SpawnActor<AOneWayTeleportActor>();
	auto Screen = [](AOneWayTeleportActor* actor)
	{
		TArray<UStaticMeshComponent*> meshes;
		actor->GetComponents(meshes);
		for (auto* mesh : meshes) if (mesh->GetFName() == TEXT("PortalScreen")) return mesh;
		return static_cast<UStaticMeshComponent*>(nullptr);
	};
	UStaticMeshComponent* screen = Screen(portal);
	if (!TestNotNull(TEXT("Spatial screen component"),screen) || !TestNotNull(TEXT("Generated ellipse"),screen->GetStaticMesh().Get()))
	{ world->DestroyWorld(false); GEngine->DestroyWorldContext(world); return false; }
	UStaticMesh* generated = screen->GetStaticMesh();
	TestTrue(TEXT("New portals default to oval openings"),portal->bEllipticalOpening);
	TestTrue(TEXT("Ellipse geometry is transient rather than a Content asset"),generated->HasAnyFlags(RF_Transient));
	TestTrue(TEXT("Identical portal geometry is shared across instances"),Screen(other)->GetStaticMesh() == generated);
	TestTrue(TEXT("The default preview/collision depth is only 2cm"),FMath::IsNearlyEqual(portal->GetComponentByClass<UBoxComponent>()->GetScaledBoxExtent().X,1.0));
	TestTrue(TEXT("The legacy portal's depth is not changed"),FMath::IsNearlyEqual(legacy->GetComponentByClass<UBoxComponent>()->GetScaledBoxExtent().X,50.0));
	const auto& lod = generated->GetRenderData()->LODResources[0];
	TestEqual(TEXT("Smooth oval needs just 128 triangles"),lod.GetNumTriangles(),uint32(128));
	for (uint32 i=0; i<lod.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++i)
	{
		const FVector3f p = lod.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
		const FVector2f uv = lod.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i,0);
		TestTrue(TEXT("The ellipse contains no rectangular corner geometry"),p.X*p.X+p.Y*p.Y <= 2500.01f && FMath::Abs(p.Z)<0.001f);
		TestTrue(TEXT("Ellipse UVs preserve the existing capture orientation"),FMath::IsNearlyEqual(uv.X,0.5f+p.X/100,0.001f) && FMath::IsNearlyEqual(uv.Y,0.5f+p.Y/100,0.001f));
	}
	UStaticMesh* plane = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
	auto Winding = [](const FStaticMeshLODResources& mesh)
	{
		const FVector3f a = mesh.VertexBuffers.PositionVertexBuffer.VertexPosition(mesh.IndexBuffer.GetIndex(0));
		const FVector3f b = mesh.VertexBuffers.PositionVertexBuffer.VertexPosition(mesh.IndexBuffer.GetIndex(1));
		const FVector3f c = mesh.VertexBuffers.PositionVertexBuffer.VertexPosition(mesh.IndexBuffer.GetIndex(2));
		return FVector3f::CrossProduct(b-a,c-a).Z;
	};
	TestTrue(TEXT("Ellipse triangles face the same side as the original engine plane"),Winding(lod)*Winding(plane->GetRenderData()->LODResources[0])>0);
	const FQuat rotation = portal->GetActorQuat();
	portal->SnapPortalToFloor();
	TestTrue(TEXT("One button lifts a half-buried oval so its lower edge touches the floor"),FMath::IsNearlyEqual(portal->GetPortalFrameTransform().GetLocation().Z-portal->GetOpeningHalfSize().Y,0.0,0.01));
	TestNull(TEXT("Floor snap never attaches the portal to the floor actor"),portal->GetAttachParentActor());
	TestTrue(TEXT("Floor snap never changes rotation"),portal->GetActorQuat().Equals(rotation));
	portal->AddActorWorldOffset(FVector(0,0,100));
	const FVector manuallyMoved = portal->GetActorLocation();
	portal->RerunConstructionScripts();
	TestTrue(TEXT("Manual movement and reconstruction never re-snap the portal"),portal->GetActorLocation().Equals(manuallyMoved));
	portal->SnapPortalToFloor();
	TestTrue(TEXT("Pressing the button again deliberately re-snaps"),FMath::IsNearlyEqual(portal->GetActorLocation().Z,250.0,0.01));
	portal->SetActorLocationAndRotation(FVector(0,0,30),FRotator(0,60,30));
	portal->SetActorScale3D(FVector(2,0.75,1.25));
	portal->RerunConstructionScripts();
	const FQuat tiltedRotation = portal->GetActorQuat();
	portal->SnapPortalToFloor();
	double minimumZ = DBL_MAX;
	for (uint32 i=0; i<lod.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++i)
		minimumZ = FMath::Min(minimumZ,screen->GetComponentTransform().TransformPosition(FVector(lod.VertexBuffers.PositionVertexBuffer.VertexPosition(i))).Z);
	TestTrue(FString::Printf(TEXT("Rotated, non-uniformly scaled oval geometry rests on the floor (min Z %.4f)"),minimumZ),minimumZ>=-0.01 && minimumZ<0.2);
	TestTrue(TEXT("Tilt and yaw survive floor snapping"),portal->GetActorQuat().Equals(tiltedRotation));
	portal->SetActorLocation(FVector(8000,0,30));
	const FVector noFloor = portal->GetActorLocation();
	portal->SnapPortalToFloor();
	TestTrue(TEXT("A missing floor leaves the actor unchanged"),portal->GetActorLocation().Equals(noFloor));
	portal->bEllipticalOpening = false;
	portal->RerunConstructionScripts();
	TestTrue(TEXT("Rectangle remains selectable"),screen->GetStaticMesh() == plane);
	portal->bEllipticalOpening = true;
	portal->RerunConstructionScripts();
	TestTrue(TEXT("Switching back reuses the cached oval geometry"),screen->GetStaticMesh() == generated);
	FStructProperty* scale = FindFProperty<FStructProperty>(portal->GetClass(),TEXT("portalShapeScale"));
	*scale->ContainerPtrToValuePtr<FVector2D>(portal) = FVector2D(4,4);
	portal->RerunConstructionScripts();
	TestTrue(TEXT("Equal shape scale produces a circle"),FMath::IsNearlyEqual(portal->GetOpeningHalfSize().X/0.75,portal->GetOpeningHalfSize().Y/1.25,0.01));
	FObjectProperty* shape = FindFProperty<FObjectProperty>(portal->GetClass(),TEXT("portalShapeMesh"));
	shape->SetObjectPropertyValue_InContainer(portal,plane);
	portal->RerunConstructionScripts();
	TestTrue(TEXT("An explicitly assigned designer mesh is never replaced by a generated oval"),screen->GetStaticMesh() == plane);
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
#endif
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalLifetimeTest, "ShootingArena.Portal.Spatial.LifetimeSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalLifetimeTest::RunTest(const FString&)
{
	const auto values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &values);
	if (!TestNotNull(TEXT("Isolated lifetime regression world"), world)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	auto* subsystem = world->GetSubsystem<USeamlessPortalTraversalSubsystem>();
	if (!TestNotNull(TEXT("Traversal lifetime subsystem"), subsystem))
	{ world->DestroyWorld(false); GEngine->DestroyWorldContext(world); return false; }

	ATargetPoint* target = world->SpawnActor<ATargetPoint>();
	target->SetActorLocation(FVector(1000, 0, 0));
	for (bool bShutdownOnArrival : {false, true})
	{
		auto* portal = world->SpawnActor<ASeamlessPortalActor>();
		portal->bAlignActorExitToFloor = false;
		portal->SetDestination(target);
		auto* character = world->SpawnActor<ACharacter>();
		character->GetCapsuleComponent()->SetCapsuleSize(20, 40);
		character->SetActorLocation(FVector(100, 0, 0));
		subsystem->RegisterPortal(portal);
		subsystem->Tick(1.f / 60);
		character->SetActorLocation(FVector(10, 0, 0));
		bool bArrivalCallback = false;
		const FDelegateHandle handle = character->GetRootComponent()->TransformUpdated.AddLambda(
			[&](USceneComponent* component, EUpdateTransformFlags, ETeleportType)
			{
				if (bArrivalCallback || component->GetComponentLocation().X < 500) return;
				bArrivalCallback = true;
				// These callbacks run synchronously inside TeleportTo, before Tick resumes.
				subsystem->UnregisterPortal(portal);
				portal->Destroy();
				if (bShutdownOnArrival) world->bIsTearingDown = true;
			});
		subsystem->Tick(1.f / 60);
		character->GetRootComponent()->TransformUpdated.Remove(handle);
		TestTrue(TEXT("Arrival callback executed during traversal"), bArrivalCallback);
		TestFalse(TEXT("Portal can be destroyed synchronously without retaining an invalid iterator"), IsValid(portal));
		TestTrue(TEXT("A successful teleport is not rolled back after its portal is removed"), character->GetActorLocation().X > 1000);
		TestTrue(TEXT("Arrival still starts the shared anti-reentry lock"), PortalTraversalGuard::IsLocked(character, world->GetTimeSeconds()));
		// Let DestroyWorld own subsystem Deinitialize; calling it manually would deinitialize it twice.
		world->bIsTearingDown = false;
		PortalTraversalGuard::Unlock(character);
		character->Destroy();
	}
	world->DestroyWorld(false);
	GEngine->DestroyWorldContext(world);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalDefaultSurfaceTest, "ShootingArena.Portal.Spatial.DefaultSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalDefaultSurfaceTest::RunTest(const FString&)
{
	UMaterialInterface* material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"));
	if (!TestNotNull(TEXT("Cookable engine surface material exists"), material)) return false;
	UTexture* texture = nullptr;
	TestTrue(TEXT("Default material exposes the capture texture"), material->GetTextureParameterValue(FMaterialParameterInfo(TEXT("SlateUI")), texture));
	TestEqual(TEXT("Default portal surface is opaque"), material->GetBlendMode(), BLEND_Opaque);
	TestTrue(TEXT("The spatial surface does not relight captured scene radiance"),material->GetShadingModels().HasShadingModel(MSM_Unlit));
	UStaticMesh* mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!TestNotNull(TEXT("Default aperture mesh"), mesh) || !TestNotNull(TEXT("Mesh render data"), mesh->GetRenderData())) return false;
	const FStaticMeshLODResources& lod = mesh->GetRenderData()->LODResources[0];
	const FBoxSphereBounds bounds = mesh->GetBounds();
	for (uint32 i = 0; i < lod.VertexBuffers.PositionVertexBuffer.GetNumVertices(); ++i)
	{
		const FVector3f p = lod.VertexBuffers.PositionVertexBuffer.VertexPosition(i);
		const FVector2f uv = lod.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i,0);
		TestTrue(TEXT("Default plane UVs agree with the spatial projection orientation"),
			FMath::IsNearlyEqual(double(uv.X), 0.5 + (p.X - bounds.Origin.X) / (bounds.BoxExtent.X * 2),0.001)
			&& FMath::IsNearlyEqual(double(uv.Y), 0.5 + (p.Y - bounds.Origin.Y) / (bounds.BoxExtent.Y * 2),0.001));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalAppearanceTest, "ShootingArena.Portal.Spatial.CaptureAppearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpatialPortalAppearanceTest::RunTest(const FString&)
{
	USceneCaptureComponent2D* capture = NewObject<USceneCaptureComponent2D>();
	const FTransform destination(FRotator(15,70,0),FVector(1000,-500,250));
	capture->SetWorldTransform(destination);
	capture->bUseCustomProjectionMatrix = true;
	const FMatrix projection(FPlane(2,0,0,0),FPlane(0,3,0,0),FPlane(0.2,0.1,0,1),FPlane(0,0,100,0));
	capture->CustomProjectionMatrix = projection;
	for (bool bHighQuality : {false,true})
	{
		UPortalViewSubsystem::ConfigureCaptureAppearance(*capture,true,bHighQuality);
		TestTrue(TEXT("Spatial radiance is HDR even when costly lighting is disabled"),capture->CaptureSource == SCS_SceneColorHDR);
		TestTrue(TEXT("Deferred capture inherits the player's resolved post-process settings"),capture->bMainViewFamily && capture->bInheritMainViewCameraPostProcessSettings);
		TestFalse(TEXT("Matching appearance never replaces the exit camera with the entrance camera"),capture->ShouldRenderWithMainViewCamera());
		TestFalse(TEXT("Matching appearance never replaces the dynamic target resolution with viewport resolution"),capture->ShouldRenderWithMainViewResolution());
		TestTrue(TEXT("Off-axis portal projection is preserved"),capture->bUseCustomProjectionMatrix && capture->CustomProjectionMatrix.Equals(projection));
		TestTrue(TEXT("Portal camera transform is preserved"),capture->GetComponentTransform().Equals(destination));
		TestFalse(TEXT("No independent capture eye adaptation is baked into the HDR texture"),capture->ShowFlags.EyeAdaptation);
		TestEqual(TEXT("Capture does not blend a second post-process stack over the player settings"),capture->PostProcessBlendWeight,0.0f);
		TestFalse(TEXT("Bloom is applied by the player camera once"),capture->ShowFlags.Bloom);
		TestTrue(TEXT("Fog is retained so a spatial window does not silently change atmospheric brightness"),capture->ShowFlags.Fog && capture->ShowFlags.VolumetricFog);
		TestEqual(TEXT("Lumen GI cost still obeys the existing quality toggle"),bool(capture->ShowFlags.LumenGlobalIllumination),bHighQuality);
		TestEqual(TEXT("Dynamic shadow cost still obeys the existing quality toggle"),bool(capture->ShowFlags.DynamicShadows),bHighQuality);
	}
	UPortalViewSubsystem::ConfigureCaptureAppearance(*capture,false,false);
	TestTrue(TEXT("Legacy low-cost portal keeps its existing LDR capture path"),capture->CaptureSource == SCS_FinalColorLDR);
	TestFalse(TEXT("Legacy captures never wait for a main view family"),capture->bMainViewFamily);
	TestFalse(TEXT("Legacy captures do not inherit new spatial-only PP behavior"),capture->bInheritMainViewCameraPostProcessSettings);
	TestEqual(TEXT("Legacy post-process blend behavior is unchanged"),capture->PostProcessBlendWeight,1.0f);
	TestFalse(TEXT("Legacy low-cost volumetric fog behavior is unchanged"),capture->ShowFlags.VolumetricFog);
	UPortalViewSubsystem::ConfigureCaptureAppearance(*capture,false,true);
	TestTrue(TEXT("Legacy high-quality HDR capture remains available"),capture->CaptureSource == SCS_SceneColorHDR);
	TestTrue(TEXT("Legacy high-quality post-process flags remain unchanged"),capture->ShowFlags.Bloom && capture->ShowFlags.EyeAdaptation);
	return true;
}

#endif

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

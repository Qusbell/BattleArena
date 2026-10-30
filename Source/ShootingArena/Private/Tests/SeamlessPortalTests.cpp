#if WITH_DEV_AUTOMATION_TESTS

#include "../../Teleport/SeamlessPortalActor.h"
#include "../../Teleport/SeamlessPortalMath.h"
#include "../../Teleport/SeamlessPortalTraversalSubsystem.h"
#include "../../Teleport/PortalTraversalGuard.h"
#include "../../Teleport/PortalViewSubsystem.h"
#include "../../Teleport/SeamlessPortalSyncComponent.h"
#include "Character/CharacterPoolComponent.h"
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
		TestTrue(FString::Printf(TEXT("Walking through the rotated entrance reaches %s without jumping"),label),entry->TryTraverseFromOverlap(character,safeExit));
		TestTrue(TEXT("Arrival clears the exit plane without changing the mapped foot height"),
			character->GetActorLocation().Equals(safeExit,0.01)
			&& FMath::IsNearlyEqual(character->GetActorLocation().Z,
				SeamlessPortalMath::MapPoint(source,destination,current).Z,0.01));
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
	TestFalse(TEXT("Floor alignment cannot bypass an obstructing ceiling"),entry->TryTraverseFromOverlap(character,safeExit));
	TestTrue(TEXT("An unsafe ordinary exit leaves the player at the entrance"),character->GetActorLocation().Equals(current));
	TestFalse(TEXT("An unsafe ordinary exit never acquires a teleport lock"),PortalTraversalGuard::IsLocked(character,world->GetTimeSeconds()));
	ceiling->Destroy();
	entry->bAlignActorExitToFloor = false;
	entry->SetDestination(point);
	FTransform exact;
	TestTrue(TEXT("Exact airborne anchor mode is available"),entry->GetDestinationFrame(exact));
	TestTrue(TEXT("Opting out preserves the literal actor transform"),exact.GetLocation().Equals(point->GetActorLocation()));
	TestFalse(TEXT("Exact mode does not bypass floor collision"),entry->TryTraverseFromOverlap(character,safeExit));
	AActor* solidTarget = SpawnBox(FVector(2000,0,250),FVector(100,100,100));
	entry->SetDestination(solidTarget);
	TestFalse(TEXT("A solid actor target is not silently excluded from destination collision"),entry->TryTraverseFromOverlap(character,safeExit));
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
	TestTrue(TEXT("A non-yaw ordinary target can still be traversed"),entry->TryTraverseFromOverlap(character,safeExit));
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
		entry->TryTraverseFromOverlap(character,safeExit));
	entry->bCenterExitHorizontally = true;
	const FVector localVelocity(-600,30,10);
	for (double side : {-154.0,0.0,154.0})
	{
		PortalTraversalGuard::Unlock(character);
		character->SetActorLocation(source.TransformPositionNoScale(FVector(30,side,-115.85)));
		character->GetCharacterMovement()->Velocity = source.TransformVectorNoScale(localVelocity);
		TestTrue(TEXT("Left, centre and right contacts all reach the centre despite the side obstruction"),
			entry->TryTraverseFromOverlap(character,safeExit));
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
			entry->TryTraverseFromOverlap(character,safeExit));
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
			entry->TryTraverseFromOverlap(character,safeExit));
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

namespace PortalOverlapTests
{
	struct FWorld
	{
		UWorld* world=nullptr;
		ASeamlessPortalActor* entry=nullptr;
		ATargetPoint* exit=nullptr;
		ACharacter* character=nullptr;
		USeamlessPortalTraversalSubsystem* subsystem=nullptr;
		FWorld()
		{
			const auto values=UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
			world=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&values);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
			world->InitializeActorsForPlay(FURL());
			entry=world->SpawnActor<ASeamlessPortalActor>();
			exit=world->SpawnActor<ATargetPoint>(); exit->SetActorLocation(FVector(1000,0,0));
			entry->bAlignActorExitToFloor=false; entry->SetDestination(exit);
			entry->DispatchBeginPlay();
			subsystem=world->GetSubsystem<USeamlessPortalTraversalSubsystem>();
			character=SpawnCharacter(FVector(500,0,0));
			// PrimitiveComponent suppresses overlap delegates before the world has begun play.
			// This isolated physics world has no GameMode to set the flag for us.
			world->SetBegunPlay(true);
		}
		ACharacter* SpawnCharacter(const FVector& location)
		{
			auto* result=world->SpawnActor<ACharacter>(ACharacter::StaticClass(),location,FRotator::ZeroRotator);
			result->GetCapsuleComponent()->SetCapsuleSize(20,40);
			result->GetCapsuleComponent()->SetGenerateOverlapEvents(true);
			result->DispatchBeginPlay();
			return result;
		}
		UBoxComponent* EntryBox() const
		{
			TArray<UBoxComponent*> boxes; entry->GetComponents(boxes);
			for (auto* box:boxes) if(box->GetFName()==TEXT("EntryCollision")) return box;
			return nullptr;
		}
		AActor* Wall(const FVector& location,const FVector& extent)
		{
			auto* actor=world->SpawnActor<AActor>();
			auto* box=NewObject<UBoxComponent>(actor);
			actor->SetRootComponent(box); actor->AddInstanceComponent(box);
			box->SetBoxExtent(extent); box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			box->SetCollisionResponseToAllChannels(ECR_Block); box->RegisterComponent();
			actor->SetActorLocation(location);
			return actor;
		}
		void Move(const FVector& location) { character->SetActorLocation(location,false,nullptr,ETeleportType::TeleportPhysics); }
		void Tick() { subsystem->Tick(1.f/60); }
		~FWorld()
		{
			world->bIsTearingDown=false;
			world->EndPlay(EEndPlayReason::Quit);
			world->DestroyWorld(false); GEngine->DestroyWorldContext(world);
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalOverlapEntryTest,"ShootingArena.Portal.Spatial.OverlapEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalOverlapEntryTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	f.Move(FVector(100,0,0)); f.Tick();
	TestTrue(TEXT("Nearby registration is not a teleport trigger"),f.character->GetActorLocation().Equals(FVector(100,0,0)));
	f.character->GetCharacterMovement()->Velocity=FVector(-600,30,0);
	f.Move(FVector(15,187,260));
	TestTrue(TEXT("Real engine overlap accepts a capsule partly outside the rectangular opening"),f.EntryBox()->IsOverlappingActor(f.character));
	f.Tick();
	TestTrue(TEXT("Capsule centre need not cross the portal plane"),f.character->GetActorLocation().X>1000);
	TestTrue(TEXT("Existing horizontal exit-centre placement is retained"),FMath::Abs(f.character->GetActorLocation().Y)<0.01);
	TestTrue(TEXT("Existing redirected momentum is retained"),f.character->GetVelocity().Equals(FVector(600,-30,0),0.01));
	const FVector arrival=f.character->GetActorLocation();
	f.EntryBox()->OnComponentBeginOverlap.Broadcast(f.EntryBox(),f.character,f.character->GetCapsuleComponent(),0,false,FHitResult());
	f.Tick();
	TestTrue(TEXT("Repeated callbacks cannot bypass the shared reentry lock"),f.character->GetActorLocation().Equals(arrival));
	PortalTraversalGuard::Unlock(f.character);
	f.Move(FVector(-500,0,0)); f.Tick();
	f.Move(FVector(-15,0,0)); f.Tick();
	TestTrue(TEXT("Overlap entry has no hidden front/centre crossing condition"),f.character->GetActorLocation().X>1000);
	auto* unreported=f.SpawnCharacter(FVector(500,0,200));
	unreported->GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	unreported->SetActorLocation(FVector(0,0,200));
	f.Tick();
	TestTrue(TEXT("Unreported characters are not found by a hidden world scan or distance trigger"),unreported->GetActorLocation().Equals(FVector(0,0,200)));
	unreported->Destroy();
	auto* first=f.SpawnCharacter(FVector(500,0,120));
	auto* second=f.SpawnCharacter(FVector(500,0,220));
	first->SetActorLocation(FVector(15,0,120)); second->SetActorLocation(FVector(15,0,220));
	f.Tick();
	TestTrue(TEXT("Authority processes all overlapping characters, including AI, without a world scan"),
		first->GetActorLocation().X>1000 && second->GetActorLocation().X>1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalOverlapSafetyTest,"ShootingArena.Portal.Spatial.OverlapSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalOverlapSafetyTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	auto* obstruction=f.Wall(FVector(1022,0,0),FVector(80,80,80));
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Blocked exit rejects an actual overlap without moving to an unsafe location"),f.character->GetActorLocation().X<100);
	TestFalse(TEXT("Rejected overlaps do not acquire the teleport lock"),PortalTraversalGuard::IsLocked(f.character,f.world->GetTimeSeconds()));
	obstruction->Destroy(); f.Move(FVector(500,0,0));
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Leaving and re-entering succeeds after an exit obstruction is removed"),f.character->GetActorLocation().X>1000);
	PortalTraversalGuard::Unlock(f.character);
	f.Move(FVector(500,0,0)); f.entry->SetDestination(nullptr);
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Destination-only portals cannot teleport overlap entrants"),f.character->GetActorLocation().X<100);
	f.Move(FVector(500,0,0)); f.entry->SetDestination(f.exit);
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Assigning an exit at runtime re-enables engine overlap entry"),f.character->GetActorLocation().X>1000);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalOverlapWallsTest,"ShootingArena.Portal.Spatial.OverlapWallLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalOverlapWallsTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	auto* wall=f.Wall(FVector::ZeroVector,FVector(5,175,250));
	auto* unrelated=f.world->SpawnActor<AActor>();
	f.entry->passageWalls.Add(wall);
	f.character->GetCapsuleComponent()->IgnoreActorWhenMoving(unrelated,true);
	f.Move(FVector(100,0,0)); f.Tick();
	TestTrue(TEXT("Nearby overlap prepares wall exclusions before thin entry contact"),f.character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(wall));
	f.Move(FVector(5,0,0));
	f.entry->Destroy();
	f.Tick();
	TestTrue(TEXT("Removing a portal cannot leave its capsule embedded in the old wall"),f.character->GetActorLocation().X>=25);
	TestFalse(TEXT("Portal removal releases only owned exclusions"),f.character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(wall));
	TestTrue(TEXT("Another system's exclusions survive portal removal"),f.character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(unrelated));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalOverlapPoolTest,"ShootingArena.Portal.Spatial.OverlapPoolAndClient",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalOverlapPoolTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	f.Move(FVector(15,0,0)); // queues a real entry for this life
	f.character->SetActorEnableCollision(false);
	f.subsystem->ForgetCharacter(f.character);
	f.Tick();
	TestTrue(TEXT("Pool reset invalidates queued entries belonging to the old life"),f.character->GetActorLocation().X<100);
	f.Move(FVector(100,0,0));
	f.character->SetActorEnableCollision(true);
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Re-enabled pooled capsules are registered by fresh overlap events"),f.character->GetActorLocation().X>1000);
	PortalTraversalGuard::Unlock(f.character);
	f.Move(FVector(500,0,0));
	auto* pc=f.world->SpawnActor<APlayerController>();
	pc->Possess(f.character); f.character->SetRole(ROLE_AutonomousProxy);
	auto* wall=f.Wall(FVector::ZeroVector,FVector(5,175,250));
	f.entry->passageWalls.Add(wall);
	f.Move(FVector(100,0,0)); f.Tick();
	TestTrue(TEXT("Only the local owner's nearby overlaps predict wall exclusions"),f.character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(wall));
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Owning client overlaps never execute an authoritative teleport"),f.character->GetActorLocation().X<100);
	auto* remote=f.SpawnCharacter(FVector(500,0,150));
	remote->SetRole(ROLE_SimulatedProxy);
	remote->SetActorLocation(FVector(15,0,150)); f.Tick();
	TestFalse(TEXT("Remote replicated characters are not processed for local collision prediction"),remote->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(wall));
	f.character->SetRole(ROLE_Authority); remote->SetRole(ROLE_Authority); pc->UnPossess();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalOverlapCallbackTest,"ShootingArena.Portal.Spatial.OverlapCallbackSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalOverlapCallbackTest::RunTest(const FString&)
{
	for (bool tearDown:{false,true})
	{
		PortalOverlapTests::FWorld f;
		bool called=false;
		const auto handle=f.character->GetRootComponent()->TransformUpdated.AddLambda(
			[&](USceneComponent* component,EUpdateTransformFlags,ETeleportType)
			{
				if (called || component->GetComponentLocation().X<900) return;
				called=true; f.entry->Destroy();
				if (tearDown) f.world->bIsTearingDown=true;
			});
		f.Move(FVector(15,0,0)); f.Tick();
		f.character->GetRootComponent()->TransformUpdated.Remove(handle);
		TestTrue(TEXT("Arrival invokes synchronous destruction/teardown callback"),called);
		TestTrue(TEXT("Accepted arrival is not rolled back when its source is destroyed"),f.character->GetActorLocation().X>1000);
		f.world->bIsTearingDown=false;
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalRegistryTest,"ShootingArena.Portal.Spatial.RegistryLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalRegistryTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	auto* views=f.world->GetSubsystem<UPortalViewSubsystem>();
	TestEqual(TEXT("Seamless BeginPlay registers through the shared legacy base"),views->GetRegisteredPortalCount(),1);
	auto* legacy=f.world->SpawnActor<AOneWayTeleportActor>(); legacy->DispatchBeginPlay();
	TestEqual(TEXT("Legacy portals remain registered alongside Seamless portals"),views->GetRegisteredPortalCount(),2);
	views->RegisterPortal(f.entry);
	TestEqual(TEXT("Duplicate registration does not duplicate capture candidates"),views->GetRegisteredPortalCount(),2);
	legacy->Destroy();
	TestEqual(TEXT("Legacy EndPlay unregisters its rendering candidate"),views->GetRegisteredPortalCount(),1);
	f.entry->Destroy();
	TestEqual(TEXT("Seamless EndPlay also unregisters its rendering candidate"),views->GetRegisteredPortalCount(),0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalOverlapSweepTest,"ShootingArena.Portal.Spatial.OverlapExitPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalOverlapSweepTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	auto* obstruction=f.Wall(FVector(1100,0,0),FVector(5,80,80));
	f.Move(FVector(-200,0,0));
	FVector safeExit;
	TestFalse(TEXT("An empty landing point cannot bypass an intervening exit obstruction"),
		f.entry->TryTraverseFromOverlap(f.character,safeExit));
	TestTrue(TEXT("Rejected exit sweeps retain the original character position"),f.character->GetActorLocation().Equals(FVector(-200,0,0)));
	obstruction->Destroy();
	TestTrue(TEXT("The same mapped arrival succeeds when its exit path is clear"),f.entry->TryTraverseFromOverlap(f.character,safeExit));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalDisabledEntryTest,"ShootingArena.Portal.Spatial.DisabledQueuedEntry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalDisabledEntryTest::RunTest(const FString&)
{
	for (bool disableActor:{false,true})
	{
		PortalOverlapTests::FWorld f;
		f.Move(FVector(15,0,0));
		if (disableActor) f.character->SetActorEnableCollision(false);
		else f.character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		f.Tick();
		TestTrue(TEXT("Disabling collision invalidates a previously queued entry"),f.character->GetActorLocation().X<100);
		TestFalse(TEXT("Disabled entrants never acquire the teleport lock"),PortalTraversalGuard::IsLocked(f.character,f.world->GetTimeSeconds()));
		f.Move(FVector(500,0,0));
		f.character->SetActorEnableCollision(true);
		f.character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		f.Move(FVector(15,0,0)); f.Tick();
		TestTrue(TEXT("Fresh overlap entry still works after collision is restored"),f.character->GetActorLocation().X>1000);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalLifeCallbackTest,"ShootingArena.Portal.Spatial.ArrivalLifeChange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalLifeCallbackTest::RunTest(const FString&)
{
	for (bool clientArrival:{false,true})
	{
		PortalOverlapTests::FWorld f;
		auto* pool=NewObject<UCharacterPoolComponent>(f.character);
		f.character->AddInstanceComponent(pool); pool->RegisterComponent(); pool->Life.bManaged=true;
		auto* pc=f.world->SpawnActor<APlayerController>(); pc->Possess(f.character);
		auto* sync=USeamlessPortalSyncComponent::FindOrCreate(f.character);
		if (clientArrival) f.character->SetRole(ROLE_AutonomousProxy);
		const FVector respawn(500,0,120), freshVelocity(0,170,0);
		bool called=false;
		const auto handle=f.character->GetRootComponent()->TransformUpdated.AddLambda(
			[&](USceneComponent* component,EUpdateTransformFlags,ETeleportType)
			{
				if (called || component->GetComponentLocation().X<900) return;
				called=true;
				f.subsystem->ForgetCharacter(f.character);
				++pool->Life.Generation;
				f.character->SetActorLocation(respawn,false,nullptr,ETeleportType::TeleportPhysics);
				// No floor exists in this isolated world; Flying is a valid new-life mode
				// that the engine's own OnTeleported will preserve before our portal resumes.
				f.character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
				f.character->GetCharacterMovement()->Velocity=freshVelocity;
			});
		if (clientArrival)
		{
			FSeamlessPortalArrival arrival; arrival.bValid=true; arrival.location=FVector(1022,0,0); arrival.velocity=FVector(600,0,0);
			TestFalse(TEXT("Owner arrival stops updating an actor whose pooled life changed during teleport"),sync->ApplyArrival(arrival));
		}
		else { f.Move(FVector(15,0,0)); f.Tick(); }
		f.character->GetRootComponent()->TransformUpdated.Remove(handle);
		TestTrue(TEXT("Life reset was invoked from the actual teleport transform callback"),called);
		TestTrue(TEXT("Old arrival cannot overwrite the new life position"),f.character->GetActorLocation().Equals(respawn));
		TestTrue(FString::Printf(TEXT("Old arrival cannot overwrite the new life velocity (actual %s)"),*f.character->GetVelocity().ToString()),
			f.character->GetVelocity().Equals(freshVelocity));
		TestEqual(TEXT("Old arrival cannot overwrite the new life movement mode"),int32(f.character->GetCharacterMovement()->MovementMode),int32(MOVE_Flying));
		f.character->SetRole(ROLE_Authority); pc->UnPossess();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalExitGraceTest,"ShootingArena.Portal.Spatial.ExitGraceLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalExitGraceTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	auto* exit=f.world->SpawnActor<ASeamlessPortalActor>(); exit->SetActorLocation(FVector(1000,0,0)); exit->DispatchBeginPlay();
	auto* wall=f.Wall(FVector(1000,0,0),FVector(5,175,250)); exit->passageWalls.Add(wall);
	f.entry->SetDestination(exit);
	f.Move(FVector(15,0,0)); f.Tick();
	TestTrue(TEXT("Accepted spatial arrival temporarily retains its exit wall exclusion"),f.character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(wall));
	f.Move(f.character->GetActorLocation()+FVector(0,1000,0)); f.Tick();
	TestFalse(TEXT("Leaving the exit sideways releases grace even at the same plane depth"),f.character->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(wall));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalShutdownTest,"ShootingArena.Portal.Spatial.TraversalShutdown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalShutdownTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld first,second;
	PortalTraversalGuard::Lock(first.character,first.world->GetTimeSeconds()+100);
	PortalTraversalGuard::Lock(second.character,second.world->GetTimeSeconds()+100);
	first.Move(FVector(100,0,0));
	first.subsystem->Deinitialize();
	TestFalse(TEXT("Shutdown removes locks owned by its world"),PortalTraversalGuard::IsLocked(first.character,first.world->GetTimeSeconds()));
	TestTrue(TEXT("Shutdown leaves another world's independent locks intact"),PortalTraversalGuard::IsLocked(second.character,second.world->GetTimeSeconds()));
	first.Move(FVector(15,0,0)); first.Tick(); first.subsystem->Deinitialize();
	TestTrue(TEXT("Late callbacks and repeat shutdown cannot restart traversal"),first.character->GetActorLocation().X<100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpatialPortalRenderShutdownTest,"ShootingArena.Portal.Spatial.RenderShutdownCallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSpatialPortalRenderShutdownTest::RunTest(const FString&)
{
	PortalOverlapTests::FWorld f;
	auto* views=f.world->GetSubsystem<UPortalViewSubsystem>();
	auto& view=views->FindOrCreateView(f.entry);
	auto* capture=view.sceneCapture.Get();
	if (!TestNotNull(TEXT("Capture component for the shutdown callback"),capture)) return false;
	bool called=false;
	const auto handle=capture->TransformUpdated.AddLambda([&](USceneComponent*,EUpdateTransformFlags,ETeleportType)
	{
		called=true; views->Deinitialize();
		TestEqual(TEXT("In-flight view storage remains valid until its update unwinds"),views->portalViews.Num(),1);
		TestTrue(TEXT("In-flight capture is not destroyed under its transform callback"),capture->IsRegistered());
		views->Tick(1.f/60);
	});
	views->bUpdatingViews=true;
	capture->SetWorldLocation(FVector(20,0,0));
	capture->TransformUpdated.Remove(handle);
	TestTrue(TEXT("Shutdown was invoked inside a real capture transform callback"),called);
	views->FinishViewUpdate();
	TestEqual(TEXT("Unwinding releases all view instances"),views->portalViews.Num(),0);
	TestEqual(TEXT("Unwinding clears cached portal membership"),views->GetRegisteredPortalCount(),0);
	TestFalse(TEXT("Unwinding unregisters the capture component"),capture->IsRegistered());
	views->RegisterPortal(f.entry); views->Tick(1.f/60); views->Deinitialize();
	TestEqual(TEXT("Late registration and repeat shutdown cannot recreate rendering state"),views->GetRegisteredPortalCount(),0);
	return true;
}

#endif

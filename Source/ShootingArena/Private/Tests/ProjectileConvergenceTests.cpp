#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/WeaponProjectileAimLibrary.h"
#include "Weapon/ProjectileConvergenceComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileConvergenceTest, "ShootingArena.Weapon.ProjectileConvergence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectileConvergenceTest::RunTest(const FString& Parameters)
{
	FVector Join;
	const FVector Camera = FVector::ZeroVector, Forward = FVector::ForwardVector, Muzzle(0, 50, 0);
	TestTrue(TEXT("Barrel and camera rays intersect at 1m"),
		UWeaponProjectileAimLibrary::FindMuzzleRayJoin(Camera, Forward, Muzzle, FVector(100, -50, 0), 2000, 2, Join));
	TestTrue(TEXT("Join stays on the crosshair ray"), Join.Equals(FVector(100, 0, 0), 0.01));
	TestFalse(TEXT("Parallel barrel falls back"), UWeaponProjectileAimLibrary::FindMuzzleRayJoin(Camera, Forward, Muzzle, Forward, 2000, 2, Join));
	TestFalse(TEXT("Diverging barrel falls back"), UWeaponProjectileAimLibrary::FindMuzzleRayJoin(Camera, Forward, Muzzle, FVector(100, 50, 0), 2000, 2, Join));
	TestFalse(TEXT("Skew rays beyond tolerance fall back"), UWeaponProjectileAimLibrary::FindMuzzleRayJoin(Camera, Forward, FVector(0,50,10), FVector(100,-50,0), 2000, 2, Join));
	TestFalse(TEXT("Distant intersection falls back"), UWeaponProjectileAimLibrary::FindMuzzleRayJoin(Camera, Forward, Muzzle, FVector(100,-50,0), 50, 2, Join));

	UProjectileConvergenceSettings* Settings = NewObject<UProjectileConvergenceSettings>();
	FVector Target, FlightForward;
	bool bConverge;
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, nullptr, Camera, FVector(1000,0,0), Muzzle,
		FVector(1000,0,0), Target, FlightForward, bConverge);
	TestTrue(TEXT("Default join is 1m"), bConverge && Target.Equals(FVector(100,0,0)) && FlightForward.Equals(Forward));
	Settings->Mode = EProjectileConvergenceMode::MuzzleRayIntersection;
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, nullptr, Camera, FVector(1000,0,0), Muzzle,
		FVector(50,0,0), Target, FlightForward, bConverge);
	TestTrue(TEXT("Missing socket falls back and clamps to nearby camera hit"), bConverge && Target.Equals(FVector(50,0,0)));
	Settings->bEnabled = false;
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, nullptr, Camera, FVector(1000,0,0), Muzzle,
		FVector(1000,0,0), Target, FlightForward, bConverge);
	TestTrue(TEXT("Disabled restores direct muzzle aim"), !bConverge && Target.Equals(FVector(1000,0,0)));
	Settings->bEnabled = true;
	Settings->Mode = EProjectileConvergenceMode::FixedDistance;
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, nullptr, Camera, FVector(1000,0,0), FVector(120,50,0),
		FVector(1000,0,0), Target, FlightForward, bConverge);
	TestFalse(TEXT("Never bend backwards to a join behind the muzzle"), bConverge);

	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	// Exercise the real socket transform, including an animated barrel angle and per-weapon axis correction.
	AActor* SocketActor = World->SpawnActor<AActor>();
	UStaticMeshComponent* SocketMesh = NewObject<UStaticMeshComponent>(SocketActor);
	SocketActor->SetRootComponent(SocketMesh);
	SocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UStaticMesh* Mesh = NewObject<UStaticMesh>();
	UStaticMeshSocket* Socket = NewObject<UStaticMeshSocket>(Mesh);
	Socket->SocketName = TEXT("Muzzle");
	Socket->RelativeRotation = FRotator(0, -FMath::RadiansToDegrees(FMath::Atan(50.f / 200.f)), 0);
	Mesh->AddSocket(Socket);
	SocketMesh->SetStaticMesh(Mesh);
	SocketMesh->RegisterComponent();
	SocketActor->SetActorLocation(Muzzle);
	Settings->Mode = EProjectileConvergenceMode::MuzzleRayIntersection;
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, SocketMesh, Camera, FVector(1000,0,0), Muzzle,
		FVector(1000,0,0), Target, FlightForward, bConverge);
	TestTrue(TEXT("Real Muzzle socket angle selects the 2m intersection"), bConverge && Target.Equals(FVector(200,0,0),0.01));
	Socket->RelativeRotation = FRotator(0, -FMath::RadiansToDegrees(FMath::Atan(50.f / 400.f)), 0);
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, SocketMesh, Camera, FVector(1000,0,0), Muzzle,
		FVector(1000,0,0), Target, FlightForward, bConverge);
	TestTrue(TEXT("Changing the barrel angle changes the sampled join"), bConverge && Target.Equals(FVector(400,0,0),0.01));
	Settings->MuzzleDirectionOffset = FRotator(0,90,0);
	UWeaponProjectileAimLibrary::ResolveProjectileConvergence(Settings, SocketMesh, Camera, FVector(1000,0,0), Muzzle,
		FVector(1000,0,0), Target, FlightForward, bConverge);
	TestTrue(TEXT("Diverging corrected socket axis uses 1m fallback"), bConverge && Target.Equals(FVector(100,0,0),0.01));
	SocketActor->Destroy();
	auto SpawnProjectile = [World, Muzzle]()
	{
		AActor* Actor = World->SpawnActor<AActor>();
		USphereComponent* Sphere = NewObject<USphereComponent>(Actor);
		Actor->SetRootComponent(Sphere);
		Sphere->SetSphereRadius(1);
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetCollisionResponseToAllChannels(ECR_Block);
		Sphere->RegisterComponent();
		Actor->SetActorLocation(Muzzle);
		UProjectileMovementComponent* Movement = NewObject<UProjectileMovementComponent>(Actor);
		Movement->ProjectileGravityScale = 0;
		Movement->bRotationFollowsVelocity = true;
		Movement->Velocity = FVector(1000,0,0);
		Movement->RegisterComponent();
		Movement->SetUpdatedComponent(Sphere);
		Movement->Activate(true);
		return Actor;
	};
	const double FirstDistance = FVector::Distance(Muzzle, FVector(100,0,0));
	for (float Step : {0.3f, 0.01f, 0.005f})
	{
		AActor* Actor = SpawnProjectile();
		TestTrue(TEXT("Existing movement configured"), UWeaponProjectileAimLibrary::ConfigureProjectileConvergence(Actor, FVector(100,0,0), Forward, true));
		UProjectileConvergenceComponent* Driver = Actor->FindComponentByClass<UProjectileConvergenceComponent>();
		for (int32 i = 0; i < FMath::RoundToInt(0.3f / Step); ++i)
			Driver->TickComponent(Step, LEVELTICK_All, &Driver->PrimaryComponentTick);
		TestTrue(TEXT("Crossing a frame joins exactly and spends remaining time forward"), Actor->GetActorLocation().Equals(FVector(100 + 300 - FirstDistance,0,0), 0.05));
		TestTrue(TEXT("Post-join speed is preserved"), Actor->FindComponentByClass<UProjectileMovementComponent>()->Velocity.Equals(Forward * 1000, 0.01));
		Actor->Destroy();
	}
	for (float WallX : {50.f, 150.f})
	{
		AActor* Wall = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
		Wall->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(1,100,100));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Wall->SetActorLocation(FVector(WallX,0,0));
		AActor* Actor = SpawnProjectile();
		UWeaponProjectileAimLibrary::ConfigureProjectileConvergence(Actor, FVector(100,0,0), Forward, true);
		UProjectileConvergenceComponent* Driver = Actor->FindComponentByClass<UProjectileConvergenceComponent>();
		Driver->TickComponent(0.3f, LEVELTICK_All, &Driver->PrimaryComponentTick);
		TestTrue(TEXT("Sweeps hit walls before and after joining"), Actor->GetActorLocation().X < WallX);
		TestNull(TEXT("Existing stop-on-hit path runs"), Actor->FindComponentByClass<UProjectileMovementComponent>()->UpdatedComponent.Get());
		Actor->Destroy();
		Wall->Destroy();
	}
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif

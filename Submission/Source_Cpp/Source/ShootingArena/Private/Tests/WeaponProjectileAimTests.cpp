#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/WeaponProjectileAimLibrary.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponProjectileAimTest, "ShootingArena.Weapon.ProjectileMuzzleAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponProjectileAimTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Test world"), World)) return false;
	const ETraceTypeQuery Channel = UEngineTypes::ConvertToTraceType(ECC_Visibility);
	const FVector CameraStart(0, 0, 100);
	const FVector CameraEnd(1000, 0, 100);
	FVector Start, Target;

	UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(World, nullptr, nullptr, nullptr,
		CameraStart, CameraEnd, Channel, Start, Target);
	TestTrue(TEXT("No hit uses maximum range"), Target.Equals(CameraEnd));
	TestTrue(TEXT("Missing mesh uses camera origin"), Start.Equals(CameraStart));

	AActor* Weapon = World->SpawnActor<AActor>();
	UStaticMeshComponent* Muzzle = NewObject<UStaticMeshComponent>(Weapon);
	Weapon->SetRootComponent(Muzzle);
	UStaticMesh* Mesh = NewObject<UStaticMesh>();
	UStaticMeshSocket* Socket = NewObject<UStaticMeshSocket>(Mesh);
	Socket->SocketName = TEXT("Muzzle");
	Socket->RelativeLocation = FVector(40, 25, -10);
	Mesh->AddSocket(Socket);
	Muzzle->SetStaticMesh(Mesh);
	Muzzle->RegisterComponent();
	Weapon->SetActorLocation(CameraStart);
	UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(World, Weapon, nullptr, Muzzle,
		CameraStart, CameraEnd, Channel, Start, Target);
	TestTrue(TEXT("Origin uses off-axis Muzzle socket"), Start.Equals(FVector(40, 25, 90)));
	TestTrue(TEXT("Clear shot retains camera range endpoint"), Target.Equals(CameraEnd));
	APawn* LocalShooter = World->SpawnActor<APawn>();
	APlayerController* LocalController = World->SpawnActor<APlayerController>();
	LocalController->Possess(LocalShooter);
	const FVector ServerStart(60, 10, 30);
	TestTrue(TEXT("Authority retains its resolved start even for a local shooter"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(LocalShooter, Muzzle, ServerStart).Equals(ServerStart));
	LocalShooter->SetRole(ROLE_AutonomousProxy);
	TestTrue(TEXT("Owning client uses its selected animated muzzle instead of the third-person server start"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(LocalShooter, Muzzle, ServerStart).Equals(Start));
	Weapon->SetActorLocation(CameraStart + FVector(0, 0, 15));
	TestTrue(TEXT("Owning client follows the current muzzle transform"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(LocalShooter, Muzzle, ServerStart).Equals(Start + FVector(0, 0, 15)));
	Weapon->SetActorLocation(CameraStart);
	APawn* RemoteShooter = World->SpawnActor<APawn>();
	RemoteShooter->SetRole(ROLE_SimulatedProxy);
	TestTrue(TEXT("Remote observer uses its selected third-person muzzle"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(RemoteShooter, Muzzle, ServerStart).Equals(Start));
	TestTrue(TEXT("Missing view mesh retains the server start"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(LocalShooter, nullptr, ServerStart).Equals(ServerStart));
	TestTrue(TEXT("Missing shooter retains the server start"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(nullptr, Muzzle, ServerStart).Equals(ServerStart));

	auto SpawnBlocker = [World](FVector Location)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(FVector(10, 100, 100));
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	AActor* Shooter = SpawnBlocker(FVector(100, 0, 100));
	AActor* Wall = SpawnBlocker(FVector(500, 0, 100));
	UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(World, Weapon, Shooter, Muzzle,
		CameraStart, CameraEnd, Channel, Start, Target);
	TestTrue(TEXT("Ignores shooter and targets first wall surface"), Target.Equals(FVector(490, 0, 100), 0.1));
	TestTrue(TEXT("Wall hit does not move the muzzle origin"), Start.Equals(FVector(40, 25, 90)));
	TestTrue(TEXT("Launch vector converges on camera hit"),
		(Target - Start).GetSafeNormal().Equals(FVector(450, -25, 10).GetSafeNormal()));

	// The wall becomes the ignored weapon, ensuring the second exclusion works.
	UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(World, Wall, Shooter, nullptr,
		CameraStart, CameraEnd, Channel, Start, Target);
	TestTrue(TEXT("Ignores weapon as well as shooter"), Target.Equals(CameraEnd));
	Muzzle->SetStaticMesh(nullptr);
	TestTrue(TEXT("Missing view socket retains the server start rather than using the mesh pivot"),
		UWeaponProjectileAimLibrary::ResolveProjectileViewStart(LocalShooter, Muzzle, ServerStart).Equals(ServerStart));
	UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(World, Weapon, Shooter, Muzzle,
		CameraStart, CameraEnd, Channel, Start, Target);
	TestTrue(TEXT("Missing socket uses camera origin instead of mesh pivot"), Start.Equals(CameraStart));
	World->DestroyWorld(false);
	return true;
}

#endif

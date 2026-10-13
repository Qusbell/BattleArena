#if WITH_DEV_AUTOMATION_TESTS
#include "Camera/ShooterNicknameComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterNicknameSphereTraceTest, "ShootingArena.Nickname.SphereFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterNicknameSphereTraceTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated nickname query world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* PC = World->SpawnActor<APlayerController>();
	UShooterNicknameComponent* Nickname = NewObject<UShooterNicknameComponent>(PC);
	Nickname->RegisterComponent();
	Nickname->ShooterClass = APawn::StaticClass();
	Nickname->bUseEquippedWeaponTraceChannel = false;
	auto Body = [&](FVector Location)
	{
		APawn* Pawn = World->SpawnActor<APawn>();
		USphereComponent* Collision = NewObject<USphereComponent>(Pawn);
		Pawn->SetRootComponent(Collision);
		Collision->InitSphereRadius(20);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
		Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
		Collision->RegisterComponent();
		Pawn->SetActorLocation(Location);
		return TPair<APawn*, USphereComponent*>(Pawn, Collision);
	};
	auto Obstacle = [&](FVector Location, FVector Extent)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Collision = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Collision);
		Collision->SetBoxExtent(Extent);
		Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Collision->SetCollisionResponseToAllChannels(ECR_Block);
		Collision->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	const FVector Origin(0, 0, 80);
	auto Aim = [&]() { return Nickname->FindAimTarget(Origin, FVector::ForwardVector); };
	auto DirectHits = [&]()
	{
		TArray<FHitResult> Hits;
		World->LineTraceMultiByChannel(Hits, Origin, Origin + FVector(20000, 0, 0), ECC_GameTraceChannel2);
		return Hits.Num();
	};
	const auto Blue = Body(FVector(600, 0, 80));
	const auto Yellow = Body(FVector(300, 30, 80));
	TestEqual(TEXT("Exact blue hit wins over nearer yellow sphere-only candidate"), Aim(), Blue.Key);
	Blue.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestEqual(TEXT("Original damage line still misses the offset yellow body"), DirectHits(), 0);
	TestEqual(TEXT("Line miss falls back to the nearby yellow body"), Aim(), Yellow.Key);
	TestEqual(TEXT("Sphere search never enlarges the real damage collider"), Yellow.Value->GetUnscaledSphereRadius(), 20.0f);
	Nickname->SetNicknameSphereTraceEnabled(false);
	TestNull(TEXT("Fallback OFF restores original line-only targeting"), Aim());
	Blue.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Fallback OFF keeps direct target detection working"), Aim(), Blue.Key);
	Blue.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Nickname->SetNicknameSphereTraceEnabled(true);
	Nickname->SetNicknameSphereTraceRadius(0);
	TestNull(TEXT("Zero radius disables the additional query"), Aim());
	Nickname->SetNicknameSphereTraceRadius(5);
	TestNull(TEXT("A candidate outside the configured radius is rejected"), Aim());
	Nickname->SetNicknameSphereTraceRadius(15);
	Yellow.Value->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	TestEqual(TEXT("Overlap damage colliders remain eligible for sphere fallback"), Aim(), Yellow.Key);
	Yellow.Value->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	TestNull(TEXT("A collider ignoring the weapon channel is not a sphere candidate"), Aim());
	Yellow.Value->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	Yellow.Key->SetActorHiddenInGame(true);
	TestNull(TEXT("Hidden shooter is never selected"), Aim());
	Yellow.Key->SetActorHiddenInGame(false);
	Yellow.Key->SetCanBeDamaged(false);
	TestNull(TEXT("Undamageable shooter is never selected"), Aim());
	Yellow.Key->SetCanBeDamaged(true);
	AActor* Wall = Obstacle(FVector(200, 0, 80), FVector(10, 100, 100));
	TestNull(TEXT("Wall blocks the offset target even when the candidate sphere reaches it"), Aim());
	Wall->Destroy();
	Wall = Obstacle(FVector(150, -10, 80), FVector(5, 4, 100));
	TestEqual(TEXT("A wall grazed by the sphere does not hide a visible nearby target"), Aim(), Yellow.Key);
	Wall->Destroy();
	Yellow.Value->SetSphereRadius(5);
	Nickname->SetNicknameSphereTraceRadius(30);
	Wall = Obstacle(FVector(150, 13, 80), FVector(5, 2, 100));
	TestEqual(TEXT("Crosshair line itself is unobstructed in angled occlusion case"), DirectHits(), 0);
	TestNull(TEXT("Angled visibility ray rejects an offset body hidden by a wall"), Aim());
	Wall->Destroy();
	Yellow.Value->SetSphereRadius(20);
	Nickname->SetNicknameSphereTraceRadius(15);
	AActor* Gun = Obstacle(FVector(200, 8, 80), FVector(10, 8, 30));
	Gun->SetOwner(Yellow.Key);
	TestNull(TEXT("A blocking gun is never promoted to its shooter owner"), Aim());
	Gun->Destroy();
	Yellow.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Gun = Obstacle(FVector(300, 10, 80), FVector(10, 4, 30));
	Gun->SetOwner(Yellow.Key);
	TestNull(TEXT("Gun-only sphere contact cannot select a shooter"), Aim());
	Gun->Destroy();
	Yellow.Value->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Yellow.Value->SetSphereRadius(5);
	Yellow.Key->SetActorLocation(FVector(510, 15, 80));
	Nickname->MaxDistance = 500;
	TestNull(TEXT("Sphere end cap cannot extend the configured target range"), Aim());
	Nickname->MaxDistance = 20000;
	Yellow.Key->SetActorLocation(FVector(5, 12, 80));
	TestEqual(TEXT("Initial sphere overlap resolves a valid nearby body"), Aim(), Yellow.Key);
	Wall = Obstacle(FVector(2, 6, 80), FVector(10, 1, 100));
	TestNull(TEXT("Initial sphere overlap cannot bypass a nearby wall"), Aim());
	Wall->Destroy();
	Yellow.Key->SetActorLocation(FVector(-5, 12, 80));
	TestNull(TEXT("Initial overlap behind the camera is rejected"), Aim());
	Yellow.Key->SetActorLocation(FVector(300, 30, 80));
	Yellow.Value->SetSphereRadius(20);
	Nickname->SetNicknameSphereTraceRadius(std::numeric_limits<float>::quiet_NaN());
	TestNull(TEXT("Invalid radius safely disables the sphere query"), Aim());
	Nickname->SetNicknameSphereTraceRadius(std::numeric_limits<float>::infinity());
	TestNull(TEXT("Infinite radius cannot enter physics queries"), Aim());
	Nickname->SetNicknameSphereTraceRadius(-50);
	TestNull(TEXT("Negative radius cannot enter physics queries"), Aim());
	Nickname->SetNicknameSphereTraceRadius(15);
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	TestNull(TEXT("NaN ray origin is rejected before physics"), Nickname->FindAimTarget(FVector(NaN, 0, 80), FVector::ForwardVector));
	TestNull(TEXT("Zero ray direction is rejected before physics"), Nickname->FindAimTarget(Origin, FVector::ZeroVector));
	TestNull(TEXT("Overflowing direction normalization is rejected before physics"),
		Nickname->FindAimTarget(Origin, FVector(std::numeric_limits<double>::max(), 0, 0)));
	TestNull(TEXT("Infinite ray direction is rejected before physics"),
		Nickname->FindAimTarget(Origin, FVector(std::numeric_limits<float>::infinity(), 0, 0)));
	Nickname->MaxDistance = NaN;
	TestNull(TEXT("NaN range is rejected before physics"), Aim());
	Nickname->MaxDistance = std::numeric_limits<float>::infinity();
	TestNull(TEXT("Infinite range is rejected before physics"), Aim());
	Nickname->MaxDistance = 20000;
	Yellow.Value->UnregisterComponent();
	TestNull(TEXT("Unregistered damage collider cannot remain a candidate"), Aim());
	Yellow.Value->RegisterComponent();
	TestEqual(TEXT("Re-registering damage collider restores valid fallback"), Aim(), Yellow.Key);
	World->bIsTearingDown = true;
	TestNull(TEXT("World shutdown rejects new traces"), Aim());
	World->bIsTearingDown = false;
	Yellow.Key->Destroy();
	TestNull(TEXT("Destroyed candidate is safely ignored"), Aim());
	Nickname->DestroyComponent();
	Nickname->SetNicknameSphereTraceEnabled(true);
	Nickname->SetNicknameSphereTraceRadius(15);
	TestNull(TEXT("Destroyed nickname component rejects further queries"), Aim());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}
#endif

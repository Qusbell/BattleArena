#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/ProjectileWorldCollisionComponent.h"
#include "Weapon/WeaponProjectileAimLibrary.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Misc/AutomationTest.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FProjectileSplitCollisionTest, "ShootingArena.Weapon.ProjectileSplitCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FProjectileSplitCollisionTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated query world"), World)) return false;
	AActor* Projectile = World->SpawnActor<AActor>();
	USphereComponent* PlayerShape = NewObject<USphereComponent>(Projectile);
	Projectile->SetRootComponent(PlayerShape);
	PlayerShape->SetSphereRadius(40.f);
	PlayerShape->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlayerShape->SetCollisionObjectType(ECC_GameTraceChannel1);
	PlayerShape->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlayerShape->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PlayerShape->RegisterComponent();
	auto* Environment = NewObject<UProjectileWorldCollisionComponent>(Projectile);
	Environment->SetupAttachment(PlayerShape);
	Environment->SetCollisionObjectType(ECC_GameTraceChannel1);
	Environment->RegisterComponent();
	Environment->PlayerCollision = PlayerShape;
	const FVector Start(0, 0, 100), End(1000, 0, 100);
	Projectile->SetActorLocation(Start);
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(10, 100, 100));
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Block);
	Box->RegisterComponent();
	Wall->SetActorLocation(FVector(500, 0, 100));
	ACharacter* Target = World->SpawnActor<ACharacter>();
	Target->GetCapsuleComponent()->SetCapsuleSize(10.f, 30.f);
	Target->SetActorLocation(FVector(600, 0, 100));
	FHitResult Hit;
	TestTrue(TEXT("Finds environment before a player behind it"), Environment->FindImpact(Start, End, Hit));
	TestEqual(TEXT("Wall wins before a later player"), Hit.GetActor(), Wall);
	TestTrue(TEXT("Environment radius controls wall contact"), FMath::IsNearlyEqual(Hit.Location.X, 482.0, 0.2));
	Target->SetActorLocation(FVector(350, 0, 100));
	TestTrue(TEXT("Finds player before the wall"), Environment->FindImpact(Start, End, Hit));
	TestEqual(TEXT("Player wins before a later wall"), Hit.GetActor(), static_cast<AActor*>(Target));
	TestTrue(TEXT("Player contact uses the larger existing root radius"), Hit.Location.X < 310.0);
	Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
	TestTrue(TEXT("A target ignoring projectiles is passed through"), Environment->FindImpact(Start, End, Hit));
	TestEqual(TEXT("Projectile-ignore targets do not receive a direct impact"), Hit.GetActor(), Wall);
	Target->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	PlayerShape->IgnoreActorWhenMoving(Target, true);
	TestTrue(TEXT("Existing per-projectile actor exclusions are retained"), Environment->FindImpact(Start, End, Hit));
	TestEqual(TEXT("Ignored player cannot become a manual sweep impact"), Hit.GetActor(), Wall);
	PlayerShape->IgnoreActorWhenMoving(Target, false);
	Box->SetBoxExtent(FVector(1000, 2, 100));
	Wall->SetActorLocation(FVector(500, 30, 100));
	Target->SetActorLocation(FVector(500, 45, 100));
	TestFalse(TEXT("A large player detector cannot hit through an adjacent thin wall"), Environment->FindImpact(Start, End, Hit));
	Target->SetActorLocation(FVector(500, 500, 100));
	TestFalse(TEXT("Large player radius does not hit a nearby side wall"), Environment->FindImpact(Start, End, Hit));
	TestFalse(TEXT("Muzzle safety also uses the small environment radius"),
		UWeaponProjectileAimLibrary::IsProjectileStartBlocked(World, Environment, Start, Projectile, nullptr));
	PlayerShape->SetSphereRadius(80.f);
	TestEqual(TEXT("Changing player radius does not change environment radius"), Environment->GetScaledSphereRadius(), 8.f);
	TestFalse(TEXT("Enlarging player radius does not make the side wall block the flight"), Environment->FindImpact(Start, End, Hit));
	Environment->LastPosition = Start;
	Environment->bHasLastPosition = true;
	Projectile->SetActorLocation(FVector(100, 0, 100));
	Environment->RecordTravel(PlayerShape, EUpdateTransformFlags::None, ETeleportType::None);
	Projectile->SetActorLocation(FVector(100, 100, 100));
	Environment->RecordTravel(PlayerShape, EUpdateTransformFlags::None, ETeleportType::None);
	TestEqual(TEXT("A convergence turn preserves separate sweep segments"), Environment->Travel.Num(), 2);
	Environment->RecordTravel(PlayerShape, EUpdateTransformFlags::None, ETeleportType::TeleportPhysics);
	TestTrue(TEXT("Teleport does not leave an old flight path to sweep"), Environment->Travel.IsEmpty());
	// A protruding door trim exposes the difference between authoritative TP and
	// cosmetic FP muzzle paths. Small world shapes must retain this difference;
	// the authoritative impact notification is responsible for retiring copies.
	Box->SetBoxExtent(FVector(10, 10, 100));
	Wall->SetActorLocation(FVector(500, 0, 100));
	const FVector VisualOffset(0, 25, 0);
	TestTrue(TEXT("Server path hits the protruding trim"), Environment->FindImpact(Start, End, Hit));
	TestFalse(TEXT("FP visual path can clear the same trim"), Environment->FindImpact(Start + VisualOffset, End + VisualOffset, Hit));
	Environment->InitSphereRadius(64.f);
	TestTrue(TEXT("A legacy large wall shape hid this path difference"), Environment->FindImpact(Start + VisualOffset, End + VisualOffset, Hit));
	Environment->InitSphereRadius(8.f);

	// Lifecycle checks operate only on this isolated preview component; no game
	// world, actor BeginPlay, PIE, or world simulation is started.
	Environment->RegisterAllComponentTickFunctions(true);
	Environment->BeginPlay();
	TestTrue(TEXT("Tracking binds to the existing live root"), Environment->HasLiveContext());
	Projectile->SetActorLocation(FVector(200, 100, 100));
	TestEqual(TEXT("The real transform delegate records movement"), Environment->Travel.Num(), 1);
	Environment->Activate(false);
	TestEqual(TEXT("Activating an already active component retains the current flight segment"), Environment->Travel.Num(), 1);
	Environment->SetComponentTickEnabled(false);
	for (int32 i = 0; i < 100; ++i) Projectile->SetActorLocation(FVector(300 + i, 100, 100));
	TestTrue(TEXT("Disabled ticks cannot accumulate travel indefinitely"), Environment->Travel.IsEmpty());
	const FVector ResumePosition = PlayerShape->GetComponentLocation();
	Environment->SetComponentTickEnabled(true);
	Projectile->SetActorLocation(ResumePosition + FVector(10, 0, 0));
	TestEqual(TEXT("Re-enabled tracking records only the new flight"), Environment->Travel.Num(), 1);
	if (!Environment->Travel.IsEmpty()) TestTrue(TEXT("Resuming does not sweep an old disabled path"), Environment->Travel[0].Start.Equals(ResumePosition));
	Environment->UnregisterComponent();
	TestFalse(TEXT("Unregister releases the old root binding"), Environment->PlayerCollision.IsValid());
	Environment->RegisterComponent();
	TestTrue(TEXT("Re-register restores tracking"), Environment->HasLiveContext());
	Projectile->SetActorLocation(ResumePosition + FVector(20, 0, 0));
	TestEqual(TEXT("Re-register binds exactly one movement callback"), Environment->Travel.Num(), 1);
	Environment->Deactivate();
	TestTrue(TEXT("Deactivation clears pending travel"), Environment->Travel.IsEmpty());
	Environment->Activate();
	TestTrue(TEXT("Reactivation restores a valid root binding"), Environment->HasLiveContext());

	const float NaN = std::numeric_limits<float>::quiet_NaN();
	Environment->InitSphereRadius(NaN);
	TestFalse(TEXT("Invalid environment radius is rejected before a physics query"), Environment->FindImpact(Start, End, Hit));
	Environment->InitSphereRadius(8.f);
	FHitResult InvalidHit;
	InvalidHit.ImpactNormal.X = NaN;
	Environment->ResolveImpact(InvalidHit);
	TestFalse(TEXT("A non-finite impact normal cannot reach BP effects or damage"), Environment->bResolved);
	InvalidHit = FHitResult(); InvalidHit.Time = NaN;
	Environment->ResolveImpact(InvalidHit);
	TestFalse(TEXT("An invalid hit time cannot resolve a projectile"), Environment->bResolved);

	auto* Movement = NewObject<UProjectileMovementComponent>(Projectile);
	Movement->RegisterComponent(); Movement->SetUpdatedComponent(PlayerShape);
	// Simulate a listener removing the collider during the position correction.
	const FDelegateHandle RemoveDuringMove = PlayerShape->TransformUpdated.AddLambda(
		[Environment](USceneComponent*, EUpdateTransformFlags, ETeleportType) { Environment->UnregisterComponent(); });
	FHitResult Impact;
	Impact.Location = PlayerShape->GetComponentLocation() + FVector(10, 0, 0);
	Impact.ImpactPoint = Impact.Location;
	Environment->ResolveImpact(Impact);
	PlayerShape->TransformUpdated.Remove(RemoveDuringMove);
	TestTrue(TEXT("Impact is marked resolved before reentrant callbacks"), Environment->bResolved);
	TestFalse(TEXT("Callback unregistration is respected"), Environment->IsRegistered());
	TestEqual(TEXT("No later movement event is invoked after losing the live context"), Movement->UpdatedComponent.Get(), static_cast<USceneComponent*>(PlayerShape));
	TestFalse(TEXT("The preview actor never entered gameplay"), Projectile->HasActorBegunPlay());
	World->DestroyWorld(false);
	return true;
}

#endif

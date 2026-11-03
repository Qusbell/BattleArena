#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Debug/DebugSpectatorProtectionComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDebugSpectatorProtectionTest,
	"ShootingArena.Debug.SpectatorProtection.Restore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDebugSpectatorProtectionTest::RunTest(const FString&)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("World"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* Controller = World->SpawnActor<APlayerController>();
	auto* Pawn = World->SpawnActor<ACharacter>();
	if (!Controller || !Pawn) return false;
	Controller->Possess(Pawn);
	auto* Caller = NewObject<USceneComponent>(Controller);
	Caller->RegisterComponent();
	const FTransform Original(FRotator(0, 45, 0), FVector(120, 340, 560));
	Pawn->SetActorTransform(Original);
	Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Pawn->GetCharacterMovement()->Velocity = FVector(10, 20, 30);
	TestTrue(TEXT("Begin succeeds"), UDebugSpectatorProtectionComponent::BeginDebugSpectating(Caller));
	TestFalse(TEXT("Damage disabled"), Pawn->CanBeDamaged());
	TestFalse(TEXT("Collision disabled"), Pawn->GetActorEnableCollision());
	TestEqual(TEXT("Movement disabled"), Pawn->GetCharacterMovement()->MovementMode.GetValue(), MOVE_None);
	TestTrue(TEXT("Parked away from arena"), Pawn->GetActorLocation().Equals(FVector(0, 0, 500000)));
	TestTrue(TEXT("Switching AI succeeds"), UDebugSpectatorProtectionComponent::BeginDebugSpectating(Caller));
	UDebugSpectatorProtectionComponent::EndDebugSpectating(Caller);
	TestTrue(TEXT("Original transform survives repeated begin"), Pawn->GetActorTransform().Equals(Original));
	TestTrue(TEXT("Damage restored"), Pawn->CanBeDamaged());
	TestTrue(TEXT("Collision restored"), Pawn->GetActorEnableCollision());
	TestEqual(TEXT("Movement restored"), Pawn->GetCharacterMovement()->MovementMode.GetValue(), MOVE_Flying);
	TestTrue(TEXT("Velocity restored"), Pawn->GetVelocity().Equals(FVector(10, 20, 30)));
	UDebugSpectatorProtectionComponent::EndDebugSpectating(Caller);
	TestTrue(TEXT("Repeated end leaves transform intact"), Pawn->GetActorTransform().Equals(Original));
	Pawn->SetCanBeDamaged(false);
	Pawn->SetActorEnableCollision(false);
	UDebugSpectatorProtectionComponent::BeginDebugSpectating(Caller);
	UDebugSpectatorProtectionComponent::EndDebugSpectating(Caller);
	TestFalse(TEXT("Existing invulnerability preserved"), Pawn->CanBeDamaged());
	TestFalse(TEXT("Existing disabled collision preserved"), Pawn->GetActorEnableCollision());
	return !HasAnyErrors();
}

#endif

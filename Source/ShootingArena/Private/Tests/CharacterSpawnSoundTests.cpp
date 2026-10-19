#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/CharacterSpawnSoundComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/ChildConnection.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Loading/LoadingScreenSubsystem.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "Tests/AutomationCommon.h"
#include "UObject/GarbageCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterSpawnSoundInitialDeliveryTest,
	"ShootingArena.Audio.Spawn.InitialConnectionReadiness",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterSpawnSoundInitialDeliveryTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::EditorPreview)) return false;
	AActor* ViewTarget = Context.GetTestWorld()->SpawnActor<AActor>();
	UChildConnection* Connection = NewObject<UChildConnection>();
	TestFalse(TEXT("No connection cannot receive the initial multicast"),
		UCharacterSpawnSoundComponent::HasReadyInitialConnection(nullptr));
	Connection->SetConnectionState(USOCK_Open);
	TestFalse(TEXT("An open PostLogin connection without a ViewTarget must wait"),
		UCharacterSpawnSoundComponent::HasReadyInitialConnection(Connection));
	Connection->ViewTarget = ViewTarget;
	TestTrue(TEXT("The initial request can be sent after replication prepares the ViewTarget"),
		UCharacterSpawnSoundComponent::HasReadyInitialConnection(Connection));
	Connection->SetConnectionState(USOCK_Closed);
	TestFalse(TEXT("A disconnected client cannot receive a pending request"),
		UCharacterSpawnSoundComponent::HasReadyInitialConnection(Connection));
	Connection->ViewTarget = nullptr;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterSpawnSoundInitialQueueTest,
	"ShootingArena.Audio.Spawn.InitialLoadingCameraAndOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterSpawnSoundInitialQueueTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	UGameInstance* GI = World->GetGameInstance();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
	APlayerController* PC = World->SpawnActor<APlayerController>();
	APawn* Pawn = World->SpawnActor<APawn>();
	PC->SetPlayer(Player);
	PC->PlayerCameraManager = World->SpawnActor<APlayerCameraManager>();
	if (!TestNotNull(TEXT("Local player camera"), PC->PlayerCameraManager.Get())) return false;
	PC->PlayerCameraManager->InitializeFor(PC);
	PC->Possess(Pawn);
	PC->SetViewTarget(Pawn);
	World->SetBegunPlay(true);
	Pawn->DispatchBeginPlay();
	World->TimeSeconds = 1.0;
	PC->PlayerCameraManager->FillCameraCache(FMinimalViewInfo());

	UCharacterSpawnSoundComponent* Sounds = NewObject<UCharacterSpawnSoundComponent>(Pawn);
	Pawn->AddInstanceComponent(Sounds);
	Sounds->RegisterComponent();
	USoundWave* Sound = NewObject<USoundWave>(Sounds);
	Sound->Duration = 1.0f;
	FCharacterSpawnSoundEntry Shared;
	Shared.Sound = Sound;
	Shared.DelaySeconds = 1.0f;
	FCharacterSpawnSoundEntry OwnerOnly = Shared;
	OwnerOnly.bAudibleToOthers = false;
	OwnerOnly.DelaySeconds = 0.0f;
	OwnerOnly.Space = ECharacterSpawnSoundSpace::TwoDimensional;
	Sounds->InitialSpawnSounds = { Shared, OwnerOnly };
	ULoadingScreenSubsystem* Loading = GI->GetSubsystem<ULoadingScreenSubsystem>();
	if (!TestNotNull(TEXT("Loading subsystem"), Loading)) return false;
	Loading->BeginExternalLoading(TEXT("Spawn sound regression test"));
	Sounds->QueueInitialSpawnSounds(FVector::ZeroVector, true);
	Sounds->QueueInitialSpawnSounds(FVector::ZeroVector, false);
	TestEqual(TEXT("Shared and owner-only requests are retained together during loading"),
		Sounds->PendingInitialAudience, uint8(3));
	TestEqual(TEXT("No initial sounds are consumed beneath the loading screen"),
		Sounds->PlayedInitialAudience, uint8(0));
	TestTrue(TEXT("A pending initial request has a retry timer"),
		World->GetTimerManager().IsTimerActive(Sounds->InitialLocalRetryTimer));

	Loading->EndExternalLoading();
	Sounds->TryPlayPendingInitialSpawnSounds(); // Observe the possessed pawn; wait for a subsequent camera update.
	World->TimeSeconds = 2.0;
	Sounds->TryPlayPendingInitialSpawnSounds();
	TestEqual(TEXT("Removing the overlay cannot consume an event before the camera updates"),
		Sounds->PlayedInitialAudience, uint8(0));
	PC->PlayerCameraManager->FillCameraCache(FMinimalViewInfo());
	Sounds->TryPlayPendingInitialSpawnSounds();
	TestEqual(TEXT("Both audiences are consumed after loading, possession and camera readiness"),
		Sounds->PlayedInitialAudience, uint8(3));
	TestEqual(TEXT("The consumed initial event is no longer pending"), Sounds->PendingInitialAudience, uint8(0));
	TestFalse(TEXT("The readiness poll stops after playback"),
		World->GetTimerManager().IsTimerActive(Sounds->InitialLocalRetryTimer));
	if (!TestEqual(TEXT("Only the shared entry schedules its configured delay"), Sounds->DelayedSoundTimers.Num(), 1)) return false;
	const FTimerHandle DelayedHandle = Sounds->DelayedSoundTimers[0];
	Sounds->QueueInitialSpawnSounds(FVector::ZeroVector, true);
	Sounds->QueueInitialSpawnSounds(FVector::ZeroVector, false);
	TestEqual(TEXT("Repeated deliveries cannot schedule either audience a second time"),
		Sounds->DelayedSoundTimers.Num(), 1);
	TestEqual(TEXT("Repeated deliveries cannot reopen the pending event"), Sounds->PendingInitialAudience, uint8(0));

	Sounds->DestroyComponent();
	TestFalse(TEXT("Destroying the pawn component cancels delayed sound playback"),
		World->GetTimerManager().IsTimerActive(DelayedHandle));
	GI->RemoveLocalPlayer(Player);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterSpawnSoundDelayedLifetimeTest,
	"ShootingArena.Audio.Spawn.DelayedAssetsAndPendingTeardown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterSpawnSoundDelayedLifetimeTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	APawn* Pawn = World->SpawnActor<APawn>();
	World->SetBegunPlay(true);
	Pawn->DispatchBeginPlay();
	UCharacterSpawnSoundComponent* Sounds = NewObject<UCharacterSpawnSoundComponent>(Pawn);
	Pawn->AddInstanceComponent(Sounds);
	Sounds->RegisterComponent();

	USoundWave* Sound = NewObject<USoundWave>();
	Sound->Duration = 1.0f;
	USoundAttenuation* Attenuation = NewObject<USoundAttenuation>();
	TWeakObjectPtr<USoundWave> WeakSound(Sound);
	TWeakObjectPtr<USoundAttenuation> WeakAttenuation(Attenuation);
	FCharacterSpawnSoundEntry Entry;
	Entry.Sound = Sound;
	Entry.AttenuationSettings = Attenuation;
	Entry.DelaySeconds = 10.0f;
	Sounds->RespawnSounds.Add(Entry);
	Sounds->PlayLocalSpawnSounds(ECharacterSpawnSoundPhase::Respawn, FVector::ZeroVector, true);
	if (!TestEqual(TEXT("One delayed playback is scheduled"), Sounds->DelayedSoundTimers.Num(), 1)) return false;
	const FTimerHandle DelayedHandle = Sounds->DelayedSoundTimers[0];

	// BlueprintReadWrite arrays can be replaced while an earlier sound is waiting.
	Sounds->RespawnSounds.Reset();
	Entry.Sound = nullptr;
	Entry.AttenuationSettings = nullptr;
	CollectGarbage(RF_NoFlags, true);
	TestTrue(TEXT("A pending playback keeps its original sound alive after the array changes"), WeakSound.IsValid());
	TestTrue(TEXT("A pending playback keeps its original attenuation alive after the array changes"), WeakAttenuation.IsValid());

	// No local controller/camera exists in this world, so the initial event stays queued.
	Sounds->QueueInitialSpawnSounds(FVector::ZeroVector, true);
	const FTimerHandle ReadinessHandle = Sounds->InitialLocalRetryTimer;
	TestTrue(TEXT("The initial request waits safely without a local controller"),
		World->GetTimerManager().IsTimerActive(ReadinessHandle));
	Sounds->DestroyComponent();
	TestFalse(TEXT("EndPlay removes the readiness timer before its next callback"),
		World->GetTimerManager().IsTimerActive(ReadinessHandle));
	TestFalse(TEXT("EndPlay removes the delayed playback timer before its next callback"),
		World->GetTimerManager().IsTimerActive(DelayedHandle));
	CollectGarbage(RF_NoFlags, true);
	TestFalse(TEXT("Cancelling playback releases the retained sound"), WeakSound.IsValid());
	TestFalse(TEXT("Cancelling playback releases the retained attenuation"), WeakAttenuation.IsValid());
	return true;
}

#endif

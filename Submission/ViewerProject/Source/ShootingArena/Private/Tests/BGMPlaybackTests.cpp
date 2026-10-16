#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/BGMPlayback.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeConcatenator.h"
#include "Sound/SoundNodeLooping.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundWaveProcedural.h"

#if WITH_EDITOR
#include "ActiveSound.h"
#include "Audio.h"
#include "AudioDevice.h"
#include "AudioThread.h"
#include "Audio/BGMSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/WorldSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/UObjectIterator.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBGMNativeWaveLoopTest, "ShootingArena.Audio.BGM.NativeWaveLoop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBGMNativeWaveLoopTest::RunTest(const FString& Parameters)
{
	USoundWave* Wave = NewObject<USoundWave>();
	Wave->Duration = 2.0f;
	Wave->bLooping = false;
	Wave->SoundClassObject = NewObject<USoundClass>();
	USoundCue* Playback = Cast<USoundCue>(BGMPlayback::CreateLoopingSound(GetTransientPackage(), Wave));
	if (!TestNotNull(TEXT("A transient looping cue is created"), Playback)) return false;
	USoundNodeWavePlayer* Player = Cast<USoundNodeWavePlayer>(Playback->FirstNode);
	if (!TestNotNull(TEXT("Native wave player"), Player)) return false;
	TestTrue(TEXT("Audio-thread native looping, independent of paused game-thread callbacks"), !!Player->bLooping);
	TestTrue(TEXT("Wave data is shared, not duplicated"), Player->GetSoundWave() == Wave);
	TestFalse(TEXT("Original wave remains a one-shot"), !!Wave->bLooping);
	TestTrue(TEXT("BGM volume SoundClass preserved"), Playback->SoundClassObject == Wave->SoundClassObject);
	TestEqual(TEXT("Wrapper adds no gain adjustment"), Playback->VolumeMultiplier, 1.0f);
	TestEqual(TEXT("Runtime looping duration"), Playback->Duration, INDEFINITELY_LOOPING_DURATION);
	TestTrue(TEXT("Runtime only"), Playback->HasAnyFlags(RF_Transient));
	USoundWaveProcedural* Procedural = NewObject<USoundWaveProcedural>();
	TestTrue(TEXT("Procedural sources such as MetaSounds keep their own loop graph"),
		BGMPlayback::CreateLoopingSound(GetTransientPackage(), Procedural) == Procedural);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBGMNativeCueLoopTest, "ShootingArena.Audio.BGM.NativeCueLoop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBGMNativeCueLoopTest::RunTest(const FString& Parameters)
{
	USoundWave* Wave = NewObject<USoundWave>();
	Wave->Duration = 2.0f;
	USoundCue* Source = NewObject<USoundCue>();
	Source->VolumeMultiplier = 0.4f;
	Source->PitchMultiplier = 0.8f;
	Source->SoundClassObject = NewObject<USoundClass>();
	USoundNodeWavePlayer* OriginalPlayer = NewObject<USoundNodeWavePlayer>(Source);
	OriginalPlayer->SetSoundWave(Wave);
	OriginalPlayer->bLooping = false;
	Source->FirstNode = OriginalPlayer;
	USoundCue* Playback = Cast<USoundCue>(BGMPlayback::CreateLoopingSound(GetTransientPackage(), Source));
	if (!TestNotNull(TEXT("Cue duplicate"), Playback)) return false;
	USoundNodeWavePlayer* Player = Cast<USoundNodeWavePlayer>(Playback->FirstNode);
	if (!TestNotNull(TEXT("Duplicated wave player"), Player)) return false;
	TestTrue(TEXT("Cue and player are isolated from the shared source asset"), Playback != Source && Player != OriginalPlayer);
	TestTrue(TEXT("Wave reference survives duplication"), Player->GetSoundWave() == Wave);
	TestTrue(TEXT("Native looping enabled on runtime copy"), !!Player->bLooping);
	TestFalse(TEXT("Original cue is unchanged"), !!OriginalPlayer->bLooping);
	TestTrue(TEXT("Original graph root unchanged"), Source->FirstNode == OriginalPlayer);
	TestEqual(TEXT("Cue gain preserved"), Playback->VolumeMultiplier, Source->VolumeMultiplier);
	TestEqual(TEXT("Cue pitch preserved"), Playback->PitchMultiplier, Source->PitchMultiplier);
	TestTrue(TEXT("SoundClass preserved"), Playback->SoundClassObject == Source->SoundClassObject);
	TestEqual(TEXT("Duration set for packaged builds too"), Playback->Duration, INDEFINITELY_LOOPING_DURATION);
	USoundCue* ProceduralCue = NewObject<USoundCue>();
	USoundNodeWavePlayer* ProceduralPlayer = NewObject<USoundNodeWavePlayer>(ProceduralCue);
	ProceduralPlayer->SetSoundWave(NewObject<USoundWaveProcedural>());
	ProceduralCue->FirstNode = ProceduralPlayer;
	TestTrue(TEXT("A cue containing procedural generators also keeps its own loop graph"),
		BGMPlayback::CreateLoopingSound(GetTransientPackage(), ProceduralCue) == ProceduralCue);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBGMWholeGraphLoopTest, "ShootingArena.Audio.BGM.WholeGraphLoop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBGMWholeGraphLoopTest::RunTest(const FString& Parameters)
{
	USoundCue* Source = NewObject<USoundCue>();
	USoundNodeConcatenator* Sequence = NewObject<USoundNodeConcatenator>(Source);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		USoundWave* Wave = NewObject<USoundWave>();
		Wave->Duration = 2.0f;
		USoundNodeWavePlayer* Player = NewObject<USoundNodeWavePlayer>(Source);
		Player->SetSoundWave(Wave);
		Player->bLooping = false;
		Sequence->ChildNodes.Add(Player);
		Sequence->InputVolume.Add(1.0f);
	}
	Source->FirstNode = Sequence;
	USoundCue* Playback = Cast<USoundCue>(BGMPlayback::CreateLoopingSound(GetTransientPackage(), Source));
	if (!TestNotNull(TEXT("Runtime cue"), Playback)) return false;
	USoundNodeLooping* Loop = Cast<USoundNodeLooping>(Playback->FirstNode);
	if (!TestNotNull(TEXT("Whole sequence is looped"), Loop)) return false;
	TestTrue(TEXT("Indefinite audio-thread loop"), !!Loop->bLoopIndefinitely);
	if (!TestEqual(TEXT("One graph under the loop"), Loop->ChildNodes.Num(), 1)) return false;
	USoundNode* RuntimeSequence = Loop->ChildNodes[0];
	TestTrue(TEXT("Sequence graph duplicated"), RuntimeSequence != Sequence && RuntimeSequence->IsA<USoundNodeConcatenator>());
	for (USoundNode* Child : RuntimeSequence->ChildNodes)
	{
		USoundNodeWavePlayer* Player = Cast<USoundNodeWavePlayer>(Child);
		if (TestNotNull(TEXT("Sequence retains wave players"), Player))
		{
			TestFalse(TEXT("Individual leaves must finish to advance the sequence"), !!Player->bLooping);
		}
	}
	TestTrue(TEXT("Source graph unchanged"), Source->FirstNode == Sequence);
	return true;
}

#if WITH_EDITOR

// Starts a standalone PIE smoke test and restores the user's play settings afterward.
// Check the actual audio thread after crossing the song's end, not IsPlaying(),
// which can remain stale while completion callbacks are queued during Pause.
struct FBGMPauseTestContext
{
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<UBGMSubsystem> BGM;
	TWeakObjectPtr<APlayerState> PreviousPauser;
	uint64 ComponentID = 0;
	double StartedAt = 0.0;
	double PausedGameTime = 0.0;
	bool bRequestedSnapshot = false;
	TAtomic<bool> bSnapshotReady{false};
	TAtomic<bool> bNativeLoopActive{false};
	EPlayNetMode PreviousNetMode = PIE_Standalone;
	int32 PreviousClientCount = 1;
};

using FBGMPauseTestContextPtr = TSharedPtr<FBGMPauseTestContext, ESPMode::ThreadSafe>;

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCheckBGMDuringPause,
	FBGMPauseTestContextPtr, Context, FAutomationTestBase*, Test);

bool FCheckBGMDuringPause::Update()
{
	UWorld* World = Context->World.Get();
	if (!World)
	{
		Test->AddError(TEXT("PIE ended before the paused BGM test completed."));
		return true;
	}
	if (FPlatformTime::Seconds() - Context->StartedAt < 5.0) return false;
	if (!Context->bRequestedSnapshot)
	{
		Context->bRequestedSnapshot = true;
		FAudioDevice* Device = World->GetAudioDeviceRaw();
		FAudioThread::RunCommandOnAudioThread([Context = Context, Device]()
		{
			if (FActiveSound* Sound = Device->FindActiveSound(Context->ComponentID))
			{
				for (const auto& Pair : Sound->GetWaveInstances())
				{
					if (Pair.Value->LoopingMode == LOOP_Forever && !Pair.Value->bIsFinished)
					{
						Context->bNativeLoopActive.Store(true);
					}
				}
			}
			Context->bSnapshotReady.Store(true);
		});
	}
	if (!Context->bSnapshotReady.Load()) return false;
	Test->TestTrue(TEXT("World stayed paused across the song's end"), World->IsPaused());
	Test->TestEqual(TEXT("Game time did not advance"), World->GetTimeSeconds(), Context->PausedGameTime);
	Test->TestTrue(TEXT("Audio thread still has a live native-looping wave after the end"), Context->bNativeLoopActive.Load());
	World->GetWorldSettings()->SetPauserPlayerState(Context->PreviousPauser.Get());
	if (UBGMSubsystem* BGM = Context->BGM.Get()) BGM->StopBGM(0.0f);
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FBeginBGMPauseTest,
	FBGMPauseTestContextPtr, Context, FAutomationTestBase*, Test);

bool FBeginBGMPauseTest::Update()
{
	UWorld* World = nullptr;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		UWorld* Candidate = WorldContext.World();
		if (Candidate && Candidate->WorldType == EWorldType::PIE && Candidate->GetNetMode() != NM_DedicatedServer)
		{
			World = Candidate;
			break;
		}
	}
	if (!Test->TestNotNull(TEXT("Standalone PIE world started"), World)) return true;
	APlayerController* PC = World->GetFirstPlayerController();
	UBGMSubsystem* BGM = World->GetGameInstance()->GetSubsystem<UBGMSubsystem>();
	USoundCue* Source = LoadObject<USoundCue>(nullptr, TEXT("/Game/QuakeLike_Base/SDM/SoundPack_SDM/Cue/PortalMap_Cue.PortalMap_Cue"));
	if (!PC || !PC->PlayerState || !BGM || !Source || !World->GetAudioDeviceRaw())
	{
		Test->AddError(TEXT("The PIE player, audio device or PortalMap_Cue is unavailable."));
		return true;
	}
	BGM->PlayPreviewBGM(Source, true, 0.0f, 0.0f);
	UAudioComponent* Component = nullptr;
	for (TObjectIterator<UAudioComponent> It; It; ++It)
	{
		if (It->GetSound() && It->GetSound()->GetOuter() == BGM && It->IsPlaying()) Component = *It;
	}
	if (!Test->TestNotNull(TEXT("BGM subsystem created a runtime component"), Component)) return true;
	Context->World = World;
	Context->BGM = BGM;
	Context->PreviousPauser = World->GetWorldSettings()->GetPauserPlayerState();
	Context->ComponentID = Component->GetAudioComponentID();
	World->GetWorldSettings()->SetPauserPlayerState(PC->PlayerState);
	Context->PausedGameTime = World->GetTimeSeconds();
	Context->StartedAt = FPlatformTime::Seconds();
	// Cross the first end after one second without waiting for the full 82-second song.
	Component->Play(FMath::Max(0.0f, Source->GetDuration() - 1.0f));
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FRestoreBGMPauseTestSettings, FBGMPauseTestContextPtr, Context);

bool FRestoreBGMPauseTestSettings::Update()
{
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->SetPlayNetMode(Context->PreviousNetMode);
	Settings->SetPlayNumberOfClients(Context->PreviousClientCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBGMPausedPIETest, "ShootingArena.PIE.Audio.PausedBGM",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBGMPausedPIETest::RunTest(const FString& Parameters)
{
	auto Context = MakeShared<FBGMPauseTestContext, ESPMode::ThreadSafe>();
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(Context->PreviousNetMode);
	Settings->GetPlayNumberOfClients(Context->PreviousClientCount);
	Settings->SetPlayNetMode(PIE_Standalone);
	Settings->SetPlayNumberOfClients(1);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(5.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FBeginBGMPauseTest(Context, this));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckBGMDuringPause(Context, this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreBGMPauseTestSettings(Context));
	return true;
}

#endif // WITH_EDITOR

#endif // WITH_DEV_AUTOMATION_TESTS

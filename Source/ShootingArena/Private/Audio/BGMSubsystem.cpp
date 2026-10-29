#include "Audio/BGMSubsystem.h"
#include "Audio/BGMPlayback.h"

#include "Components/AudioComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ShootingArenaGameInstance.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

void UBGMSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (IsRunningDedicatedServer())
	{
		return;
	}

	if (const UShootingArenaGameInstance* GameInstance = Cast<UShootingArenaGameInstance>(GetGameInstance()))
	{
		Settings = GameInstance->BGMSettings.LoadSynchronous();
	}

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this, &UBGMSubsystem::HandlePreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UBGMSubsystem::HandlePostLoadMap);
	UE_LOG(LogTemp, Log, TEXT("[BGM] Subsystem initialized. Settings=%s"), *GetNameSafe(Settings));
}

void UBGMSubsystem::Deinitialize()
{
	bDeinitializing = true;
	StopBGM(0.0f);
	++PlayRequestId;
	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
	}
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	}
	Super::Deinitialize();
}

void UBGMSubsystem::SetSettings(UBGMSettingsDataAsset* NewSettings)
{
	StopBGM(0.0f);
	Settings = NewSettings;
	ApplyVolumeOverride(0.0f);
	HandlePostLoadMap(GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr);
}

void UBGMSubsystem::PlayMainMenuBGM()
{
	if (Settings) PlayConfiguredTrack(Settings->MainMenuBGM, EBGMState::MainMenu, TEXT("MainMenu"));
}

void UBGMSubsystem::PlayLobbyBGM()
{
	PlayMainMenuBGM();
}

bool UBGMSubsystem::PlayGameplayBGM(FName MapID)
{
	if (!Settings) return false;
	MapID = MapID.IsNone() ? GetCurrentMapName() : NormalizeMapName(MapID.ToString());
	const FBGMTrack* Track = Settings->GameplayBGMByMap.Find(MapID);
	if (!Track)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BGM] No Gameplay BGM is mapped for MapID '%s'."), *MapID.ToString());
		return false;
	}
	PlayConfiguredTrack(*Track, EBGMState::Gameplay, *FString::Printf(TEXT("Gameplay:%s"), *MapID.ToString()));
	return true;
}

void UBGMSubsystem::PlayResultBGM()
{
	// Intentionally empty: old Blueprint nodes must not restart or replace Gameplay BGM.
}

void UBGMSubsystem::BeginLoadingBGM(const FString& DestinationMap)
{
	if (bDeinitializing || IsRunningDedicatedServer() || !GetGameInstance()) return;
	if (const UWorld* World = GetGameInstance()->GetWorld(); World && World->GetNetMode() == NM_DedicatedServer) return;
	const FName Destination = NormalizeMapName(DestinationMap);
	if (!bLoadingTransition)
	{
		// The destination may use the SAME track. Do not stop it until we know.
		++PlayRequestId;
		if (PendingLoad) PendingLoad->CancelHandle();
		PendingLoad.Reset();
		PendingSoundPath.Reset();
		bLoadingTransition = true;
		if (IsValid(ActiveComponent) && ActiveComponent->IsPlaying())
		{
			ActiveComponent->AdjustVolume(0.15f, GetLoadingGain());
		}
	}
	if (Destination.IsNone()) return; // Server/matchmaking has not supplied the destination yet.
	if (Settings)
	{
		const FBGMTrack* GameplayTrack = Settings->GameplayBGMByMap.Find(Destination);
		const FBGMTrack& NextTrack = GameplayTrack ? *GameplayTrack : Settings->MainMenuBGM;
		if (CurrentSound != NextTrack.Sound.Get() || bCurrentTrackLoops != NextTrack.bLoop)
		{
			// Different destination track: stop the previous screen BEFORE loading it.
			StopBGM(0.0f);
			bLoadingTransition = true;
		}
	}
	// PreLoadMap precedes a blocking map load. Async completion would only run AFTER
	// that load, leaving the loading screen silent. Resolve this one track beforehand.
	PlayMapBGM(Destination, true);
}

void UBGMSubsystem::EndLoadingBGM()
{
	if (!bLoadingTransition || !CanPlayAudio()) return;
	bLoadingTransition = false;
	// Also recovers from external-loading cancellation / connection failure.
	PlayMapBGM(GetCurrentMapName());
	if (IsValid(ActiveComponent) && ActiveComponent->IsPlaying())
	{
		ActiveComponent->AdjustVolume(Settings ? FMath::Max(0.0f, Settings->LoadingVolumeRestoreTime) : 0.5f, 1.0f);
	}
}

bool UBGMSubsystem::IsGameplayMap(const FString& MapNameOrPath) const
{
	return Settings && Settings->GameplayBGMByMap.Contains(NormalizeMapName(MapNameOrPath));
}

void UBGMSubsystem::StopBGM(float FadeOutTime)
{
	++PlayRequestId;
	if (PendingLoad) PendingLoad->CancelHandle();
	PendingLoad.Reset();
	PendingSoundPath.Reset();
	const float Fade = FadeOutTime >= 0.0f ? FadeOutTime : CurrentTrackFadeOut;
	if (IsValid(FadingComponent))
	{
		FadingComponent->OnAudioFinishedNative.RemoveAll(this);
		FadingComponent->Stop();
		FadingComponent->DestroyComponent();
		FadingComponent = nullptr;
	}
	if (IsValid(ActiveComponent))
	{
		ActiveComponent->OnAudioFinishedNative.RemoveAll(this);
		if (Fade > 0.0f)
		{
			ActiveComponent->bAutoDestroy = true;
			ActiveComponent->FadeOut(Fade, 0.0f);
			FadingComponent = ActiveComponent;
		}
		else
		{
			ActiveComponent->Stop();
			ActiveComponent->DestroyComponent();
		}
		ActiveComponent = nullptr;
	}
	CurrentSound = nullptr;
	CurrentState = EBGMState::None;
	bCurrentTrackLoops = false;
	bUseCompletionLoop = false;
	bLoadingTransition = false;
	UE_LOG(LogTemp, Log, TEXT("[BGM] Stop (FadeOut=%.2f)."), Fade);
}

void UBGMSubsystem::SetBGMVolume(float NormalizedVolume, float FadeTime)
{
	BGMVolume = FMath::Clamp(NormalizedVolume, 0.0f, 1.0f);
	ApplyVolumeOverride(FMath::Max(0.0f, FadeTime));
}

void UBGMSubsystem::SetBGMVolumePercent(float VolumePercent, float FadeTime)
{
	SetBGMVolume(FMath::Clamp(VolumePercent, 0.0f, 100.0f) / 100.0f, FadeTime);
}

void UBGMSubsystem::PlayPreviewBGM(USoundBase* TestSound, bool bLoop, float FadeInTime, float FadeOutTime)
{
	if (!IsValid(TestSound))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BGM] Preview ignored: TestSound is null."));
		return;
	}
	FBGMTrack Track;
	Track.Sound = TestSound;
	Track.bLoop = bLoop;
	Track.FadeInTime = FMath::Max(0.0f, FadeInTime);
	Track.FadeOutTime = FMath::Max(0.0f, FadeOutTime);
	PlayLoadedTrack(TestSound, Track, EBGMState::Preview, TEXT("Preview"), ++PlayRequestId);
}

void UBGMSubsystem::HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName)
{
	if (WorldContext.OwningGameInstance == GetGameInstance()) BeginLoadingBGM(MapName);
}

void UBGMSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!Settings || !LoadedWorld || LoadedWorld != (GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)) return;
	ApplyVolumeOverride(0.0f);
	PlayMapBGM(GetCurrentMapName());
}

void UBGMSubsystem::PlayMapBGM(FName MapName, bool bLoadSynchronously)
{
	if (!Settings || MapName.IsNone()) return;
	if (const FBGMTrack* Track = Settings->GameplayBGMByMap.Find(MapName))
	{
		PlayConfiguredTrack(*Track, EBGMState::Gameplay, *FString::Printf(TEXT("Gameplay:%s"), *MapName.ToString()), bLoadSynchronously);
	}
	else
	{
		PlayConfiguredTrack(Settings->MainMenuBGM, EBGMState::MainMenu, TEXT("Menu"), bLoadSynchronously);
	}
}

void UBGMSubsystem::PlayConfiguredTrack(const FBGMTrack& Track, EBGMState NewState, const TCHAR* DebugName, bool bLoadSynchronously)
{
	if (!CanPlayAudio()) return;
	if (!Track.IsConfigured())
	{
		// A missing destination asset must not leave the previous screen playing.
		const bool bWasLoading = bLoadingTransition;
		StopBGM(0.0f);
		bLoadingTransition = bWasLoading;
		UE_LOG(LogTemp, Warning, TEXT("[BGM] %s track is not configured."), DebugName);
		return;
	}

	const FSoftObjectPath SoundPath = Track.Sound.ToSoftObjectPath();
	if (!bLoadSynchronously && PendingLoad && PendingLoad->IsActive() && PendingSoundPath == SoundPath) return;
	const uint32 RequestId = ++PlayRequestId;
	if (PendingLoad) PendingLoad->CancelHandle();
	PendingLoad.Reset();
	PendingSoundPath.Reset();
	if (USoundBase* LoadedSound = Track.Sound.Get())
	{
		PlayLoadedTrack(LoadedSound, Track, NewState, DebugName, RequestId);
		return;
	}
	if (bLoadSynchronously)
	{
		PlayLoadedTrack(Track.Sound.LoadSynchronous(), Track, NewState, DebugName, RequestId);
		return;
	}

	TWeakObjectPtr<UBGMSubsystem> WeakThis(this);
	PendingSoundPath = SoundPath;
	PendingLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(SoundPath, FStreamableDelegate::CreateLambda(
		[WeakThis, Track, NewState, DebugName = FString(DebugName), RequestId]()
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->PlayLoadedTrack(Track.Sound.Get(), Track, NewState, DebugName, RequestId);
		}));
}

void UBGMSubsystem::PlayLoadedTrack(USoundBase* Sound, const FBGMTrack& Track, EBGMState NewState, FString DebugName, uint32 RequestId)
{
	if (RequestId != PlayRequestId || !IsValid(Sound) || !CanPlayAudio()) return;
	if (CurrentSound == Sound && bCurrentTrackLoops == Track.bLoop && IsValid(ActiveComponent) && ActiveComponent->IsPlaying())
	{
		CurrentState = NewState;
		CurrentTrackFadeOut = ResolveFadeOut(Track);
		ActiveComponent->SetVolumeMultiplier(Track.VolumeMultiplier);
		UE_LOG(LogTemp, Log, TEXT("[BGM] %s already playing; keeping playback position."), *DebugName);
		return;
	}

	const float FadeIn = ResolveFadeIn(Track);
	const float FadeOut = CurrentTrackFadeOut;
	if (IsValid(FadingComponent))
	{
		FadingComponent->OnAudioFinishedNative.RemoveAll(this);
		FadingComponent->Stop();
		FadingComponent->DestroyComponent();
		FadingComponent = nullptr;
	}
	if (IsValid(ActiveComponent))
	{
		ActiveComponent->OnAudioFinishedNative.RemoveAll(this);
		if (!bLoadingTransition && FadeOut > 0.0f)
		{
			ActiveComponent->bAutoDestroy = true;
			ActiveComponent->FadeOut(FadeOut, 0.0f);
			FadingComponent = ActiveComponent;
		}
		else
		{
			ActiveComponent->Stop();
			ActiveComponent->DestroyComponent();
		}
	}
	CurrentSound = nullptr;
	CurrentState = EBGMState::None;

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	USoundBase* PlaybackSound = Track.bLoop ? BGMPlayback::CreateLoopingSound(this, Sound) : Sound;
	// Create without auto-playing: start once below with the intended fade.
	ActiveComponent = UGameplayStatics::CreateSound2D(World, PlaybackSound, Track.VolumeMultiplier, 1.0f, 0.0f, nullptr, true, false);
	if (!IsValid(ActiveComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BGM] Failed to spawn %s (%s)."), *DebugName, *GetNameSafe(Sound));
		return;
	}
	// SoundWave/SoundCue repeat on the audio thread, without a game-thread callback.
	// Keep the legacy fallback only for unsupported sources such as MetaSounds.
	bUseCompletionLoop = Track.bLoop && PlaybackSound == Sound;
	if (bUseCompletionLoop)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BGM] %s needs looping authored in its own sound graph for Pause-safe playback (%s)."), *DebugName, *GetNameSafe(Sound));
		ActiveComponent->OnAudioFinishedNative.AddUObject(this, &UBGMSubsystem::HandleAudioFinished);
	}
	// Component gain = per-track correction; the audio fader = loading attenuation.
	// Do not multiply the track correction into BOTH (the old code squared it on FadeIn).
	ActiveComponent->FadeIn(FadeIn, GetLoadingGain());

	CurrentSound = Sound;
	CurrentState = NewState;
	bCurrentTrackLoops = Track.bLoop;
	CurrentTrackFadeOut = ResolveFadeOut(Track);
	UE_LOG(LogTemp, Log, TEXT("[BGM] Playing %s: %s (FadeIn=%.2f, previous FadeOut=%.2f)."), *DebugName, *GetNameSafe(Sound), FadeIn, FadeOut);
}

void UBGMSubsystem::HandleAudioFinished(UAudioComponent* FinishedComponent)
{
	// Unsupported sound types only. Game-thread completion is deferred during Pause,
	// so standard BGM must use CreateLoopingSound instead of relying on this callback.
	if (bUseCompletionLoop && bCurrentTrackLoops && IsValid(FinishedComponent) && FinishedComponent == ActiveComponent && CurrentState != EBGMState::None)
	{
		FinishedComponent->FadeIn(0.0f, GetLoadingGain());
		UE_LOG(LogTemp, Verbose, TEXT("[BGM] Loop restart: %s"), *GetNameSafe(CurrentSound));
	}
}

void UBGMSubsystem::ApplyVolumeOverride(float FadeTime)
{
	if (!Settings || !Settings->SoundMix || !Settings->BGMSoundClass || !CanPlayAudio())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[BGM] Volume cached at %.2f; SoundMix/SoundClass is not configured yet."), BGMVolume);
		return;
	}
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	UGameplayStatics::PushSoundMixModifier(World, Settings->SoundMix);
	UGameplayStatics::SetSoundMixClassOverride(World, Settings->SoundMix, Settings->BGMSoundClass, BGMVolume, 1.0f, FadeTime, true);
}

FName UBGMSubsystem::GetCurrentMapName() const
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return NAME_None;
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	return NormalizeMapName(MapName);
}

FName UBGMSubsystem::NormalizeMapName(const FString& MapNameOrPath)
{
	FString MapName = MapNameOrPath;
	// Travel URL 옵션과 오브젝트 경로를 제거하고 실제 레벨 에셋 이름만 비교합니다.
	FString UnusedOptions;
	MapName.Split(TEXT("?"), &MapName, &UnusedOptions);
	int32 SlashIndex = INDEX_NONE;
	if (MapName.FindLastChar(TEXT('/'), SlashIndex))
	{
		MapName.RightChopInline(SlashIndex + 1);
	}
	int32 DotIndex = INDEX_NONE;
	if (MapName.FindChar(TEXT('.'), DotIndex))
	{
		MapName.LeftInline(DotIndex);
	}
	// 외부 호출에서 PIE 이름이 들어온 경우에도 DataAsset에는 원본 맵 이름만 적을 수 있게 합니다.
	const FString PIEPrefix(TEXT("UEDPIE_"));
	if (MapName.StartsWith(PIEPrefix))
	{
		const int32 FirstSeparator = MapName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, PIEPrefix.Len());
		if (FirstSeparator != INDEX_NONE)
		{
			MapName.RightChopInline(FirstSeparator + 1);
		}
	}
	return FName(*MapName);
}

float UBGMSubsystem::ResolveFadeIn(const FBGMTrack& Track) const
{
	return Track.FadeInTime >= 0.0f ? Track.FadeInTime : (Settings ? Settings->DefaultFadeInTime : 1.0f);
}

float UBGMSubsystem::ResolveFadeOut(const FBGMTrack& Track) const
{
	return Track.FadeOutTime >= 0.0f ? Track.FadeOutTime : (Settings ? Settings->DefaultFadeOutTime : 1.0f);
}

bool UBGMSubsystem::CanPlayAudio() const
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	return !bDeinitializing && !IsRunningDedicatedServer() && World && World->GetNetMode() != NM_DedicatedServer;
}

float UBGMSubsystem::GetLoadingGain() const
{
	return bLoadingTransition && Settings ? FMath::Clamp(Settings->LoadingVolumeMultiplier, 0.01f, 1.0f) : 1.0f;
}

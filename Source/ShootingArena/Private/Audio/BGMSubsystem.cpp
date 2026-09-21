#include "Audio/BGMSubsystem.h"

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

	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UBGMSubsystem::HandlePreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UBGMSubsystem::HandlePostLoadMap);
	UE_LOG(LogTemp, Log, TEXT("[BGM] Subsystem initialized. Settings=%s"), *GetNameSafe(Settings));
}

void UBGMSubsystem::Deinitialize()
{
	++PlayRequestId;
	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	}
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	}
	if (IsValid(ActiveComponent)) ActiveComponent->Stop();
	if (IsValid(FadingComponent)) FadingComponent->Stop();
	ActiveComponent = nullptr;
	FadingComponent = nullptr;
	CurrentSound = nullptr;
	Super::Deinitialize();
}

void UBGMSubsystem::SetSettings(UBGMSettingsDataAsset* NewSettings)
{
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
	if (Settings) PlayConfiguredTrack(Settings->LobbyBGM, EBGMState::Lobby, TEXT("Lobby"));
}

bool UBGMSubsystem::PlayGameplayBGM(FName MapID)
{
	if (!Settings) return false;
	if (MapID.IsNone()) MapID = GetCurrentMapName();
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
	if (Settings) PlayConfiguredTrack(Settings->ResultBGM, EBGMState::Result, TEXT("Result"));
}

bool UBGMSubsystem::IsGameplayMap(const FString& MapNameOrPath) const
{
	return Settings && Settings->GameplayBGMByMap.Contains(NormalizeMapName(MapNameOrPath));
}

void UBGMSubsystem::StopBGM(float FadeOutTime)
{
	++PlayRequestId;
	const float Fade = FadeOutTime >= 0.0f ? FadeOutTime : CurrentTrackFadeOut;
	if (IsValid(FadingComponent))
	{
		FadingComponent->Stop();
		FadingComponent = nullptr;
	}
	if (IsValid(ActiveComponent))
	{
		if (Fade > 0.0f) ActiveComponent->FadeOut(Fade, 0.0f);
		else ActiveComponent->Stop();
		FadingComponent = ActiveComponent;
		ActiveComponent = nullptr;
	}
	CurrentSound = nullptr;
	CurrentState = EBGMState::None;
	bCurrentTrackLoops = false;
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

void UBGMSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// Loading BGM은 사용하지 않는다. 실제 플레이 맵으로 이동할 때 기존 메뉴/로비 음악만
	// 자연스럽게 종료하고, Gameplay 시작 신호가 올 때까지 무음 상태를 유지한다.
	if (IsGameplayMap(MapName))
	{
		UE_LOG(LogTemp, Log, TEXT("[BGM] Gameplay map travel detected; fading out current BGM: %s"), *MapName);
		StopBGM();
	}
}

void UBGMSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (!Settings || !LoadedWorld || LoadedWorld != (GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)) return;
	ApplyVolumeOverride(0.0f);
	const FName MapName = GetCurrentMapName();
	if (Settings->MainMenuMapNames.Contains(MapName))
	{
		PlayMainMenuBGM();
	}
	else if (Settings->LobbyMapNames.Contains(MapName))
	{
		PlayLobbyBGM();
	}
}

void UBGMSubsystem::PlayConfiguredTrack(const FBGMTrack& Track, EBGMState NewState, const TCHAR* DebugName)
{
	if (!CanPlayAudio()) return;
	if (!Track.IsConfigured())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BGM] %s track is not configured."), DebugName);
		return;
	}

	const uint32 RequestId = ++PlayRequestId;
	const FSoftObjectPath SoundPath = Track.Sound.ToSoftObjectPath();
	if (USoundBase* LoadedSound = Track.Sound.Get())
	{
		PlayLoadedTrack(LoadedSound, Track, NewState, DebugName, RequestId);
		return;
	}

	TWeakObjectPtr<UBGMSubsystem> WeakThis(this);
	UAssetManager::GetStreamableManager().RequestAsyncLoad(SoundPath, FStreamableDelegate::CreateLambda(
		[WeakThis, Track, NewState, DebugName = FString(DebugName), RequestId]()
		{
			if (!WeakThis.IsValid()) return;
			WeakThis->PlayLoadedTrack(Track.Sound.Get(), Track, NewState, DebugName, RequestId);
		}));
}

void UBGMSubsystem::PlayLoadedTrack(USoundBase* Sound, const FBGMTrack& Track, EBGMState NewState, FString DebugName, uint32 RequestId)
{
	if (RequestId != PlayRequestId || !IsValid(Sound) || !CanPlayAudio()) return;
	if (CurrentSound == Sound && IsValid(ActiveComponent) && ActiveComponent->IsPlaying())
	{
		CurrentState = NewState;
		UE_LOG(LogTemp, Log, TEXT("[BGM] %s already playing; keeping playback position."), *DebugName);
		return;
	}

	const float FadeIn = ResolveFadeIn(Track);
	const float FadeOut = CurrentTrackFadeOut;
	if (IsValid(FadingComponent))
	{
		FadingComponent->Stop();
		FadingComponent = nullptr;
	}
	if (IsValid(ActiveComponent))
	{
		if (FadeOut > 0.0f) ActiveComponent->FadeOut(FadeOut, 0.0f);
		else ActiveComponent->Stop();
		FadingComponent = ActiveComponent;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	ActiveComponent = UGameplayStatics::SpawnSound2D(World, Sound, Track.VolumeMultiplier, 1.0f, 0.0f, nullptr, true, false);
	if (!IsValid(ActiveComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[BGM] Failed to spawn %s (%s)."), *DebugName, *GetNameSafe(Sound));
		return;
	}
	ActiveComponent->OnAudioFinishedNative.AddUObject(this, &UBGMSubsystem::HandleAudioFinished);
	if (FadeIn > 0.0f) ActiveComponent->FadeIn(FadeIn, Track.VolumeMultiplier);
	else ActiveComponent->Play();

	CurrentSound = Sound;
	CurrentState = NewState;
	bCurrentTrackLoops = Track.bLoop;
	CurrentTrackFadeOut = ResolveFadeOut(Track);
	UE_LOG(LogTemp, Log, TEXT("[BGM] Playing %s: %s (FadeIn=%.2f, previous FadeOut=%.2f)."), *DebugName, *GetNameSafe(Sound), FadeIn, FadeOut);
}

void UBGMSubsystem::HandleAudioFinished(UAudioComponent* FinishedComponent)
{
	// Fade Out된 이전 채널의 종료 알림은 무시합니다. 현재 활성 채널의 자연 종료만 반복합니다.
	if (bCurrentTrackLoops && IsValid(FinishedComponent) && FinishedComponent == ActiveComponent && CurrentState != EBGMState::None)
	{
		FinishedComponent->Play(0.0f);
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
	return !IsRunningDedicatedServer() && GetGameInstance() && GetGameInstance()->GetWorld();
}

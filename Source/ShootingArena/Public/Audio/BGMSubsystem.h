#pragma once

#include "CoreMinimal.h"
#include "Audio/BGMSettingsDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BGMSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UWorld;

UENUM(BlueprintType)
enum class EBGMState : uint8
{
	None,
	MainMenu,
	Lobby,
	Gameplay,
	Result,
	Preview
};

/** 레벨 전환을 넘어 유지되는 클라이언트 전용 BGM 재생/전환 관리자입니다. */
UCLASS()
class SHOOTINGARENA_API UBGMSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void SetSettings(UBGMSettingsDataAsset* NewSettings);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio|BGM")
	UBGMSettingsDataAsset* GetSettings() const { return Settings; }

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayMainMenuBGM();

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayLobbyBGM();

	/** MapID가 None이면 현재 월드의 맵 이름을 사용합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	bool PlayGameplayBGM(FName MapID = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayResultBGM();

	UFUNCTION(BlueprintCallable, Category = "Audio|BGM", meta = (AdvancedDisplay = "FadeOutTime"))
	void StopBGM(float FadeOutTime = -1.0f);

	/** 옵션 UI의 0~1 BGM 슬라이더 값과 연결합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void SetBGMVolume(float NormalizedVolume, float FadeTime = 0.1f);

	/** DT_Setting의 BGMVolume처럼 0~100 범위를 사용하는 옵션 값과 직접 연결합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM", meta = (DisplayName = "Set BGM Volume (0-100)"))
	void SetBGMVolumePercent(float VolumePercent, float FadeTime = 0.1f);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio|BGM")
	float GetBGMVolume() const { return BGMVolume; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio|BGM")
	float GetBGMVolumePercent() const { return BGMVolume * 100.0f; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Audio|BGM")
	EBGMState GetCurrentState() const { return CurrentState; }

	/** 정식 BGM 에셋이 오기 전 임시 SoundCue/SoundWave의 2D 재생과 Fade를 확인합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM|Test")
	void PlayPreviewBGM(USoundBase* TestSound, bool bLoop = false, float FadeInTime = 0.25f, float FadeOutTime = 0.25f);

	/** 로딩 화면 서브시스템이 로비 -> 매치 travel인지 판단할 때 사용합니다. */
	bool IsPlayingState(EBGMState State) const { return CurrentState == State; }

	/** 전체 경로 또는 짧은 맵 이름이 GameplayBGMByMap에 등록되어 있는지 확인합니다. */
	bool IsGameplayMap(const FString& MapNameOrPath) const;

private:
	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void PlayConfiguredTrack(const FBGMTrack& Track, EBGMState NewState, const TCHAR* DebugName);
	void PlayLoadedTrack(USoundBase* Sound, const FBGMTrack& Track, EBGMState NewState, FString DebugName, uint32 RequestId);
	void HandleAudioFinished(UAudioComponent* FinishedComponent);
	void ApplyVolumeOverride(float FadeTime);
	FName GetCurrentMapName() const;
	static FName NormalizeMapName(const FString& MapNameOrPath);
	float ResolveFadeIn(const FBGMTrack& Track) const;
	float ResolveFadeOut(const FBGMTrack& Track) const;
	bool CanPlayAudio() const;

	UPROPERTY(Transient)
	TObjectPtr<UBGMSettingsDataAsset> Settings;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> ActiveComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> FadingComponent;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> CurrentSound;

	EBGMState CurrentState = EBGMState::None;
	float CurrentTrackFadeOut = 1.0f;
	float BGMVolume = 1.0f;
	bool bCurrentTrackLoops = false;
	uint32 PlayRequestId = 0;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
};

#pragma once

#include "CoreMinimal.h"
#include "Audio/BGMSettingsDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BGMSubsystem.generated.h"

class UAudioComponent;
class USoundBase;
class UWorld;
struct FWorldContext;
struct FStreamableHandle;

UENUM(BlueprintType)
enum class EBGMState : uint8
{
	None,
	MainMenu,
	Lobby UMETA(Hidden), // Serialized enum values retained for existing Blueprints.
	Gameplay,
	Result UMETA(Hidden),
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

	/** 기존 Blueprint 호출 호환: 메인/로비 모두 공용 메뉴 곡을 사용합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void PlayLobbyBGM();

	/** MapID가 None이면 현재 월드의 맵 이름을 사용합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	bool PlayGameplayBGM(FName MapID = NAME_None);

	/** 기존 Blueprint 호환용 no-op. 결과창은 현재 Gameplay BGM을 그대로 유지합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM", meta = (DeprecatedFunction, DeprecationMessage = "Result keeps Gameplay BGM; remove this call."))
	void PlayResultBGM();

	/** 같은 곡은 유지하며 로딩 볼륨 적용. 목적지가 없으면 현재 곡을 낮춰두고 목적지 확정 시 다시 호출합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|BGM")
	void BeginLoadingBGM(const FString& DestinationMap = TEXT(""));

	/** 로딩 화면이 완전히 제거될 때 호출. 곡을 다시 시작하지 않고 볼륨만 복구합니다. */
	void EndLoadingBGM();

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

	/** 현재 재생 분류를 확인합니다. 로딩은 별도 곡/상태가 아닌 볼륨 보정입니다. */
	bool IsPlayingState(EBGMState State) const { return CurrentState == State; }

	/** 전체 경로 또는 짧은 맵 이름이 GameplayBGMByMap에 등록되어 있는지 확인합니다. */
	bool IsGameplayMap(const FString& MapNameOrPath) const;

private:
	friend class FBGMLoadingPolicyTest;
	void HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void PlayMapBGM(FName MapName, bool bLoadSynchronously = false);
	void PlayConfiguredTrack(const FBGMTrack& Track, EBGMState NewState, const TCHAR* DebugName, bool bLoadSynchronously = false);
	void PlayLoadedTrack(USoundBase* Sound, const FBGMTrack& Track, EBGMState NewState, FString DebugName, uint32 RequestId);
	void HandleAudioFinished(UAudioComponent* FinishedComponent);
	void ApplyVolumeOverride(float FadeTime);
	FName GetCurrentMapName() const;
	static FName NormalizeMapName(const FString& MapNameOrPath);
	float ResolveFadeIn(const FBGMTrack& Track) const;
	float ResolveFadeOut(const FBGMTrack& Track) const;
	bool CanPlayAudio() const;
	float GetLoadingGain() const;

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
	bool bUseCompletionLoop = false;
	bool bLoadingTransition = false;
	bool bDeinitializing = false;
	TSharedPtr<FStreamableHandle> PendingLoad;
	FSoftObjectPath PendingSoundPath;
	uint32 PlayRequestId = 0;
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
};

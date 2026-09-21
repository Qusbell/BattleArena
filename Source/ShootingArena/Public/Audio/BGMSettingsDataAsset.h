#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "BGMSettingsDataAsset.generated.h"

class USoundClass;
class USoundMix;

/** BGM 한 곡과 곡별 재생 보정값입니다. 반복 여부는 SoundCue/MetaSound에서 설정합니다. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FBGMTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VolumeMultiplier = 1.0f;

	/** SoundWave/SoundCue 자체의 Loop 설정과 무관하게, 종료 시 이 트랙을 다시 재생합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	bool bLoop = true;

	/** 음수이면 DataAsset의 기본 Fade In 시간을 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "-1.0"))
	float FadeInTime = -1.0f;

	/** 음수이면 DataAsset의 기본 Fade Out 시간을 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "-1.0"))
	float FadeOutTime = -1.0f;

	bool IsConfigured() const { return !Sound.IsNull(); }
};

/** 기획자가 공용 BGM, 맵별 BGM 및 Fade 값을 한 곳에서 교체하는 설정 에셋입니다. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UBGMSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UBGMSettingsDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Tracks")
	FBGMTrack MainMenuBGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Tracks")
	FBGMTrack LobbyBGM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Tracks")
	FBGMTrack ResultBGM;

	/** 키는 GetCurrentLevelName의 PIE 접두사 제거 후 맵 이름 또는 별도의 MapID입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Tracks")
	TMap<FName, FBGMTrack> GameplayBGMByMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Transition", meta = (ClampMin = "0.0"))
	float DefaultFadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Transition", meta = (ClampMin = "0.0"))
	float DefaultFadeOutTime = 1.0f;

	/** 이 이름의 맵 진입 시 Main Menu BGM을 자동 재생합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Automatic Maps")
	TSet<FName> MainMenuMapNames;

	/** 이 이름의 맵 진입 시 Lobby BGM을 자동 재생합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Automatic Maps")
	TSet<FName> LobbyMapNames;

	/** 옵션의 BGM 볼륨 override에 사용할 SoundMix입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Volume")
	TObjectPtr<USoundMix> SoundMix;

	/** 모든 BGM 에셋에 지정할 공용 SoundClass입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Volume")
	TObjectPtr<USoundClass> BGMSoundClass;
};

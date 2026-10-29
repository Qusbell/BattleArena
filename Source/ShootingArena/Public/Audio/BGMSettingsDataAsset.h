#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundBase.h"
#include "BGMSettingsDataAsset.generated.h"

class USoundClass;
class USoundMix;

/** BGM 한 곡과 곡별 재생 보정값입니다. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FBGMTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VolumeMultiplier = 1.0f;

	/** SoundWave/SoundCue는 원본 변경 없이 오디오 스레드에서 반복합니다. MetaSound는 그래프 내부 Loop 설정이 필요합니다. */
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
	/** 메인/로비/맵 선택 등 모든 비 Gameplay 화면의 공용 곡. 기존 에셋 값을 유지합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Tracks", meta = (DisplayName = "Menu BGM"))
	FBGMTrack MainMenuBGM;

	/** 키는 GetCurrentLevelName의 PIE 접두사 제거 후 맵 이름 또는 별도의 MapID입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Tracks")
	TMap<FName, FBGMTrack> GameplayBGMByMap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Transition", meta = (ClampMin = "0.0"))
	float DefaultFadeInTime = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Transition", meta = (ClampMin = "0.0"))
	float DefaultFadeOutTime = 1.0f;

	/** 옵션 볼륨과 별개로 로딩 중에만 적용하는 배율입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Transition", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float LoadingVolumeMultiplier = 0.5f;

	/** 로딩 화면 제거 후 정상 볼륨으로 복구하는 시간입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Transition", meta = (ClampMin = "0.0"))
	float LoadingVolumeRestoreTime = 0.5f;

	/** 옵션의 BGM 볼륨 override에 사용할 SoundMix입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Volume")
	TObjectPtr<USoundMix> SoundMix;

	/** 모든 BGM 에셋에 지정할 공용 SoundClass입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "BGM|Volume")
	TObjectPtr<USoundClass> BGMSoundClass;
};

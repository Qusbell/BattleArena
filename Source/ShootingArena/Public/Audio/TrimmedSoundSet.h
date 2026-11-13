#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TrimmedSoundSet.generated.h"

class UAudioComponent;

/** 원본 에셋을 수정하지 않고 시작/끝 구간을 보정해 재생하는 사운드 한 개의 설정입니다. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FTrimmedSoundEntry
{
	GENERATED_BODY()

	/** 재생 요청에 사용하는 이름입니다. 한 세트 안에서 겹치지 않게 지정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundBase> Sound;

	/**
	 * 시작 보정(초)입니다.
	 * 양수: 소리 앞부분을 그만큼 잘라 더 빨리 들리게 합니다.
	 * 음수: 재생 전에 그만큼 기다려 늦게 들리게 합니다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "-5.0", ClampMax = "30.0", Units = "s"))
	float StartOffsetSeconds = 0.0f;

	/** 소리 뒷부분을 잘라낼 길이(초)입니다. 반복 재생되는 소리에는 적용되지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0", ClampMax = "30.0", Units = "s"))
	float EndTrimSeconds = 0.0f;

	/** 뒷부분을 자를 때 딱 소리가 나지 않도록 적용하는 페이드 아웃 시간(초)입니다. 0이면 즉시 끊습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0", ClampMax = "1.0", Units = "s"))
	float EndFadeOutSeconds = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.1", ClampMax = "4.0"))
	float PitchMultiplier = 1.0f;
};

/** 여러 사운드의 시작/끝 보정값을 한 곳에서 관리하는 설정 에셋입니다. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UTrimmedSoundSet : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sounds", meta = (TitleProperty = "Id"))
	TArray<FTrimmedSoundEntry> Sounds;

	/** Id에 해당하는 항목을 찾습니다. 없으면 nullptr입니다. */
	const FTrimmedSoundEntry* FindEntry(FName Id) const;
};

UCLASS()
class SHOOTINGARENA_API UTrimmedSoundLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 세트의 Id 항목을 보정값대로 2D 재생합니다. 데디케이티드 서버에서는 재생하지 않습니다.
	 * 시작을 늦추는 경우 반환되는 컴포넌트는 지연 후에 재생이 시작됩니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Audio|Trimmed Sound", meta = (WorldContext = "WorldContextObject"))
	static UAudioComponent* PlayTrimmedSound2D(const UObject* WorldContextObject, const UTrimmedSoundSet* SoundSet, FName Id);

	/** 세트를 거치지 않고 항목 하나를 직접 재생합니다. */
	UFUNCTION(BlueprintCallable, Category = "Audio|Trimmed Sound", meta = (WorldContext = "WorldContextObject"))
	static UAudioComponent* PlayTrimmedEntry2D(const UObject* WorldContextObject, const FTrimmedSoundEntry& Entry);
};

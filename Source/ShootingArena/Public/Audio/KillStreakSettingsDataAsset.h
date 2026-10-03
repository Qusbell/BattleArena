#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KillStreakSettingsDataAsset.generated.h"

class USoundBase;

/** 킬 스트릭 기준 횟수와 해당 단계에서 재생할 사운드 목록입니다. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FKillStreakVoiceTier
{
	GENERATED_BODY()

	/** 이 단계의 보이스 재생에 필요한 최소 연속 처치 횟수입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭 보이스|단계", meta = (ClampMin = "1", UIMin = "1", DisplayName = "기준 연속 처치 수"))
	int32 RequiredKillCount = 1;

	/** 해당 단계에서 무작위로 선택할 사운드 목록입니다. 비어 있으면 이 단계의 보이스를 재생하지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭 보이스|단계", meta = (DisplayName = "재생 사운드 목록"))
	TArray<TObjectPtr<USoundBase>> Sounds;
};

/** 킬 이벤트 배치, 보이스 재생 및 최근 가해자 귀속 판정에 공용으로 사용하는 설정입니다. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UKillStreakSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 첫 킬 이벤트부터 배치 처리를 시작하기까지의 고정 대기 시간입니다. 후속 킬 이벤트는 시간을 연장하지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭 보이스|재생", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "킬 이벤트 배치 시간"))
	float KillEventBatchDelay = 0.2f;

	/** 보이스 재생 후 다음 보이스 요청을 제한하는 시간입니다. 제한 시간 내 요청은 버립니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭 보이스|재생", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "보이스 재생 제한 시간"))
	float KillVoiceOverlapTime = 0.0f;

	/** 복수 킬 배치에서 우선 재생할 단발 사운드입니다. 비어 있거나 반복 사운드면 기존 킬 스트릭 보이스를 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭 보이스|재생", meta = (DisplayName = "복수 킬 보이스"))
	TObjectPtr<USoundBase> RevengeSound = nullptr;

	/** 연속 처치 기준별 사운드를 설정합니다. 정확한 기준이 없으면 무음이며, 최고 기준을 넘으면 최고 단계를 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭 보이스|단계", meta = (DisplayName = "단계별 보이스 설정"))
	TArray<FKillStreakVoiceTier> KillStreakTiers;

	/** 자살 또는 환경사 시 최근 가해자에게 킬 수혜를 귀속할 수 있는 유효 시간입니다. 0이면 최근 가해자를 인정하지 않습니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "킬 스트릭|킬 귀속", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "최근 가해자 유효 시간"))
	float AttackerMemoryTime = 5.0f;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "KillStreakVoiceSettingsDataAsset.generated.h"

class USoundBase;

/** A kill-streak threshold and the alternative one-shot voices for that threshold. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FKillStreakVoiceTier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Streak Voice", meta = (ClampMin = "1", UIMin = "1"))
	int32 RequiredKillCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Streak Voice")
	TArray<TObjectPtr<USoundBase>> Sounds;
};

/** Tunable batching, overlap, and sound selection settings for kill-streak voices. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UKillStreakVoiceSettingsDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Fixed window started by the first credited kill event in a batch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Streak Voice|Timing", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float KillEventBatchDelay = 0.2f;

	/** Time after sending a voice during which subsequent kill voices are discarded. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Streak Voice|Timing", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float KillVoiceOverlapTime = 0.0f;

	/** Thresholds may be added or removed without changing the component implementation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kill Streak Voice|Tiers")
	TArray<FKillStreakVoiceTier> KillStreakTiers;
};

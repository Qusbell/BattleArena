#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CascadeEffectPoolSettings.generated.h"

class UParticleSystem;
class UWorld;

USTRUCT()
struct FCascadeEffectPrewarmEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Prewarm")
	TSoftObjectPtr<UParticleSystem> Effect;

	UPROPERTY(EditAnywhere, Category="Prewarm", meta=(ClampMin="0", UIMin="0"))
	int32 Count = 1;
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Cascade Effect Pool"))
class SHOOTINGARENA_API UCascadeEffectPoolSettings final : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	virtual FName GetSectionName() const override { return TEXT("CascadeEffectPool"); }

	UPROPERTY(Config, EditAnywhere, Category="Prewarm")
	bool bEnablePrewarm = true;

	/** Prewarm before play in these maps. An empty list disables prewarming. */
	UPROPERTY(Config, EditAnywhere, Category="Prewarm")
	TArray<TSoftObjectPtr<UWorld>> MatchMaps;

	/** Idle components to prepare per effect and world, capped by the effect's Max Pool Size. */
	UPROPERTY(Config, EditAnywhere, Category="Prewarm", meta=(TitleProperty="Effect"))
	TArray<FCascadeEffectPrewarmEntry> Effects;
};

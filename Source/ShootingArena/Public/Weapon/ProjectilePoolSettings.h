#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ProjectilePoolSettings.generated.h"

/** Validated defaults captured separately by each game world. */
struct FProjectilePoolConfiguration
{
	bool bEnabled = true;
	int32 PlasmaPrewarm = 32;
	int32 PlasmaCapacity = 128;
	int32 RocketPrewarm = 16;
	int32 RocketCapacity = 256;
	int32 PrewarmPerTick = 4;
};

/** Plasma and Rocket pools are shared within each game world, not allocated per weapon. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Projectile Pool"))
class SHOOTINGARENA_API UProjectilePoolSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	virtual FName GetSectionName() const override { return TEXT("ProjectilePool"); }

	UPROPERTY(Config, EditAnywhere, Category="General", meta=(DisplayName="Enable Projectile Pool", ToolTip="Defaults apply to new game worlds and PIE sessions. Console overrides, when explicitly set, take priority."))
	bool bEnabled = true;

	UPROPERTY(Config, EditAnywhere, Category="Plasma", meta=(DisplayName="Initial Pool Size", ClampMin="0", UIMin="0", ToolTip="Plasma actors pre-created per game world. Clamped to Maximum Retained Size. Applies on the next PIE/game session."))
	int32 PlasmaPrewarm = 32;

	UPROPERTY(Config, EditAnywhere, Category="Plasma", meta=(DisplayName="Maximum Retained Size", ClampMin="0", UIMin="0", ToolTip="Maximum Plasma actors retained for reuse per world, including active and idle actors. Excess shots use temporary actors and still fire normally. Zero disables retention."))
	int32 PlasmaCapacity = 128;

	UPROPERTY(Config, EditAnywhere, Category="Rocket", meta=(DisplayName="Initial Pool Size", ClampMin="0", UIMin="0", ToolTip="Rocket actors pre-created per game world. Clamped to Maximum Retained Size. Applies on the next PIE/game session."))
	int32 RocketPrewarm = 16;

	UPROPERTY(Config, EditAnywhere, Category="Rocket", meta=(DisplayName="Maximum Retained Size", ClampMin="0", UIMin="0", ToolTip="Maximum Rocket actors retained for reuse per world, including active and idle actors. Excess shots use temporary actors and still fire normally. Zero disables retention."))
	int32 RocketCapacity = 256;

	UPROPERTY(Config, EditAnywhere, AdvancedDisplay, Category="Prewarming", meta=(DisplayName="Prewarm Actors Per Tick", ClampMin="1", UIMin="1", ToolTip="Combined Plasma/Rocket creation budget per world tick during initial prewarming."))
	int32 PrewarmPerTick = 4;

	FProjectilePoolConfiguration GetConfiguration() const;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

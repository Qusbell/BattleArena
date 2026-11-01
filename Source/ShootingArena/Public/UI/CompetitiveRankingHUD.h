#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CompetitiveRankingHUD.generated.h"

class AGameStateBase;
class UBorder;
class UCanvasPanelSlot;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UKillMedalUISettings;

/** CompShooter HUD base: compact leader/local-player standings beside its existing timer. */
UCLASS()
class SHOOTINGARENA_API UCompetitiveRankingHUD : public UUserWidget
{
	GENERATED_BODY()
public:
	UCompetitiveRankingHUD(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Kill Medal", meta = (DisplayName = "연속 킬 메달 UI 설정"))
	TSoftObjectPtr<UKillMedalUISettings> KillMedalUISettings = TSoftObjectPtr<UKillMedalUISettings>(
		FSoftObjectPath(TEXT("/Game/QuakeLike_1_0/UI/PlayerUI/KillMedals/DA_KillMedalUISettings.DA_KillMedalUISettings")));
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
private:
	friend class FCompetitiveRankingPIETest;
	friend class FKillMedalAutomaticAwardsTest;
	UFUNCTION()
	void RefreshRanking();
	void BindScoreChanges(AGameStateBase* State);
	void PositionBesideTimer(const FGeometry& Geometry);
	void RefreshStreakMedals(int32 Count);
	void HideStreakMedals();

	UPROPERTY() TSubclassOf<UUserWidget> StreakHighlightClass;
	UPROPERTY(Transient) TObjectPtr<UUserWidget> StreakHighlight;
	FTimerHandle StreakDisplayTimer;
	UPROPERTY(Transient) TObjectPtr<UKillMedalUISettings> ActiveStreakSettings;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StreakPanel;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> StreakIcons;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StreakCountLabel;
	int32 DisplayedStreak = INDEX_NONE;

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> RankingPanel;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> RankingSlot;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> LeaderProgress;
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> RowBorders;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> RankLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> NameLabels;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ScoreLabels;
	UPROPERTY(Transient) TWeakObjectPtr<AGameStateBase> BoundGameState;
	float RefreshElapsed = 0.0f;
};

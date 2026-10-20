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

/** CompShooter HUD base: compact leader/local-player standings beside its existing timer. */
UCLASS()
class SHOOTINGARENA_API UCompetitiveRankingHUD : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
private:
	friend class FCompetitiveRankingPIETest;
	UFUNCTION()
	void RefreshRanking();
	void BindScoreChanges(AGameStateBase* State);
	void PositionBesideTimer(const FGeometry& Geometry);

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

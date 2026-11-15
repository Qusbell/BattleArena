#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "SinglePlaySteps.generated.h"

class UButton;
class UTextBlock;
class UTexture2D;

UCLASS(Abstract, Blueprintable)
class SHOOTINGARENA_API USinglePlaySteps : public UUserWidget
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Single Play") void OpenMapStep();
    UFUNCTION(BlueprintCallable, Category="Single Play") void OpenDifficultyStep(int32 SelectedMap = -1);
    UFUNCTION(BlueprintCallable, Category="Single Play") void HoverMap(int32 Index);
    UFUNCTION(BlueprintPure, Category="Single Play") bool IsDifficultyStep() const { return bDifficultyStep; }
    UFUNCTION(BlueprintCallable, Category="Single Play") void SetBackdropBrush(const FSlateBrush& Brush);
    UFUNCTION(BlueprintPure, Category="Single Play") UTextBlock* GetLaunchErrorText() const;
    UFUNCTION(BlueprintPure, Category="Single Play") int32 GetInitialDifficultyIndex() const { return DifficultyIndex; }
    void SetMapBackdrop(UTexture2D* Texture);
    void UpdateStepVisuals(int32 Map, int32 Difficulty, float DeltaTime);
    static bool IsStepKey(const FKey& Key);
    bool HandleStepKey(const FKey& Key, bool bRepeat);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    UFUNCTION(BlueprintImplementableEvent, Category="Single Play") void ChooseMap(int32 Index);
    UFUNCTION(BlueprintImplementableEvent, Category="Single Play") void ChooseEasy();
    UFUNCTION(BlueprintImplementableEvent, Category="Single Play") void ChooseNormal();
    UFUNCTION(BlueprintImplementableEvent, Category="Single Play") void ChooseHard();
    UFUNCTION(BlueprintImplementableEvent, Category="Single Play") void StartMatch();

private:
    UFUNCTION() void ReturnToMap();
    void ChangeStep(bool bNext, bool bAnimate);
    void UpdateBackdrop();
    void UpdateDifficultyVisuals(int32 Difficulty, float DeltaTime);
    UButton* MapButton(int32 Index) const;
    UButton* DifficultyButton(int32 Index) const;

    bool bDifficultyStep = false;
    bool bHasOpenedDifficulty = false;
    bool bInitialMapFocus = true;
    int32 MapIndex = 0;
    int32 DifficultyIndex = 1;
    int32 PreviousDifficultyIndex = 1;
    float DifficultySelectionElapsed = 0.56f;
    float TransitionElapsed = 0.32f;
    float BackdropFadeRemaining = 0.0f;
    bool bStartFocused = false;
    float StartOutlineInset = 7.0f;
    TWeakObjectPtr<UButton> DifficultyNavigationTarget;
    UPROPERTY(Transient) FSlateBrush MapBackdrop;
    UPROPERTY(Transient) FSlateBrush PreviewBackdrop;
    UPROPERTY(EditDefaultsOnly, Category="Single Play") TSubclassOf<USinglePlaySteps> DifficultyScreenClass;
    UPROPERTY(Transient) TObjectPtr<USinglePlaySteps> DifficultyScreen;
    TWeakObjectPtr<USinglePlaySteps> MapScreen;
};

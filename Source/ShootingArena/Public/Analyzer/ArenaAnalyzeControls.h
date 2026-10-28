#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Analyzer/ArenaAnalyzeSubsystem.h"
#include "ArenaAnalyzeControls.generated.h"

/** Compact editor analyzer controls, using the containing utility's existing Files selection. */
UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeControls : public UUserWidget
{
    GENERATED_BODY()
public:
    static void ConfigureCombo(class UComboBoxString* Combo);
protected:
    virtual bool Initialize() override;
private:
    UPROPERTY(Transient) TObjectPtr<class UComboBoxString> Weapons;
    UPROPERTY(Transient) TObjectPtr<class UComboBoxString> Actors;
    UPROPERTY(Transient) TObjectPtr<class UComboBoxString> Events;
    UPROPERTY(Transient) TObjectPtr<class UTextBlock> Summary;
    UPROPERTY(Transient) FArenaAnalyzeSession Session;
    FString LoadedFile;
    TArray<FString> WeaponIds;
    FString SelectedFile() const;
    AActor* GetVisualizer() const;
    UFUNCTION() UWidget* GenerateOption(FString Option);
    UFUNCTION() void LoadSession();
    UFUNCTION() void DrawRecords();
    UFUNCTION() void ClearRecords();
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "LanguageSubsystem.generated.h"

class UFont;
class UTextBlock;
class UButton;
class UBlueprintComboStyle;
class UCommonTextStyle;

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FLanguageRow : public FTableRowBase
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString CultureCode;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool IsEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool IsDefault = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UFont> Font;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnArenaLanguageChanged);

/** Device-local presentation preferences. Culture is never replicated. */
UCLASS()
class SHOOTINGARENA_API ULanguageSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintPure, Category="Language") TArray<FLanguageRow> GetEnabledLanguages() const;
    UFUNCTION(BlueprintPure, Category="Language") FString GetCurrentLanguageCode() const { return CurrentCode; }
    UFUNCTION(BlueprintCallable, Category="Language") bool SetLanguage(const FString& CultureCode);
    UPROPERTY(BlueprintAssignable, Category="Language") FOnArenaLanguageChanged OnLanguageChanged;
    static UDataTable* GetLanguageTable();
    static FString ChooseStartupLanguage(const TArray<FLanguageRow>& Languages, bool HasSaved,
        const FString& Saved, const FString& OS);
private:
    bool Apply(const FString& Code, bool Save);
    FString CurrentCode;
    FString PreviousCulture;
    bool WasPreviewEnabled = false;
    UPROPERTY(Transient) TObjectPtr<UDataTable> ActiveTable;
};

UCLASS()
class SHOOTINGARENA_API ULanguageLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    static const TCHAR* StringTablePath();
    UFUNCTION(BlueprintPure, Category="Language") static FText Text(FName Key);
    UFUNCTION(BlueprintPure, Category="Language", meta=(AutoCreateRefTerm="A,B,C,D,E,F,G,H")) static FText FormatText(FName Key,
        const FText& A, const FText& B, const FText& C, const FText& D, const FText& E, const FText& F, const FText& G, const FText& H);
    /** Keeps text history across pre-existing String-based UI interfaces. Never use for gameplay IDs. */
    UFUNCTION(BlueprintPure, Category="Language", meta=(AutoCreateRefTerm="A,B,C,D,E,F,G,H")) static FString DisplayString(FName Key,
        const FText& A, const FText& B, const FText& C, const FText& D, const FText& E, const FText& F, const FText& G, const FText& H);
    UFUNCTION(BlueprintPure, Category="Language") static FText ReadDisplayString(const FString& Value);
    UFUNCTION(BlueprintPure, Category="Language") static FText DisplayName(FName Domain, FName Id);
    UFUNCTION(BlueprintPure, Category="Language") static FString ParticipantDisplayString(class APlayerState* State);
    static FSlateFontInfo ResolveFont(FSlateFontInfo Existing);
    static bool IsWidgetLocalizationExcluded(const UObject* Widget);
    static void RefreshViewportFonts(UWorld* World);
};

/** Shared settings row. Options are identified by CultureCode, never translated labels. */
UCLASS()
class SHOOTINGARENA_API ULanguageSelector : public UUserWidget
{
    GENERATED_BODY()
public:
    /** Compact Korean/English shortcuts used below Quit on the main menu. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language|Appearance") bool bInlineChoices = false;
    /** A separate Language heading inside the gameplay settings list. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language|Appearance") bool bShowSectionHeading = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language|Appearance") TSubclassOf<UBlueprintComboStyle> ComboStyle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Language|Appearance") TSubclassOf<UCommonTextStyle> HeadingStyle;
    UFUNCTION(BlueprintCallable, Category="Language") void SetSettingsTab(int32 TabIndex);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
private:
    friend class FLanguageSelectorPlacementTest;
    UFUNCTION() void Refresh();
    UFUNCTION() void Selected(FString Item, ESelectInfo::Type SelectionType);
    UFUNCTION() UWidget* GenerateOption(FString Item);
    UFUNCTION() void SelectKorean();
    UFUNCTION() void SelectEnglish();
    void ApplyChoice(const FString& Code);
    UPROPERTY(Transient) TObjectPtr<UComboBoxString> Combo;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
    UPROPERTY(Transient) TObjectPtr<UButton> KoreanButton;
    UPROPERTY(Transient) TObjectPtr<UButton> EnglishButton;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> KoreanLabel;
    UPROPERTY(Transient) TObjectPtr<UTextBlock> EnglishLabel;
    TWeakObjectPtr<ULanguageSubsystem> BoundLanguage;
    TArray<FLanguageRow> Options;
    bool Refreshing = false;
};

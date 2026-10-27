#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/EngineSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIPalette.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UUserWidget;
class UBlueprint;
class FObjectPreSaveContext;
class FObjectPostSaveContext;

/** A color property on a style CDO, data asset, or named widget template. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FUIPaletteObjectBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TSoftClassPtr<UObject> TargetClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TSoftObjectPtr<UObject> TargetAsset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") FName WidgetName;
    /** Dot-separated reflected property path, with optional array indices. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TMap<FString, FName> Fields;
};

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FUIPaletteMaterialBinding
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TSoftObjectPtr<UMaterialInterface> Parent;
    /** Material vector parameter name -> palette role. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TMap<FName, FName> Parameters;
};

/** The existing PDA_UIPalette Blueprint keeps its authored color variables. */
UCLASS(BlueprintType, Blueprintable)
class SHOOTINGARENA_API UUIThemePalette : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TMap<FName, FLinearColor> AdditionalColors;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TArray<FUIPaletteObjectBinding> ObjectBindings;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Palette") TArray<FUIPaletteMaterialBinding> MaterialBindings;

    bool TryGetColor(FName Role, FLinearColor& OutColor) const;
    void RefreshMaterials();
    UMaterialInterface* ResolveMaterial(UMaterialInterface* Material) const;
    UFUNCTION(BlueprintPure, Category="UI|Palette") UMaterialInterface* ResolveOriginalMaterial(UMaterialInterface* Material) const;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
    virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& Event) override;
#endif

private:
    /** Runtime-only instances preserve the original material assets on disk. */
    UPROPERTY(Transient) TMap<TObjectPtr<UMaterialInterface>, TObjectPtr<UMaterialInstanceDynamic>> ResolvedMaterials;
};

UCLASS()
class SHOOTINGARENA_API UUIThemePaletteLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="UI|Palette") static UUIThemePalette* GetPalette();
    UFUNCTION(BlueprintPure, Category="UI|Palette") static FLinearColor GetColor(FName Role);
    UFUNCTION(BlueprintCallable, Category="UI|Palette") static void PreparePalette();
    UFUNCTION(BlueprintCallable, Category="UI|Palette") static void ApplyObject(UObject* Object);
    UFUNCTION(BlueprintCallable, Category="UI|Palette") static void ApplyPaletteToObject(UObject* Object);
    UFUNCTION(BlueprintCallable, Category="UI|Palette") static void ApplyPalette();
    UFUNCTION(BlueprintCallable, Category="UI|Palette") static void ApplyWidgetTree(UUserWidget* Screen);
    UFUNCTION(BlueprintCallable, Category="UI|Palette|Authoring") static FString DescribeColorProperties(UObject* Object);

    /** Coalesces editor changes and asset loads into a safe, single deferred refresh. */
    static void RequestRefresh();
};

/** Runs before gameplay widgets are created, including each PIE game instance. */
UCLASS()
class SHOOTINGARENA_API UUIThemePaletteSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UFUNCTION(BlueprintCallable, Category="UI|Palette") void RefreshPalette();
};

/** Initializes editor previews without loading assets from a PostLoad callback. */
UCLASS()
class SHOOTINGARENA_API UUIThemePaletteEngineSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    friend class UUIThemePaletteLibrary;
    UPROPERTY(Transient) TObjectPtr<UUIThemePalette> ActivePalette;
    void OnAssetLoaded(UObject* Object);
    FDelegateHandle AssetLoadedHandle;
#if WITH_EDITOR
    void RegisterBlueprintCompileHooks();
    void OnBlueprintPreCompile(UBlueprint* Blueprint);
    void OnBlueprintCompiled();
    void OnObjectPreSave(UObject* Object, FObjectPreSaveContext Context);
    void OnPackagePreSave(UPackage* Package, FObjectPreSaveContext Context);
    void OnPackageSaved(const FString& Filename, UPackage* Package, FObjectPostSaveContext Context);
    void RestorePackageMaterials(UPackage* Package, bool bPreparingSave = true);
    FDelegateHandle BlueprintPreCompileHandle;
    FDelegateHandle BlueprintCompiledHandle;
    FDelegateHandle BlueprintReinstancedHandle;
    FDelegateHandle ObjectPreSaveHandle;
    FDelegateHandle PackagePreSaveHandle;
    FDelegateHandle PackageSavedHandle;
#endif
};

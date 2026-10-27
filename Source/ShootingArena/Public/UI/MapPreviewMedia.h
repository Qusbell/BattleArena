#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Styling/SlateBrush.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MapPreviewMedia.generated.h"

class UButton;
class UImage;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMediaPlayer;
class UMediaSource;
class UMediaTexture;
class UTexture2D;
class UUserWidget;
class UMapPreviewBinding;
struct FWorldContext;

/** Row names match DT_MapData. The still image and video are independent assets. */
USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FMapPreviewMediaRow : public FTableRowBase
{
    GENERATED_BODY()

    /** If empty, keep the image supplied by DT_MapData.Map_Image. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Preview")
    TObjectPtr<UTexture2D> StillImage;

    /** FileMediaSource (normally under Content/Movies), or another MediaSource. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map Preview")
    TObjectPtr<UMediaSource> PreviewVideo;
};

/** One live decoder per map, shared by selection and difficulty screens. */
UCLASS(Transient)
class SHOOTINGARENA_API UMapPreviewPlayback : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UMediaSource* InSource, UMaterialInterface* VideoMaterial);
    void SetActive(UMapPreviewBinding* Binding, bool bActive);
    void Shutdown();
    bool IsReady() const;
    UMaterialInstanceDynamic* GetVideoMaterial() const { return Material; }
    UMediaSource* GetSource() const { return Source; }
    UMediaPlayer* GetPlayer() const { return Player; }

private:
    UFUNCTION() void HandleMediaOpened(FString OpenedUrl);
    UFUNCTION() void HandleMediaOpenFailed(FString FailedUrl);
    void RefreshBindings();
    bool HasActiveBindings();

    UPROPERTY(Transient) TObjectPtr<UMediaSource> Source;
    UPROPERTY(Transient) TObjectPtr<UMediaPlayer> Player;
    UPROPERTY(Transient) TObjectPtr<UMediaTexture> Texture;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    TSet<TWeakObjectPtr<UMapPreviewBinding>> ActiveBindings;
    bool bOpening = false;
    bool bReady = false;
};

/** Owns hover delegates and restores the card's original image when inactive. */
UCLASS(Transient, BlueprintType)
class SHOOTINGARENA_API UMapPreviewBinding : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(UUserWidget* InScreen, UImage* InImage, UButton* InButton,
        UMapPreviewPlayback* InPlayback, bool bInAlwaysPlay);
    void Release();
    void RefreshBrush();
    bool Matches(UUserWidget* InScreen, UImage* InImage = nullptr) const;
    bool IsScreenValid() const { return Screen.IsValid(); }

    UFUNCTION(BlueprintCallable, Category="UI|Map Preview")
    void SetPreviewActive(bool bActive);

    UFUNCTION(BlueprintPure, Category="UI|Map Preview")
    UMediaPlayer* GetMediaPlayer() const;

private:
    UFUNCTION() void HandleHovered();
    UFUNCTION() void HandleUnhovered();

    TWeakObjectPtr<UUserWidget> Screen;
    TWeakObjectPtr<UImage> Image;
    TWeakObjectPtr<UButton> Button;
    UPROPERTY(Transient) TObjectPtr<UMapPreviewPlayback> Playback;
    UPROPERTY(Transient) FSlateBrush StillBrush;
    bool bActive = false;
    bool bAlwaysPlay = false;
};

UCLASS()
class SHOOTINGARENA_API UMapPreviewSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    UMapPreviewBinding* Configure(UUserWidget* Screen, UImage* Image, UButton* HoverButton,
        FName MapId, UDataTable* PreviewMedia, UMaterialInterface* VideoMaterial, bool bAlwaysPlay);
    void ReleaseScreen(UUserWidget* Screen);

private:
    friend class FMapPreviewTravelCleanupTest;
    void HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName);
    void HandleWorldBeginTearDown(UWorld* World);
    void ReleaseAllPreviews();
    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle WorldTearDownHandle;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<UMapPreviewPlayback>> Playbacks;
    UPROPERTY(Transient) TArray<TObjectPtr<UMapPreviewBinding>> Bindings;
};

UCLASS()
class SHOOTINGARENA_API UMapPreviewLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Call after setting the map image. Use Always Play on the difficulty screen. */
    UFUNCTION(BlueprintCallable, Category="UI|Map Preview", meta=(DefaultToSelf="Screen"))
    static UMapPreviewBinding* ConfigureMapPreview(UUserWidget* Screen, UImage* Image,
        UButton* HoverButton, FName MapId, UDataTable* PreviewMedia,
        UMaterialInterface* VideoMaterial, bool bAlwaysPlay = false);

    /** Call from Destruct. Other screens using the same playback keep playing. */
    UFUNCTION(BlueprintCallable, Category="UI|Map Preview", meta=(DefaultToSelf="Screen"))
    static void ReleaseMapPreviews(UUserWidget* Screen);
};

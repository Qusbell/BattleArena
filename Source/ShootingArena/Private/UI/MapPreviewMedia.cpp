#include "UI/MapPreviewMedia.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MediaPlayer.h"
#include "MediaSource.h"
#include "MediaTexture.h"
#include "UObject/UObjectGlobals.h"

void UMapPreviewPlayback::Initialize(UMediaSource* InSource, UMaterialInterface* VideoMaterial)
{
    Source = InSource;
    Player = NewObject<UMediaPlayer>(this);
    Player->PlayOnOpen = false;
    Player->SetLooping(true);
    Player->OnMediaOpened.AddDynamic(this, &ThisClass::HandleMediaOpened);
    Player->OnMediaOpenFailed.AddDynamic(this, &ThisClass::HandleMediaOpenFailed);
    Texture = NewObject<UMediaTexture>(this);
    Texture->AutoClear = false;
    Texture->NewStyleOutput = true;
    Texture->SetMediaPlayer(Player);
    Texture->UpdateResource();
    Material = UMaterialInstanceDynamic::Create(VideoMaterial, this);
    Material->SetTextureParameterValue(TEXT("VideoTexture"), Texture);
}

bool UMapPreviewPlayback::HasActiveBindings()
{
    for (auto It = ActiveBindings.CreateIterator(); It; ++It)
    {
        if (!It->IsValid())
        {
            It.RemoveCurrent();
        }
    }
    return !ActiveBindings.IsEmpty();
}

void UMapPreviewPlayback::SetActive(UMapPreviewBinding* Binding, bool bActive)
{
    if (!Player)
    {
        return;
    }
    if (bActive)
    {
        ActiveBindings.Add(Binding);
    }
    else
    {
        ActiveBindings.Remove(Binding);
    }

    if (!HasActiveBindings())
    {
        // Pause preserves the exact decoder position; don't close/reopen or seek on hover.
        Player->Pause();
    }
    else if (IsReady())
    {
        Player->Play();
    }
    else if (!bOpening)
    {
        bOpening = true;
        if (!Player->OpenSource(Source))
        {
            HandleMediaOpenFailed(Source->GetUrl());
        }
    }
}

bool UMapPreviewPlayback::IsReady() const
{
    return bReady && Player && Player->IsReady();
}

void UMapPreviewPlayback::HandleMediaOpened(FString OpenedUrl)
{
    bOpening = false;
    bReady = true;
    Player->SetLooping(true);
    if (HasActiveBindings())
    {
        Player->Play();
    }
    else
    {
        Player->Pause();
    }
    RefreshBindings();
}

void UMapPreviewPlayback::HandleMediaOpenFailed(FString FailedUrl)
{
    bOpening = false;
    bReady = false;
    UE_LOG(LogTemp, Warning, TEXT("Map preview video could not be opened: %s"), *FailedUrl);
    RefreshBindings();
}

void UMapPreviewPlayback::RefreshBindings()
{
    for (const TWeakObjectPtr<UMapPreviewBinding>& Binding : ActiveBindings)
    {
        if (Binding.IsValid())
        {
            Binding->RefreshBrush();
        }
    }
}

void UMapPreviewPlayback::Shutdown()
{
    ActiveBindings.Empty();
    bReady = false;
    bOpening = false;
    if (Player)
    {
        Player->OnMediaOpened.RemoveAll(this);
        Player->OnMediaOpenFailed.RemoveAll(this);
        Player->Close();
    }
    if (Material)
    {
        Material->ClearParameterValues();
    }
    if (Texture)
    {
        Texture->SetMediaPlayer(nullptr);
        Texture->ReleaseResource();
    }
    // Close the decoder immediately and drop all strong references. UObject
    // reclamation follows normal GC; don't force a collection during travel.
    Material = nullptr;
    Texture = nullptr;
    Player = nullptr;
    Source = nullptr;
}

void UMapPreviewBinding::Initialize(UUserWidget* InScreen, UImage* InImage, UButton* InButton,
    UMapPreviewPlayback* InPlayback, bool bInAlwaysPlay, float InHoverDelaySeconds)
{
    Screen = InScreen;
    Image = InImage;
    Button = InButton;
    Playback = InPlayback;
    StillBrush = InImage->GetBrush();
    bAlwaysPlay = bInAlwaysPlay;
    HoverDelaySeconds = FMath::Max(0.0f, InHoverDelaySeconds);
    if (InButton && !bAlwaysPlay)
    {
        InButton->OnHovered.AddUniqueDynamic(this, &ThisClass::HandleHovered);
        InButton->OnUnhovered.AddUniqueDynamic(this, &ThisClass::HandleUnhovered);
    }
    if (bAlwaysPlay)
    {
        SetPreviewActive(true);
    }
    else if (InButton && InButton->IsHovered())
    {
        HandleHovered();
    }
}

void UMapPreviewBinding::SetPreviewActive(bool bInActive)
{
    if (!bInActive)
    {
        bHoverPending = false;
        if (UWorld* World = Screen.IsValid() ? Screen->GetWorld() : nullptr)
        {
            World->GetTimerManager().ClearTimer(HoverDelayTimer);
        }
    }
    else if (UWorld* World = Screen.IsValid() ? Screen->GetWorld() : nullptr)
    {
        World->GetTimerManager().ClearTimer(HoverDelayTimer);
        bHoverPending = false;
    }

    bActive = bAlwaysPlay || bInActive;
    if (Playback)
    {
        Playback->SetActive(this, bActive);
    }
    RefreshBrush();
}

void UMapPreviewBinding::RefreshBrush()
{
    if (UImage* Target = Image.Get())
    {
        if (bActive && Playback && Playback->IsReady())
        {
            // Keep the still image's desired size, tint and card layout.
            FSlateBrush VideoBrush = StillBrush;
            VideoBrush.SetResourceObject(Playback->GetVideoMaterial());
            Target->SetBrush(VideoBrush);
        }
        else
        {
            Target->SetBrush(StillBrush);
        }
    }
}

void UMapPreviewBinding::HandleHovered()
{
    if (bAlwaysPlay || HoverDelaySeconds <= 0.0f)
    {
        SetPreviewActive(true);
        return;
    }

    UWorld* World = Screen.IsValid() ? Screen->GetWorld() : nullptr;
    if (!World)
    {
        SetPreviewActive(true);
        return;
    }

    bHoverPending = true;
    World->GetTimerManager().SetTimer(HoverDelayTimer, this,
        &ThisClass::ActivatePreviewAfterDelay, HoverDelaySeconds, false);
}

void UMapPreviewBinding::HandleUnhovered()
{
    SetPreviewActive(false);
}

void UMapPreviewBinding::ActivatePreviewAfterDelay()
{
    if (!bHoverPending)
    {
        return;
    }

    bHoverPending = false;
    if (UButton* Target = Button.Get(); Target && Target->IsHovered())
    {
        SetPreviewActive(true);
    }
}

UMediaPlayer* UMapPreviewBinding::GetMediaPlayer() const
{
    return Playback ? Playback->GetPlayer() : nullptr;
}

bool UMapPreviewBinding::Matches(UUserWidget* InScreen, UImage* InImage) const
{
    return Screen == InScreen && (!InImage || Image == InImage);
}

void UMapPreviewBinding::Release()
{
    bAlwaysPlay = false;
    SetPreviewActive(false);
    if (UButton* Target = Button.Get())
    {
        Target->OnHovered.RemoveAll(this);
        Target->OnUnhovered.RemoveAll(this);
    }
    Playback = nullptr;
    Screen.Reset();
    Image.Reset();
    Button.Reset();
}

bool UMapPreviewSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UGameInstance* Instance = Cast<UGameInstance>(Outer);
    return Super::ShouldCreateSubsystem(Outer) && Instance && !Instance->IsDedicatedServerInstance();
}

void UMapPreviewSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(
        this, &ThisClass::HandlePreLoadMap);
    WorldTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(
        this, &ThisClass::HandleWorldBeginTearDown);
}

void UMapPreviewSubsystem::HandlePreLoadMap(const FWorldContext& WorldContext, const FString& MapName)
{
    if (WorldContext.OwningGameInstance == GetGameInstance())
    {
        ReleaseAllPreviews();
    }
}

void UMapPreviewSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
    if (World && World->GetGameInstance() == GetGameInstance())
    {
        ReleaseAllPreviews();
    }
}

UMapPreviewBinding* UMapPreviewSubsystem::Configure(UUserWidget* Screen, UImage* Image,
    UButton* HoverButton, FName MapId, UDataTable* PreviewMedia,
    UMaterialInterface* VideoMaterial, bool bAlwaysPlay)
{
    if (!Screen || !Image)
    {
        return nullptr;
    }

    if (!HoverButton && !bAlwaysPlay)
    {
        for (UWidget* Parent = Image->GetParent(); Parent; Parent = Parent->GetParent())
        {
            if (UButton* ParentButton = Cast<UButton>(Parent))
            {
                HoverButton = ParentButton;
                break;
            }
        }
    }

    // RefreshMapPage may reuse a card for another map. Unbind the previous row first.
    const FSlateBrush CurrentBrush = Image->GetBrush();
    for (int32 Index = Bindings.Num() - 1; Index >= 0; --Index)
    {
        if (!Bindings[Index]->IsScreenValid() || Bindings[Index]->Matches(Screen, Image))
        {
            Bindings[Index]->Release();
            Bindings.RemoveAt(Index);
        }
    }
    Image->SetBrush(CurrentBrush);

    const FMapPreviewMediaRow* Row = PreviewMedia && PreviewMedia->GetRowStruct() == FMapPreviewMediaRow::StaticStruct()
        ? PreviewMedia->FindRow<FMapPreviewMediaRow>(MapId, TEXT("MapPreview"), false) : nullptr;
    if (Row && Row->StillImage)
    {
        Image->SetBrushFromTexture(Row->StillImage, false);
    }
    if (!Row || !Row->PreviewVideo || !VideoMaterial || MapId.IsNone())
    {
        return nullptr;
    }

    TObjectPtr<UMapPreviewPlayback>& Playback = Playbacks.FindOrAdd(MapId);
    if (Playback && Playback->GetSource() != Row->PreviewVideo)
    {
        Playback->Shutdown();
        Playback = nullptr;
    }
    if (!Playback)
    {
        Playback = NewObject<UMapPreviewPlayback>(this);
        Playback->Initialize(Row->PreviewVideo, VideoMaterial);
    }

    UMapPreviewBinding* Binding = NewObject<UMapPreviewBinding>(this);
    Bindings.Add(Binding);
    Binding->Initialize(Screen, Image, HoverButton, Playback, bAlwaysPlay,
        Row->HoverDelaySeconds);
    return Binding;
}

void UMapPreviewSubsystem::ReleaseScreen(UUserWidget* Screen)
{
    for (int32 Index = Bindings.Num() - 1; Index >= 0; --Index)
    {
        if (!Bindings[Index]->IsScreenValid() || Bindings[Index]->Matches(Screen))
        {
            Bindings[Index]->Release();
            Bindings.RemoveAt(Index);
        }
    }
}

void UMapPreviewSubsystem::ReleaseAllPreviews()
{
    for (UMapPreviewBinding* Binding : Bindings)
    {
        Binding->Release();
    }
    Bindings.Empty();
    for (const auto& Pair : Playbacks)
    {
        Pair.Value->Shutdown();
    }
    Playbacks.Empty();
}

void UMapPreviewSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
    FWorldDelegates::OnWorldBeginTearDown.Remove(WorldTearDownHandle);
    PreLoadMapHandle.Reset();
    WorldTearDownHandle.Reset();
    ReleaseAllPreviews();
    Super::Deinitialize();
}

UMapPreviewBinding* UMapPreviewLibrary::ConfigureMapPreview(UUserWidget* Screen, UImage* Image,
    UButton* HoverButton, FName MapId, UDataTable* PreviewMedia,
    UMaterialInterface* VideoMaterial, bool bAlwaysPlay)
{
    UGameInstance* Instance = Screen ? Screen->GetGameInstance() : nullptr;
    UMapPreviewSubsystem* Subsystem = Instance ? Instance->GetSubsystem<UMapPreviewSubsystem>() : nullptr;
    return Subsystem ? Subsystem->Configure(Screen, Image, HoverButton, MapId,
        PreviewMedia, VideoMaterial, bAlwaysPlay) : nullptr;
}

void UMapPreviewLibrary::ReleaseMapPreviews(UUserWidget* Screen)
{
    UGameInstance* Instance = Screen ? Screen->GetGameInstance() : nullptr;
    if (UMapPreviewSubsystem* Subsystem = Instance ? Instance->GetSubsystem<UMapPreviewSubsystem>() : nullptr)
    {
        Subsystem->ReleaseScreen(Screen);
    }
}

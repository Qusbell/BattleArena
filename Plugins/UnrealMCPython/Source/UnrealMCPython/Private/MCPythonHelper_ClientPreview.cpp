#include "MCPythonHelper.h"

#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PlayInEditorDataTypes.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

bool UMCPythonHelper::StartClientPreview(int32 Width, int32 Height)
{
    if (!GEditor || GEditor->PlayWorld || Width <= 0 || Height <= 0)
        return false;

    ULevelEditorPlaySettings* Settings = DuplicateObject(GetDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
    Settings->SetPlayNetMode(PIE_Client);
    Settings->SetRunUnderOneProcess(true);
    Settings->SetPlayNumberOfClients(1);
    Settings->NewWindowWidth = Width;
    Settings->NewWindowHeight = Height;

    TSharedRef<SWindow> Window = SNew(SWindow)
        .Title(FText::FromString(TEXT("Play As Client")))
        .ClientSize(FVector2D(Width, Height))
        .ScreenPosition(FVector2D::ZeroVector)
        .AutoCenter(EAutoCenter::None)
        .SaneWindowPlacement(false)
        .CreateTitleBar(false)
        .UseOSWindowBorder(false)
        .SizingRule(ESizingRule::FixedSize)
        .AdjustInitialSizeAndPositionForDPIScale(false);
    FSlateApplication::Get().AddWindow(Window);

    FRequestPlaySessionParams Request;
    Request.EditorPlaySettings = Settings;
    Request.CustomPIEWindow = Window;
    GEditor->RequestPlaySession(Request);
    return true;
}

FString UMCPythonHelper::CaptureClientPreview(const FString& Filename)
{
    if (!GEngine || FPaths::GetExtension(Filename) != TEXT("png"))
        return TEXT("A PNG filename and a PIE client are required.");

    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        UWorld* World = Context.World();
        if (Context.WorldType != EWorldType::PIE || !World || World->GetNetMode() != NM_Client)
            continue;
        UGameViewportClient* GameViewport = World->GetGameViewport();
        const TSharedPtr<SViewport> Widget = GameViewport ? GameViewport->GetGameViewportWidget() : nullptr;
        if (!Widget)
            continue;

        TArray<FColor> Pixels;
        FIntVector Size;
        if (!FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(), Pixels, Size))
            return TEXT("Could not read the client viewport.");
        if (Pixels.Num() != Size.X * Size.Y)
            return TEXT("Unexpected screenshot buffer size.");

        for (FColor& Pixel : Pixels)
            Pixel.A = 255;
        TArray64<uint8> Encoded;
        FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, Encoded);
        if (!FFileHelper::SaveArrayToFile(Encoded, *Filename))
            return TEXT("Could not write the screenshot.");
        return FString::Printf(TEXT("%dx%d"), Size.X, Size.Y);
    }

    return TEXT("No PIE client viewport found.");
}


#pragma once

#include "CoreMinimal.h"
#include "CommonGameViewportClient.h"
#include "ArenaGameViewportClient.generated.h"

class UBuffDurationRingSettings;
class FCanvas;

/** Draws engine sound subtitles with the independent caption fields in the buff HUD DA. */
UCLASS(Config = Engine)
class SHOOTINGARENA_API UArenaGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()

public:
	UArenaGameViewportClient();
	virtual void Draw(FViewport* InViewport, FCanvas* SceneCanvas) override;
	virtual void GetSubtitleRegion(FVector2D& MinPos, FVector2D& MaxPos) override;

	UPROPERTY(EditAnywhere, Category = "Subtitles")
	TObjectPtr<UBuffDurationRingSettings> SubtitleSettings;

	/** Normalized screen height; smaller values move the shared engine subtitles upwards. */
	UPROPERTY(Config, EditAnywhere, Category = "Subtitles", meta = (ClampMin = "0.1", ClampMax = "0.9"))
	float SubtitleBottom = 0.7f;

private:
	friend class FArenaSubtitleSettingsTest;
	void DrawSubtitleText(const FText& Text);
	FCanvas* SubtitleCanvas = nullptr;
	FText CachedSubtitleText;
	TWeakObjectPtr<UFont> CachedSubtitleFont;
	float CachedSubtitleScale = -1.0f;
	int32 CachedSubtitleWidth = 0;
	TArray<FString> WrappedSubtitleLines;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "KillMedalUISettings.generated.h"

class UTexture2D;

/** Designer settings for the consecutive-kill medal display. Positions use DPI-scaled UI units. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UKillMedalUISettings : public UDataAsset
{
	GENERATED_BODY()
public:
	UKillMedalUISettings();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|이미지", meta = (DisplayName = "전환 기준 미만 누적 메달 이미지"))
	TObjectPtr<UTexture2D> MedalTexture;

	/** At this kill count and above, replace the individual medals with one large medal and a kill count. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|단계", meta = (DisplayName = "큰 메달 전환 킬 수 (이상)", ClampMin = "1"))
	int32 LargeMedalMinKills = 11;

	/** Falls back to MedalTexture when left empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|이미지", meta = (DisplayName = "전환 기준 이상 메달 이미지"))
	TObjectPtr<UTexture2D> LargeMedalTexture;

	/** Normalized screen position: (0,0) top left, (0.5,0.5) centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|위치", meta = (DisplayName = "화면 기준 위치", ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D MedalAnchor = FVector2D(0.5, 0.15);

	/** Offset from the anchor; +X right, +Y down. The medal row is horizontally centred here. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|위치", meta = (DisplayName = "메달 위치 보정"))
	FVector2D MedalPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|크기", meta = (DisplayName = "첫 메달 크기", ClampMin = "1.0"))
	FVector2D FirstMedalSize = FVector2D(80, 80);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|크기", meta = (DisplayName = "추가 메달 크기", ClampMin = "1.0"))
	FVector2D AdditionalMedalSize = FVector2D(40, 40);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|크기", meta = (DisplayName = "전환 기준 이상 큰 메달 크기", ClampMin = "1.0"))
	FVector2D LargeMedalSize = FVector2D(96, 96);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|위치", meta = (DisplayName = "메달 사이 간격", ClampMin = "0.0"))
	float MedalGap = 4.0f;

	/** Restarted by each credited kill; the streak itself is retained when the UI hides. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|효과", meta = (DisplayName = "메달 표시 시간", ClampMin = "0.01", Units = "s"))
	float DisplayDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|효과", meta = (DisplayName = "반짝임 불투명도", ClampMin = "0.0", ClampMax = "1.0"))
	float HighlightOpacity = 0.25f;

	/** Expand this field to edit the font family, typeface, size and outline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "문구", meta = (DisplayName = "킬 수 문구 폰트 및 크기"))
	FSlateFontInfo CountFont;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "문구", meta = (DisplayName = "킬 수 숫자 색상"))
	FLinearColor CountColor = FLinearColor::FromSRGBColor(FColor(162, 123, 0));

	/** Offset from the centre above the medal row; +X right, +Y down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "문구", meta = (DisplayName = "킬 수 문구 위치 보정"))
	FVector2D CountPosition = FVector2D::ZeroVector;
};

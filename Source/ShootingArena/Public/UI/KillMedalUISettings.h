#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "KillMedalUISettings.generated.h"

class UTexture2D;

/** Designer settings for the consecutive-kill medals and personal kill message. */
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|배경", meta = (DisplayName = "메달 배경 색상", HideAlphaChannel))
	FLinearColor MedalBackgroundColor = FLinearColor(0.012f, 0.016f, 0.024f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|배경", meta = (DisplayName = "메달 배경 불투명도", ClampMin = "0.0", ClampMax = "1.0"))
	float MedalBackgroundOpacity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "You Killed|배경", meta = (DisplayName = "처치 알림 배경 색상", HideAlphaChannel))
	FLinearColor KillMessageBackgroundColor = FLinearColor(0.02f, 0.025f, 0.035f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "You Killed|배경", meta = (DisplayName = "처치 알림 배경 불투명도", ClampMin = "0.0", ClampMax = "1.0"))
	float KillMessageBackgroundOpacity = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "You Killed|그림자", meta = (DisplayName = "글자 그림자 색상 및 알파"))
	FLinearColor KillMessageShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "You Killed|그림자", meta = (DisplayName = "글자 그림자 거리 배율", ToolTip = "0은 그림자 끄기, 1은 기존 그림자 거리입니다.", ClampMin = "0.0", ClampMax = "2.0"))
	float KillMessageShadowScale = 0.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|도장 애니메이션", meta = (DisplayName = "등장 시작 크기 배율", ToolTip = "최종 메달 크기에 곱하는 배율입니다. 2는 2배 크기이며, 높일수록 등장 동작이 커집니다.", ClampMin = "0.01"))
	float StampStartScale = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|도장 애니메이션", meta = (DisplayName = "착지까지 걸리는 시간", ToolTip = "등장 크기에서 착지 크기까지 줄어드는 시간입니다. 짧을수록 빠르게 찍힙니다.", ClampMin = "0.01", Units = "s"))
	float StampImpactTime = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|도장 애니메이션", meta = (DisplayName = "착지 순간 압축 배율", ToolTip = "1은 압축 없음, 0.8은 최종 크기의 80%입니다. 작을수록 착지 후 반동이 커집니다.", ClampMin = "0.01", ClampMax = "1.0"))
	float StampImpactScale = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "메달|도장 애니메이션", meta = (DisplayName = "착지 후 복원 시간", ToolTip = "착지 크기에서 최종 크기로 돌아오는 시간입니다. 짧을수록 반동이 빠릅니다.", ClampMin = "0.01", Units = "s"))
	float StampSettleTime = 0.08f;

	/** Expand this field to edit the font family, typeface, size and outline. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "문구", meta = (DisplayName = "킬 수 문구 폰트 및 크기"))
	FSlateFontInfo CountFont;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "문구", meta = (DisplayName = "킬 수 숫자 색상"))
	FLinearColor CountColor = FLinearColor::FromSRGBColor(FColor(162, 123, 0));

	/** Offset from the centre above the medal row; +X right, +Y down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "문구", meta = (DisplayName = "킬 수 문구 위치 보정"))
	FVector2D CountPosition = FVector2D::ZeroVector;
};

#pragma once

#include "Engine/DataAsset.h"
#include "Fonts/SlateFontInfo.h"
#include "MatchTimerStyle.generated.h"

/** 경기 화면 상단 타이머의 분, 콜론, 초에 적용하는 글꼴 설정입니다. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UMatchTimerStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer", meta = (
		DisplayName = "타이머 글꼴",
		ToolTip = "경기 화면 상단 타이머의 분, 콜론, 초에 적용합니다. Font Family에서 글꼴을, Typeface에서 굵기를, Size에서 글자 크기를 설정합니다. 기본 크기 45는 HP 숫자와 같습니다."))
	FSlateFontInfo Font = FSlateFontInfo(static_cast<const UObject*>(nullptr), 45.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timer", meta = (
		DisplayName = "순위 패널 간격", ClampMin = "0", UIMin = "0", UIMax = "40",
		ToolTip = "타이머 오른쪽 끝과 순위 패널 사이의 가로 간격입니다. UMG UI 단위이며, 타이머와 순위 패널 묶음은 화면 가운데를 유지합니다."))
	float RankingGap = 5.0f;
};

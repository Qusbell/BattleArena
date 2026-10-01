#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerDefaultNameSettings.generated.h"

/** 닉네임을 정하지 않은 플레이어에게 부여할 기본 닉네임 설정 에셋입니다. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UPlayerDefaultNameSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 기본 닉네임 접두어. 예: "Player" -> Player1, Player2 ... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname")
	FString DefaultNamePrefix = TEXT("Player");

	/** 멀티플레이: true면 접두어 뒤에 (현재 접속자와 겹치지 않는 가장 작은) 번호를 붙입니다. false면 접두어만 사용합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname")
	bool bAppendNumberInMultiplayer = true;

	/** 캠페인(싱글): true면 번호를 붙입니다. 기본값 false = 접두어만 사용. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname")
	bool bAppendNumberInCampaign = false;
};

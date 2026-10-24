#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "UIBackInputSettings.generated.h"

/**
 * 메뉴 "뒤로가기/취소/창 닫기" 공용 입력 설정. Project Settings > Game > UI Back Input.
 * 에디터에서는 Esc가 PIE를 종료시키므로 임시로 F10을 쓰고, 패키징 후에는 BackKeys에 Escape만 남기면 됩니다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "UI Back Input"))
class SHOOTINGARENA_API UUIBackInputSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUIBackInputSettings();

	/** 뒤로가기로 취급할 키들. */
	UPROPERTY(Config, EditAnywhere, Category = "Back")
	TArray<FKey> BackKeys;

	/**
	 * 활성 화면 안에서 "클릭해서 뒤로 가는" 버튼의 위젯 이름들(앞쪽이 우선).
	 * 새 메뉴는 이 이름 중 하나로 버튼을 만들면 코드/BP 수정 없이 키가 적용됩니다.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Back")
	TArray<FName> BackButtonNames;
};

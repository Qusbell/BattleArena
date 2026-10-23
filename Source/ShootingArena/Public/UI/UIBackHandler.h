#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "UIBackHandler.generated.h"

UINTERFACE(BlueprintType)
class SHOOTINGARENA_API UUIBackHandler : public UInterface
{
	GENERATED_BODY()
};

/**
 * 뒤로가기 키를 위젯이 직접 처리하고 싶을 때 구현합니다(예: 저장 확인창을 띄우고 닫기를 막기).
 * 구현하지 않아도 Btn_Back/Btn_Cancel/Btn_Close 버튼이 있으면 UUIBackInputSubsystem이 그 버튼을 눌러 줍니다.
 */
class SHOOTINGARENA_API IUIBackHandler
{
	GENERATED_BODY()

public:
	/** 처리했으면 true. false면 기본 처리(버튼 클릭 → 비활성화)로 넘어갑니다. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "UI")
	bool HandleBackAction();
	virtual bool HandleBackAction_Implementation() { return false; }
};

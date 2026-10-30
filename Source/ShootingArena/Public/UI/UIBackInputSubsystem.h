#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UIBackInputSubsystem.generated.h"

class UUserWidget;
class FUIBackInputProcessor;

/**
 * 모든 메뉴 공용 "뒤로가기 / 취소 / 창 닫기" 키 처리.
 * Slate 입력 프리프로세서로 키를 받으므로 위젯 포커스와 상관없이 동작합니다.
 *
 * 처리 순서(위에 떠 있는 것부터):
 *  1. 뷰포트에 직접 붙은 팝업 위젯(CommonUI 스택이 없는 최상위 위젯, 예: WBP_MapPickPop)
 *  2. CommonUI 스택의 활성 화면 (이름에 "Modal"이 든 스택 → 그 외 스택. 일반 스택은 2개 이상 쌓였을 때만)
 * 각 위젯은 IUIBackHandler → BackButtonNames 버튼 클릭 → (스택이면) DeactivateWidget 순으로 시도합니다.
 */
UCLASS()
class SHOOTINGARENA_API UUIBackInputSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 현재 가장 위에 있는 창을 닫거나 뒤로 갑니다. 처리했으면 true. BP에서 직접 호출해도 됩니다. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool HandleBackAction();

	/** 프리프로세서가 호출합니다. 설정된 뒤로가기 키이고 처리했으면 true(= 입력 소비). */
	bool HandleKeyPressed(const FKey& Key);

private:
	bool TryHandleWidget(UUserWidget* Widget) const;
	bool TryClickBackButton(UUserWidget* Widget) const;
	bool TryClickButtonByName(UUserWidget* Widget, const FName& ButtonName) const;
	bool HandleHotkey(const FKey& Key);

	TSharedPtr<FUIBackInputProcessor> InputProcessor;
};

#include "UI/UIBackInputSettings.h"

UUIBackInputSettings::UUIBackInputSettings()
{
	CategoryName = TEXT("Game");

	BackKeys = { EKeys::F10 };
	BackButtonNames = { TEXT("Btn_Back"), TEXT("Btn_Cancel"), TEXT("Btn_Close"), TEXT("Btn_Leave") };

	// 멀티 로비: Q/E 맵 변경, Enter 게임 시작(방장) / 준비(게스트). Btn_Main이 상태에 따라 시작/준비를 구분합니다.
	HotkeyButtons = {
		{ EKeys::Q, TEXT("Btn_PrevMap") },
		{ EKeys::E, TEXT("Btn_NextMap") },
		{ EKeys::Enter, TEXT("Btn_Main") },
	};
}

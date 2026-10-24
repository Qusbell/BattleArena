#include "UI/UIBackInputSettings.h"

UUIBackInputSettings::UUIBackInputSettings()
{
	CategoryName = TEXT("Game");

	BackKeys = { EKeys::F10 };
	BackButtonNames = { TEXT("Btn_Back"), TEXT("Btn_Cancel"), TEXT("Btn_Close") };
}

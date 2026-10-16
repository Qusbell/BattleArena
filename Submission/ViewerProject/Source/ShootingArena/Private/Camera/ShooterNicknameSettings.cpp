#include "Camera/ShooterNicknameSettings.h"
#include "Styling/CoreStyle.h"

FShooterNicknameStyle::FShooterNicknameStyle()
{
	Font = FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 20);
	Font.OutlineSettings.OutlineSize = 2;
	Font.OutlineSettings.OutlineColor = FLinearColor::Black;
}

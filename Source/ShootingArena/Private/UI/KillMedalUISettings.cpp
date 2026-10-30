#include "UI/KillMedalUISettings.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

UKillMedalUISettings::UKillMedalUISettings()
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> Medal(
		TEXT("/Game/QuakeLike_1_0/UI/PlayerUI/KillMedals/T_KillMedal_FirstBlood.T_KillMedal_FirstBlood"));
	static ConstructorHelpers::FObjectFinder<UFont> Font(
		TEXT("/Game/QuakeLike_Base/SDM/F_BITCORE1.F_BITCORE1"));
	MedalTexture = Medal.Object;
	CountFont = FSlateFontInfo(Font.Object, 24, FName(TEXT("Default")));
}

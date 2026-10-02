#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/EngineTypes.h"
#include "Fonts/SlateFontInfo.h"
#include "ShooterNicknameSettings.generated.h"

class APawn;

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FShooterNicknameStyle
{
	GENERATED_BODY()
	FShooterNicknameStyle();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	FLinearColor Color = FLinearColor::White;
	/** Font, size, and outline size/colour are independently editable in this font structure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	FSlateFontInfo Font;
	/** DPI-scaled offset from the actual crosshair; positive Y is below it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	FVector2D Offset = FVector2D(0.0, 28.0);
	/** Seconds to retain the last name after aim leaves it; zero hides immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname", meta = (ClampMin = "0.0", Units = "s"))
	float LingerTime = 0.0f;
};

UCLASS(BlueprintType)
class SHOOTINGARENA_API UShooterNicknameSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname")
	FShooterNicknameStyle Style;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting")
	TSoftClassPtr<APawn> ShooterClass;
	/** Match the equipped weapon's existing trace channel and range when weapon data is available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting")
	bool bUseEquippedWeaponTraceChannel = true;
	/** Fallback damage query channel when no equipped weapon data is available, or automatic selection is disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting", meta = (ClampMin = "1.0", Units = "cm"))
	float MaxDistance = 20000.0f;
	/** Coordinates within the camera's constrained view rectangle, usually its centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting")
	FVector2D CrosshairUV = FVector2D(0.5, 0.5);
};

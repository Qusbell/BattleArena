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
	/** False: below the crosshair. True: above the crosshair. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname", meta = (DisplayName = "Show Above Crosshair"))
	bool bShowAboveCrosshair = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	FLinearColor RevengeColor = FLinearColor::Red;
	/** Font, size, and outline size/colour are independently editable in this font structure. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nickname")
	FSlateFontInfo Font;
	/** DPI-scaled offset. X moves horizontally; absolute Y is the gap above or below the crosshair. */
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
	/** Retained for older assets only. Weapon settings no longer affect nickname targeting. */
	UPROPERTY(BlueprintReadOnly, Category = "Nickname|Targeting", meta = (DeprecatedProperty, DeprecationMessage = "Nickname targeting always uses its own TraceChannel and MaxDistance."))
	bool bUseEquippedWeaponTraceChannel = true;
	/** Shared nickname query channel for every weapon. Must be detected by the character's existing collision. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting", meta = (DisplayName = "Nickname Trace Channel"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_GameTraceChannel2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting", meta = (ClampMin = "1.0", Units = "cm"))
	float MaxDistance = 20000.0f;
	/** Search nearby damage colliders only when the crosshair line finds no valid shooter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting")
	bool bUseSphereTraceFallback = true;
	/** Nickname-only search radius. Zero uses the original line query alone; damage collision is unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting", meta = (ClampMin = "0.0", ClampMax = "1000.0", Units = "cm"))
	float SphereTraceRadius = 15.0f;
	/** Coordinates within the camera's constrained view rectangle, usually its centre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Targeting")
	FVector2D CrosshairUV = FVector2D(0.5, 0.5);
	/** Compare periodic timer searches against the original per-frame targeting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Performance")
	bool bUseTimerBasedSearch = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nickname|Performance", meta = (ClampMin = "0.01", ClampMax = "1.0", Units = "s", EditCondition = "bUseTimerBasedSearch"))
	float TargetSearchInterval = 0.05f;
};

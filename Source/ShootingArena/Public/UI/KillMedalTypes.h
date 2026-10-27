#pragma once

#include "CoreMinimal.h"
#include "KillMedalTypes.generated.h"

class UUserWidget;

UENUM(BlueprintType)
enum class EKillMedalType : uint8
{
	None,
	FirstBlood,
	DoubleKill,
	TripleKill,
	Rampage,
	RevengeShot
};

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FKillMedalStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal")
	TSubclassOf<UUserWidget> MedalWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal|Layout")
	FVector2D PositionOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal|Layout", meta = (ClampMin = "0.1", UIMin = "0.1"))
	FVector2D SizeScale = FVector2D::UnitVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal")
	FText Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kill Medal")
	FLinearColor HighlightColor = FLinearColor::White;
};

USTRUCT()
struct FKillMedalRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FKillMedalStyle Style;

	UPROPERTY()
	double DisplayTime = 1.2;
};

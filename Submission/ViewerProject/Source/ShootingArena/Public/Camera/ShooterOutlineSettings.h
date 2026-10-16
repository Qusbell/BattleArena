#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShooterOutlineSettings.generated.h"

class UMaterialInterface;
class APawn;

USTRUCT(BlueprintType)
struct SHOOTINGARENA_API FShooterOutlineStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	FLinearColor Color = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline")
	FLinearColor RevengeColor = FLinearColor::Red;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline", meta = (ClampMin = "0.0", ClampMax = "8.0"))
	float Thickness = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Outline", meta = (ClampMin = "0.0", ClampMax = "5.0", Units = "cm"))
	float DepthTolerance = 0.5f;
};

/** Designer defaults; runtime preferences are copied, never written back to this asset. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UShooterOutlineSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
	FShooterOutlineStyle Style;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Setup")
	TObjectPtr<UMaterialInterface> Material;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Setup")
	TSoftClassPtr<APawn> ShooterClass;
	/** Dedicated gameplay stencil. Value 1 remains reserved for DeathCam. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Setup", meta = (ClampMin = "2", ClampMax = "255"))
	int32 StencilValue = 2;
	/** Separate revenge group; must differ from the normal group and DeathCam value 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Setup", meta = (ClampMin = "2", ClampMax = "255"))
	int32 RevengeStencilValue = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline|Setup", meta = (ClampMin = "0.05", Units = "s"))
	float RefreshInterval = 0.2f;
};

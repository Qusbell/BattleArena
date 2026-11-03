#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BuffWeaponGlow.generated.h"

class UMaterialInterface;

/** Weapon (and third-person body) overlay glow while any buff is active. Colours come from DA_BuffScreenEffect. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UBuffWeaponGlowSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "효과 사용"))
	bool bEnabled = true;

	/** Translucent overlay with Color1/Color2/TintStrength/RimStrength/RimPower/PulseMinimum/PulsePeriod parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "오버레이 머티리얼"))
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	/** The owner's body is Owner No See, so this only shows to other players; first person still shows just the gun. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "3인칭 캐릭터 몸도 발광"))
	bool bGlowCharacter = true;

	/** Kept low so the character's own mesh stays readable under the rim glow. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "몸 내부 틴트 세기", ClampMin = "0.0", EditCondition = "bGlowCharacter"))
	float BodyTintStrength = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "전체 틴트 세기", ClampMin = "0.0"))
	float TintStrength = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "테두리 발광 세기", ClampMin = "0.0"))
	float RimStrength = 1.5f;

	/** Higher values keep the rim glow thinner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "테두리 두께 (클수록 얇음)", ClampMin = "0.1"))
	float RimPower = 3.0f;

	/** 1 keeps a constant brightness; lower values give a slow pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "맥동 최소 밝기 비율", ClampMin = "0.0", ClampMax = "1.0"))
	float PulseMinimum = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "맥동 한 주기", ClampMin = "0.1", Units = "s"))
	float PulsePeriod = 2.5f;

	/** Caps the brightest channel at the pulse and wave crest; the hue is kept. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "총기 발광", meta = (DisplayName = "최고 밝기", ClampMin = "0.0"))
	float MaxBrightness = 1.5f;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "BuffWeaponGlow.generated.h"

class UMaterialInterface;

/** One overlay look (gun or body). Values feed M_BuffGlowOverlay-style material parameters. */
USTRUCT(BlueprintType)
struct FBuffGlowLook
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "사용"))
	bool bEnabled = true;

	/** Additive overlay with Color1/Color2/Tint/Rim/Pulse/Shimmer/Wave parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "오버레이 머티리얼"))
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	/** Off: use Color1/Color2 below instead of DA_BuffScreenEffect's colours. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|색", meta = (DisplayName = "화면 효과 색 사용"))
	bool bUseScreenEffectColors = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|색", meta = (DisplayName = "색 1", EditCondition = "!bUseScreenEffectColors"))
	FLinearColor Color1 = FLinearColor::FromSRGBColor(FColor(35, 100, 255));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|색", meta = (DisplayName = "색 2", EditCondition = "!bUseScreenEffectColors"))
	FLinearColor Color2 = FLinearColor::FromSRGBColor(FColor(30, 190, 255));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "내부 틴트 세기", ClampMin = "0.0"))
	float TintStrength = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "테두리 발광 세기", ClampMin = "0.0"))
	float RimStrength = 1.5f;

	/** Higher values keep the rim glow thinner. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "테두리 두께 (클수록 얇음)", ClampMin = "0.1"))
	float RimPower = 3.0f;

	/** 1 keeps a constant brightness; lower values give a slow pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|맥동", meta = (DisplayName = "맥동 최소 밝기 비율", ClampMin = "0.0", ClampMax = "1.0"))
	float PulseMinimum = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|맥동", meta = (DisplayName = "맥동 한 주기", ClampMin = "0.1", Units = "s"))
	float PulsePeriod = 2.5f;

	/** Caps the brightest channel at the pulse and wave crest; the hue is kept. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "최고 밝기", ClampMin = "0.0"))
	float MaxBrightness = 1.5f;

	/** 0 turns the sweeping wave off and keeps just the pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|웨이브", meta = (DisplayName = "웨이브 세기 (0이면 꺼짐)", ClampMin = "0.0"))
	float WaveStrength = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|웨이브", meta = (DisplayName = "웨이브 속도", ClampMin = "0.0"))
	float WaveSpeed = 2.0f;

	/** Bands per cm along the sweep direction; 0.06 is roughly one band per metre. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|웨이브", meta = (DisplayName = "웨이브 간격 (클수록 촘촘)", ClampMin = "0.0"))
	float WaveFrequency = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|웨이브", meta = (DisplayName = "웨이브 띠 폭 (클수록 좁음)", ClampMin = "0.1"))
	float WaveWidth = 3.0f;

	/** How far the band's colour leans from Color1 toward Color2; 0 keeps one colour and the wave reads as brightness only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|웨이브", meta = (DisplayName = "웨이브 색 변화 (0이면 같은 색)", ClampMin = "0.0", ClampMax = "1.0"))
	float WaveColorShift = 0.1f;

	/** Mesh-local direction the band travels along. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광|웨이브", meta = (DisplayName = "웨이브 방향 (메시 로컬)"))
	FVector WaveDirection = FVector(1.0, 0.0, 1.0);
};

/** Gun and body looks for the buffs matching BuffTag. */
USTRUCT(BlueprintType)
struct FBuffGlowProfile
{
	GENERATED_BODY()

	/** Matches child tags too (Buff.Atk covers Buff.Atk.*); empty matches any buff. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "버프 태그"))
	FGameplayTag BuffTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "총 (1인칭·3인칭)"))
	FBuffGlowLook Gun;

	/** The owner's body is Owner No See, so this only shows to other players. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "발광", meta = (DisplayName = "몸 (3인칭)"))
	FBuffGlowLook Body;
};

/** Gun and third-person body overlay glow per buff. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UBuffWeaponGlowSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 발광", meta = (DisplayName = "효과 사용"))
	bool bEnabled = true;

	/** First matching profile wins, so list specific tags before an empty catch-all. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 발광", meta = (DisplayName = "버프별 설정", TitleProperty = "BuffTag"))
	TArray<FBuffGlowProfile> Profiles;

	const FBuffGlowProfile* FindProfile(const FGameplayTag& Tag) const
	{
		return Profiles.FindByPredicate([&Tag](const FBuffGlowProfile& Profile)
		{
			return !Profile.BuffTag.IsValid() || Tag.MatchesTag(Profile.BuffTag);
		});
	}
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "BuffScreenEffect.generated.h"

class UBuffPresentationComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType)
class SHOOTINGARENA_API UBuffScreenEffectSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	/** DPI-scaled offset from bottom centre: +X right, +Y down. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 아이콘·시간", meta = (DisplayName = "묶음 위치 (하단 중앙 기준)", ToolTip = "X: 좌우 이동, Y: 상하 이동. Y가 음수이면 화면 아래에서 위로 올라갑니다."))
	FVector2D StatusPosition = FVector2D(0.0, -60.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 아이콘·시간", meta = (DisplayName = "시간과 아이콘 사이 간격", ClampMin = "0.0"))
	float StatusGap = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "효과 사용"))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "가장자리 색상 1"))
	FLinearColor Color1 = FLinearColor::FromSRGBColor(FColor(35, 100, 255));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "가장자리 색상 2"))
	FLinearColor Color2 = FLinearColor::FromSRGBColor(FColor(30, 190, 255));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "전체 강도", ClampMin = "0.0", ClampMax = "1.0"))
	float Opacity = 0.15f;

	/** 1 keeps a constant brightness; lower values give a soft pulse. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "맥동 최소 밝기 비율", ClampMin = "0.0", ClampMax = "1.0"))
	float PulseMinimum = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "맥동 한 주기", ClampMin = "0.1", Units = "s"))
	float PulsePeriod = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "나타나는 시간", ClampMin = "0.0", Units = "s"))
	float FadeInSeconds = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과", meta = (DisplayName = "사라지는 시간", ClampMin = "0.0", Units = "s"))
	float FadeOutSeconds = 0.4f;

	/** UI material with Color1/Color2/Intensity/Burst parameters; empty keeps the gradient only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과|전기", meta = (DisplayName = "전기 머티리얼"))
	TObjectPtr<UMaterialInterface> ElectricMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과|전기", meta = (DisplayName = "평상시 지직거림 세기", ClampMin = "0.0"))
	float ElectricIntensity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과|전기", meta = (DisplayName = "번개 빈도", ClampMin = "0.1", Units = "Hz"))
	float ElectricFrequency = 12.0f;

	/** Multiplier for how far the electric effect reaches inward from the screen edges. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과|전기", meta = (DisplayName = "번개 크기 비율", ClampMin = "0.1", ClampMax = "2.0"))
	float ElectricCoverage = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과|전기", meta = (DisplayName = "획득 순간 세기", ClampMin = "0.0"))
	float BurstIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "버프 화면 효과|전기", meta = (DisplayName = "번쩍임 지속 시간", ClampMin = "0.0", Units = "s"))
	float BurstSeconds = 0.5f;

	/** Hard reference to CompShooter's existing effect, assigned in DA_BuffScreenEffect. */
	UPROPERTY(EditDefaultsOnly, Category = "연결", meta = (DisplayName = "CompShooter 효과 위젯"))
	TSubclassOf<UUserWidget> EffectClass;
};

/** Owns a separate CompShooter instance so buff tint never overwrites damage feedback. */
UCLASS()
class SHOOTINGARENA_API UBuffScreenEffectWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	bool InitializeEffect(UBuffPresentationComponent* Buff, const UBuffScreenEffectSettings* InSettings);
	bool HasFinished() const { return bFinished; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<const UBuffScreenEffectSettings> Settings;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Effect;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ElectricMID;

	TWeakObjectPtr<UBuffPresentationComponent> Source;
	float Fade = 0.0f;
	float BurstTime = 0.0f;
	float AppliedBurst = -1.0f;
	double LastRemaining = 0.0;
	bool bFinished = false;

	friend class FBuffScreenEffectTest;
};

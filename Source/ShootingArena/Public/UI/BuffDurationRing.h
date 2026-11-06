#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/DataAsset.h"
#include "BuffDurationRing.generated.h"

class UImage;
class UTextBlock;
class USizeBox;
class UTexture2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/** Independent of DA_BuffScreenEffect: these settings affect only the HUD buff indicator. */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UBuffDurationRingSettings : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Hides caption text only; sound playback and subtitle timing are unchanged. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "기본 자막", meta = (DisplayName = "기본 대사 자막 표시", ToolTip = "체크를 끄면 기본 사운드 자막의 글자만 숨깁니다. 음성, 효과음 및 버프 동작은 그대로 유지됩니다."))
	bool bShowSubtitles = true;

	/** Shared engine sound captions, including buff pickup dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "기본 자막", meta = (DisplayName = "자막 위치 비율 (X=좌우, Y=높이)", ToolTip = "화면 좌상단=0,0 / 우하단=1,1. X=0.5는 중앙, Y가 작을수록 위로 이동합니다.", ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D SubtitlePosition = FVector2D(0.5, 0.7);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "기본 자막", meta = (DisplayName = "자막 글자 크기 배율", ToolTip = "1=기존 크기, 1.5=150%, 0.75=75%. 기본 자막 전체에 공통 적용됩니다.", ClampMin = "0.25", ClampMax = "4.0"))
	float SubtitleTextScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "위치·크기", meta = (DisplayName = "위치 (하단 중앙 기준)"))
	FVector2D Position = FVector2D(0.0, -60.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "위치·크기", meta = (DisplayName = "원형 게이지 지름", ClampMin = "16.0", ClampMax = "1024.0"))
	float Diameter = 212.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "링 표시"))
	bool bShowRing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "링 두께 (1에 가까울수록 얇음)", ClampMin = "1.001", ClampMax = "2.0"))
	float RingSize = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "링 패턴 반복 수", ClampMin = "1.0", ClampMax = "1000.0"))
	float RingSteps = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "시작 각도 (-90 = 12시)"))
	float StartAngle = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "쿼드 색상"))
	FLinearColor QuadColor = FLinearColor(0.2f, 0.65f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "프로텍션 색상"))
	FLinearColor ProtectionColor = FLinearColor(0.4f, 1.0f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지", meta = (DisplayName = "소진된 구간 색상·불투명도"))
	FLinearColor EmptyColor = FLinearColor(0.15f, 0.2f, 0.25f, 0.28f);

	/** Display-only highlight on the moving end of the remaining arc. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지|끝부분 발광", meta = (DisplayName = "게이지 끝부분 발광 사용", ToolTip = "체크를 끄면 기존 5분할 링으로 표시합니다. 숫자, 아이콘 및 버프 기능에는 영향을 주지 않습니다."))
	bool bFillEdgeGlowEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지|끝부분 발광", meta = (DisplayName = "끝부분 발광 밝기", EditCondition = "bFillEdgeGlowEnabled", ClampMin = "0.0", ClampMax = "1.0"))
	float FillEdgeGlowStrength = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "원형 게이지|끝부분 발광", meta = (DisplayName = "끝부분 발광 폭 (px)", EditCondition = "bFillEdgeGlowEnabled", ClampMin = "1.0", ClampMax = "32.0"))
	float FillEdgeGlowWidth = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 표시"))
	bool bShowIcon = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 텍스처"))
	TObjectPtr<UTexture2D> IconTexture;

	/** Independent icon tint. A white source texture reproduces this colour without a baked-in hue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 색상", ToolTip = "원형 링 색상과 독립적입니다. 색상의 알파는 아이콘 불투명도에 곱해집니다."))
	FLinearColor IconColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 크기", ClampMin = "1.0"))
	FVector2D IconSize = FVector2D(180.0, 180.0);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 위치 (중앙 기준)"))
	FVector2D IconOffset = FVector2D::ZeroVector;

	/** Alpha of the icon pixels only, independent of the ring, numbers and screen effects. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 불투명도 (0=투명, 1=불투명)", ClampMin = "0.0", ClampMax = "1.0"))
	float IconOpacity = 0.22f;

	/** Gaussian sampling radius in icon UI pixels, independent of texture resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 아이콘", meta = (DisplayName = "아이콘 블러 강도 (0=선명)", ClampMin = "0.0", ClampMax = "12.0"))
	float IconBlurStrength = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "아이콘 반짝임", meta = (DisplayName = "반짝임 사용"))
	bool bIconFlashEnabled = true;

	/** Additional icon brightness at the peak; 0 keeps its normal brightness. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "아이콘 반짝임", meta = (DisplayName = "반짝임 강도", EditCondition = "bIconFlashEnabled", ClampMin = "0.0", ClampMax = "4.0"))
	float IconFlashStrength = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "아이콘 반짝임", meta = (DisplayName = "반짝임 간격 (초)", EditCondition = "bIconFlashEnabled", ClampMin = "0.05", ClampMax = "60.0", Units = "s"))
	float IconFlashInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "아이콘 반짝임", meta = (DisplayName = "반짝임 지속시간 (초)", EditCondition = "bIconFlashEnabled", ClampMin = "0.01", ClampMax = "2.0", Units = "s"))
	float IconFlashDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 숫자", meta = (DisplayName = "남은 시간 표시"))
	bool bShowTime = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 숫자", meta = (DisplayName = "숫자 글자 크기", ClampMin = "8", ClampMax = "128"))
	int32 TimeFontSize = 44;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 숫자", meta = (DisplayName = "숫자 위치 (중앙 기준)"))
	FVector2D TimeOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 숫자", meta = (DisplayName = "숫자 색상"))
	FLinearColor TimeColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 숫자", meta = (DisplayName = "초 단위 s 표시"))
	bool bShowSecondsSuffix = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "가운데 숫자", meta = (DisplayName = "숫자 외곽선 두께", ClampMin = "0", ClampMax = "8"))
	int32 TimeOutlineSize = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "숫자 종료 경고", meta = (DisplayName = "남은 시간 숫자 깜빡임 사용"))
	bool bTimeWarningFlashEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "숫자 종료 경고", meta = (DisplayName = "경고 시작 잔여시간 (초)", EditCondition = "bTimeWarningFlashEnabled", ClampMin = "0.1", ClampMax = "60.0", Units = "s"))
	float TimeWarningStartSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "숫자 종료 경고", meta = (DisplayName = "숫자 경고 색상", EditCondition = "bTimeWarningFlashEnabled"))
	FLinearColor TimeWarningColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "숫자 종료 경고", meta = (DisplayName = "숫자 깜빡임 주기 (초)", ToolTip = "한 주기의 절반은 경고 색상, 나머지는 기본 숫자 색상으로 표시합니다.", EditCondition = "bTimeWarningFlashEnabled", ClampMin = "0.05", ClampMax = "10.0", Units = "s"))
	float TimeWarningFlashInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "연결", meta = (DisplayName = "구매 HUD 원형 머티리얼"))
	TObjectPtr<UMaterialInterface> RingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "연결", meta = (DisplayName = "아이콘 전용 블러 머티리얼"))
	TObjectPtr<UMaterialInterface> IconMaterial;
};

/** Display-only derivative of CompShooter's CircleImg; driven by the existing HUD tick. */
UCLASS(meta = (DisableNativeTick))
class SHOOTINGARENA_API UBuffDurationRingWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Buff|HUD", meta = (DisplayName = "버프 원형 UI 설정"))
	TObjectPtr<UBuffDurationRingSettings> Settings;

	const UBuffDurationRingSettings* GetRingSettings() const;
	void ApplyStatus(double Remaining, float Progress, bool bQuad, UTextBlock* TimeText, UImage* Icon);

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CircleImg;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> RingSizeBox;

private:
	void ApplyRingStyle();
	void ApplyIconStyle(UImage* Icon);
	float GetIconFlashMultiplier(double Remaining) const;
	FLinearColor GetTimeDisplayColor(double Remaining) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RingMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> IconMID;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> AppliedRingMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> AppliedIconMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> AppliedIconTexture;

	TWeakObjectPtr<UImage> StyledIcon;
	float AppliedProgress = -1.0f;
	float AppliedSize = -1.0f;
	float AppliedSteps = -1.0f;
	float AppliedEdgeGlowStrength = -1.0f;
	float AppliedEdgeGlowWidth = -1.0f;
	float AppliedBlur = -1.0f;
	float AppliedIconOpacity = -1.0f;
	FLinearColor AppliedFillColor = FLinearColor::Transparent;
	FLinearColor AppliedEmptyColor = FLinearColor::Transparent;
	FLinearColor AppliedIconTint = FLinearColor::Transparent;
	FVector2D AppliedIconSize = FVector2D::ZeroVector;

	friend class FBuffDurationRingTest;
};

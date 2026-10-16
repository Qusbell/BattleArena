#pragma once

#include "CoreMinimal.h"
#include "Extensions/UserWidgetExtension.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CrosshairRecoilLibrary.generated.h"

class UWidgetAnimation;
class UWidget;

/** One reusable, transient animation per widget; authored assets are never edited at runtime. */
UCLASS()
class SHOOTINGARENA_API UCrosshairRecoilExtension : public UUserWidgetExtension
{
	GENERATED_BODY()

private:
	friend class UCrosshairRecoilLibrary;
	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> SourceAnimation;
	UPROPERTY(Transient)
	TObjectPtr<UWidgetAnimation> RuntimeAnimation;
	FGuid SourceSceneSignature;
	bool bPreparingRecoil = false;
	bool bStoppingRecoil = false;
};

UCLASS()
class SHOOTINGARENA_API UCrosshairRecoilLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Returns an instance-local recoil animation whose maximum additional movement is SpreadAmount
	 * in local UMG units. Only Padding/Translation motion is normalized; scale, color and timing remain authored.
	 * Zero suppresses spacing movement. Negative/non-finite values are treated as zero; distances are capped at 100000.
	 * Animations without spacing tracks (Crosshair 05/07) retain their original visual effects.
	 */
	UFUNCTION(BlueprintCallable, Category="Crosshair|Recoil", meta=(DefaultToSelf="Widget"))
	static UWidgetAnimation* PrepareRecoilWithSpread(UUserWidget* Widget, UWidgetAnimation* Animation, float SpreadAmount = 20.f);

	/** Called before legacy Recoil so legacy and adjustable animation players cannot compete. */
	UFUNCTION(BlueprintCallable, Category="Crosshair|Recoil", meta=(DefaultToSelf="Widget"))
	static void StopRecoilWithSpread(UUserWidget* Widget);

	/** Returns null for an uninitialized tree, missing panel, or empty crosshair container. */
	UFUNCTION(BlueprintPure, Category="Crosshair|Recoil", meta=(DefaultToSelf="Widget"))
	static UWidget* GetCrosshairChildSafely(UUserWidget* Widget);
};

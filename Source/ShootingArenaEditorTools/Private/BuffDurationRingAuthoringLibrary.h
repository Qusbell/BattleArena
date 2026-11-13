#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BuffDurationRingAuthoringLibrary.generated.h"

/** Narrow, repeatable migration; saves only the new ring assets and the selected player HUD. */
UCLASS()
class UBuffDurationRingAuthoringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString InstallBuffDurationRing();

	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString InstallBuffIconWhiteShine();

	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString InstallBuffRingWhiteShine();

	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString InstallBuffVisualOptions();

	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString InstallConnectedBuffRing();

	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString InstallBuffIconBackground();

	/** Creates a separate white RGB copy with the original texture's exact source alpha. */
	UFUNCTION(BlueprintCallable, Category = "Editor|Buff HUD")
	static FString CreateWhiteBuffIconTexture();
};

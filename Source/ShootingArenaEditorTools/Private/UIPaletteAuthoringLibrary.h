#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UIPaletteAuthoringLibrary.generated.h"

class UBlueprint;

/** Editor-only migration helpers. Color values remain authored in the UI palette. */
UCLASS()
class UUIPaletteAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Lists unconnected color inputs and constant Make Color outputs in every Blueprint graph. */
    UFUNCTION(BlueprintCallable, Category="Editor|UI Palette")
    static FString InspectBlueprintColors(UBlueprint* Blueprint);

    /** Keys are graphName|nodeName|pinName, as returned by InspectBlueprintColors.
     * The caller compiles and saves the Blueprint after reviewing the result.
     */
    UFUNCTION(BlueprintCallable, Category="Editor|UI Palette")
    static FString BindBlueprintColors(UBlueprint* Blueprint, const TMap<FString, FName>& Bindings);
};

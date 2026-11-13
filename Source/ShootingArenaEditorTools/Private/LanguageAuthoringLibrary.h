#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LanguageAuthoringLibrary.generated.h"
/** Builds validated locres bytes without modifying files; editor-only. */
UCLASS()
class ULanguageAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Editor|Language") static bool BuildLocalizationResource(
        const FString& Culture, const TMap<FString,FString>& Translations, TArray<uint8>& ResourceData);
};

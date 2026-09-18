#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Analyzer/ArenaAnalyzeSubsystem.h"

#include "ArenaAnalyzeLibrary.generated.h"


UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	

public:
	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static bool LoadAnalyzeFile(
		const FString& FilePath,
		FArenaAnalyzeSession& OutSession
	);

	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static TArray<FString> GetAnalyzeFiles();

	UFUNCTION(BlueprintCallable, Category = "Arena Analyze")
	static bool LoadAnalyzeFileByName(
		const FString& FileName,
		FArenaAnalyzeSession& OutSession
	);
};

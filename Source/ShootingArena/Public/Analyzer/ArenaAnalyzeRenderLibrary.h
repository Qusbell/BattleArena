#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Analyzer/ArenaAnalyzeSubsystem.h"
#include "ArenaAnalyzeRenderLibrary.generated.h"

/** Uses the existing visualizer's ISM meshes/materials, with bulk instance uploads. */
UCLASS()
class SHOOTINGARENA_API UArenaAnalyzeRenderLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static int32 RenderSession(AActor* Visualizer, const FArenaAnalyzeSession& Session, int32 EventKind = 0);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void ClearVisualizer(AActor* Visualizer);
};

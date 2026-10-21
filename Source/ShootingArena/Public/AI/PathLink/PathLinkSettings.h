#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PathLinkSettings.generated.h"

/** Legacy heuristic settings retained for serialized/config compatibility.
 * Detailed routing now prioritizes all endpoints using baked estimates and certifies termination with lower bounds.
 * The properties below no longer restrict candidates, inflate costs, or cap validation passes.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Path Link"))
class SHOOTINGARENA_API UPathLinkSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** Legacy compatibility only; no longer used by detailed routing. */
    UPROPERTY(Config, EditAnywhere, Category = "Legacy Route Heuristics", meta = (ClampMin = "1", UIMin = "1", UIMax = "32"))
    int32 DynamicCandidateCount = 6;

    /** Legacy compatibility only; no longer used by detailed routing. */
    UPROPERTY(Config, EditAnywhere, Category = "Legacy Route Heuristics", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "4.0"))
    float BlockedTraceDistanceMultiplier = 1.5f;

    /** Legacy compatibility only; no longer used by detailed routing. */
    UPROPERTY(Config, EditAnywhere, Category = "Legacy Route Heuristics", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "250.0"))
    float CandidateTraceHeight = 100.0f;

    /** Legacy compatibility only; no longer used by detailed routing. */
    UPROPERTY(Config, EditAnywhere, Category = "Legacy Route Heuristics", meta = (ClampMin = "1.0", UIMin = "1.0", UIMax = "10.0"))
    float ExcessiveNavDistanceRatio = 3.5f;

    /** Legacy compatibility only; no longer used by detailed routing. */
    UPROPERTY(Config, EditAnywhere, Category = "Legacy Route Heuristics", meta = (ClampMin = "1", UIMin = "1", UIMax = "16"))
    int32 CandidateExpansionStep = 3;

    /** Legacy compatibility only; no longer used by detailed routing. */
    UPROPERTY(Config, EditAnywhere, Category = "Legacy Route Heuristics", meta = (ClampMin = "0", UIMin = "0", UIMax = "8"))
    int32 MaxCandidateExpansionPasses = 3;
};

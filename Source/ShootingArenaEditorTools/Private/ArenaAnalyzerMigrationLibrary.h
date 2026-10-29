#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaAnalyzerMigrationLibrary.generated.h"
class UBlueprint;
class APawn;
UCLASS()
class UArenaAnalyzerMigrationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    /** Invoked exclusively through Unreal MCP. Does not save or compile automatically. */
    UFUNCTION(BlueprintCallable, Category="Editor|Arena Analyzer")
    static FString ConnectAnalyzer(UBlueprint* Blueprint);
    /** Transient PIE-only weapon fixture, callable through MCP for integration checks. */
    UFUNCTION(BlueprintCallable, Category="Editor|Arena Analyzer")
    static AActor* SpawnAnalyzerProbeWeapon(APawn* Shooter, TSubclassOf<AActor> WeaponClass);
};

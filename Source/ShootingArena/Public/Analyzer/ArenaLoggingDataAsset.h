#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaLoggingDataAsset.generated.h"

/**
 * Editor-side switch for match recording (DA_ArenaLogging). Read by IsArenaLoggingEnabled() when a world is created.
 * Keep it off (the default) in what you commit; turn it on locally while you need the database / JSON records.
 */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UArenaLoggingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** On: the analyzer JSON and the match log database record each match. Applies from the next world on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena Logging")
	bool bLoggingEnabled = false;

	/** Where the asset lives; IsArenaLoggingEnabled() looks it up here and treats a missing asset as off. */
	static const TCHAR* GetAssetPath() { return TEXT("/Game/QuakeLike_1_0/Data/DA_ArenaLogging.DA_ArenaLogging"); }
};

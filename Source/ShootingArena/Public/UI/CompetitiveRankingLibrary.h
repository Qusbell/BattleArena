#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CompetitiveRankingLibrary.generated.h"

UCLASS()
class SHOOTINGARENA_API UCompetitiveRankingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Sorts an array of PlayerStates, including arrays typed to a Blueprint subclass. */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "Ranking", meta = (ArrayParm = "Players"))
	static void SortRankingPlayers(const TArray<int32>& Players);
	DECLARE_FUNCTION(execSortRankingPlayers);

	/** Returns the competition rank at an index in a sorted PlayerState array. */
	UFUNCTION(BlueprintPure, CustomThunk, Category = "Ranking", meta = (ArrayParm = "SortedPlayers"))
	static int32 GetCompetitionRank(const TArray<int32>& SortedPlayers, int32 Index);
	DECLARE_FUNCTION(execGetCompetitionRank);
};

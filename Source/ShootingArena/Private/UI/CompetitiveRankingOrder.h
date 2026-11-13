#pragma once

#include "CoreMinimal.h"

class APlayerState;

struct FCompetitiveRankEntry
{
	APlayerState* Player = nullptr;
	int32 Kills = 0;
	int32 Deaths = 0;
	FString DisplayName;
	int32 Rank = 0;
};

FCompetitiveRankEntry ReadCompetitiveRankEntry(APlayerState* Player);

inline bool HasSameCompetitiveScore(const FCompetitiveRankEntry& A, const FCompetitiveRankEntry& B)
{
	return A.Kills == B.Kills && A.Deaths == B.Deaths;
}

inline void SortCompetitiveRanking(TArray<FCompetitiveRankEntry>& Players)
{
	Players.StableSort([](const FCompetitiveRankEntry& A, const FCompetitiveRankEntry& B)
	{
		if (A.Kills != B.Kills) return A.Kills > B.Kills;
		if (A.Deaths != B.Deaths) return A.Deaths < B.Deaths;
		return A.DisplayName.Compare(B.DisplayName, ESearchCase::IgnoreCase) < 0;
	});

	int32 Rank = 0;
	for (int32 Index = 0; Index < Players.Num(); ++Index)
	{
		if (Index == 0 || !HasSameCompetitiveScore(Players[Index], Players[Index - 1]))
		{
			Rank = Index + 1;
		}
		Players[Index].Rank = Rank;
	}
}

inline TArray<FCompetitiveRankEntry> BuildCompactRanking(TArray<FCompetitiveRankEntry> Players, APlayerState* LocalPlayer)
{
	SortCompetitiveRanking(Players);
	const int32 LocalIndex = Players.IndexOfByPredicate([LocalPlayer](const auto& Entry) { return Entry.Player == LocalPlayer; });
	if (!LocalPlayer || LocalIndex == INDEX_NONE) return {};
	TArray<FCompetitiveRankEntry> Result{Players[0]};
	const int32 OtherIndex = LocalIndex == 0 ? 1 : LocalIndex;
	if (Players.IsValidIndex(OtherIndex)) Result.Add(Players[OtherIndex]);
	return Result;
}

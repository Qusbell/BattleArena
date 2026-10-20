#pragma once

#include "CoreMinimal.h"

class APlayerState;

struct FCompetitiveRankEntry
{
	APlayerState* Player = nullptr;
	int32 Kills = 0;
	int32 Rank = 0;
};

inline TArray<FCompetitiveRankEntry> BuildCompactRanking(TArray<FCompetitiveRankEntry> Players, APlayerState* LocalPlayer)
{
	// Match the Tab scoreboard: descending kills, preserving PlayerArray order on ties.
	Players.StableSort([](const auto& A, const auto& B) { return A.Kills > B.Kills; });
	const int32 LocalIndex = Players.IndexOfByPredicate([LocalPlayer](const auto& Entry) { return Entry.Player == LocalPlayer; });
	if (!LocalPlayer || LocalIndex == INDEX_NONE) return {};
	for (int32 Index = 0; Index < Players.Num(); ++Index) Players[Index].Rank = Index + 1;
	TArray<FCompetitiveRankEntry> Result{Players[0]};
	const int32 OtherIndex = LocalIndex == 0 ? 1 : LocalIndex;
	if (Players.IsValidIndex(OtherIndex)) Result.Add(Players[OtherIndex]);
	return Result;
}

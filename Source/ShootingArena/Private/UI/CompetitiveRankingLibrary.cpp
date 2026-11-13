#include "UI/CompetitiveRankingLibrary.h"

#include "Camera/ShooterDisplayLibrary.h"
#include "GameFramework/PlayerState.h"
#include "UI/CompetitiveRankingOrder.h"
#include "UObject/UnrealType.h"

namespace
{
	FCompetitiveRankEntry ReadCompetitiveScore(APlayerState* Player)
	{
		FCompetitiveRankEntry Entry;
		Entry.Player = Player;
		if (IsValid(Player))
		{
			if (const auto* Kills = FindFProperty<FIntProperty>(Player->GetClass(), TEXT("KillCount")))
			{
				Entry.Kills = Kills->GetPropertyValue_InContainer(Player);
			}
			if (const auto* Deaths = FindFProperty<FIntProperty>(Player->GetClass(), TEXT("DeathCount")))
			{
				Entry.Deaths = Deaths->GetPropertyValue_InContainer(Player);
			}
		}
		return Entry;
	}

	APlayerState* GetRankingPlayer(FScriptArrayHelper& Array, const FObjectProperty* Property, int32 Index)
	{
		return Cast<APlayerState>(Property->GetObjectPropertyValue(Array.GetRawPtr(Index)));
	}
}

FCompetitiveRankEntry ReadCompetitiveRankEntry(APlayerState* Player)
{
	FCompetitiveRankEntry Entry = ReadCompetitiveScore(Player);
	Entry.DisplayName = UShooterDisplayLibrary::GetPlayerStateDisplayName(Player).ToString();
	return Entry;
}

DEFINE_FUNCTION(UCompetitiveRankingLibrary::execSortRankingPlayers)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddress = Stack.MostRecentPropertyAddress;
	const auto* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	P_FINISH;

	P_NATIVE_BEGIN;
	const auto* PlayerProperty = ArrayProperty ? CastField<FObjectProperty>(ArrayProperty->Inner) : nullptr;
	if (ArrayAddress && PlayerProperty && PlayerProperty->PropertyClass->IsChildOf(APlayerState::StaticClass()))
	{
		FScriptArrayHelper Array(ArrayProperty, ArrayAddress);
		TArray<FCompetitiveRankEntry> Ranking;
		Ranking.Reserve(Array.Num());
		for (int32 Index = 0; Index < Array.Num(); ++Index)
		{
			Ranking.Add(ReadCompetitiveRankEntry(GetRankingPlayer(Array, PlayerProperty, Index)));
		}
		SortCompetitiveRanking(Ranking);
		for (int32 Index = 0; Index < Ranking.Num(); ++Index)
		{
			PlayerProperty->SetObjectPropertyValue(Array.GetRawPtr(Index), Ranking[Index].Player);
		}
	}
	P_NATIVE_END;
}

DEFINE_FUNCTION(UCompetitiveRankingLibrary::execGetCompetitionRank)
{
	Stack.MostRecentProperty = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddress = Stack.MostRecentPropertyAddress;
	const auto* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	P_GET_PROPERTY(FIntProperty, Index);
	P_FINISH;

	P_NATIVE_BEGIN;
	int32 Rank = 0;
	const auto* PlayerProperty = ArrayProperty ? CastField<FObjectProperty>(ArrayProperty->Inner) : nullptr;
	if (ArrayAddress && PlayerProperty && PlayerProperty->PropertyClass->IsChildOf(APlayerState::StaticClass()))
	{
		FScriptArrayHelper Array(ArrayProperty, ArrayAddress);
		if (Array.IsValidIndex(Index))
		{
			const FCompetitiveRankEntry Score = ReadCompetitiveScore(GetRankingPlayer(Array, PlayerProperty, Index));
			while (Index > 0 && HasSameCompetitiveScore(Score, ReadCompetitiveScore(GetRankingPlayer(Array, PlayerProperty, Index - 1))))
			{
				--Index;
			}
			Rank = Index + 1;
		}
	}
	*static_cast<int32*>(RESULT_PARAM) = Rank;
	P_NATIVE_END;
}

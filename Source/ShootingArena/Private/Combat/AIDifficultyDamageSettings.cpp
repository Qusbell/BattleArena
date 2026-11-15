#include "Combat/AIDifficultyDamageSettings.h"

#include "AI/QuakeTargetSelectionAIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPtr.h"

namespace
{
	const FAIDifficultyDamageRates* FindDamageRates(ELobbyDifficulty Difficulty)
	{
		static const TSoftObjectPtr<UAIDifficultyDamageSettings> SettingsAsset(
			FSoftObjectPath(TEXT("/Game/QuakeLike_1_0/Data/AI/DA_AIDifficultyDamageSettings.DA_AIDifficultyDamageSettings")));
		const UAIDifficultyDamageSettings* Settings = SettingsAsset.LoadSynchronous();
		if (!Settings)
		{
			return nullptr;
		}

		switch (Difficulty)
		{
		case ELobbyDifficulty::Easy: return &Settings->Easy;
		case ELobbyDifficulty::Normal: return &Settings->Normal;
		case ELobbyDifficulty::Hard: return &Settings->Hard;
		}
		return nullptr;
	}
}

double UAIDifficultyDamageLibrary::ScaleAIDifficultyDamage(AActor* Victim, AController* InstigatedBy, double Damage)
{
	const APawn* VictimPawn = Cast<APawn>(Victim);
	if (!IsValid(VictimPawn) || !VictimPawn->HasAuthority() || !IsValid(InstigatedBy) || Damage <= 0.0)
	{
		return Damage;
	}

	const AController* VictimController = VictimPawn->GetController();
	const bool bDamageDealtByAI = Cast<APlayerController>(VictimController) != nullptr;
	const AQuakeTargetSelectionAIController* Bot = bDamageDealtByAI
		? Cast<AQuakeTargetSelectionAIController>(InstigatedBy)
		: Cast<AQuakeTargetSelectionAIController>(VictimController);
	if (!IsValid(Bot) || (!bDamageDealtByAI && !Cast<APlayerController>(InstigatedBy)))
	{
		return Damage;
	}

	const TOptional<ELobbyDifficulty> Difficulty = Bot->GetDamageDifficulty();
	const FAIDifficultyDamageRates* Rates = Difficulty.IsSet() ? FindDamageRates(Difficulty.GetValue()) : nullptr;
	if (!Rates)
	{
		return Damage;
	}

	const double Percent = bDamageDealtByAI ? Rates->DamageDealtPercent : Rates->DamageTakenPercent;
	if (!FMath::IsFinite(Percent))
	{
		return Damage;
	}

	return Damage * ((100.0 + FMath::Max(Percent, -99.0)) / 100.0);
}

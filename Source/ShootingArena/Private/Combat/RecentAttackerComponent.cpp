#include "Combat/RecentAttackerComponent.h"

#include "Audio/KillStreakSettingsDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

URecentAttackerComponent::URecentAttackerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
}

void URecentAttackerComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	if (!Cast<APawn>(Owner))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Recent attacker component is intended for a Pawn owner: %s."),
			*GetNameSafe(Owner));
	}

	if (Owner->HasAuthority())
	{
		Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandleOwnerTakeAnyDamage);
	}
}

void URecentAttackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.RemoveDynamic(this, &ThisClass::HandleOwnerTakeAnyDamage);
	}

	Super::EndPlay(EndPlayReason);
}

void URecentAttackerComponent::HandleOwnerTakeAnyDamage(
	AActor* DamagedActor,
	float Damage,
	const UDamageType* DamageType,
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority() || DamagedActor != Owner || Damage <= 0.0f)
	{
		return;
	}

	APlayerState* VictimPlayerState = GetVictimPlayerState();
	APlayerState* AttackerPlayerState = ResolvePlayerState(InstigatedBy, DamageCauser);
	if (!IsValid(AttackerPlayerState) || AttackerPlayerState == VictimPlayerState)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown)
	{
		return;
	}

	LastAttackerPlayerState = AttackerPlayerState;
	LastAttackerTime = World->GetTimeSeconds();
	bHasLastAttackerTime = true;
}

FKillCreditResolution URecentAttackerComponent::ResolveDeathCredit(
	AController* InstigatedBy,
	AActor* DamageCauser)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return FKillCreditResolution();
	}

	if (bDeathCreditResolved)
	{
		// Do not return the previous credited PlayerState to repeated BP death callbacks.
		return FKillCreditResolution();
	}

	bDeathCreditResolved = true;
	FKillCreditResolution Resolution;
	Resolution.VictimPlayerState = GetVictimPlayerState();

	APlayerState* DirectPlayerState = ResolvePlayerState(InstigatedBy, DamageCauser);
	if (IsValid(DirectPlayerState) && DirectPlayerState != Resolution.VictimPlayerState)
	{
		Resolution.DeathCause = EKillDeathCause::Player;
		Resolution.CreditedPlayerState = DirectPlayerState;
		Resolution.CreditSource = EKillCreditSource::DirectPlayerKill;
	}
	else
	{
		Resolution.DeathCause = IsSelfCaused(InstigatedBy, DamageCauser)
			? EKillDeathCause::Suicide
			: EKillDeathCause::Environment;

		UWorld* World = GetWorld();
		APlayerState* RecentAttacker = LastAttackerPlayerState.Get();
		const bool bRecentAttackerIsValid =
			IsValid(RecentAttacker)
			&& RecentAttacker != Resolution.VictimPlayerState
			&& bHasLastAttackerTime
			&& IsValid(World)
			&& !World->bIsTearingDown
			&& World->GetTimeSeconds() >= LastAttackerTime
			&& World->GetTimeSeconds() - LastAttackerTime <= GetAttackerMemoryTime();

		if ((Resolution.DeathCause == EKillDeathCause::Suicide
			|| Resolution.DeathCause == EKillDeathCause::Environment)
			&& bRecentAttackerIsValid)
		{
			Resolution.CreditedPlayerState = RecentAttacker;
			Resolution.CreditSource = EKillCreditSource::RecentAttacker;
		}
	}

	OnKillCreditResolved.Broadcast(Resolution);
	return Resolution;
}

APlayerState* URecentAttackerComponent::ResolvePlayerState(
	AController* InstigatedBy,
	AActor* DamageCauser) const
{
	if (IsValid(InstigatedBy))
	{
		if (APlayerState* PlayerState = InstigatedBy->GetPlayerState<APlayerState>())
		{
			return PlayerState;
		}
	}

	if (!IsValid(DamageCauser))
	{
		return nullptr;
	}

	if (AController* DamageInstigatorController = DamageCauser->GetInstigatorController())
	{
		if (APlayerState* PlayerState = DamageInstigatorController->GetPlayerState<APlayerState>())
		{
			return PlayerState;
		}
	}

	if (APawn* DamageInstigator = DamageCauser->GetInstigator())
	{
		if (AController* DamageInstigatorController = DamageInstigator->GetController())
		{
			if (APlayerState* PlayerState = DamageInstigatorController->GetPlayerState<APlayerState>())
			{
				return PlayerState;
			}
		}
	}

	AActor* DamageOwner = DamageCauser->GetOwner();
	if (AController* OwnerController = Cast<AController>(DamageOwner))
	{
		return OwnerController->GetPlayerState<APlayerState>();
	}

	if (APawn* OwnerPawn = Cast<APawn>(DamageOwner))
	{
		if (AController* OwnerController = OwnerPawn->GetController())
		{
			return OwnerController->GetPlayerState<APlayerState>();
		}
	}

	return nullptr;
}

bool URecentAttackerComponent::IsSelfCaused(AController* InstigatedBy, AActor* DamageCauser) const
{
	AActor* Owner = GetOwner();
	APawn* OwnerPawn = Cast<APawn>(Owner);
	AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	if (IsValid(OwnerController) && InstigatedBy == OwnerController)
	{
		return true;
	}

	if (!IsValid(DamageCauser))
	{
		return false;
	}

	if (DamageCauser == Owner || DamageCauser->GetInstigator() == Owner)
	{
		return true;
	}

	if (IsValid(OwnerController)
		&& (DamageCauser->GetInstigatorController() == OwnerController
			|| DamageCauser->GetOwner() == OwnerController))
	{
		return true;
	}

	if (DamageCauser->GetOwner() == Owner)
	{
		return true;
	}

	return false;
}

APlayerState* URecentAttackerComponent::GetVictimPlayerState() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	return OwnerController ? OwnerController->GetPlayerState<APlayerState>() : nullptr;
}

float URecentAttackerComponent::GetAttackerMemoryTime() const
{
	return IsValid(Settings) && FMath::IsFinite(Settings->AttackerMemoryTime)
		? FMath::Max(0.0f, Settings->AttackerMemoryTime)
		: 5.0f;
}

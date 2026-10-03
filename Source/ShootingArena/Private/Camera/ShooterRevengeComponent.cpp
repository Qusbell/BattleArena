#include "Camera/ShooterRevengeComponent.h"
#include "Combat/RecentAttackerComponent.h"
#include "Engine/World.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

UShooterRevengeComponent::UShooterRevengeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UShooterRevengeComponent::BeginPlay()
{
	Super::BeginPlay();
	bEndingPlay = false;
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!IsValid(PC) || PC->IsActorBeingDestroyed() || !PC->HasAuthority()) return;
	PC->GetOnNewPawnNotifier().Remove(PawnChangedHandle);
	PawnChangedHandle = PC->GetOnNewPawnNotifier().AddUObject(this, &ThisClass::BindPawnDeathCredit);
	BindPawnDeathCredit(PC->GetPawn());
}

void UShooterRevengeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		PC->GetOnNewPawnNotifier().Remove(PawnChangedHandle);
	}
	if (URecentAttackerComponent* Credit = BoundDeathCredit.Get())
	{
		Credit->OnKillCreditResolved.RemoveDynamic(this, &ThisClass::HandleKillCreditResolved);
	}
	BoundDeathCredit.Reset();
	PawnChangedHandle.Reset();
	TargetPlayerState = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UShooterRevengeComponent::BindPawnDeathCredit(APawn* NewPawn)
{
	// A death can unpossess the pawn before all death callbacks finish. Keep its binding
	// until the replacement pawn arrives; respawning never clears the stored target.
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || !PC->HasAuthority()
		|| !IsValid(NewPawn) || NewPawn->IsActorBeingDestroyed() || NewPawn->GetController() != PC) return;
	URecentAttackerComponent* NewCredit = NewPawn->FindComponentByClass<URecentAttackerComponent>();
	if (BoundDeathCredit.Get() == NewCredit) return;
	if (URecentAttackerComponent* OldCredit = BoundDeathCredit.Get())
	{
		OldCredit->OnKillCreditResolved.RemoveDynamic(this, &ThisClass::HandleKillCreditResolved);
	}
	BoundDeathCredit = NewCredit;
	if (IsValid(NewCredit) && !NewCredit->IsBeingDestroyed())
	{
		NewCredit->OnKillCreditResolved.AddUniqueDynamic(this, &ThisClass::HandleKillCreditResolved);
	}
}

void UShooterRevengeComponent::HandleKillCreditResolved(const FKillCreditResolution& Resolution)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !PC->HasAuthority() || !World || World->bIsTearingDown) return;
	APlayerState* VictimState = PC->GetPlayerState<APlayerState>();
	if (!IsValid(VictimState)) return;
	// A pawn loses its PlayerState on UnPossess. This event is still from our bound
	// death-credit component, so use the persistent controller identity in that case.
	if (Resolution.VictimPlayerState && Resolution.VictimPlayerState != VictimState) return;
	// Recent-attacker credit for suicide/environment deaths is not a direct kill.
	if (Resolution.DeathCause != EKillDeathCause::Player
		|| Resolution.CreditSource != EKillCreditSource::DirectPlayerKill
		|| !IsValid(Resolution.CreditedPlayerState)
		|| Resolution.CreditedPlayerState->IsActorBeingDestroyed()
		|| Resolution.CreditedPlayerState->GetWorld() != World
		|| Resolution.CreditedPlayerState == VictimState) return;
	// The victim's confirmed death also completes the killer's revenge, if this
	// victim was their target. Keep other players' targets independent.
	APlayerController* KillerPC = Resolution.CreditedPlayerState->GetPlayerController();
	UShooterRevengeComponent* KillerRevenge = IsValid(KillerPC)
		? KillerPC->FindComponentByClass<UShooterRevengeComponent>() : nullptr;
	if (IsValid(KillerRevenge) && !KillerRevenge->IsBeingDestroyed()
		&& KillerRevenge->GetRevengeTargetPlayerState() == VictimState)
	{
		KillerRevenge->ClearRevengeTarget();
	}
	SetRevengeTargetPlayerState(Resolution.CreditedPlayerState);
}

void UShooterRevengeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UShooterRevengeComponent, TargetPlayerState, COND_OwnerOnly);
}

void UShooterRevengeComponent::SetRevengeTargetPlayerState(APlayerState* Target)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !PC->HasAuthority() || !World || World->bIsTearingDown) return;
	if (IsValid(Target) && Target->GetWorld() != World) return;
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || Target == PC->GetPlayerState<APlayerState>()) Target = nullptr;
	if (TargetPlayerState == Target) return;
	TargetPlayerState = Target;
	PC->ForceNetUpdate();
	OnRep_TargetPlayerState();
}

void UShooterRevengeComponent::SetRevengeTarget(APawn* Target)
{
	SetRevengeTargetPlayerState(IsValid(Target) ? Target->GetPlayerState() : nullptr);
}

void UShooterRevengeComponent::ClearRevengeTarget()
{
	SetRevengeTargetPlayerState(nullptr);
}

APlayerState* UShooterRevengeComponent::GetRevengeTargetPlayerState() const
{
	return !bEndingPlay && !IsBeingDestroyed() && IsValid(TargetPlayerState)
		&& !TargetPlayerState->IsActorBeingDestroyed() ? TargetPlayerState.Get() : nullptr;
}

bool UShooterRevengeComponent::IsRevengeTarget(const APawn* Shooter) const
{
	APlayerState* Target = GetRevengeTargetPlayerState();
	return Target && IsValid(Shooter) && Shooter->GetPlayerState() == Target;
}

void UShooterRevengeComponent::OnRep_TargetPlayerState()
{
	if (!bEndingPlay && !IsBeingDestroyed()) OnRevengeTargetChanged.Broadcast(GetRevengeTargetPlayerState());
}

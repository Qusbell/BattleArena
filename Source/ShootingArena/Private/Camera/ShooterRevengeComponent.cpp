#include "Camera/ShooterRevengeComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

namespace
{
FObjectPropertyBase* FindRevengeProperty(const APlayerState* State)
{
	if (!IsValid(State) || State->IsActorBeingDestroyed()) return nullptr;
	FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(State->GetClass(), TEXT("RevengeTarget"));
	return Property && Property->ArrayDim == 1 && Property->PropertyClass
		&& Property->PropertyClass->IsChildOf(APlayerState::StaticClass())
		? Property : nullptr;
}
}

UShooterRevengeComponent::UShooterRevengeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	SetIsReplicatedByDefault(true);
}

void UShooterRevengeComponent::BeginPlay()
{
	Super::BeginPlay();
	bEndingPlay = false;
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	SetComponentTickEnabled(IsValid(PC) && !PC->IsActorBeingDestroyed() && PC->HasAuthority());
	RefreshRevengeTargetFromPlayerState();
}

void UShooterRevengeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshRevengeTargetFromPlayerState();
}

void UShooterRevengeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	SetComponentTickEnabled(false);
	TargetPlayerState = nullptr;
	Super::EndPlay(EndPlayReason);
}

APlayerState* UShooterRevengeComponent::ReadPlayerStateRevengeTarget() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !World || World->bIsTearingDown) return nullptr;
	APlayerState* State = PC->GetPlayerState<APlayerState>();
	if (!IsValid(State) || State->GetWorld() != World) return nullptr;
	FObjectPropertyBase* Property = FindRevengeProperty(State);
	APlayerState* Target = Property ? Cast<APlayerState>(Property->GetObjectPropertyValue_InContainer(State)) : nullptr;
	return IsValid(Target) && !Target->IsActorBeingDestroyed() && Target->GetWorld() == World && Target != State
		? Target : nullptr;
}

void UShooterRevengeComponent::RefreshRevengeTargetFromPlayerState()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	const UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !PC->HasAuthority() || !World || World->bIsTearingDown) return;
	// PlayerState owns the kill/death rules. This component only mirrors the result,
	// including changes made by its existing Blueprint events, to the owning client.
	APlayerState* Target = ReadPlayerStateRevengeTarget();
	if (TargetPlayerState == Target) return;
	TargetPlayerState = Target;
	PC->ForceNetUpdate();
	OnRep_TargetPlayerState();
}

void UShooterRevengeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UShooterRevengeComponent, TargetPlayerState, COND_OwnerOnly);
}

void UShooterRevengeComponent::SetRevengeTargetPlayerState(APlayerState* Target)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	const UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !PC->HasAuthority() || !World || World->bIsTearingDown) return;
	if (IsValid(Target) && Target->GetWorld() != World) return;
	APlayerState* State = PC->GetPlayerState<APlayerState>();
	if (!IsValid(State) || State->GetWorld() != World) return;
	FObjectPropertyBase* Property = FindRevengeProperty(State);
	if (!Property) return;
	if (!IsValid(Target) || Target->IsActorBeingDestroyed() || Target == State) Target = nullptr;
	if (Target && !Target->IsA(Property->PropertyClass)) return;
	Property->SetObjectPropertyValue_InContainer(State, Target);
	State->ForceNetUpdate();
	RefreshRevengeTargetFromPlayerState();
}

void UShooterRevengeComponent::SetRevengeTarget(APawn* Target)
{
	SetRevengeTargetPlayerState(IsValid(Target) && !Target->IsActorBeingDestroyed() ? Target->GetPlayerState() : nullptr);
}

void UShooterRevengeComponent::ClearRevengeTarget()
{
	SetRevengeTargetPlayerState(nullptr);
}

APlayerState* UShooterRevengeComponent::GetRevengeTargetPlayerState() const
{
	const APlayerController* PC = Cast<APlayerController>(GetOwner());
	const UWorld* World = GetWorld();
	if (bEndingPlay || IsBeingDestroyed() || !IsValid(PC) || PC->IsActorBeingDestroyed()
		|| !World || World->bIsTearingDown) return nullptr;
	if (PC->HasAuthority()) return ReadPlayerStateRevengeTarget();
	return IsValid(TargetPlayerState) && !TargetPlayerState->IsActorBeingDestroyed()
		&& TargetPlayerState->GetWorld() == World && TargetPlayerState != PC->GetPlayerState<APlayerState>()
		? TargetPlayerState.Get() : nullptr;
}

bool UShooterRevengeComponent::IsRevengeTarget(const APawn* Shooter) const
{
	APlayerState* Target = GetRevengeTargetPlayerState();
	return Target && IsValid(Shooter) && !Shooter->IsActorBeingDestroyed()
		&& Shooter->GetWorld() == GetWorld() && Shooter->GetPlayerState() == Target;
}

void UShooterRevengeComponent::OnRep_TargetPlayerState()
{
	if (!bEndingPlay && !IsBeingDestroyed()) OnRevengeTargetChanged.Broadcast(GetRevengeTargetPlayerState());
}

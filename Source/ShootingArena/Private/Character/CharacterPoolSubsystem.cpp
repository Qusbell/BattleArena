#include "Character/CharacterPoolSubsystem.h"

#include "Character/CharacterPoolComponent.h"
#include "Character/CharacterPoolSettings.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "MyReconnectionGameMode.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterPool, Log, All);

bool UCharacterPoolSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

bool UCharacterPoolSubsystem::Supports(const UClass* Class)
{
	if (!Class) return false;
	const FString Path = Class->GetPathName();
	return Path == TEXT("/Game/QuakeLike_1_0/Character/BP_PlayerShooter.BP_PlayerShooter_C")
		|| Path == TEXT("/Game/QuakeLike_1_0/Character/BP_AIShooter.BP_AIShooter_C");
}

APawn* UCharacterPoolSubsystem::FindOwnedPawn(AController* Controller) const
{
	if (!IsValid(Controller)) return nullptr;
	for (const auto& Entry : Entries)
		if (Entry.Controller.Get() == Controller && IsValid(Entry.Pawn) && !Entry.Pawn->IsActorBeingDestroyed()) return Entry.Pawn;
	return nullptr;
}

bool UCharacterPoolSubsystem::IsReserved(APawn* Pawn) const
{
	for (const auto& E : Entries) if (E.Pawn == Pawn) return !E.ReconnectToken.IsEmpty();
	return false;
}

APawn* UCharacterPoolSubsystem::Acquire(AController* Controller, TSubclassOf<APawn> PawnClass, const FTransform& Transform)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_Acquire);
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || World->GetNetMode() == NM_Client || !IsValid(Controller) || !Supports(PawnClass)) return nullptr;
	if (APawn* Owned = FindOwnedPawn(Controller))
	{
		if (Owned->GetClass() != PawnClass || Owned->GetController()) return nullptr;
		auto* Life = Owned->FindComponentByClass<UCharacterPoolComponent>();
		if (!Life || !Life->PrepareRespawn(Transform)) return nullptr;
		++Reused;
		return Owned;
	}
	if (!GetDefault<UCharacterPoolSettings>()->bEnabled) return nullptr;
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_Create);
	APawn* Pawn = World->SpawnActorDeferred<APawn>(PawnClass, Transform, Controller, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!IsValid(Pawn)) return nullptr;
	FOwnedCharacterPawn& Entry = Entries.AddDefaulted_GetRef();
	Entry.Pawn = Pawn;
	Entry.Controller = Controller;
	Controller->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnControllerDestroyed);
	Pawn->FinishSpawning(Transform);
	if (!IsValid(Pawn) || !Pawn->FindComponentByClass<UCharacterPoolComponent>())
	{
		DestroyOwnedPawn(Pawn);
		UE_LOG(LogCharacterPool, Error, TEXT("Pawn class has no CharacterPool component: %s"), *GetNameSafe(PawnClass.Get()));
		return nullptr;
	}
	++Created;
	UE_LOG(LogCharacterPool, Log, TEXT("Created dedicated pawn %s for %s (owned=%d, created=%d)"),
		*GetNameSafe(Pawn), *GetNameSafe(Controller), Entries.Num(), Created);
	return Pawn;
}

void UCharacterPoolSubsystem::BindController(APawn* Pawn, AController* Controller)
{
	if (!IsValid(Pawn) || !IsValid(Controller) || !Pawn->HasAuthority()) return;
	for (auto& Entry : Entries)
	{
		if (Entry.Pawn != Pawn) continue;
		if (Entry.Controller.IsValid() && Entry.Controller.Get() != Controller && !Entry.bClaiming)
		{
			UE_LOG(LogCharacterPool, Error, TEXT("Rejected reassignment of participant-owned pawn %s"), *Pawn->GetName());
			return;
		}
		if (AController* Old = Entry.Controller.Get(); Old && Old != Controller) Old->OnDestroyed.RemoveDynamic(this, &ThisClass::OnControllerDestroyed);
		Entry.Controller = Controller;
		Entry.bClaiming = false;
		GetWorld()->GetTimerManager().ClearTimer(Entry.Expiry);
		Controller->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnControllerDestroyed);
		return;
	}
}

void UCharacterPoolSubsystem::Reserve(APlayerController* Controller, float Seconds)
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || World->GetNetMode() == NM_Client || !IsValid(Controller)) return;
	APawn* Pawn = FindOwnedPawn(Controller);
	// Also preserve the old reconnection behavior when pooling is disabled for newly spawned pawns.
	if (!Pawn) Pawn = Controller->GetPawn();
	if (!IsValid(Pawn)) return;
	FString Token;
	if (Controller->GetClass()->ImplementsInterface(UReconnectionInterface::StaticClass()))
		Token = IReconnectionInterface::Execute_GetConnectionToken(Controller);
	if (Token.IsEmpty() || !FMath::IsFinite(Seconds) || Seconds <= 0.f)
	{
		Controller->UnPossess();
		DestroyOwnedPawn(Pawn);
		return;
	}
	// A token must never alias two participants' reservations.
	for (const auto& Existing : Entries)
		if (Existing.Pawn != Pawn && Existing.ReconnectToken == Token)
		{
			UE_LOG(LogCharacterPool, Warning, TEXT("Duplicate reconnection reservation rejected"));
			Controller->UnPossess();
			DestroyOwnedPawn(Pawn);
			return;
		}
	int32 Index = Entries.IndexOfByPredicate([Pawn](const auto& E) { return E.Pawn == Pawn; });
	if (Index == INDEX_NONE) { Index = Entries.AddDefaulted(); Entries[Index].Pawn = Pawn; Entries[Index].Controller = Controller; }
	Entries[Index].Controller = Controller;
	World->GetTimerManager().ClearTimer(Entries[Index].Expiry);
	Entries[Index].ReconnectToken = Token;
	Controller->OnDestroyed.AddUniqueDynamic(this, &ThisClass::OnControllerDestroyed);
	Controller->UnPossess();
	const TWeakObjectPtr<APawn> WeakPawn(Pawn);
	FTimerHandle Expiry;
	World->GetTimerManager().SetTimer(Expiry, FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn, Token]
	{
		for (const auto& E : Entries)
			if (E.Pawn == WeakPawn.Get() && E.ReconnectToken == Token) { DestroyOwnedPawn(WeakPawn.Get()); break; }
	}), Seconds, false);
	// UnPossess invokes Blueprint callbacks; look the record up again rather than retaining an array reference.
	for (auto& E : Entries) if (E.Pawn == Pawn) { E.Expiry = Expiry; return; }
	World->GetTimerManager().ClearTimer(Expiry);
}

APawn* UCharacterPoolSubsystem::Claim(const FString& Token)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_Reconnect);
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || World->GetNetMode() == NM_Client || Token.IsEmpty()) return nullptr;
	for (auto& Entry : Entries)
	{
		if (Entry.ReconnectToken != Token || !IsValid(Entry.Pawn) || Entry.Pawn->IsActorBeingDestroyed()) continue;
		APawn* Pawn = Entry.Pawn;
		World->GetTimerManager().ClearTimer(Entry.Expiry);
		Entry.ReconnectToken.Reset();
		Entry.bClaiming = true;
		Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::OnClaimedControllerChanged);
		const TWeakObjectPtr<APawn> WeakPawn(Pawn);
		Entry.Expiry = World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn]
		{
			for (const auto& E : Entries) if (E.Pawn == WeakPawn.Get() && E.bClaiming) { DestroyOwnedPawn(WeakPawn.Get()); break; }
		}));
		if (auto* Component = Pawn->FindComponentByClass<UCharacterPoolComponent>()) Component->MarkReconnectClaim();
		++Reconnected;
		return Pawn;
	}
	return nullptr;
}

void UCharacterPoolSubsystem::ForgetPawn(APawn* Pawn)
{
	for (int32 I = Entries.Num() - 1; I >= 0; --I)
	{
		if (Entries[I].Pawn != Pawn) continue;
		if (Pawn) Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::OnClaimedControllerChanged);
		if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(Entries[I].Expiry);
		if (AController* Controller = Entries[I].Controller.Get()) Controller->OnDestroyed.RemoveDynamic(this, &ThisClass::OnControllerDestroyed);
		Entries.RemoveAtSwap(I);
	}
}

void UCharacterPoolSubsystem::OnClaimedControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (!IsValid(NewController)) return;
	BindController(Pawn, NewController);
	Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::OnClaimedControllerChanged);
	const auto* Life = Pawn->FindComponentByClass<UCharacterPoolComponent>();
	if (!Life || !Life->IsManaged()) ForgetPawn(Pawn);
}

void UCharacterPoolSubsystem::DestroyOwnedPawn(APawn* Pawn)
{
	ForgetPawn(Pawn);
	if (IsValid(Pawn) && !Pawn->IsActorBeingDestroyed()) Pawn->Destroy();
}

void UCharacterPoolSubsystem::OnControllerDestroyed(AActor* Actor)
{
	APawn* Pawn = nullptr;
	for (const auto& Entry : Entries)
		if (Entry.Controller.Get(true) == Actor) { Pawn = Entry.Pawn; break; }
	if (!Pawn || !GetWorld() || GetWorld()->bIsTearingDown) return;
	for (const auto& Entry : Entries)
		if (Entry.Pawn == Pawn && (!Entry.ReconnectToken.IsEmpty() || Entry.bClaiming)) return;
	const TWeakObjectPtr<APawn> WeakPawn(Pawn);
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, WeakPawn]
	{
		if (WeakPawn.IsValid()) DestroyOwnedPawn(WeakPawn.Get());
	}));
}

FCharacterPoolStats UCharacterPoolSubsystem::GetPoolStats() const
{
	FCharacterPoolStats Stats;
	Stats.Created = Created; Stats.Reused = Reused; Stats.Reconnected = Reconnected;
	for (const auto& E : Entries) if (IsValid(E.Pawn) && !E.Pawn->IsActorBeingDestroyed())
	{
		++Stats.OwnedPawns;
		if (!E.ReconnectToken.IsEmpty()) ++Stats.ReservedPawns;
	}
	return Stats;
}

void UCharacterPoolSubsystem::Deinitialize()
{
	for (auto& E : Entries)
	{
		if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(E.Expiry);
		if (AController* Controller = E.Controller.Get()) Controller->OnDestroyed.RemoveDynamic(this, &ThisClass::OnControllerDestroyed);
	}
	Entries.Reset();
	Super::Deinitialize();
}

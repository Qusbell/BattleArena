#include "Audio/CharacterSpawnSoundComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/GameInstance.h"
#include "Engine/NetConnection.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Loading/LoadingScreenSubsystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterSpawnSound, Log, All);

namespace
{
	constexpr uint8 SharedInitialAudience = 1;
	constexpr uint8 OwnerInitialAudience = 2;
	constexpr float InitialReadinessPollSeconds = 0.05f;
}

UCharacterSpawnSoundComponent::UCharacterSpawnSoundComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCharacterSpawnSoundComponent::PlaySpawnSounds(ECharacterSpawnSoundPhase Phase)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const UWorld* World = GetWorld();
	if (!IsValid(Pawn) || !Pawn->HasAuthority() || !IsValid(Pawn->GetController())
		|| !IsValid(World) || World->bIsTearingDown)
	{
		return;
	}

	if (Phase == ECharacterSpawnSoundPhase::Initial)
	{
		if (bInitialSpawnRequested || InitialSpawnSounds.IsEmpty())
		{
			return;
		}
		bInitialSpawnRequested = true;
		bInitialServerDispatchPending = true;
		InitialSpawnController = Pawn->GetController();
		InitialSpawnLocation = Pawn->GetActorLocation();
		GetWorld()->GetTimerManager().SetTimer(InitialServerRetryTimer, this,
			&ThisClass::TryDispatchInitialSpawnSounds, InitialReadinessPollSeconds, true);
		UE_LOG(LogCharacterSpawnSound, Log, TEXT("Initial spawn sound queued on server: %s (%s)."),
			*GetNameSafe(Pawn), *World->GetName());
		TryDispatchInitialSpawnSounds();
		return;
	}

	DispatchSpawnSounds(Phase, Pawn->GetActorLocation());
}

bool UCharacterSpawnSoundComponent::HasReadyInitialConnection(const UNetConnection* Connection)
{
	// Reliable multicast still excludes a connection with no ViewTarget during PostLogin.
	return IsValid(Connection) && Connection->GetConnectionState() == USOCK_Open
		&& IsValid(Connection->ViewTarget);
}

void UCharacterSpawnSoundComponent::TryDispatchInitialSpawnSounds()
{
	UWorld* World = GetWorld();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!bInitialServerDispatchPending || !IsValid(World)) return;
	if (World->bIsTearingDown || !IsValid(Pawn) || !Pawn->HasAuthority()
		|| !InitialSpawnController.IsValid() || Pawn->GetController() != InitialSpawnController.Get())
	{
		bInitialServerDispatchPending = false;
		World->GetTimerManager().ClearTimer(InitialServerRetryTimer);
		return;
	}

	if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()); PC && !PC->IsLocalController())
	{
		if (!HasReadyInitialConnection(PC->GetNetConnection()) || !PC->HasClientLoadedCurrentWorld())
		{
			return;
		}
	}

	bInitialServerDispatchPending = false;
	World->GetTimerManager().ClearTimer(InitialServerRetryTimer);
	UE_LOG(LogCharacterSpawnSound, Log, TEXT("Initial spawn sound dispatched: %s (%s)."),
		*GetNameSafe(Pawn), *World->GetName());
	DispatchSpawnSounds(ECharacterSpawnSoundPhase::Initial, InitialSpawnLocation);
}

void UCharacterSpawnSoundComponent::DispatchSpawnSounds(ECharacterSpawnSoundPhase Phase, const FVector& SpawnLocation)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!IsValid(Pawn)) return;
	const TArray<FCharacterSpawnSoundEntry>& Entries = Phase == ECharacterSpawnSoundPhase::Initial
		? InitialSpawnSounds : RespawnSounds;
	bool bHasSharedSound = false;
	bool bHasOwnerSound = false;
	for (const FCharacterSpawnSoundEntry& Entry : Entries)
	{
		if (IsValid(Entry.Sound))
		{
			if (!Entry.Sound->IsOneShot())
			{
				UE_LOG(LogCharacterSpawnSound, Warning,
					TEXT("Looping spawn sound skipped: %s on %s."),
					*GetNameSafe(Entry.Sound), *GetNameSafe(Pawn));
				continue;
			}
			bHasSharedSound |= Entry.bAudibleToOthers;
			bHasOwnerSound |= !Entry.bAudibleToOthers;
		}
	}

	if (bHasSharedSound)
	{
		MulticastPlaySpawnSounds(Phase, SpawnLocation);
	}
	if (bHasOwnerSound && Pawn->IsPlayerControlled())
	{
		ClientPlaySpawnSounds(Phase, SpawnLocation);
	}
}

void UCharacterSpawnSoundComponent::MulticastPlaySpawnSounds_Implementation(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation)
{
	if (Phase == ECharacterSpawnSoundPhase::Initial)
	{
		QueueInitialSpawnSounds(SpawnLocation, true);
		return;
	}
	PlayLocalSpawnSounds(Phase, SpawnLocation, true);
}

void UCharacterSpawnSoundComponent::ClientPlaySpawnSounds_Implementation(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation)
{
	if (Phase == ECharacterSpawnSoundPhase::Initial)
	{
		QueueInitialSpawnSounds(SpawnLocation, false);
		return;
	}
	PlayLocalSpawnSounds(Phase, SpawnLocation, false);
}

void UCharacterSpawnSoundComponent::QueueInitialSpawnSounds(const FVector& SpawnLocation, bool bAudibleToOthers)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer) return;
	const uint8 Audience = bAudibleToOthers ? SharedInitialAudience : OwnerInitialAudience;
	if ((PlayedInitialAudience & Audience) != 0) return;
	PendingInitialAudience |= Audience;
	PendingInitialLocation = SpawnLocation;
	if (!World->GetTimerManager().IsTimerActive(InitialLocalRetryTimer))
	{
		World->GetTimerManager().SetTimer(InitialLocalRetryTimer, this,
			&ThisClass::TryPlayPendingInitialSpawnSounds, InitialReadinessPollSeconds, true);
	}
	TryPlayPendingInitialSpawnSounds();
}

bool UCharacterSpawnSoundComponent::IsLocalInitialPlaybackReady()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || !World->HasBegunPlay() || !World->bAllowAudioPlayback)
	{
		return false;
	}
	UGameInstance* GI = World->GetGameInstance();
	const ULoadingScreenSubsystem* Loading = GI ? GI->GetSubsystem<ULoadingScreenSubsystem>() : nullptr;
	if (Loading && Loading->IsLoadingScreenActive()) return false;
	APlayerController* LocalPC = GI ? GI->GetFirstLocalPlayerController(World) : nullptr;
	APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;
	if (!IsValid(LocalPC) || !LocalPC->IsLocalController() || !IsValid(LocalPawn)
		|| !LocalPawn->HasActorBegunPlay() || !IsValid(LocalPC->PlayerCameraManager)
		|| LocalPC->GetViewTarget() != LocalPawn)
	{
		return false;
	}
	if (ObservedLocalPawn.Get() != LocalPawn)
	{
		ObservedLocalPawn = LocalPawn;
		LocalPawnObservedAt = World->GetTimeSeconds();
		return false;
	}
	// Possess can change ViewTarget before the first camera/audio-listener update.
	return World->GetTimeSeconds() > LocalPawnObservedAt
		&& LocalPC->PlayerCameraManager->GetCameraCacheTime() > LocalPawnObservedAt;
}

void UCharacterSpawnSoundComponent::TryPlayPendingInitialSpawnSounds()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || PendingInitialAudience == 0 || !IsLocalInitialPlaybackReady()) return;
	const uint8 Audience = PendingInitialAudience;
	PendingInitialAudience = 0;
	PlayedInitialAudience |= Audience; // Consume before playback to reject repeated callbacks/RPCs.
	World->GetTimerManager().ClearTimer(InitialLocalRetryTimer);
	UE_LOG(LogCharacterSpawnSound, Log, TEXT("Initial spawn sound ready on client: %s (%s), audience=%u."),
		*GetNameSafe(GetOwner()), *World->GetName(), static_cast<uint32>(Audience));
	if ((Audience & SharedInitialAudience) != 0)
	{
		PlayLocalSpawnSounds(ECharacterSpawnSoundPhase::Initial, PendingInitialLocation, true);
	}
	if ((Audience & OwnerInitialAudience) != 0)
	{
		PlayLocalSpawnSounds(ECharacterSpawnSoundPhase::Initial, PendingInitialLocation, false);
	}
}

void UCharacterSpawnSoundComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitialServerRetryTimer);
		World->GetTimerManager().ClearTimer(InitialLocalRetryTimer);
		for (FTimerHandle& Handle : DelayedSoundTimers) World->GetTimerManager().ClearTimer(Handle);
	}
	bInitialServerDispatchPending = false;
	PendingInitialAudience = 0;
	DelayedSoundTimers.Reset();
	Super::EndPlay(EndPlayReason);
}

void UCharacterSpawnSoundComponent::PlayLocalSpawnSounds(ECharacterSpawnSoundPhase Phase, const FVector& SpawnLocation, bool bAudibleToOthers)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const TArray<FCharacterSpawnSoundEntry>& Entries = Phase == ECharacterSpawnSoundPhase::Initial
		? InitialSpawnSounds : RespawnSounds;
	for (const FCharacterSpawnSoundEntry& Entry : Entries)
	{
		if (!IsValid(Entry.Sound) || !Entry.Sound->IsOneShot()
			|| Entry.bAudibleToOthers != bAudibleToOthers)
		{
			continue;
		}
		if (!FMath::IsFinite(Entry.VolumeMultiplier) || !FMath::IsFinite(Entry.DelaySeconds))
		{
			UE_LOG(LogCharacterSpawnSound, Warning,
				TEXT("Spawn sound skipped: invalid volume or delay on %s."), *GetNameSafe(GetOwner()));
			continue;
		}

		if (Entry.DelaySeconds <= 0.0f)
		{
			PlayLocalEntry(Entry, SpawnLocation);
			continue;
		}

		FTimerHandle TimerHandle;
		TWeakObjectPtr<UCharacterSpawnSoundComponent> WeakThis(this);
		// A lambda's copied USTRUCT is not visited by GC if Blueprint replaces the source array.
		TStrongObjectPtr<USoundBase> PendingSound(Entry.Sound.Get());
		TStrongObjectPtr<USoundAttenuation> PendingAttenuation(Entry.AttenuationSettings.Get());
		World->GetTimerManager().SetTimer(TimerHandle,
			[WeakThis, Entry, SpawnLocation, PendingSound, PendingAttenuation]()
			{
				if (UCharacterSpawnSoundComponent* Component = WeakThis.Get();
					Component && Component->IsRegistered() && IsValid(Component->GetOwner()))
				{
					FCharacterSpawnSoundEntry PlaybackEntry = Entry;
					PlaybackEntry.Sound = PendingSound.Get();
					PlaybackEntry.AttenuationSettings = PendingAttenuation.Get();
					Component->PlayLocalEntry(PlaybackEntry, SpawnLocation);
				}
			}, FMath::Max(0.0f, Entry.DelaySeconds), false);
		DelayedSoundTimers.Add(TimerHandle);
	}
}

void UCharacterSpawnSoundComponent::PlayLocalEntry(const FCharacterSpawnSoundEntry& Entry, const FVector& SpawnLocation)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || !IsValid(Entry.Sound)
		|| World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const float Volume = FMath::Max(0.0f, Entry.VolumeMultiplier);
	if (Entry.Space == ECharacterSpawnSoundSpace::TwoDimensional)
	{
		UGameplayStatics::PlaySound2D(World, Entry.Sound, Volume);
	}
	else
	{
		if (!IsValid(Default3DAttenuation))
		{
			Default3DAttenuation = NewObject<USoundAttenuation>(this);
		}
		USoundAttenuation* Attenuation = IsValid(Entry.AttenuationSettings)
			? Entry.AttenuationSettings.Get() : Default3DAttenuation.Get();
		UGameplayStatics::PlaySoundAtLocation(World, Entry.Sound, SpawnLocation,
			Volume, 1.0f, 0.0f, Attenuation);
	}
}

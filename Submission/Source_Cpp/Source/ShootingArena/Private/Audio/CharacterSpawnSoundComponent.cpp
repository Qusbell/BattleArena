#include "Audio/CharacterSpawnSoundComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogCharacterSpawnSound, Log, All);

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

	const FVector SpawnLocation = Pawn->GetActorLocation();
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
	PlayLocalSpawnSounds(Phase, SpawnLocation, true);
}

void UCharacterSpawnSoundComponent::ClientPlaySpawnSounds_Implementation(ECharacterSpawnSoundPhase Phase, FVector SpawnLocation)
{
	PlayLocalSpawnSounds(Phase, SpawnLocation, false);
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
		World->GetTimerManager().SetTimer(TimerHandle,
			[WeakThis, Entry, SpawnLocation]()
			{
				if (UCharacterSpawnSoundComponent* Component = WeakThis.Get();
					Component && Component->IsRegistered() && IsValid(Component->GetOwner()))
				{
					Component->PlayLocalEntry(Entry, SpawnLocation);
				}
			}, FMath::Max(0.0f, Entry.DelaySeconds), false);
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

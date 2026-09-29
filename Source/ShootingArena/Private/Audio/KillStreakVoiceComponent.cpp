#include "Audio/KillStreakVoiceComponent.h"

#include "Audio/KillStreakSettingsDataAsset.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogKillStreakVoice, Log, All);

UKillStreakVoiceComponent::UKillStreakVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UKillStreakVoiceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Cast<APlayerState>(GetOwner()))
	{
		UE_LOG(LogKillStreakVoice, Error,
			TEXT("Kill-streak voice component must be owned by a PlayerState: %s."),
			*GetNameSafe(GetOwner()));
	}
}

void UKillStreakVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(KillBatchTimerHandle);
	}

	if (IsValid(ActiveVoiceComponent))
	{
		ActiveVoiceComponent->OnAudioFinished.RemoveDynamic(this, &ThisClass::HandleActiveVoiceFinished);
		ActiveVoiceComponent->Stop();
		ActiveVoiceComponent = nullptr;
	}

	PendingKillVoice = nullptr;
	Super::EndPlay(EndPlayReason);
}

void UKillStreakVoiceComponent::QueueKillEvent()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !Owner->HasAuthority() || !IsValid(World) || World->bIsTearingDown)
	{
		return;
	}

	if (PendingKillEventCount < MAX_int32)
	{
		++PendingKillEventCount;
	}

	// The first event starts the fixed window. Further events are collected without extending it.
	if (PendingKillEventCount > 1)
	{
		return;
	}
	KillBatchStartTime = World->GetTimeSeconds();

	const float Delay = IsValid(Settings) && FMath::IsFinite(Settings->KillEventBatchDelay)
		? FMath::Max(0.0f, Settings->KillEventBatchDelay)
		: 0.2f;

	if (Delay <= 0.0f)
	{
		ProcessKillBatch();
		return;
	}

	World->GetTimerManager().SetTimer(
		KillBatchTimerHandle,
		this,
		&ThisClass::ProcessKillBatch,
		Delay,
		false);
}

void UKillStreakVoiceComponent::NotifyOwnerDeath()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(KillBatchTimerHandle);
	}

	PendingKillEventCount = 0;
	CurrentStreakCount = 0;
	KillBatchStartTime = 0.0f;

	const APlayerState* PlayerState = Cast<APlayerState>(Owner);
	APlayerController* PlayerController = PlayerState ? PlayerState->GetPlayerController() : nullptr;
	if (PlayerController && PlayerController->IsLocalController())
	{
		ClearPendingVoiceLocally();
	}
	else
	{
		ClientClearPendingKillStreakVoice();
	}
}

void UKillStreakVoiceComponent::ProcessKillBatch()
{
	const int32 KillEventsInBatch = PendingKillEventCount;
	const float BatchStartTime = KillBatchStartTime;
	PendingKillEventCount = 0;
	KillBatchStartTime = 0.0f;
	if (KillEventsInBatch <= 0)
	{
		return;
	}

	CurrentStreakCount = static_cast<int32>(FMath::Min<int64>(
		static_cast<int64>(CurrentStreakCount) + KillEventsInBatch,
		MAX_int32));

	USoundBase* SelectedSound = SelectVoiceForCurrentStreak();
	if (!IsValid(SelectedSound))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (BatchStartTime < NextVoiceAllowedTime)
	{
		return;
	}

	const float OverlapTime = IsValid(Settings) && FMath::IsFinite(Settings->KillVoiceOverlapTime)
		? FMath::Max(0.0f, Settings->KillVoiceOverlapTime)
		: 0.0f;
	NextVoiceAllowedTime = CurrentTime + OverlapTime;
	DispatchVoiceToOwner(SelectedSound);
}

USoundBase* UKillStreakVoiceComponent::SelectVoiceForCurrentStreak() const
{
	if (!IsValid(Settings) || Settings->KillStreakTiers.IsEmpty() || CurrentStreakCount <= 0)
	{
		return nullptr;
	}

	const FKillStreakVoiceTier* ExactTier = nullptr;
	const FKillStreakVoiceTier* HighestTier = nullptr;
	for (const FKillStreakVoiceTier& Tier : Settings->KillStreakTiers)
	{
		if (Tier.RequiredKillCount <= 0)
		{
			continue;
		}

		if (Tier.RequiredKillCount == CurrentStreakCount && !ExactTier)
		{
			ExactTier = &Tier;
		}

		if (!HighestTier || Tier.RequiredKillCount > HighestTier->RequiredKillCount)
		{
			HighestTier = &Tier;
		}
	}

	const FKillStreakVoiceTier* SelectedTier = ExactTier;
	if (!SelectedTier && HighestTier && CurrentStreakCount > HighestTier->RequiredKillCount)
	{
		SelectedTier = HighestTier;
	}

	if (!SelectedTier)
	{
		return nullptr;
	}

	TArray<USoundBase*> ValidSounds;
	for (USoundBase* Sound : SelectedTier->Sounds)
	{
		if (!IsValid(Sound))
		{
			continue;
		}

		if (!Sound->IsOneShot())
		{
			UE_LOG(LogKillStreakVoice, Warning,
				TEXT("Looping kill-streak voice skipped: %s on %s."),
				*GetNameSafe(Sound), *GetNameSafe(GetOwner()));
			continue;
		}

		ValidSounds.Add(Sound);
	}

	if (ValidSounds.IsEmpty())
	{
		return nullptr;
	}

	return ValidSounds[FMath::RandHelper(ValidSounds.Num())];
}

void UKillStreakVoiceComponent::DispatchVoiceToOwner(USoundBase* Sound)
{
	if (!IsValid(Sound))
	{
		return;
	}

	const APlayerState* PlayerState = Cast<APlayerState>(GetOwner());
	if (!PlayerState)
	{
		UE_LOG(LogKillStreakVoice, Warning,
			TEXT("Kill-streak voice component must be owned by a PlayerState: %s."),
			*GetNameSafe(GetOwner()));
		return;
	}

	APlayerController* PlayerController = PlayerState->GetPlayerController();
	if (!PlayerController)
	{
		return;
	}

	if (PlayerController->IsLocalController())
	{
		PlayVoiceLocally(Sound);
	}
	else if (GetOwner()->HasAuthority())
	{
		ClientPlayKillStreakVoice(Sound);
	}
}

void UKillStreakVoiceComponent::ClientPlayKillStreakVoice_Implementation(USoundBase* Sound)
{
	PlayVoiceLocally(Sound);
}

void UKillStreakVoiceComponent::ClientClearPendingKillStreakVoice_Implementation()
{
	ClearPendingVoiceLocally();
}

void UKillStreakVoiceComponent::PlayVoiceLocally(USoundBase* Sound)
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer
		|| !IsValid(Sound) || !Sound->IsOneShot())
	{
		return;
	}

	if (IsValid(ActiveVoiceComponent) && ActiveVoiceComponent->IsPlaying())
	{
		// Only one pending voice is retained; the newest eligible cue replaces the prior one.
		PendingKillVoice = Sound;
		return;
	}

	if (IsValid(ActiveVoiceComponent))
	{
		ActiveVoiceComponent->OnAudioFinished.RemoveDynamic(this, &ThisClass::HandleActiveVoiceFinished);
		ActiveVoiceComponent = nullptr;
	}

	ActiveVoiceComponent = UGameplayStatics::SpawnSound2D(World, Sound);
	if (IsValid(ActiveVoiceComponent))
	{
		ActiveVoiceComponent->OnAudioFinished.AddUniqueDynamic(this, &ThisClass::HandleActiveVoiceFinished);
	}
}

void UKillStreakVoiceComponent::ClearPendingVoiceLocally()
{
	PendingKillVoice = nullptr;
}

void UKillStreakVoiceComponent::HandleActiveVoiceFinished()
{
	if (IsValid(ActiveVoiceComponent))
	{
		ActiveVoiceComponent->OnAudioFinished.RemoveDynamic(this, &ThisClass::HandleActiveVoiceFinished);
		ActiveVoiceComponent = nullptr;
	}

	USoundBase* VoiceToPlay = PendingKillVoice;
	PendingKillVoice = nullptr;
	if (IsValid(VoiceToPlay))
	{
		PlayVoiceLocally(VoiceToPlay);
	}
}

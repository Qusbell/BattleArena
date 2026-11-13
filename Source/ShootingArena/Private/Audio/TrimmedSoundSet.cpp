#include "Audio/TrimmedSoundSet.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

const FTrimmedSoundEntry* UTrimmedSoundSet::FindEntry(FName Id) const
{
	return Sounds.FindByPredicate([Id](const FTrimmedSoundEntry& Entry) { return Entry.Id == Id; });
}

UAudioComponent* UTrimmedSoundLibrary::PlayTrimmedSound2D(
	const UObject* WorldContextObject, const UTrimmedSoundSet* SoundSet, FName Id)
{
	if (!IsValid(SoundSet))
	{
		return nullptr;
	}
	const FTrimmedSoundEntry* Entry = SoundSet->FindEntry(Id);
	if (!Entry)
	{
		UE_LOG(LogTemp, Warning, TEXT("TrimmedSoundSet %s에 Id '%s' 항목이 없습니다."),
			*GetNameSafe(SoundSet), *Id.ToString());
		return nullptr;
	}
	return PlayTrimmedEntry2D(WorldContextObject, *Entry);
}

UAudioComponent* UTrimmedSoundLibrary::PlayTrimmedEntry2D(
	const UObject* WorldContextObject, const FTrimmedSoundEntry& Entry)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!IsValid(World) || World->bIsTearingDown || !IsValid(Entry.Sound)
		|| World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	const float Duration = Entry.Sound->GetDuration();
	const bool bLooping = Duration >= INDEFINITELY_LOOPING_DURATION || Entry.Sound->IsLooping();

	// 양수 오프셋은 소리 앞부분 삭제, 음수 오프셋은 재생 전 대기입니다.
	float StartTime = FMath::Max(0.0f, Entry.StartOffsetSeconds);
	const float PlayDelay = FMath::Max(0.0f, -Entry.StartOffsetSeconds);
	if (!bLooping)
	{
		// 전체를 잘라내 재생할 구간이 남지 않는 경우를 막습니다.
		StartTime = FMath::Min(StartTime, FMath::Max(0.0f, Duration - 0.01f));
	}

	UAudioComponent* Audio = UGameplayStatics::CreateSound2D(World, Entry.Sound,
		FMath::Max(0.0f, Entry.VolumeMultiplier), FMath::Max(0.1f, Entry.PitchMultiplier),
		StartTime, nullptr, false, true);
	if (!Audio)
	{
		return nullptr;
	}

	// 재생 시작 후 끝까지 남은 시간에서 EndTrim을 뺀 시점에 페이드 아웃으로 멈춥니다.
	const float PlayLength = bLooping ? 0.0f
		: FMath::Max(0.0f, Duration - StartTime - FMath::Max(0.0f, Entry.EndTrimSeconds));
	const bool bTrimEnd = !bLooping && Entry.EndTrimSeconds > 0.0f;
	const float FadeOut = FMath::Max(0.0f, Entry.EndFadeOutSeconds);

	TWeakObjectPtr<UAudioComponent> WeakAudio(Audio);
	auto StartPlayback = [WeakAudio, StartTime, PlayLength, bTrimEnd, FadeOut]()
	{
		UAudioComponent* Component = WeakAudio.Get();
		if (!IsValid(Component))
		{
			return;
		}
		Component->Play(StartTime);
		UWorld* ComponentWorld = Component->GetWorld();
		if (bTrimEnd && IsValid(ComponentWorld))
		{
			FTimerHandle StopHandle;
			ComponentWorld->GetTimerManager().SetTimer(StopHandle,
				FTimerDelegate::CreateLambda([WeakAudio, FadeOut]()
				{
					if (UAudioComponent* Playing = WeakAudio.Get(); IsValid(Playing) && Playing->IsPlaying())
					{
						if (FadeOut > 0.0f)
						{
							Playing->FadeOut(FadeOut, 0.0f);
						}
						else
						{
							Playing->Stop();
						}
					}
				}), FMath::Max(PlayLength, KINDA_SMALL_NUMBER), false);
		}
	};

	if (PlayDelay > 0.0f)
	{
		FTimerHandle DelayHandle;
		World->GetTimerManager().SetTimer(DelayHandle, FTimerDelegate::CreateLambda(StartPlayback), PlayDelay, false);
	}
	else
	{
		StartPlayback();
	}
	return Audio;
}

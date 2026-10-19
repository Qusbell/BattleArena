// Fill out your copyright notice in the Description page of Project Settings.

#include "JumpPadCommonSettings.h"

#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void UJumpPadCommonSettings::PlayJumpPadSound(const UObject* WorldContextObject, FVector Location)
{
	// 데디케이티드 서버에는 오디오 디바이스가 없다.
	if (!JumpPadSound || SoundVolume <= 0.0f || IsRunningDedicatedServer())
	{
		return;
	}

	if (!RuntimeAttenuation)
	{
		RuntimeAttenuation = NewObject<USoundAttenuation>(this, NAME_None, RF_Transient);
	}

	// 에디터에서 DA 값을 바꿔도 다음 재생부터 반영되도록 매번 값을 다시 채운다.
	const float MaxDistance = FMath::Max(MaxHearingDistance, 1.0f);
	const float InnerRadius = FMath::Clamp(FullVolumeDistance, 0.0f, MaxDistance);

	FSoundAttenuationSettings& Settings = RuntimeAttenuation->Attenuation;
	Settings.bAttenuate = true;
	Settings.bSpatialize = true;
	Settings.AttenuationShape = EAttenuationShape::Sphere;
	Settings.AttenuationShapeExtents = FVector(InnerRadius, 0.0f, 0.0f);
	Settings.FalloffDistance = FMath::Max(MaxDistance - InnerRadius, 1.0f);
	Settings.DistanceAlgorithm = EAttenuationDistanceModel::Linear;

	// 길이 제한이 없으면 기존처럼 fire-and-forget 으로 재생한다.
	if (SoundPlayDuration <= 0.0f)
	{
		UGameplayStatics::PlaySoundAtLocation(
			WorldContextObject, JumpPadSound, Location, FRotator::ZeroRotator,
			SoundVolume, 1.0f, FMath::Max(SoundStartTime, 0.0f), RuntimeAttenuation);
		return;
	}

	UAudioComponent* AudioComp = UGameplayStatics::SpawnSoundAtLocation(
		WorldContextObject, JumpPadSound, Location, FRotator::ZeroRotator,
		SoundVolume, 1.0f, FMath::Max(SoundStartTime, 0.0f), RuntimeAttenuation);

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!AudioComp || !World)
	{
		return;
	}

	// 사운드가 먼저 끝나 컴포넌트가 사라졌으면 WeakPtr 이 무효가 되어 아무 일도 하지 않는다.
	const float FadeOut = SoundFadeOutTime;
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateLambda([WeakComp = TWeakObjectPtr<UAudioComponent>(AudioComp), FadeOut]()
		{
			UAudioComponent* Comp = WeakComp.Get();
			if (!Comp || !Comp->IsPlaying())
			{
				return;
			}

			if (FadeOut > 0.0f)
			{
				Comp->FadeOut(FadeOut, 0.0f);
			}
			else
			{
				Comp->Stop();
			}
		}),
		SoundPlayDuration, false);
}

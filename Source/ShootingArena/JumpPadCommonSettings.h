// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "JumpPadCommonSettings.generated.h"

class USoundBase;
class USoundAttenuation;

/**
 * 모든 점프패드가 공유하는 공통 설정.
 * 패드마다 값이 갈리지 않는 항목만 여기에 둔다. (패드 고유 값 - 목표 지점, ApexTime 등 - 은 패드 액터에 유지)
 *
 * 사용 흐름:
 *   1) 콘텐츠 브라우저에서 이 클래스로 Data Asset 을 1개 생성 (예: DA_JumpPadCommon)
 *   2) 점프패드 BP 에 이 타입의 변수(예: CommonSettings)를 두고 위 에셋을 기본값으로 지정
 *   3) BP 에서 SetInputStopTimer 의 Duration 을 CommonSettings->IgnoreInputTime 으로 연결
 *   4) 발사 직후 CommonSettings->PlayJumpPadSound(패드 위치) 호출 → 3D One Shot 사운드 재생
 */
UCLASS(BlueprintType)
class SHOOTINGARENA_API UJumpPadCommonSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * 점프패드로 발사된 직후 캐릭터 입력을 무시할 시간(초).
	 * BPI_BodyInfo::SetInputStopTimer 로 넘어간다. 모든 점프패드 공통.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpPad",
		meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "s"))
	float IgnoreInputTime = 0.3f;

	/** 점프패드 사용 시 패드 위치에서 재생할 사운드. 비워두면 재생하지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpPad|Sound")
	TObjectPtr<USoundBase> JumpPadSound;

	/**
	 * 사운드 볼륨 배율. FullVolumeDistance 안쪽에서 이 볼륨으로 재생된다.
	 * 사운드 에셋/BP 는 건드리지 말고 이 값으로만 크기를 조절할 것. 0 이면 무음.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpPad|Sound",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0"))
	float SoundVolume = 1.0f;

	/**
	 * 사운드를 이 시간(초)만큼 건너뛴 지점부터 재생한다.
	 * 파일 앞부분에 무음이 있어 소리가 늦게 들릴 때 그 길이만큼 넣는다. (JumpPad.mp3 는 약 0.25)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpPad|Sound",
		meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "s"))
	float SoundStartTime = 0.0f;

	/** 최대 청취 거리(cm). 패드에서 이 거리 이상 떨어지면 들리지 않는다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpPad|Sound",
		meta = (ClampMin = "1.0", UIMin = "1.0", ForceUnits = "cm"))
	float MaxHearingDistance = 3000.0f;

	/**
	 * 이 거리(cm) 안쪽에서는 감쇠 없이 SoundVolume 그대로 재생되고,
	 * 여기서부터 MaxHearingDistance 까지 선형으로 줄어들어 0 이 된다.
	 * MaxHearingDistance 보다 크게 잡아도 MaxHearingDistance 로 제한된다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "JumpPad|Sound",
		meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "cm"))
	float FullVolumeDistance = 300.0f;

	/**
	 * Location(보통 점프패드 월드 위치)에서 JumpPadSound 를 3D One Shot 으로 재생한다.
	 * 볼륨/거리 감쇠는 위 설정값을 그대로 사용한다. 데디케이티드 서버에서는 아무 일도 하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "JumpPad|Sound",
		meta = (WorldContext = "WorldContextObject"))
	void PlayJumpPadSound(const UObject* WorldContextObject, FVector Location);

private:
	/** 위 거리 값으로 만든 런타임 감쇠 설정. 저장되지 않고 재생 시마다 값만 갱신한다. */
	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> RuntimeAttenuation;
};

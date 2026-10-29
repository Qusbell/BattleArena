#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MatchRulesInfo.generated.h"

/**
 * 멀티 로비에서 정한 규칙(제한 시간 / 목표 킬 수)을 매치에 적용하는 복제 액터입니다.
 *
 * - 서버: UMatchRulesSubsystem 이 매치 시작 시 생성합니다.
 * - 모든 머신: BeginPlay 에서 GameState 의 GameConfig 를 복제본으로 바꿔 MatchDuration / MaxScore 를 덮어쓰고
 *   EndKillPoint 를 맞춥니다. (원본 DA_MatchSetting 은 건드리지 않습니다.)
 * - 서버: 목표 킬 수가 있으면 주기적으로 PlayerState 의 KillCount 를 확인해 GameMode 의 EndMatch 를 호출합니다.
 *
 * 블루프린트(GameState / GameMode / PlayerState)는 수정하지 않고 리플렉션으로 이름을 찾아 씁니다.
 */
UCLASS(NotBlueprintable)
class SHOOTINGARENA_API AMatchRulesInfo : public AInfo
{
	GENERATED_BODY()

public:
	AMatchRulesInfo();

	// 제한 시간(초). 0 이하면 덮어쓰지 않습니다.
	UPROPERTY(Replicated)
	int32 MatchDurationSeconds = 0;

	// 목표 킬 수. 0 이하면 킬 조건이 없습니다.
	UPROPERTY(Replicated)
	int32 KillTarget = 0;

	// true 면 시간 제한이 없습니다. (MatchDurationSeconds 는 사실상 끝나지 않는 큰 값) HUD 는 타이머를 만들지 않습니다.
	UPROPERTY(Replicated)
	bool bUnlimitedTime = false;

	// 시간 제한 없음일 때 MatchDurationSeconds 에 넣는 값(1년).
	static constexpr int32 UnlimitedDurationSeconds = 31536000;

	// 서버 월드 시간(GameState::GetServerWorldTimeSeconds 기준)으로 본 매치 시작 시각. 0 이면 아직 시작 전입니다.
	// 클라이언트 HUD 타이머가 서로 같은 남은 시간을 보여주는 데 씁니다.
	UPROPERTY(Replicated)
	double MatchStartServerTime = 0.0;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// 클라이언트에서 이 순간의 남은 시간을 추정합니다. 복제값이 아직 없으면 음수를 돌려줍니다.
	float GetClientRemainingEstimate() const;

private:
	// 서버가 센 매치 남은 시간(초). 서버가 주기적으로 갱신해 복제하고, 클라이언트는 받은 순간부터 실제 시간으로 줄여서 씁니다.
	// 월드 시간(GetTimeSeconds)은 로딩 렉으로 실제보다 늦게 가므로 쓰지 않습니다.
	UPROPERTY(ReplicatedUsing = OnRep_ServerRemainingSeconds)
	float ServerRemainingSeconds = -1.0f;

	UFUNCTION()
	void OnRep_ServerRemainingSeconds();


	void ServerTick();
	void RestartClientHudTimers();
	void TryApplyToGameState();
	void CheckKillTarget();
	void EndMatchOnServer();

	double RemainingAnchorValue = -1.0;
	double RemainingAnchorRealTime = 0.0;
	bool bClientHudRestarted = false;
	double LastHudRestartRealTime = 0.0;
	double LastRemainingSendTime = 0.0;

	FTimerHandle ApplyTimer;
	FTimerHandle KillCheckTimer;
	bool bApplied = false;
};

/** 멀티 매치 규칙(AMatchRulesInfo)을 읽는 블루프린트 헬퍼. */
UCLASS()
class SHOOTINGARENA_API UMatchRulesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 매치 남은 시간(초). 서버 시계를 기준으로 계산해서 모든 클라이언트가 같은 값을 얻습니다.
	 * 멀티 규칙 액터가 없거나 매치가 아직 시작 전이면 DefaultRemaining 을 그대로 돌려줍니다.
	 */
	UFUNCTION(BlueprintPure, Category = "MatchRules", meta = (WorldContext = "WorldContextObject"))
	static float GetMatchRemainingSeconds(const UObject* WorldContextObject, float DefaultRemaining);

	/** 시간 제한이 없는 매치(싱글플레이 맵의 GameTimer 가 음수)면 true. HUD 타이머를 만들지 말지 정할 때 씁니다. */
	UFUNCTION(BlueprintPure, Category = "MatchRules", meta = (WorldContext = "WorldContextObject"))
	static bool IsMatchTimeUnlimited(const UObject* WorldContextObject);
};

/** 멀티 매치(GameMode 이름에 "MultiplayerAIGameMode" 포함)가 시작될 때 AMatchRulesInfo 를 만듭니다. 서버 전용. */
UCLASS()
class SHOOTINGARENA_API UMatchRulesSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};

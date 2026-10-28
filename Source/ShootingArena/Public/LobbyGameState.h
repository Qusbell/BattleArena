#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LobbyTypes.h"
#include "LobbyGameState.generated.h"

class ALobbyPlayerState;
class ALobbyPlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLobbyStateChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLobbyRuleChangedSignature, ELobbyRuleId, RuleId);

/**
 * 멀티 로비의 공유 상태. 서버가 권한을 갖고, 모든 클라이언트는 같은 FLobbySnapshot 을 받아 UI 에 그립니다.
 *
 * 값 전달 방식: 서버가 바뀔 때마다 Multicast_SyncSnapshot 으로 통째로 보내고(실시간),
 * 접속 직후 놓친 값은 ALobbyPlayerController::Server_RequestLobbySync 가 다시 채웁니다(Pull).
 * Snapshot 자체도 Replicated 라서 첫 접속 때의 초기 값은 일반 복제로도 도착합니다.
 * (이 프로젝트는 Push Model 때문에 중간에 바뀐 일반 복제 값이 클라이언트에 안 닿습니다.)
 */
UCLASS()
class SHOOTINGARENA_API ALobbyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALobbyGameState();

	// ===== 모든 머신에서 읽는 값 =====

	UPROPERTY(ReplicatedUsing = OnRep_Snapshot, BlueprintReadOnly, Category = "Lobby")
	FLobbySnapshot Snapshot;

	// 서버 전용: 현재 방장. 클라이언트 UI 는 Snapshot.HostPlayerId 를 쓰세요.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TObjectPtr<APlayerState> HostPlayerState = nullptr;

	// 현재 맵 ID (BP_LobbyGameMode::OnLobbyStartGameApproved 가 읽습니다).
	UFUNCTION(BlueprintPure, Category = "Lobby")
	FString GetSelectedMapID() const { return Snapshot.MapId; }

	// ===== UI 에 노출하는 계산 값 (명세 §6) =====

	// 이 머신의 첫 번째 로컬 플레이어가 방장인지.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsLocalPlayerHost() const;

	// 플레이어(사람) 수.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetPlayerCount() const { return Snapshot.Players.Num(); }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetBotCount() const { return Snapshot.Bots.Num(); }

	// 플레이어 + 봇.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetFilledCount() const { return Snapshot.Players.Num() + Snapshot.Bots.Num(); }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetMaxPlayers() const { return Snapshot.MaxPlayers; }

	// 방장을 뺀 참가자 수.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetGuestCount() const;

	// 방장을 뺀 참가자 중 준비 완료한 수.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetReadyCount() const;

	// 방장을 뺀 참가자 전원이 준비했는지 (참가자 0명이면 true).
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool AreAllGuestsReady() const { return GetReadyCount() >= GetGuestCount(); }

	// 이 머신의 로컬 플레이어가 지금 보여야 하는 메인 버튼 상태.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	ELobbyMainButtonState GetMainButtonState() const;

	// 카드 목록을 순서대로 만들어 돌려줍니다: 플레이어(입장 순) → 봇(추가 순) → 빈자리(최대 인원까지).
	UFUNCTION(BlueprintPure, Category = "Lobby")
	TArray<FLobbyRosterEntry> GetRosterEntries() const;

	// 봇을 넣을 빈자리가 있는지 (Btn_AddBot 활성 조건).
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool HasEmptySeat() const { return GetFilledCount() < Snapshot.MaxPlayers; }

	// 후보 맵의 최대 인원이 현재 플레이어 수 이상이라 선택 가능한지 (맵 화살표/팝업에서 막을지 판단).
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool CanSelectMapWithMaxPlayers(int32 CandidateMaxPlayers) const { return CandidateMaxPlayers >= Snapshot.Players.Num(); }

	// 방장 다음으로 방장이 될 사람의 이름 (나가기 모달 본문용). 없으면 빈 문자열.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	FString GetNextHostName() const;

	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetRuleIndex(ELobbyRuleId RuleId) const { return RuleId == ELobbyRuleId::TimeLimit ? Snapshot.TimeIdx : Snapshot.KillIdx; }

	// 난이도별 봇 수. 매치가 AIEasy/AINormal/AIHard 를 읽을 때 씁니다.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	void CountBotsByDifficulty(int32& OutEasy, int32& OutNormal, int32& OutHard) const;

	// ===== UI 가 바인딩하는 이벤트 (모든 클라이언트 공통) =====

	// 스냅샷이 갱신될 때마다 (서버, 클라 모두). 위젯은 여기에 바인딩해서 전체를 새로 그리면 됩니다.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FLobbyStateChangedSignature OnSnapshotChanged;

	// 규칙이 바뀌었을 때 (모든 화면). 바뀐 행을 디자이너가 강조합니다. 준비는 해제되지 않습니다.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FLobbyRuleChangedSignature OnRuleChanged;

	// 맵이 실제로 바뀌었을 때.
	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FLobbyStateChangedSignature OnMapChanged;

	// Snapshot 이 갱신되면 (서버, 클라 모두) 호출됩니다. 이전 구조와 같은 이름의 BP 이벤트로,
	// BP_LobbyGameState 가 이미 구현하고 있는 경우를 위해 남겨두었습니다. 새 위젯은 OnSnapshotChanged 를 쓰세요.
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnLobbyStateChanged();

	// ===== 서버 전용 (GameMode / 방장 RPC 가 호출) =====

	// 로비를 처음 구성합니다. 이미 접속한 사람이 있어도 안전합니다.
	void InitLobby(const FString& DefaultMapId, int32 InMaxPlayers, int32 DefaultTimeIdx, int32 DefaultKillIdx);

	// 새 플레이어가 PostLogin 을 마쳤을 때. JoinOrder 를 매기고, 자리가 모자라면 봇을 뒤에서부터 제거합니다.
	void RegisterPlayer(ALobbyPlayerState* NewPlayer);

	// 플레이어가 나갔을 때. 남은 사람이 있으면 JoinOrder 가 가장 빠른 사람에게 방장을 넘깁니다.
	// bWasKicked 가 true 면 방장에게 'Kicked' 기록을, 아니면 'Left' 기록을 보냅니다.
	void UnregisterPlayer(ALobbyPlayerState* ExitingPlayer, bool bWasKicked);

	// 방장을 지정합니다 (사람이 없으면 nullptr). bAnnounce 가 true 면 새 방장에게 배너/기록을 보냅니다.
	// ExcludeFromRoster 는 이미 나가는 중이라 목록에서 빼야 하는 플레이어입니다 (Logout 시점).
	void SetHost(ALobbyPlayerState* NewHost, bool bAnnounce, const ALobbyPlayerState* ExcludeFromRoster = nullptr);

	// 스냅샷을 적용합니다. Version 이 이미 적용한 것 이하면 무시합니다. 규칙/맵 변경 이벤트도 여기서 호출합니다.
	// 서버는 Multicast 로, 클라이언트는 Multicast/OnRep/Pull 로 들어옵니다.
	void ApplySnapshot(const FLobbySnapshot& NewSnapshot);

	// 맵을 바꿉니다. 현재 플레이어 수 > NewMaxPlayers 면 false.
	// 맵이 실제로 바뀌면: 참가자 준비 해제 + 봇을 새 최대 인원에 맞게 뒤에서부터 제거 + 준비가 풀린 참가자에게 MapReset 배너.
	bool ChangeMap(const FString& NewMapId, int32 NewMaxPlayers);

	// 규칙 한 단계를 바꿉니다. 범위(0~OptionCount-1) 밖이면 false. 준비는 해제하지 않습니다.
	bool SetRuleIndex(ELobbyRuleId RuleId, int32 NewIndex, int32 OptionCount);

	// 빈자리가 있으면 CharacterPool 에서 무작위로 골라 봇을 추가합니다.
	bool AddBot(const TArray<FName>& CharacterPool, ELobbyDifficulty Difficulty);
	bool SetBotCharacter(int32 BotId, FName CharacterId);
	bool SetBotDifficulty(int32 BotId, ELobbyDifficulty Difficulty);
	bool RemoveBot(int32 BotId);

	// 참가자 준비 상태를 바꿉니다 (방장은 무시).
	void SetPlayerReady(ALobbyPlayerState* Player, bool bReady);

	// 현재 상태로 스냅샷을 다시 만들어 모두에게 보냅니다 (이름 변경처럼 GameState 밖에서 값이 바뀐 경우).
	void RefreshSnapshot() { RebuildAndBroadcast(); }

	// 스냅샷을 한 클라이언트에게만 다시 보냅니다 (접속 직후 Pull 용).
	void SendSnapshotTo(ALobbyPlayerController* Target);

	// 출격 카드에 넘길 값.
	FLobbyLaunchInfo BuildLaunchInfo() const;

	// 로비에 있는 사람(AI 제외, 등록 전인 사람 제외)의 PlayerState 를 JoinOrder 순으로 돌려줍니다. 서버 전용입니다
	// (JoinOrder 는 복제하지 않습니다 — 클라이언트는 Snapshot.Players 를 쓰세요).
	TArray<ALobbyPlayerState*> GetLobbyPlayersSorted(const ALobbyPlayerState* Exclude = nullptr) const;

	ALobbyPlayerState* FindLobbyPlayerById(int32 PlayerId) const;

	// 로비 상태를 서버에서 읽는 용도(RPC 검증 등).
	const TArray<FLobbyBot>& GetServerBots() const { return Bots; }

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Snapshot();

	// 서버가 모든 클라이언트(서버 자신 포함)에 스냅샷을 보냅니다.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SyncSnapshot(const FLobbySnapshot& NewSnapshot);

private:
	// 서버: 현재 상태로 스냅샷을 새로 만들어 Version 을 올리고 모두에게 보냅니다.
	// Exclude 는 이미 나가는 중이라 목록에서 빼야 하는 플레이어입니다.
	void RebuildAndBroadcast(const ALobbyPlayerState* Exclude = nullptr);

	// 서버: 로비의 모든 LobbyPlayerController 를 돌며 콜백을 실행합니다.
	void ForEachLobbyController(TFunctionRef<void(ALobbyPlayerController*)> Callback) const;

	// 서버: PlayerId 의 컨트롤러.
	ALobbyPlayerController* FindControllerByPlayerId(int32 PlayerId) const;

	// 이 머신의 첫 번째 로컬 플레이어 PlayerId. 없으면 INDEX_NONE.
	int32 GetLocalPlayerId() const;

	// ----- 서버 전용 상태 (복제하지 않음). 스냅샷의 원본입니다. -----
	FString MapId;
	int32 MaxPlayers = 0;
	int32 TimeIdx = 0;
	int32 KillIdx = 0;
	int32 RuleChangeSerial = 0;
	ELobbyRuleId LastChangedRule = ELobbyRuleId::TimeLimit;
	int32 MapChangeSerial = 0;
	int32 NextJoinOrder = 1;
	int32 NextBotId = 1;
	int32 SnapshotVersion = 0;
	TArray<FLobbyBot> Bots;

	// ----- 모든 머신: 마지막으로 적용한 값 (중복/역전 방지, 변경 감지) -----
	int32 AppliedVersion = 0;
	bool bHasAppliedSnapshot = false;
	int32 AppliedRuleSerial = 0;
	int32 AppliedMapSerial = 0;
};

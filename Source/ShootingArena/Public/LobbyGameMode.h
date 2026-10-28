#pragma once

#include "CoreMinimal.h"
#include "MyReconnectionGameMode.h"
#include "LobbyTypes.h"
#include "LobbyGameMode.generated.h"

class ALobbyGameState;
class ALobbyPlayerState;

/**
 * 멀티 로비(Lobby_Level) 전용 GameMode.
 * 입장 제한(PreLogin), 방장 지정/승계, 강퇴, 출격(ServerTravel)을 처리합니다.
 *
 * AMyReconnectionGameMode를 상속해서, 매치 서버로 접속할 때와 마찬가지로
 * "open IP:7777?Nickname=..." 로 넘어오는 라이브 닉네임을 InitNewPlayer 단계에서
 * 정상적으로 파싱/적용받습니다 (안 그러면 기본값인 "?Name=컴퓨터이름"으로 덮어써집니다).
 */
UCLASS()
class SHOOTINGARENA_API ALobbyGameMode : public AMyReconnectionGameMode
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	virtual void BeginPlay() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	// ===== 기본 설정 (BP_LobbyGameMode 클래스 기본값에서 조정 가능) =====

	// 봇으로 쓸 DT_Character 행 이름. 명세는 6종이며, 난이도별 Characteristic_ID 가 있는 C01~C06 을 씁니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Bots")
	TArray<FName> BotCharacterPool;

	// '봇 추가'로 생긴 봇의 난이도 (DT_MultiLobby 의 Bot_DefaultDifficulty = 1 → 노말).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Bots")
	ELobbyDifficulty DefaultBotDifficulty = ELobbyDifficulty::Normal;

	// 규칙별 단계 수 (DT_MultiRules 의 Options 개수와 같아야 합니다). 서버가 범위를 검증하는 데 씁니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Rules", meta = (ClampMin = "1"))
	int32 TimeLimitOptionCount = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Rules", meta = (ClampMin = "1"))
	int32 KillTargetOptionCount = 4;

	// 규칙 기본 단계 (DT_MultiRules 의 DefaultIndex).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Rules", meta = (ClampMin = "0"))
	int32 DefaultTimeIdx = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby|Rules", meta = (ClampMin = "0"))
	int32 DefaultKillIdx = 1;

	// ===== BP 에서 호출 =====

	// 서버가 로비 레벨을 시작할 때 (BP_LobbyGameMode의 BeginPlay 등에서) 최초 1회 호출해서
	// 기본 맵과 최대 인원으로 로비를 구성합니다. MaxPlayerCount 는 BP가 DT_MapData 에서 조회해 넘겨줍니다.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void InitializeLobby(const FString& DefaultMapID, int32 MaxPlayerCount);

	// 방장이 시작을 눌러 조건(방장을 뺀 참가자 전원 준비)을 통과하면 호출됩니다.
	// 실제 맵 에셋 조회(DataTable) 및 레벨 이동은 BP에서 구현하고 TravelToMatch 를 호출합니다.
	// 호출 전에 GameInstance 의 PendingAIDifficulties / PendingBotCharacterIds / PendingTimeIdx / PendingKillIdx 가
	// 이미 채워져 있으므로 BP 는 다시 세지 않아도 됩니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnLobbyStartGameApproved(const FString& SelectedMapID);

	// 통일 서버 모델: 이 서버 프로세스 자체를 매치 맵으로 ServerTravel 시킵니다. 접속된 모든 클라이언트가
	// 함께 매치로 이동하므로 클라이언트 쪽에서 open/ClientTravel 을 따로 할 필요가 없습니다.
	// MatchTravelURL 은 BP(OnLobbyStartGameApproved)가 DT_MapData 조회로 조립한 전체 URL:
	//   "<맵경로>?Game=/Game/QuakeLike_1_0/GameMode/BP_MultiplayerAIGameMode.BP_MultiplayerAIGameMode_C" + BuildMatchOptionsString()
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void TravelToMatch(const FString& MatchTravelURL);

	// 매치로 넘길 URL 옵션 문자열: "?AIEasy=N?AINormal=N?AIHard=N?Bots=C01-1,C03-0?TimeIdx=N?KillIdx=N"
	// (Bots 는 CharacterId-난이도 쌍을 추가 순서대로 나열)
	UFUNCTION(BlueprintPure, Category = "Lobby")
	FString BuildMatchOptionsString() const;

	// 이 규칙의 단계 수.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	int32 GetRuleOptionCount(ELobbyRuleId RuleId) const;

	// 출격이 시작된 뒤에는 방장 조작과 준비 변경을 무시합니다.
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsLaunching() const { return bLaunching; }

	// ===== C++ 에서 호출 =====

	// ALobbyPlayerController::Server_StartGame 이 조건을 확인한 뒤 호출합니다:
	// 모든 클라이언트에 출격을 알리고, 매치로 넘길 값을 GameInstance 에 채운 뒤 OnLobbyStartGameApproved 를 부릅니다.
	void BeginLaunch();

	// 플레이어를 내보냅니다 (사유 'kicked'). 방장 권한 확인은 호출하는 쪽에서 합니다.
	void KickLobbyPlayer(APlayerController* Target);

private:
	ALobbyGameState* GetLobbyGameState() const;

	// 이 플레이어를 방장으로 지정하고 GameInstance에 방장 주소를 기록합니다. 배너/기록은 보내지 않습니다
	// (첫 입장자·매치 복귀·fallback 으로 정해지는 방장은 '새로 방장이 된' 것이 아니므로).
	void AssignHost(APlayerController* NewHost);

	// 방장이 아직 없으면(원래 방장이 매치에서 안 돌아온 경우 등) 현재 접속자 중 첫 번째를 방장으로.
	void EnsureHostAssignedFallback();

	FTimerHandle HostFallbackTimerHandle;

	// 출격이 시작됐는지. 이후의 Logout 은 매치로 이동하는 중의 연결 정리이므로 퇴장 기록을 남기지 않습니다.
	bool bLaunching = false;

	// 방장이 내보내는 중인 PlayerId. Logout 에서 '나갔습니다' 대신 '내보냈습니다' 로 구분하는 데 씁니다.
	TSet<int32> KickedPlayerIds;
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LobbyTypes.h"
#include "LobbyPlayerController.generated.h"

class ALobbyGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLobbyControllerFeedSignature, const FLobbyFeedItem&, FeedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLobbyControllerBannerSignature, ELobbyBannerType, BannerType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLobbyControllerLaunchSignature, const FLobbyLaunchInfo&, LaunchInfo);

/**
 * 멀티 로비에서 클라이언트가 서버에 요청을 보낼 때 쓰는 PlayerController.
 * 방장 전용 RPC 는 서버에서 호출자가 방장인지 확인하고, 아니면 무시합니다.
 * 한 사람에게만 가는 알림(기록, 배너, 출격)도 여기서 받아 UI 가 바인딩할 수 있게 합니다.
 */
UCLASS()
class SHOOTINGARENA_API ALobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// ===== 방장 전용 RPC =====

	// 맵을 바꿉니다. MaxPlayers 는 호출하는 쪽(BP)이 DT_MapData 의 Max_Player_Count 를 조회해 넘깁니다.
	// 현재 플레이어 수가 MaxPlayers 보다 많으면 무시됩니다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetMap(const FString& NewMapId, int32 NewMaxPlayers);

	// 규칙을 NewIndex 단계로 옮깁니다 (DT_MultiRules 옵션 범위 안일 때만).
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetRule(ELobbyRuleId RuleId, int32 NewIndex);

	// 빈자리가 있으면 봇을 추가합니다: 종류는 무작위, 난이도는 기본값.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_AddBot();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetBotCharacter(int32 BotId, FName CharacterId);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetBotDifficulty(int32 BotId, ELobbyDifficulty Difficulty);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_RemoveBot(int32 BotId);

	// 플레이어를 내보냅니다. 대상 클라이언트는 접속 화면으로 돌아가고 'kicked' 메시지를 봅니다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_Kick(int32 TargetPlayerId);

	// 방장을 뺀 참가자 전원이 준비 완료(또는 참가자 0명)면 출격합니다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_StartGame();

	// ===== 공용 RPC =====

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetReady(bool bReady);

	// 본인의 표시 이름(닉네임)을 설정합니다. PlayerState 이름과 GameInstance 저장본을 함께 갱신합니다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_SetPlayerName(const FString& NewName);

	// 서버가 현재 로비 상태를 이 클라이언트에게 다시 보내게 합니다. 로비 위젯이 Construct 될 때 한 번 호출하세요 —
	// 서버가 Multicast 를 보낼 때 아직 접속/로딩 중이던 클라이언트가 놓친 값을 스스로 복구합니다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_RequestLobbySync();

	// 방장이 "메인메뉴로" 나가면서 로컬 서버 프로세스를 끄기 전에 호출해 매치 서버(있다면)를 정리합니다.
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void Server_CleanupMatchServer();

	// ===== 서버 -> 이 클라이언트 =====

	UFUNCTION(Client, Reliable)
	void Client_SyncSnapshot(const FLobbySnapshot& Snapshot);

	// 로비 기록에 한 줄 추가합니다 (이 클라이언트 관점으로 이미 정해진 종류).
	UFUNCTION(Client, Reliable)
	void Client_AddFeed(ELobbyFeedType FeedType, const FString& Name);

	// 배너를 띄웁니다.
	UFUNCTION(Client, Reliable)
	void Client_ShowBanner(ELobbyBannerType BannerType);

	// 출격이 시작됐습니다. 로비 조작을 막고 LaunchCard 를 띄우세요.
	UFUNCTION(Client, Reliable)
	void Client_LaunchStarted(const FLobbyLaunchInfo& InLaunchInfo);

	// ===== UI 가 읽고 바인딩하는 값 =====

	// 지금까지 받은 로비 기록 (오래된 것부터). 위젯이 늦게 만들어져도 이미 받은 항목을 그릴 수 있습니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TArray<FLobbyFeedItem> FeedHistory;

	// 출격 중이면 true. 로비 조작(ESC 포함)을 무시해야 합니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bLaunching = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FLobbyLaunchInfo LaunchInfo;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FLobbyControllerFeedSignature OnFeedAdded;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FLobbyControllerBannerSignature OnBannerRequested;

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FLobbyControllerLaunchSignature OnLaunchStarted;

	// 로비 GameState (없으면 nullptr).
	UFUNCTION(BlueprintPure, Category = "Lobby")
	ALobbyGameState* GetLobbyGameState() const;

	virtual void BeginPlay() override;

	// 강퇴당했을 때 서버가 보내는 안내. 접속 화면에 'kicked' 메시지가 뜨도록 GameInstance 에 기록합니다.
	virtual void ClientWasKicked_Implementation(const FText& KickReason) override;

private:
	bool IsHost() const;

	// 기록은 오래된 것부터 버려 최대 이만큼만 보관합니다.
	static constexpr int32 MaxFeedItems = 50;
};

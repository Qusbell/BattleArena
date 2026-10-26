#pragma once

#include "CoreMinimal.h"
#include "LobbyTypes.generated.h"

// 로비에서 봇에 적용하는 난이도입니다. 0=이지 1=노말 2=하드 (DT_Character::Characteristic_ID 인덱스와 같습니다).
UENUM(BlueprintType)
enum class ELobbyDifficulty : uint8
{
	Easy	UMETA(DisplayName = "Easy"),
	Normal	UMETA(DisplayName = "Normal"),
	Hard	UMETA(DisplayName = "Hard")
};

// 로비 기록(Feed)에 쌓이는 항목 종류입니다. (명세 §3-5)
UENUM(BlueprintType)
enum class ELobbyFeedType : uint8
{
	SelfJoined	UMETA(DisplayName = "SelfJoined"),	// 로비에 입장했습니다
	Joined		UMETA(DisplayName = "Joined"),		// {이름} 님이 입장했습니다
	Left		UMETA(DisplayName = "Left"),		// {이름} 님이 나갔습니다
	Kicked		UMETA(DisplayName = "Kicked"),		// {이름} 님을 내보냈습니다 (방장 화면)
	BecameHost	UMETA(DisplayName = "BecameHost")	// 내가 방장이 되었습니다
};

// 배너 종류입니다. 해당 클라이언트에게만 표시합니다. (명세 §3-5)
UENUM(BlueprintType)
enum class ELobbyBannerType : uint8
{
	HostAssigned	UMETA(DisplayName = "HostAssigned"),	// 새 방장 본인
	MapReset		UMETA(DisplayName = "MapReset")			// 맵 변경으로 준비가 해제된 참가자
};

// 서버가 클라이언트에 알리는 로비 이벤트의 원본 종류입니다. 클라이언트가 자기 PlayerId와 비교해 FeedType 으로 바꿉니다.
UENUM(BlueprintType)
enum class ELobbyEventType : uint8
{
	PlayerJoined,
	PlayerLeft,
	PlayerKicked,
	HostChanged
};

// 방장 전용 규칙(DT_MultiRules)의 행 번호입니다.
UENUM(BlueprintType)
enum class ELobbyRuleId : uint8
{
	TimeLimit	UMETA(DisplayName = "TimeLimit"),
	KillTarget	UMETA(DisplayName = "KillTarget")
};

// 메인 버튼(Btn_Main) 상태입니다. (명세 §3-6)
UENUM(BlueprintType)
enum class ELobbyMainButtonState : uint8
{
	StartDisabled	UMETA(DisplayName = "StartDisabled"),
	StartEnabled	UMETA(DisplayName = "StartEnabled"),
	Ready			UMETA(DisplayName = "Ready"),
	ReadyDone		UMETA(DisplayName = "ReadyDone")
};

// 참가자 카드 종류입니다. (명세 §3-2)
UENUM(BlueprintType)
enum class ELobbyCardType : uint8
{
	Player,
	Bot,
	Empty
};

// 접속 화면(WBP_MultiConnect)의 상태입니다. (명세 §2)
UENUM(BlueprintType)
enum class ELobbyConnectState : uint8
{
	Idle,
	Connecting,
	Joined
};

// 로비에 있는 봇 하나입니다. 이름은 저장하지 않고 CharacterId(DT_Character 행 이름)로 UI가 조회합니다.
USTRUCT(BlueprintType)
struct FLobbyBot
{
	GENERATED_BODY()

	// 추가될 때 서버가 매기는 고유 번호입니다. 추가 순서이자 봇을 가리키는 키입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 BotId = INDEX_NONE;

	// DT_Character 의 행 이름 (예: C01).
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FName CharacterId;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	ELobbyDifficulty Difficulty = ELobbyDifficulty::Normal;
};

// 로비에 접속한 사람 한 명의 복제용 요약입니다. PlayerState 포인터는 클라이언트에서 아직 풀리지 않을 수 있어
// PlayerId 로 식별합니다.
USTRUCT(BlueprintType)
struct FLobbyPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 JoinOrder = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bReady = false;
};

// 로비 상태 전체입니다. Push Model 환경에서 값이 중간에 안 바뀌는 문제가 있어서(메모리: replication pushmodel gotcha),
// 서버가 바뀔 때마다 이 구조체를 통째로 Multicast 로 보내고, 접속 직후에는 클라이언트가 Pull 합니다.
// Version 이 더 큰 쪽만 적용하므로 늦게 도착한 이전 값이 최신 값을 덮어쓰지 않습니다.
USTRUCT(BlueprintType)
struct FLobbySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 Version = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString MapId;

	// 맵의 Max_Player_Count (플레이어 + 봇).
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 TimeIdx = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 KillIdx = 0;

	// 규칙이 바뀔 때마다 1씩 증가합니다. 클라이언트는 증가를 보고 OnRuleChanged 를 호출합니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 RuleChangeSerial = 0;

	// 가장 최근에 바뀐 규칙.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	ELobbyRuleId LastChangedRule = ELobbyRuleId::TimeLimit;

	// 맵이 실제로 바뀔 때마다 1씩 증가합니다. 클라이언트는 증가를 보고 OnMapChanged 를 호출합니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 MapChangeSerial = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 HostPlayerId = INDEX_NONE;

	// JoinOrder 오름차순.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TArray<FLobbyPlayerInfo> Players;

	// 추가 순서.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	TArray<FLobbyBot> Bots;
};

// 참가자 카드 하나를 그리는 데 필요한 값입니다. 순서는 플레이어(입장 순) → 봇(추가 순) → 빈자리입니다.
USTRUCT(BlueprintType)
struct FLobbyRosterEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	ELobbyCardType CardType = ELobbyCardType::Empty;

	// Player: 플레이어 PlayerId / Bot: BotId
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 EntryId = INDEX_NONE;

	// Player: 캐릭터 이름 / Bot: 비워둠 (UI 가 BotCharacterId 로 DT_Character 에서 조회)
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsSelf = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bReady = false;

	// 방장 화면이고 방장이 아닌 플레이어 카드일 때 true (강퇴 버튼 표시 대상).
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bCanKick = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FName BotCharacterId;

	// 0=이지 1=노말 2=하드
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 BotDifficulty = 1;

	// 같은 종류 봇이 여러 개일 때 이름 뒤에 붙일 번호입니다. 1이면 번호를 붙이지 않고, 2 이상이면 " 2", " 3" 을 붙입니다.
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 BotNameNumber = 1;
};

// 로비 기록(Feed) 한 줄입니다. 위젯이 만들어지기 전에 도착한 항목도 보이도록 PlayerController 가 보관합니다.
USTRUCT(BlueprintType)
struct FLobbyFeedItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	ELobbyFeedType FeedType = ELobbyFeedType::Joined;

	// {이름} 자리에 들어갈 이름 (SelfJoined / BecameHost 는 비어 있음).
	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString Name;
};

// 출격 카드(LaunchCard)에 넘길 값입니다. 진행률은 로딩 쪽에서 따로 갱신합니다.
USTRUCT(BlueprintType)
struct FLobbyLaunchInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	FString MapId;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 TimeIdx = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 KillIdx = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 PlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 BotCount = 0;
};

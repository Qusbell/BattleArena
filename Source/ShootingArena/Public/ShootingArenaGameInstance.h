#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "LobbyTypes.h"
#include "ShootingArenaGameInstance.generated.h"

class UNetDriver;
class UBGMSettingsDataAsset;
class UPlayerDefaultNameSettings;

/**
 * 프로젝트 전역 GameInstance. 레벨 이동(비-심리스 트래블) 중에도 서버 프로세스에서 계속 살아있으므로,
 * "같은 접속을 유지한 플레이어"에게 레벨이 바뀐 뒤에도 계속 적용되어야 하는 값(예: 닉네임)을 여기 저장합니다.
 * 플레이어 식별은 PlayerState::SavedNetworkAddress(엔진이 접속 시 자동으로 채워주는, 재접속 매칭용 주소 문자열)를 키로 사용합니다.
 */
UCLASS()
class SHOOTINGARENA_API UShootingArenaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	/** BGM 시스템에서 사용할 기획 설정 에셋. BP_QuakeGameInstance 기본값에 지정합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio|BGM")
	TSoftObjectPtr<UBGMSettingsDataAsset> BGMSettings;

	/** 닉네임을 정하지 않은 플레이어의 기본 닉네임 설정. BP_QuakeGameInstance 기본값에 지정합니다. 비어있으면 "Player"+번호를 씁니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player")
	TObjectPtr<UPlayerDefaultNameSettings> PlayerDefaultNameSettings;

	/** 기본 닉네임을 만듭니다. 현재 월드의 접속자 이름과 겹치지 않는 번호를 붙입니다. 서버 전용입니다. */
	FString MakeDefaultPlayerName(bool bCampaign) const;

	// NetworkAddress(PlayerState::SavedNetworkAddress)를 키로 닉네임을 저장합니다. 서버 전용입니다.
	UFUNCTION(BlueprintCallable, Category = "Player")
	void SetSavedNickname(const FString& NetworkAddress, const FString& Nickname);

	// 저장된 닉네임을 찾습니다. 없으면 빈 문자열을 반환합니다. 서버 전용입니다.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Player")
	FString GetSavedNickname(const FString& NetworkAddress) const;

	// 로비에서 설정한 봇들의 난이도 목록입니다. 방장이 게임 시작을 누르면
	// ALobbyGameMode::BeginLaunch 가 로비 봇 목록으로 이 배열을 채우고(PendingBotCharacterIds 와 같은 순서),
	// 매치 레벨의 GameMode(예: BP_MultiplayerAIGameMode)가 레벨 이동 후 이 값을 읽어
	// 그 개수/난이도만큼 AI를 스폰합니다. 서버 전용입니다.
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	TArray<ELobbyDifficulty> PendingAIDifficulties;

	// 위 PendingAIDifficulties 와 같은 순서의 봇 종류(DT_Character 행 이름). 서버 전용입니다.
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	TArray<FName> PendingBotCharacterIds;

	// 로비에서 정한 규칙 단계 (DT_MultiRules 의 옵션 인덱스). 서버 전용입니다.
	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 PendingTimeIdx = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Lobby")
	int32 PendingKillIdx = 0;

	// 방장(호스트)의 네트워크 주소(포트 없는 IP). 매치를 다녀와 로비가 새로 구성될 때
	// 이 값과 일치하는 플레이어를 다시 방장으로 지정해, 재접속 순서와 상관없이 방장이 유지됩니다.
	// ALobbyGameMode 가 관리합니다. 서버 전용입니다.
	UPROPERTY()
	FString SavedHostNetworkAddress;

	// ===== 멀티 서버 접속 (클라이언트 전용) =====

	// 마지막으로 접속을 시도한 서버 주소. 강퇴/끊김으로 접속 화면에 돌아왔을 때 입력 칸을 채웁니다.
	UPROPERTY(BlueprintReadWrite, Category = "Multi|Connect")
	FString LastServerIP;

	UPROPERTY(BlueprintReadWrite, Category = "Multi|Connect", meta = (ClampMin = "1", ClampMax = "65535"))
	int32 LastServerPort = 7777;

	// 접속 화면에서 접속을 누를 때 호출합니다. 주소를 기억하고 '접속 시도 중' 상태로 만듭니다.
	// 이 상태에서 네트워크 실패가 오면 fail/full, 아니면 lost 로 구분합니다.
	UFUNCTION(BlueprintCallable, Category = "Multi|Connect")
	void BeginConnectAttempt(const FString& IP, int32 Port);

	// 접속 시도를 취소하거나 시간 초과로 포기할 때 호출합니다 (UI 의 Connect_Timeout 타이머).
	UFUNCTION(BlueprintCallable, Category = "Multi|Connect")
	void EndConnectAttempt();

	// 로비에 들어왔을 때 호출됩니다 (ALobbyPlayerController::BeginPlay).
	void NotifyLobbyJoined();

	// 접속 화면이 띄울 메시지 키 (ip/port/fail/full/kicked/lost). 이미 값이 있으면 덮어쓰지 않습니다 —
	// 강퇴(kicked)가 뒤따르는 연결 끊김(lost)으로 바뀌지 않게 하기 위함입니다.
	void SetPendingConnectMessage(FName MessageKey);

	// 접속 화면이 Construct 될 때 호출합니다. 메시지 키를 돌려주고 비웁니다 (없으면 None).
	UFUNCTION(BlueprintCallable, Category = "Multi|Connect")
	FName ConsumePendingConnectMessage();

	// 접속 화면으로 돌아와야 하는 상황인지 (강퇴/끊김/가득 참/실패). 메인 메뉴가 열릴 때 확인하세요.
	UFUNCTION(BlueprintPure, Category = "Multi|Connect")
	bool HasPendingConnectMessage() const { return !PendingConnectMessage.IsNone(); }

	// 현재 접속 상태 (UI 의 ConnectState 가 참고).
	UFUNCTION(BlueprintPure, Category = "Multi|Connect")
	ELobbyConnectState GetConnectState() const { return ConnectState; }

private:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	FName PendingConnectMessage;
	ELobbyConnectState ConnectState = ELobbyConnectState::Idle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;

	// 메인 메뉴 레벨 진입 시 로컬 싱글 서버를 종료하기 위한 맵 로드 콜백.
	void HandlePreLoadMap(const FString& MapName);

	FDelegateHandle PostLoadMapHandle;

	UPROPERTY()
	TMap<FString, FString> NicknameByNetworkAddress;
};

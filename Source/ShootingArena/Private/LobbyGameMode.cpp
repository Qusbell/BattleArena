#include "LobbyGameMode.h"
#include "LobbyGameState.h"
#include "LobbyPlayerState.h"
#include "LobbyPlayerController.h"
#include "ShootingArenaGameInstance.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "TimerManager.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerStateClass = ALobbyPlayerState::StaticClass();

	for (int32 Index = 1; Index <= 6; ++Index)
	{
		BotCharacterPool.Add(*FString::Printf(TEXT("C%02d"), Index));
	}
}

ALobbyGameState* ALobbyGameMode::GetLobbyGameState() const
{
	return GetGameState<ALobbyGameState>();
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 매치에서 로비로 돌아왔는데 원래 방장이 (아직) 재접속하지 않은 경우를 대비한 안전장치.
	// 잠시 뒤에도 방장이 없으면 현재 접속자 중 첫 번째를 방장으로 지정합니다.
	GetWorldTimerManager().SetTimer(HostFallbackTimerHandle, this, &ALobbyGameMode::EnsureHostAssignedFallback, 5.0f, false);
}

void ALobbyGameMode::AssignHost(APlayerController* NewHost)
{
	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	ALobbyPlayerState* HostPS = NewHost ? Cast<ALobbyPlayerState>(NewHost->PlayerState) : nullptr;
	if (!LobbyGameState || !HostPS)
	{
		return;
	}

	LobbyGameState->SetHost(HostPS, /*bAnnounce=*/false);

	if (UShootingArenaGameInstance* GameInstance = GetGameInstance<UShootingArenaGameInstance>())
	{
		GameInstance->SavedHostNetworkAddress = HostPS->SavedNetworkAddress;
	}
}

void ALobbyGameMode::EnsureHostAssignedFallback()
{
	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (!LobbyGameState || LobbyGameState->HostPlayerState != nullptr)
	{
		return;
	}

	// JoinOrder 가 가장 빠른 사람을 방장으로.
	const TArray<ALobbyPlayerState*> Players = LobbyGameState->GetLobbyPlayersSorted();
	if (Players.Num() > 0)
	{
		if (APlayerController* PC = Cast<APlayerController>(Players[0]->GetOwner()))
		{
			UE_LOG(LogTemp, Log, TEXT("[Lobby] 원래 방장 미복귀 -> %s 를 방장으로 지정"), *Players[0]->GetPlayerName());
			AssignHost(PC);
		}
	}
}

void ALobbyGameMode::InitializeLobby(const FString& DefaultMapID, int32 MaxPlayerCount)
{
	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->InitLobby(DefaultMapID, MaxPlayerCount, DefaultTimeIdx, DefaultKillIdx);
	}
}

int32 ALobbyGameMode::GetRuleOptionCount(ELobbyRuleId RuleId) const
{
	return RuleId == ELobbyRuleId::TimeLimit ? TimeLimitOptionCount : KillTargetOptionCount;
}

FString ALobbyGameMode::BuildMatchOptionsString() const
{
	const ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (!LobbyGameState)
	{
		return FString();
	}

	int32 Easy = 0, Normal = 0, Hard = 0;
	LobbyGameState->CountBotsByDifficulty(Easy, Normal, Hard);

	TArray<FString> BotPairs;
	for (const FLobbyBot& Bot : LobbyGameState->GetServerBots())
	{
		BotPairs.Add(FString::Printf(TEXT("%s-%d"), *Bot.CharacterId.ToString(), static_cast<int32>(Bot.Difficulty)));
	}

	return FString::Printf(TEXT("?AIEasy=%d?AINormal=%d?AIHard=%d?Bots=%s?TimeIdx=%d?KillIdx=%d"),
		Easy, Normal, Hard, *FString::Join(BotPairs, TEXT(",")),
		LobbyGameState->GetRuleIndex(ELobbyRuleId::TimeLimit), LobbyGameState->GetRuleIndex(ELobbyRuleId::KillTarget));
}

void ALobbyGameMode::BeginLaunch()
{
	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (!LobbyGameState || bLaunching)
	{
		return;
	}

	bLaunching = true;

	// 매치 레벨의 GameMode 가 읽을 값. GameInstance 는 레벨 이동을 넘어 살아있습니다.
	if (UShootingArenaGameInstance* GameInstance = GetGameInstance<UShootingArenaGameInstance>())
	{
		GameInstance->PendingAIDifficulties.Reset();
		GameInstance->PendingBotCharacterIds.Reset();
		for (const FLobbyBot& Bot : LobbyGameState->GetServerBots())
		{
			GameInstance->PendingAIDifficulties.Add(Bot.Difficulty);
			GameInstance->PendingBotCharacterIds.Add(Bot.CharacterId);
		}
		GameInstance->PendingTimeIdx = LobbyGameState->GetRuleIndex(ELobbyRuleId::TimeLimit);
		GameInstance->PendingKillIdx = LobbyGameState->GetRuleIndex(ELobbyRuleId::KillTarget);
	}

	// 방장을 포함한 모든 클라이언트가 로비 조작을 막고 LaunchCard 를 띄웁니다.
	const FLobbyLaunchInfo LaunchInfo = LobbyGameState->BuildLaunchInfo();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALobbyPlayerController* PC = Cast<ALobbyPlayerController>(It->Get()))
		{
			PC->Client_LaunchStarted(LaunchInfo);
		}
	}

	// 맵 에셋 조회(DataTable)와 TravelToMatch 호출은 BP 에서 합니다. 취소 대기 구간 없이 바로 로딩으로 넘어갑니다.
	OnLobbyStartGameApproved(LobbyGameState->GetSelectedMapID());
}

void ALobbyGameMode::TravelToMatch(const FString& MatchTravelURL)
{
	UWorld* World = GetWorld();
	if (!World || MatchTravelURL.IsEmpty())
	{
		return;
	}

	// 방장이 "시작"을 연타하거나, 이미 다른 경로로 트래블이 예약된 경우 중복 실행을 막습니다.
	// ServerTravel 이 예약되면 World->NextURL 이 채워집니다.
	if (!World->NextURL.IsEmpty() || World->IsInSeamlessTravel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] TravelToMatch 무시: 이미 트래블이 예약되어 있습니다. (NextURL=%s)"), *World->NextURL);
		return;
	}

	// bAbsolute=true: 로비를 열 때 붙었던 "?Game=BP_LobbyGameMode" 등 이전 URL 옵션을
	// 이어받지 않고, MatchTravelURL 에 명시된 GameMode/옵션으로만 매치 맵을 엽니다.
	UE_LOG(LogTemp, Log, TEXT("[Lobby] TravelToMatch: %s"), *MatchTravelURL);
	World->ServerTravel(MatchTravelURL, /*bAbsolute=*/true);
}

void ALobbyGameMode::KickLobbyPlayer(APlayerController* Target)
{
	if (!Target || !Target->PlayerState)
	{
		return;
	}

	KickedPlayerIds.Add(Target->PlayerState->GetPlayerId());

	// KickPlayer 는 대상에게 ClientWasKicked 를 보낸 뒤 컨트롤러를 정리하고, 그 과정에서 Logout 이 호출됩니다.
	const FText Reason = FText::FromString(TEXT("kicked"));
	if (!GameSession || !GameSession->KickPlayer(Target, Reason))
	{
		Target->ClientWasKicked(Reason);
		Target->Destroy();
	}
}

void ALobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 플레이어 수 >= 최대 인원이면 거절합니다. 클라이언트는 'full' 을 받습니다.
	// (최대 인원은 현재 맵의 Max_Player_Count 이고, 봇은 새 플레이어에게 자리를 양보하므로 사람 수만 셉니다.)
	if (const ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		const int32 MaxPlayers = LobbyGameState->GetMaxPlayers();
		if (MaxPlayers > 0 && GetNumPlayers() >= MaxPlayers)
		{
			ErrorMessage = TEXT("full");
		}
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (!LobbyGameState || !NewPlayer || !NewPlayer->PlayerState)
	{
		return;
	}

	ALobbyPlayerState* NewPS = Cast<ALobbyPlayerState>(NewPlayer->PlayerState);
	if (!NewPS)
	{
		UE_LOG(LogTemp, Error, TEXT("[Lobby] PlayerState 가 ALobbyPlayerState 가 아닙니다 (%s). BP_LobbyGameMode 의 Player State Class 를 확인하세요."),
			*NewPlayer->PlayerState->GetClass()->GetName());
		return;
	}

	// 입장 순서(JoinOrder)를 매기고, 자리가 모자라면 봇을 뒤에서부터 제거하고, 입장 기록을 보냅니다.
	LobbyGameState->RegisterPlayer(NewPS);

	// 방장 지정/복원. 매치를 다녀와 LobbyGameState 가 새로 생성되면 HostPlayerState 가 null 이므로,
	// 재접속 순서와 무관하게 "원래 방장"을 다시 방장으로 만들어야 합니다.
	if (LobbyGameState->HostPlayerState == nullptr)
	{
		const UShootingArenaGameInstance* GameInstance = GetGameInstance<UShootingArenaGameInstance>();
		const FString SavedHostAddr = GameInstance ? GameInstance->SavedHostNetworkAddress : FString();
		const FString ThisAddr = NewPS->SavedNetworkAddress;

		if (SavedHostAddr.IsEmpty() || (!ThisAddr.IsEmpty() && SavedHostAddr == ThisAddr))
		{
			// 저장된 방장이 아직 없거나(최초 로비), 이 플레이어가 저장된 방장 본인이면 방장으로.
			AssignHost(NewPlayer);
		}
		// else: 저장된 방장이 따로 있고 이 사람은 아님 → 방장 재접속 대기 (안 오면 BeginPlay 의 fallback 타이머가 처리).
	}
}

void ALobbyGameMode::Logout(AController* Exiting)
{
	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	ALobbyPlayerState* ExitingPS = Exiting ? Cast<ALobbyPlayerState>(Exiting->PlayerState) : nullptr;

	// 출격으로 레벨이 바뀌는 중의 Logout 은 모두가 함께 이동하는 것이라 퇴장으로 보지 않습니다.
	if (LobbyGameState && ExitingPS && !bLaunching)
	{
		const bool bWasKicked = KickedPlayerIds.Remove(ExitingPS->GetPlayerId()) > 0;
		LobbyGameState->UnregisterPlayer(ExitingPS, bWasKicked);

		// 방장이 나가서 승계됐다면 저장된 방장 주소도 새 방장으로 갱신합니다.
		if (UShootingArenaGameInstance* GameInstance = GetGameInstance<UShootingArenaGameInstance>())
		{
			if (const APlayerState* NewHost = LobbyGameState->HostPlayerState)
			{
				GameInstance->SavedHostNetworkAddress = NewHost->SavedNetworkAddress;
			}
			else
			{
				GameInstance->SavedHostNetworkAddress.Empty();
			}
		}
	}

	Super::Logout(Exiting);
}

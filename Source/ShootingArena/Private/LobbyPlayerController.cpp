#include "LobbyPlayerController.h"
#include "LobbyGameState.h"
#include "LobbyGameMode.h"
#include "LobbyPlayerState.h"
#include "ShootingArenaGameInstance.h"
#include "LocalDedicatedServerLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"

ALobbyGameState* ALobbyPlayerController::GetLobbyGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ALobbyGameState>() : nullptr;
}

bool ALobbyPlayerController::IsHost() const
{
	const ALobbyGameState* LobbyGameState = GetLobbyGameState();
	return LobbyGameState && PlayerState && LobbyGameState->HostPlayerState == PlayerState;
}

void ALobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && GetNetMode() == NM_Client)
	{
		// 접속에 성공했습니다: 접속 시도 상태를 풀고 마지막 서버 주소를 확정합니다.
		if (UShootingArenaGameInstance* SAGameInstance = GetGameInstance<UShootingArenaGameInstance>())
		{
			SAGameInstance->NotifyLobbyJoined();
		}

		// 서버가 Multicast 를 보낼 때 아직 접속 중이라 놓친 값을 스스로 채웁니다.
		Server_RequestLobbySync();
	}
}

// ===================== 방장 전용 =====================

void ALobbyPlayerController::Server_SetMap_Implementation(const FString& NewMapId, int32 NewMaxPlayers)
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (!LobbyGameMode || LobbyGameMode->IsLaunching())
	{
		return;
	}

	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->ChangeMap(NewMapId, NewMaxPlayers);
	}
}

void ALobbyPlayerController::Server_SetRule_Implementation(ELobbyRuleId RuleId, int32 NewIndex)
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (!LobbyGameMode || LobbyGameMode->IsLaunching())
	{
		return;
	}

	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->SetRuleIndex(RuleId, NewIndex, LobbyGameMode->GetRuleOptionCount(RuleId));
	}
}

void ALobbyPlayerController::Server_AddBot_Implementation()
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (!LobbyGameMode || LobbyGameMode->IsLaunching())
	{
		return;
	}

	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->AddBot(LobbyGameMode->BotCharacterPool, LobbyGameMode->DefaultBotDifficulty);
	}
}

void ALobbyPlayerController::Server_SetBotCharacter_Implementation(int32 BotId, FName CharacterId)
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (!LobbyGameMode || LobbyGameMode->IsLaunching())
	{
		return;
	}

	// 봇 종류는 풀에 있는 것만 허용합니다.
	if (!LobbyGameMode->BotCharacterPool.Contains(CharacterId))
	{
		return;
	}

	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->SetBotCharacter(BotId, CharacterId);
	}
}

void ALobbyPlayerController::Server_SetBotDifficulty_Implementation(int32 BotId, ELobbyDifficulty Difficulty)
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (!LobbyGameMode || LobbyGameMode->IsLaunching())
	{
		return;
	}

	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->SetBotDifficulty(BotId, Difficulty);
	}
}

void ALobbyPlayerController::Server_RemoveBot_Implementation(int32 BotId)
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (!LobbyGameMode || LobbyGameMode->IsLaunching())
	{
		return;
	}

	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->RemoveBot(BotId);
	}
}

void ALobbyPlayerController::Server_Kick_Implementation(int32 TargetPlayerId)
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (!LobbyGameMode || !LobbyGameState || LobbyGameMode->IsLaunching())
	{
		return;
	}

	// 방장 자신은 내보낼 수 없습니다.
	if (PlayerState && PlayerState->GetPlayerId() == TargetPlayerId)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* TargetPC = It->Get();
		if (IsValid(TargetPC) && TargetPC->PlayerState && TargetPC->PlayerState->GetPlayerId() == TargetPlayerId)
		{
			LobbyGameMode->KickLobbyPlayer(TargetPC);
			return;
		}
	}
}

void ALobbyPlayerController::Server_StartGame_Implementation()
{
	if (!IsHost())
	{
		return;
	}

	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (!LobbyGameMode || !LobbyGameState)
	{
		return;
	}

	// 방장을 뺀 참가자 전원이 준비했을 때만 (참가자 0명이면 바로) 출격합니다.
	if (!LobbyGameState->AreAllGuestsReady())
	{
		return;
	}

	LobbyGameMode->BeginLaunch();
}

// ===================== 공용 =====================

void ALobbyPlayerController::Server_SetReady_Implementation(bool bReady)
{
	ALobbyGameMode* LobbyGameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ALobbyGameMode>() : nullptr;
	if (LobbyGameMode && LobbyGameMode->IsLaunching())
	{
		return;
	}

	ALobbyGameState* LobbyGameState = GetLobbyGameState();
	if (LobbyGameState && PlayerState)
	{
		LobbyGameState->SetPlayerReady(Cast<ALobbyPlayerState>(PlayerState), bReady);
	}
}

void ALobbyPlayerController::Server_SetPlayerName_Implementation(const FString& NewName)
{
	if (!PlayerState)
	{
		return;
	}

	// 앞뒤 공백 제거 + 너무 긴 이름 방지.
	const FString TrimmedName = NewName.TrimStartAndEnd().Left(20);
	if (TrimmedName.IsEmpty())
	{
		return;
	}

	PlayerState->SetPlayerName(TrimmedName);

	// 다음 레벨(실제 게임)로 넘어간 뒤에도 이 이름을 계속 쓸 수 있도록, 접속 주소를 키로 GameInstance에 저장해둡니다.
	// (AMyReconnectionGameMode::PostLogin이 새 레벨에서 이 값을 다시 읽어 PlayerState에 적용합니다.)
	if (UShootingArenaGameInstance* SAGameInstance = Cast<UShootingArenaGameInstance>(GetWorld() ? GetWorld()->GetGameInstance() : nullptr))
	{
		SAGameInstance->SetSavedNickname(PlayerState->SavedNetworkAddress, TrimmedName);
	}

	// 이름이 바뀌었으니 모든 화면의 카드를 갱신합니다.
	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->RefreshSnapshot();
	}
}

void ALobbyPlayerController::Server_RequestLobbySync_Implementation()
{
	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->SendSnapshotTo(this);
	}
}

void ALobbyPlayerController::Server_CleanupMatchServer_Implementation()
{
	if (!IsHost())
	{
		return;
	}

	ULocalDedicatedServerLibrary::StopMatchServer();
}

// ===================== 서버 -> 이 클라이언트 =====================

void ALobbyPlayerController::Client_SyncSnapshot_Implementation(const FLobbySnapshot& Snapshot)
{
	if (ALobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->ApplySnapshot(Snapshot);
	}
}

void ALobbyPlayerController::Client_AddFeed_Implementation(ELobbyFeedType FeedType, const FString& Name)
{
	FLobbyFeedItem Item;
	Item.FeedType = FeedType;
	Item.Name = Name;

	FeedHistory.Add(Item);
	if (FeedHistory.Num() > MaxFeedItems)
	{
		FeedHistory.RemoveAt(0, FeedHistory.Num() - MaxFeedItems);
	}

	OnFeedAdded.Broadcast(Item);
}

void ALobbyPlayerController::Client_ShowBanner_Implementation(ELobbyBannerType BannerType)
{
	OnBannerRequested.Broadcast(BannerType);
}

void ALobbyPlayerController::Client_LaunchStarted_Implementation(const FLobbyLaunchInfo& InLaunchInfo)
{
	bLaunching = true;
	LaunchInfo = InLaunchInfo;
	OnLaunchStarted.Broadcast(InLaunchInfo);
}

void ALobbyPlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	Super::ClientWasKicked_Implementation(KickReason);

	// 곧 연결이 끊기면서 엔진이 메인 메뉴로 돌려보냅니다. 접속 화면이 'kicked' 를 보여주도록 이유를 먼저 기록합니다.
	// (뒤따르는 NetworkFailure 가 'lost' 로 덮어쓰지 않습니다 — SetPendingConnectMessage 참고.)
	if (UShootingArenaGameInstance* SAGameInstance = GetGameInstance<UShootingArenaGameInstance>())
	{
		SAGameInstance->SetPendingConnectMessage(TEXT("kicked"));

		// BP_QuakeGameInstance 의 NetworkError 는 bIntentionalDisconnect 가 false 인 ConnectionLost 를 "예상 못 한 끊김"으로 보고
		// 1초 뒤 마지막 서버에 자동 재접속합니다. 강퇴도 의도된 끊김이므로 true 로 두어 재접속을 막습니다.
		if (FBoolProperty* Intentional = CastField<FBoolProperty>(SAGameInstance->GetClass()->FindPropertyByName(TEXT("bIntentionalDisconnect"))))
		{
			Intentional->SetPropertyValue_InContainer(SAGameInstance, true);
		}
	}
}

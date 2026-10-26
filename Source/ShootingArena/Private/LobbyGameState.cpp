#include "LobbyGameState.h"
#include "LobbyPlayerState.h"
#include "LobbyPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

ALobbyGameState::ALobbyGameState()
{
}

void ALobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyGameState, Snapshot);
}

// ===================== 모든 머신: 읽기 =====================

int32 ALobbyGameState::GetLocalPlayerId() const
{
	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const APlayerController* LocalPC = GameInstance ? GameInstance->GetFirstLocalPlayerController(World) : nullptr;
	return (LocalPC && LocalPC->PlayerState) ? LocalPC->PlayerState->GetPlayerId() : INDEX_NONE;
}

bool ALobbyGameState::IsLocalPlayerHost() const
{
	const int32 LocalId = GetLocalPlayerId();
	return LocalId != INDEX_NONE && LocalId == Snapshot.HostPlayerId;
}

int32 ALobbyGameState::GetGuestCount() const
{
	int32 Count = 0;
	for (const FLobbyPlayerInfo& Info : Snapshot.Players)
	{
		if (Info.PlayerId != Snapshot.HostPlayerId)
		{
			++Count;
		}
	}
	return Count;
}

int32 ALobbyGameState::GetReadyCount() const
{
	int32 Count = 0;
	for (const FLobbyPlayerInfo& Info : Snapshot.Players)
	{
		if (Info.PlayerId != Snapshot.HostPlayerId && Info.bReady)
		{
			++Count;
		}
	}
	return Count;
}

ELobbyMainButtonState ALobbyGameState::GetMainButtonState() const
{
	if (IsLocalPlayerHost())
	{
		return AreAllGuestsReady() ? ELobbyMainButtonState::StartEnabled : ELobbyMainButtonState::StartDisabled;
	}

	const int32 LocalId = GetLocalPlayerId();
	for (const FLobbyPlayerInfo& Info : Snapshot.Players)
	{
		if (Info.PlayerId == LocalId)
		{
			return Info.bReady ? ELobbyMainButtonState::ReadyDone : ELobbyMainButtonState::Ready;
		}
	}

	return ELobbyMainButtonState::Ready;
}

TArray<FLobbyRosterEntry> ALobbyGameState::GetRosterEntries() const
{
	TArray<FLobbyRosterEntry> Entries;
	Entries.Reserve(FMath::Max(Snapshot.MaxPlayers, GetFilledCount()));

	const int32 LocalId = GetLocalPlayerId();
	const bool bViewerIsHost = LocalId != INDEX_NONE && LocalId == Snapshot.HostPlayerId;

	for (const FLobbyPlayerInfo& Info : Snapshot.Players)
	{
		FLobbyRosterEntry Entry;
		Entry.CardType = ELobbyCardType::Player;
		Entry.EntryId = Info.PlayerId;
		Entry.DisplayName = Info.CharacterName;
		Entry.bIsHost = (Info.PlayerId == Snapshot.HostPlayerId);
		Entry.bIsSelf = (Info.PlayerId == LocalId);
		Entry.bReady = Info.bReady;
		Entry.bCanKick = bViewerIsHost && !Entry.bIsHost;
		Entries.Add(Entry);
	}

	TMap<FName, int32> SeenCountByCharacter;
	for (const FLobbyBot& Bot : Snapshot.Bots)
	{
		int32& Seen = SeenCountByCharacter.FindOrAdd(Bot.CharacterId);
		++Seen;

		FLobbyRosterEntry Entry;
		Entry.CardType = ELobbyCardType::Bot;
		Entry.EntryId = Bot.BotId;
		Entry.BotCharacterId = Bot.CharacterId;
		Entry.BotDifficulty = static_cast<int32>(Bot.Difficulty);
		Entry.BotNameNumber = Seen;
		// 봇은 항상 준비 완료로 취급합니다.
		Entry.bReady = true;
		Entries.Add(Entry);
	}

	while (Entries.Num() < Snapshot.MaxPlayers)
	{
		Entries.AddDefaulted();
	}

	return Entries;
}

FString ALobbyGameState::GetNextHostName() const
{
	for (const FLobbyPlayerInfo& Info : Snapshot.Players)
	{
		if (Info.PlayerId != Snapshot.HostPlayerId)
		{
			return Info.CharacterName;
		}
	}
	return FString();
}

void ALobbyGameState::CountBotsByDifficulty(int32& OutEasy, int32& OutNormal, int32& OutHard) const
{
	OutEasy = 0;
	OutNormal = 0;
	OutHard = 0;

	for (const FLobbyBot& Bot : Snapshot.Bots)
	{
		switch (Bot.Difficulty)
		{
		case ELobbyDifficulty::Easy:   ++OutEasy;   break;
		case ELobbyDifficulty::Normal: ++OutNormal; break;
		case ELobbyDifficulty::Hard:   ++OutHard;   break;
		default: break;
		}
	}
}

FLobbyLaunchInfo ALobbyGameState::BuildLaunchInfo() const
{
	FLobbyLaunchInfo Info;
	Info.MapId = MapId;
	Info.TimeIdx = TimeIdx;
	Info.KillIdx = KillIdx;
	Info.PlayerCount = GetLobbyPlayersSorted().Num();
	Info.BotCount = Bots.Num();
	return Info;
}

// ===================== 스냅샷 전달 =====================

void ALobbyGameState::OnRep_Snapshot()
{
	ApplySnapshot(Snapshot);
}

void ALobbyGameState::Multicast_SyncSnapshot_Implementation(const FLobbySnapshot& NewSnapshot)
{
	ApplySnapshot(NewSnapshot);
}

void ALobbyGameState::ApplySnapshot(const FLobbySnapshot& NewSnapshot)
{
	if (NewSnapshot.Version <= AppliedVersion)
	{
		return;
	}

	// 복제로 들어온 경우 NewSnapshot 이 Snapshot 자신일 수 있습니다.
	if (&NewSnapshot != &Snapshot)
	{
		Snapshot = NewSnapshot;
	}
	AppliedVersion = Snapshot.Version;

	// 처음 받는 스냅샷은 "이미 지난 변경"이므로 규칙/맵 변경 이벤트를 내지 않습니다.
	const bool bRuleChanged = bHasAppliedSnapshot && Snapshot.RuleChangeSerial > AppliedRuleSerial;
	const bool bMapChanged = bHasAppliedSnapshot && Snapshot.MapChangeSerial > AppliedMapSerial;
	bHasAppliedSnapshot = true;
	AppliedRuleSerial = Snapshot.RuleChangeSerial;
	AppliedMapSerial = Snapshot.MapChangeSerial;

	OnSnapshotChanged.Broadcast();
	OnLobbyStateChanged();

	if (bRuleChanged)
	{
		OnRuleChanged.Broadcast(Snapshot.LastChangedRule);
	}
	if (bMapChanged)
	{
		OnMapChanged.Broadcast();
	}
}

void ALobbyGameState::SendSnapshotTo(ALobbyPlayerController* Target)
{
	if (HasAuthority() && Target)
	{
		Target->Client_SyncSnapshot(Snapshot);
	}
}

void ALobbyGameState::RebuildAndBroadcast(const ALobbyPlayerState* Exclude)
{
	if (!HasAuthority())
	{
		return;
	}

	FLobbySnapshot NewSnapshot;
	NewSnapshot.Version = ++SnapshotVersion;
	NewSnapshot.MapId = MapId;
	NewSnapshot.MaxPlayers = MaxPlayers;
	NewSnapshot.TimeIdx = TimeIdx;
	NewSnapshot.KillIdx = KillIdx;
	NewSnapshot.RuleChangeSerial = RuleChangeSerial;
	NewSnapshot.LastChangedRule = LastChangedRule;
	NewSnapshot.MapChangeSerial = MapChangeSerial;
	NewSnapshot.HostPlayerId = (HostPlayerState && HostPlayerState != Exclude) ? HostPlayerState->GetPlayerId() : INDEX_NONE;
	NewSnapshot.Bots = Bots;

	for (const ALobbyPlayerState* PS : GetLobbyPlayersSorted(Exclude))
	{
		FLobbyPlayerInfo Info;
		Info.PlayerId = PS->GetPlayerId();
		Info.CharacterName = PS->GetCharacterName();
		Info.JoinOrder = PS->JoinOrder;
		Info.bReady = PS->bReady;
		NewSnapshot.Players.Add(Info);
	}

	// 서버 자신(NetMulticast 는 호출한 쪽에서도 실행됩니다)과 모든 클라이언트에 한꺼번에 전달합니다.
	Multicast_SyncSnapshot(NewSnapshot);
}

// ===================== 서버: 대상 찾기 =====================

TArray<ALobbyPlayerState*> ALobbyGameState::GetLobbyPlayersSorted(const ALobbyPlayerState* Exclude) const
{
	TArray<ALobbyPlayerState*> Result;
	for (APlayerState* PS : PlayerArray)
	{
		ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS);
		if (!LobbyPS || LobbyPS == Exclude || LobbyPS->IsABot() || LobbyPS->JoinOrder <= 0)
		{
			continue;
		}
		Result.Add(LobbyPS);
	}

	Result.Sort([](const ALobbyPlayerState& A, const ALobbyPlayerState& B) { return A.JoinOrder < B.JoinOrder; });
	return Result;
}

ALobbyPlayerState* ALobbyGameState::FindLobbyPlayerById(int32 PlayerId) const
{
	for (APlayerState* PS : PlayerArray)
	{
		if (PS && PS->GetPlayerId() == PlayerId)
		{
			return Cast<ALobbyPlayerState>(PS);
		}
	}
	return nullptr;
}

void ALobbyGameState::ForEachLobbyController(TFunctionRef<void(ALobbyPlayerController*)> Callback) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALobbyPlayerController* PC = Cast<ALobbyPlayerController>(It->Get()))
		{
			Callback(PC);
		}
	}
}

ALobbyPlayerController* ALobbyGameState::FindControllerByPlayerId(int32 PlayerId) const
{
	ALobbyPlayerController* Found = nullptr;
	ForEachLobbyController([&](ALobbyPlayerController* PC)
	{
		if (!Found && PC->PlayerState && PC->PlayerState->GetPlayerId() == PlayerId)
		{
			Found = PC;
		}
	});
	return Found;
}

// ===================== 서버: 로비 구성 =====================

void ALobbyGameState::InitLobby(const FString& DefaultMapId, int32 InMaxPlayers, int32 DefaultTimeIdx, int32 DefaultKillIdx)
{
	if (!HasAuthority())
	{
		return;
	}

	MapId = DefaultMapId;
	MaxPlayers = InMaxPlayers;
	TimeIdx = DefaultTimeIdx;
	KillIdx = DefaultKillIdx;
	Bots.Reset();

	// InitLobby 보다 PostLogin 이 먼저 실행된 경우(리슨 서버의 로컬 플레이어 등)에 대비해 아직 등록 안 된 사람을 등록합니다.
	for (APlayerState* PS : PlayerArray)
	{
		if (ALobbyPlayerState* LobbyPS = Cast<ALobbyPlayerState>(PS))
		{
			if (LobbyPS->JoinOrder <= 0 && !LobbyPS->IsABot())
			{
				RegisterPlayer(LobbyPS);
			}
		}
	}

	RebuildAndBroadcast();
}

void ALobbyGameState::RegisterPlayer(ALobbyPlayerState* NewPlayer)
{
	if (!HasAuthority() || !NewPlayer || NewPlayer->JoinOrder > 0)
	{
		return;
	}

	NewPlayer->JoinOrder = NextJoinOrder++;
	NewPlayer->bReady = false;

	// 플레이어 입장으로 자리가 모자라면 봇을 뒤에서부터 제거합니다.
	if (MaxPlayers > 0)
	{
		const int32 PlayerCount = GetLobbyPlayersSorted().Num();
		while (Bots.Num() > 0 && PlayerCount + Bots.Num() > MaxPlayers)
		{
			Bots.Pop();
		}
	}

	const FString NewName = NewPlayer->GetCharacterName();
	ForEachLobbyController([&](ALobbyPlayerController* PC)
	{
		if (PC->PlayerState == NewPlayer)
		{
			PC->Client_AddFeed(ELobbyFeedType::SelfJoined, FString());
		}
		else
		{
			PC->Client_AddFeed(ELobbyFeedType::Joined, NewName);
		}
	});

	RebuildAndBroadcast();
}

void ALobbyGameState::UnregisterPlayer(ALobbyPlayerState* ExitingPlayer, bool bWasKicked)
{
	if (!HasAuthority() || !ExitingPlayer)
	{
		return;
	}

	const bool bWasHost = (HostPlayerState == ExitingPlayer);
	const FString ExitingName = ExitingPlayer->GetCharacterName();

	// 방장에게는 '내보냈습니다', 나머지에게는 '나갔습니다'. (강퇴당한 본인 컨트롤러는 곧 사라지므로 보내지 않습니다.)
	ForEachLobbyController([&](ALobbyPlayerController* PC)
	{
		if (PC->PlayerState == ExitingPlayer)
		{
			return;
		}

		const bool bPCIsHost = (PC->PlayerState == HostPlayerState);
		if (bWasKicked && bPCIsHost)
		{
			PC->Client_AddFeed(ELobbyFeedType::Kicked, ExitingName);
		}
		else
		{
			PC->Client_AddFeed(ELobbyFeedType::Left, ExitingName);
		}
	});

	ExitingPlayer->bReady = false;

	if (bWasHost)
	{
		// 남은 사람 중 JoinOrder 가 가장 빠른 사람에게 넘깁니다. 아무도 없으면 방장 없음.
		const TArray<ALobbyPlayerState*> Remaining = GetLobbyPlayersSorted(ExitingPlayer);
		SetHost(Remaining.Num() > 0 ? Remaining[0] : nullptr, /*bAnnounce=*/true, ExitingPlayer);
	}
	else
	{
		RebuildAndBroadcast(ExitingPlayer);
	}
}

void ALobbyGameState::SetHost(ALobbyPlayerState* NewHost, bool bAnnounce, const ALobbyPlayerState* ExcludeFromRoster)
{
	if (!HasAuthority())
	{
		return;
	}

	HostPlayerState = NewHost;

	if (NewHost)
	{
		// 방장은 준비 개념이 없습니다.
		NewHost->bReady = false;

		if (bAnnounce)
		{
			if (ALobbyPlayerController* HostPC = FindControllerByPlayerId(NewHost->GetPlayerId()))
			{
				HostPC->Client_ShowBanner(ELobbyBannerType::HostAssigned);
				HostPC->Client_AddFeed(ELobbyFeedType::BecameHost, FString());
			}
		}
	}

	RebuildAndBroadcast(ExcludeFromRoster);
}

bool ALobbyGameState::ChangeMap(const FString& NewMapId, int32 NewMaxPlayers)
{
	if (!HasAuthority() || NewMaxPlayers <= 0)
	{
		return false;
	}

	// 현재 플레이어(사람)가 새 맵의 최대 인원보다 많으면 거부합니다.
	if (GetLobbyPlayersSorted().Num() > NewMaxPlayers)
	{
		return false;
	}

	MaxPlayers = NewMaxPlayers;

	if (NewMapId == MapId)
	{
		// 맵이 실제로 바뀌지 않았으면 준비/봇은 건드리지 않습니다.
		RebuildAndBroadcast();
		return true;
	}

	MapId = NewMapId;
	++MapChangeSerial;

	// ① 참가자 준비 전부 해제 — 준비가 풀린 사람만 기억해서 배너를 보냅니다.
	TArray<int32> ResetPlayerIds;
	for (ALobbyPlayerState* PS : GetLobbyPlayersSorted())
	{
		if (PS != HostPlayerState && PS->bReady)
		{
			PS->bReady = false;
			ResetPlayerIds.Add(PS->GetPlayerId());
		}
	}

	// ② 봇을 새 최대 인원에 맞게 뒤에서부터 제거.
	const int32 PlayerCount = GetLobbyPlayersSorted().Num();
	while (Bots.Num() > 0 && PlayerCount + Bots.Num() > MaxPlayers)
	{
		Bots.Pop();
	}

	// ③ 준비가 해제된 참가자에게 MapReset 배너.
	for (const int32 PlayerId : ResetPlayerIds)
	{
		if (ALobbyPlayerController* PC = FindControllerByPlayerId(PlayerId))
		{
			PC->Client_ShowBanner(ELobbyBannerType::MapReset);
		}
	}

	RebuildAndBroadcast();
	return true;
}

bool ALobbyGameState::SetRuleIndex(ELobbyRuleId RuleId, int32 NewIndex, int32 OptionCount)
{
	if (!HasAuthority() || NewIndex < 0 || NewIndex >= OptionCount)
	{
		return false;
	}

	int32& Target = (RuleId == ELobbyRuleId::TimeLimit) ? TimeIdx : KillIdx;
	if (Target == NewIndex)
	{
		return true;
	}

	Target = NewIndex;
	++RuleChangeSerial;
	LastChangedRule = RuleId;

	// 규칙이 바뀌어도 준비는 해제하지 않습니다.
	RebuildAndBroadcast();
	return true;
}

bool ALobbyGameState::AddBot(const TArray<FName>& CharacterPool, ELobbyDifficulty Difficulty)
{
	if (!HasAuthority() || CharacterPool.Num() == 0)
	{
		return false;
	}

	if (GetLobbyPlayersSorted().Num() + Bots.Num() >= MaxPlayers)
	{
		return false;
	}

	FLobbyBot NewBot;
	NewBot.BotId = NextBotId++;
	NewBot.CharacterId = CharacterPool[FMath::RandRange(0, CharacterPool.Num() - 1)];
	NewBot.Difficulty = Difficulty;
	Bots.Add(NewBot);

	RebuildAndBroadcast();
	return true;
}

bool ALobbyGameState::SetBotCharacter(int32 BotId, FName CharacterId)
{
	if (!HasAuthority() || CharacterId.IsNone())
	{
		return false;
	}

	for (FLobbyBot& Bot : Bots)
	{
		if (Bot.BotId == BotId)
		{
			Bot.CharacterId = CharacterId;
			RebuildAndBroadcast();
			return true;
		}
	}
	return false;
}

bool ALobbyGameState::SetBotDifficulty(int32 BotId, ELobbyDifficulty Difficulty)
{
	if (!HasAuthority())
	{
		return false;
	}

	for (FLobbyBot& Bot : Bots)
	{
		if (Bot.BotId == BotId)
		{
			Bot.Difficulty = Difficulty;
			RebuildAndBroadcast();
			return true;
		}
	}
	return false;
}

bool ALobbyGameState::RemoveBot(int32 BotId)
{
	if (!HasAuthority())
	{
		return false;
	}

	const int32 Removed = Bots.RemoveAll([BotId](const FLobbyBot& Bot) { return Bot.BotId == BotId; });
	if (Removed > 0)
	{
		RebuildAndBroadcast();
		return true;
	}
	return false;
}

void ALobbyGameState::SetPlayerReady(ALobbyPlayerState* Player, bool bReady)
{
	// 방장은 준비 개념이 없어서 무시합니다.
	if (!HasAuthority() || !Player || Player == HostPlayerState || Player->JoinOrder <= 0)
	{
		return;
	}

	if (Player->bReady == bReady)
	{
		return;
	}

	Player->bReady = bReady;
	RebuildAndBroadcast();
}

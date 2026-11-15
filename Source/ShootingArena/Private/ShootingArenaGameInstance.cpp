#include "ShootingArenaGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformTime.h"
#include "PlayerDefaultNameSettings.h"
#include "LocalDedicatedServerLibrary.h"
#include "Loading/LoadingScreenSubsystem.h"
#include "MyReconnectionGameMode.h"
#include "Engine/NetDriver.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

void UShootingArenaGameInstance::Init()
{
	Super::Init();

	// 로컬 싱글 서버는 "싱글플레이 UI 진입 ~ 인게임" 동안만 살아 있어야 하므로,
	// 어떤 경로로든 메인 메뉴 레벨이 로드되면 서버를 정리합니다. (서버 프로세스 자신은 제외)
	if (!IsRunningDedicatedServer())
	{
		// PostLoadMap 이 아니라 PreLoadMap 을 씁니다. 메인 메뉴 위젯(맵 선택의 서버 시작)이 맵 로드 중
		// PostLoadMap 보다 먼저 실행되므로, PostLoadMap 에서 정지하면 방금 띄운 서버를 죽여버립니다.
		PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(
			this, &UShootingArenaGameInstance::HandlePreLoadMap);

		// 멀티 접속 실패/끊김을 접속 화면의 메시지 키(fail/full/lost)로 바꾸기 위해 엔진 실패 이벤트를 듣습니다.
		if (GEngine)
		{
			NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UShootingArenaGameInstance::HandleNetworkFailure);
			TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UShootingArenaGameInstance::HandleTravelFailure);
		}
	}
	else
	{
		ULocalDedicatedServerLibrary::InitServerCommandWatcher();
		ServerFrameRateLastLogTime = FPlatformTime::Seconds();
		ServerFrameRateTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UShootingArenaGameInstance::LogServerFrameRate));
	}
}

void UShootingArenaGameInstance::Shutdown()
{
	if (ServerFrameRateTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(ServerFrameRateTickerHandle);
		ServerFrameRateTickerHandle.Reset();
	}

	if (PreLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMapWithContext.Remove(PreLoadMapHandle);
		PreLoadMapHandle.Reset();
	}

	if (GEngine)
	{
		if (NetworkFailureHandle.IsValid())
		{
			GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		}
		if (TravelFailureHandle.IsValid())
		{
			GEngine->OnTravelFailure().Remove(TravelFailureHandle);
		}
	}
	NetworkFailureHandle.Reset();
	TravelFailureHandle.Reset();

	// PIE 종료(Esc)처럼 프로세스는 살아 있고 GameInstance 만 사라지는 경우에는
	// OnPreExit 도 부모 PID 워치독도 동작하지 않으므로 여기서 로컬 서버를 직접 정리합니다.
	// (서버 프로세스 자신은 제외. 매치 서버는 별도 핸들이라 건드리지 않습니다.)
	if (!IsRunningDedicatedServer())
	{
		ULocalDedicatedServerLibrary::StopLocalDedicatedServer();
	}

	Super::Shutdown();
}

bool UShootingArenaGameInstance::LogServerFrameRate(float DeltaTime)
{
	++ServerFrameCount;
	const double Now = FPlatformTime::Seconds();
	const double Elapsed = Now - ServerFrameRateLastLogTime;
	if (Elapsed >= 1.0)
	{
		UE_LOG(LogTemp, Log, TEXT("Server FPS: %.1f (%.2f ms/frame)"),
			ServerFrameCount / Elapsed, Elapsed * 1000.0 / ServerFrameCount);
		ServerFrameCount = 0;
		ServerFrameRateLastLogTime = Now;
	}
	return true;
}

void UShootingArenaGameInstance::HandlePreLoadMap(const FWorldContext& LoadingContext, const FString& MapName)
{
	if (LoadingContext.OwningGameInstance != this) return;
	FString BaseName = MapName;
	BaseName.Split(TEXT("?"), &BaseName, nullptr);
	if (BaseName.EndsWith(TEXT("MainMenu_Level")))
	{
		if (bLobbyReturnInProgress) HandleLobbyReturnFailure();
		// 결과창 "레벨 선택"으로 돌아오는 경우(bOpenMapSelectDirectly)에는 로컬 서버를 유지합니다.
		// 이 플래그는 BP_QuakeGameInstance 변수라 리플렉션으로 읽고, 메인 메뉴 Construct 에서 소비됩니다.
		bool bKeepLocalServer = false;
		if (const FBoolProperty* KeepProp = FindFProperty<FBoolProperty>(GetClass(), TEXT("bOpenMapSelectDirectly")))
		{
			bKeepLocalServer = KeepProp->GetPropertyValue_InContainer(this);
		}

		if (!bKeepLocalServer || !ULocalDedicatedServerLibrary::IsLocalDedicatedServerRunning())
		{
			ULocalDedicatedServerLibrary::StopLocalDedicatedServer();
		}

		// 메인 메뉴로 돌아오면 로비 접속은 끝난 것입니다. 접속 화면에 보여줄 메시지(PendingConnectMessage)는 남깁니다.
		ConnectState = ELobbyConnectState::Idle;
	}
}

void UShootingArenaGameInstance::BeginConnectAttempt(const FString& IP, int32 Port)
{
	SetSuppressAutomaticReconnect(false);
	MenuExitWorld.Reset();
	bLobbyReturnFailed = false;
	LastServerIP = IP;
	LastServerPort = Port;
	PendingConnectMessage = NAME_None;
	ConnectState = ELobbyConnectState::Connecting;
}

void UShootingArenaGameInstance::EndConnectAttempt()
{
	if (ConnectState == ELobbyConnectState::Connecting)
	{
		ConnectState = ELobbyConnectState::Idle;
	}
}

void UShootingArenaGameInstance::NotifyLobbyJoined()
{
	if (bLobbyReturnInProgress)
	{
		bLobbyReturnInProgress = false;
		MatchNetDriver.Reset();
		if (ULoadingScreenSubsystem* Loading = GetSubsystem<ULoadingScreenSubsystem>()) Loading->EndExternalLoading();
	}
	SetSuppressAutomaticReconnect(false);
	MenuExitWorld.Reset();
	bLobbyReturnFailed = false;
	ConnectState = ELobbyConnectState::Joined;
	PendingConnectMessage = NAME_None;
}

void UShootingArenaGameInstance::SetSuppressAutomaticReconnect(bool bSuppress)
{
	// BP_QuakeGameInstance otherwise queues its generic reconnect on the old connection's network errors.
	if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(GetClass(), TEXT("bIntentionalDisconnect")))
	{
		Property->SetPropertyValue_InContainer(this, bSuppress);
	}
}

void UShootingArenaGameInstance::BeginLobbyReturn(UWorld* World)
{
	bLobbyReturnInProgress = true;
	bLobbyReturnFailed = false;
	MatchNetDriver = World->GetNetDriver();
	PendingConnectMessage = NAME_None;
	SetSuppressAutomaticReconnect(true);
	if (ULoadingScreenSubsystem* Loading = GetSubsystem<ULoadingScreenSubsystem>())
	{
		Loading->BeginExternalLoading(TEXT("로비로 돌아가는 중..."));
	}
}

void UShootingArenaGameInstance::BeginIntentionalMenuExit(UWorld* World)
{
	bLobbyReturnInProgress = false;
	MatchNetDriver.Reset();
	MenuExitWorld = World;
	if (ConnectState == ELobbyConnectState::Joined)
	{
		SetSuppressAutomaticReconnect(true);
		ConnectState = ELobbyConnectState::Idle;
	}
}

void UShootingArenaGameInstance::HandleLobbyReturnFailure()
{
	bLobbyReturnInProgress = false;
	bLobbyReturnFailed = true;
	MatchNetDriver.Reset();
	SetPendingConnectMessage(TEXT("fail"));
	ConnectState = ELobbyConnectState::Idle;
}

void UShootingArenaGameInstance::SetPendingConnectMessage(FName MessageKey)
{
	// 먼저 정해진 이유를 유지합니다: 강퇴(kicked) 뒤에 연결이 끊겨도 'lost' 로 바뀌면 안 됩니다.
	if (PendingConnectMessage.IsNone())
	{
		PendingConnectMessage = MessageKey;
	}
}

FName UShootingArenaGameInstance::ConsumePendingConnectMessage()
{
	const FName Message = PendingConnectMessage;
	PendingConnectMessage = NAME_None;
	bLobbyReturnFailed = false;
	return Message;
}

void UShootingArenaGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 다른 GameInstance(PIE 의 서버 등)의 실패는 무시합니다.
	const FWorldContext* PendingContext = !World && NetDriver && GEngine
		? GEngine->GetWorldContextFromPendingNetGameNetDriver(NetDriver) : nullptr;
	if ((World && World->GetGameInstance() != this) || (!World && (!PendingContext || PendingContext->OwningGameInstance != this)))
	{
		return;
	}

	if (bLobbyReturnInProgress)
	{
		if (NetDriver != MatchNetDriver.Get()) HandleLobbyReturnFailure();
		return;
	}
	if (ConnectState == ELobbyConnectState::Connecting)
	{
		// 서버가 PreLogin 에서 거절한 이유가 문자열로 실려옵니다 ("full").
		const bool bFull = ErrorString.Contains(TEXT("full"), ESearchCase::IgnoreCase);
		SetPendingConnectMessage(bFull ? TEXT("full") : TEXT("fail"));
		ConnectState = ELobbyConnectState::Idle;
	}
	else if (ConnectState == ELobbyConnectState::Joined)
	{
		SetPendingConnectMessage(TEXT("lost"));
		ConnectState = ELobbyConnectState::Idle;
	}
}

void UShootingArenaGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (World && World->GetGameInstance() != this)
	{
		return;
	}

	if (bLobbyReturnInProgress)
	{
		HandleLobbyReturnFailure();
		return;
	}
	if (ConnectState == ELobbyConnectState::Connecting)
	{
		SetPendingConnectMessage(TEXT("fail"));
		ConnectState = ELobbyConnectState::Idle;
	}
	else if (ConnectState == ELobbyConnectState::Joined)
	{
		SetPendingConnectMessage(TEXT("lost"));
		ConnectState = ELobbyConnectState::Idle;
	}
}

FName UShootingArenaGameInstance::ResolveMatchBotCharacter(FName MapCharacterId, int32 SpawnIndex) const
{
	const UWorld* World = GetWorld();
	const AMyReconnectionGameMode* Mode = World ? World->GetAuthGameMode<AMyReconnectionGameMode>() : nullptr;
	const FName LobbyCharacterId = Mode ? Mode->GetLobbyBotCharacter(SpawnIndex) : NAME_None;
	return LobbyCharacterId.IsNone() ? MapCharacterId : LobbyCharacterId;
}

FString UShootingArenaGameInstance::MakeDefaultPlayerName(bool bCampaign) const
{
	FString Prefix = TEXT("Player");
	bool bAppendNumber = !bCampaign;
	if (PlayerDefaultNameSettings)
	{
		Prefix = PlayerDefaultNameSettings->DefaultNamePrefix;
		bAppendNumber = bCampaign ? PlayerDefaultNameSettings->bAppendNumberInCampaign : PlayerDefaultNameSettings->bAppendNumberInMultiplayer;
	}

	if (!bAppendNumber)
	{
		return Prefix;
	}

	TSet<FString> UsedNames;
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		for (const APlayerState* PS : GameState->PlayerArray)
		{
			if (PS)
			{
				UsedNames.Add(PS->GetPlayerName());
			}
		}
	}

	for (int32 Number = 1; ; ++Number)
	{
		const FString Candidate = FString::Printf(TEXT("%s%d"), *Prefix, Number);
		if (!UsedNames.Contains(Candidate))
		{
			return Candidate;
		}
	}
}

void UShootingArenaGameInstance::SetSavedNickname(const FString& NetworkAddress, const FString& Nickname)
{
	if (NetworkAddress.IsEmpty() || Nickname.IsEmpty())
	{
		return;
	}

	NicknameByNetworkAddress.Add(NetworkAddress, Nickname);
}

FString UShootingArenaGameInstance::GetSavedNickname(const FString& NetworkAddress) const
{
	if (const FString* Found = NicknameByNetworkAddress.Find(NetworkAddress))
	{
		return *Found;
	}

	return FString();
}

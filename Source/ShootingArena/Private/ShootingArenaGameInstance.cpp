#include "ShootingArenaGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "PlayerDefaultNameSettings.h"
#include "LocalDedicatedServerLibrary.h"
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
		PostLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
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
	}
}

void UShootingArenaGameInstance::Shutdown()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PreLoadMap.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
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

void UShootingArenaGameInstance::HandlePreLoadMap(const FString& MapName)
{
	FString BaseName = MapName;
	BaseName.Split(TEXT("?"), &BaseName, nullptr);
	if (BaseName.EndsWith(TEXT("MainMenu_Level")))
	{
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
	ConnectState = ELobbyConnectState::Joined;
	PendingConnectMessage = NAME_None;
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
	return Message;
}

void UShootingArenaGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 다른 GameInstance(PIE 의 서버 등)의 실패는 무시합니다.
	if (World && World->GetGameInstance() != this)
	{
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

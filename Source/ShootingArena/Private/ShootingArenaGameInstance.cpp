#include "ShootingArenaGameInstance.h"

#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "PlayerDefaultNameSettings.h"
#include "LocalDedicatedServerLibrary.h"
#include "UObject/UObjectGlobals.h"

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

	Super::Shutdown();
}

void UShootingArenaGameInstance::HandlePreLoadMap(const FString& MapName)
{
	FString BaseName = MapName;
	BaseName.Split(TEXT("?"), &BaseName, nullptr);
	if (BaseName.EndsWith(TEXT("MainMenu_Level")))
	{
		ULocalDedicatedServerLibrary::StopLocalDedicatedServer();
	}
}

FString UShootingArenaGameInstance::MakeDefaultPlayerName() const
{
	FString Prefix = TEXT("Player");
	bool bAppendNumber = true;
	if (PlayerDefaultNameSettings)
	{
		Prefix = PlayerDefaultNameSettings->DefaultNamePrefix;
		bAppendNumber = PlayerDefaultNameSettings->bAppendNumber;
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

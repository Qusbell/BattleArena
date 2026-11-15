#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"

#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "LobbyConnectLibrary.h"
#include "LobbyGameMode.h"
#include "LobbyGameState.h"
#include "LobbyPlayerController.h"
#include "Loading/LoadingBlueprintLibrary.h"
#include "Localization/LanguageSubsystem.h"
#include "MyPlayerState.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/UObjectIterator.h"

namespace
{
const TCHAR* LobbyMap = TEXT("/Game/QuakeLike_1_0/GameFlow/Lobby/Lobby_Level");

struct FLobbyReturnTest
{
	EPlayNetMode PreviousNetMode = PIE_Client;
	int32 PreviousClientCount = 1;
	uint16 PreviousServerPort = 0;
	bool bPreviousOneProcess = true;
	FString PreviousMap;
	int32 Stage = 0;
	double StageStarted = 0.0;
	TWeakObjectPtr<UGameInstance> LeavingClient;
	TWeakObjectPtr<UGameInstance> RemainingClient;
};

TArray<APlayerController*> GetControllers(UWorld* World)
{
	TArray<APlayerController*> Controllers;
	for (auto It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* Controller = It->Get(); IsValid(Controller)) Controllers.Add(Controller);
	}
	return Controllers;
}

bool IsMap(UWorld* World, const TCHAR* Map)
{
	return IsValid(World) && World->GetMapName().EndsWith(Map);
}

void KeepEditorServerListening(UWorld* Server)
{
	// LoadMap only restarts networking for '?listen' when the dedicated PIE world shares the editor process.
	// DefaultPort is set on the test's PIE server to keep the address across absolute travels.
	if (!Server->NextURL.IsEmpty() && !Server->NextURL.EndsWith(TEXT("?listen")))
	{
		Server->NextURL += TEXT("?listen");
	}
}
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FVerifyLobbyReturn, TSharedPtr<FLobbyReturnTest>, Context, FAutomationTestBase*, Test);

bool FVerifyLobbyReturn::Update()
{
	const double Now = FPlatformTime::Seconds();
	if (Context->StageStarted == 0.0) Context->StageStarted = Now;
	if (Now - Context->StageStarted > 45.0)
	{
		Test->AddError(FString::Printf(TEXT("Lobby flow timed out at stage %d"), Context->Stage));
		return true;
	}
	auto Advance = [&]() { ++Context->Stage; Context->StageStarted = Now; };

	UWorld* Server = nullptr;
	TArray<UWorld*> Clients;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType != EWorldType::PIE || !IsValid(WorldContext.World())) continue;
		if (WorldContext.RunAsDedicated) Server = WorldContext.World();
		else Clients.Add(WorldContext.World());
	}
	if (!Server || Clients.Num() != 2) return false;
	KeepEditorServerListening(Server);
	const auto Controllers = GetControllers(Server);
	UGameInstance* Leaving = Context->LeavingClient.Get();
	UGameInstance* Remaining = Context->RemainingClient.Get();
	UWorld* LeavingWorld = Leaving ? Leaving->GetWorld() : nullptr;
	UWorld* RemainingWorld = Remaining ? Remaining->GetWorld() : nullptr;

	switch (Context->Stage)
	{
	case 0:
		if (auto* Mode = Server->GetAuthGameMode<ALobbyGameMode>(); Mode && Controllers.Num() == 2
			&& IsMap(Clients[0], TEXT("Lobby_Level")) && IsMap(Clients[1], TEXT("Lobby_Level")))
		{
			auto* Lobby = Server->GetGameState<ALobbyGameState>();
			if (!Lobby || !Lobby->HostPlayerState || Server->GetNetMode() != NM_DedicatedServer) return false;
			if (!Lobby->AddBot({FName(TEXT("C06"))}, ELobbyDifficulty::Hard)
				|| !Lobby->AddBot({FName(TEXT("C06"))}, ELobbyDifficulty::Easy))
			{
				Test->AddError(TEXT("Unable to populate the lobby roster"));
				return true;
			}
			Context->LeavingClient = Clients[0]->GetGameInstance();
			Context->RemainingClient = Clients[1]->GetGameInstance();
			Mode->BeginLaunch();
			KeepEditorServerListening(Server);
			Advance();
		}
		break;
	case 1:
		if (IsMap(Server, TEXT("Lvl_PortalMap")) && Controllers.Num() == 2
			&& IsMap(LeavingWorld, TEXT("Lvl_PortalMap")) && IsMap(RemainingWorld, TEXT("Lvl_PortalMap")))
		{
			const AGameStateBase* ClientGameState = RemainingWorld->GetGameState();
			if (!IsValid(ClientGameState)) return false;
			const FText Empty;
			const FText BaseName = ULanguageLibrary::DisplayName(TEXT("Character"), TEXT("C06"));
			const FString SecondName = ULanguageLibrary::FormatText(TEXT("Common_NameNumber"), BaseName,
				FText::AsCultureInvariant(TEXT("2")), Empty, Empty, Empty, Empty, Empty, Empty).ToString();
			TSet<FString> Names;
			for (APlayerState* State : ClientGameState->PlayerArray)
			{
				if (!IsValid(State) || !State->IsABot()) continue;
				const FString ResultName = ULanguageLibrary::LobbyBotResultName(State, FString());
				if (ResultName.IsEmpty()) return false;
				Test->TestEqual(TEXT("Gameplay and results use the same replicated bot identity"),
					ULanguageLibrary::ParticipantDisplayString(State), ResultName);
				Names.Add(ULanguageLibrary::ReadDisplayString(ResultName).ToString());
			}
			if (Names.Num() < 2) return false;
			Test->TestTrue(TEXT("Both names retain their lobby suffixes despite difficulty sort"),
				Names.Contains(BaseName.ToString()) && Names.Contains(SecondName));
			ULoadingBlueprintLibrary::BeginLoadingAndReturnToMainMenu(LeavingWorld, TEXT("Lobby test"));
			Advance();
		}
		break;
	case 2:
		if (IsMap(LeavingWorld, TEXT("TestMainMenu_Level")) && Controllers.Num() == 1 && IsMap(Server, TEXT("Lvl_PortalMap")))
		{
			auto* State = Controllers[0]->GetPlayerState<AMyPlayerState>();
			if (!State) return false;
			State->Server_SetWantsReturnToLobby(true);
			KeepEditorServerListening(Server);
			Advance();
		}
		break;
	case 3:
		if (auto* Mode = Server->GetAuthGameMode<ALobbyGameMode>(); Mode && Controllers.Num() == 1
			&& IsMap(RemainingWorld, TEXT("Lobby_Level")) && IsMap(LeavingWorld, TEXT("TestMainMenu_Level")))
		{
			Mode->BeginLaunch();
			KeepEditorServerListening(Server);
			Advance();
		}
		break;
	case 4:
		if (IsMap(RemainingWorld, TEXT("Lvl_PortalMap")) && Controllers.Num() == 1)
		{
			const APlayerController* LocalController = RemainingWorld->GetFirstPlayerController();
			if (!LocalController || !LocalController->PlayerState) return false;
			ULoadingBlueprintLibrary::BeginLoadingAndReturnToMainMenu(RemainingWorld, TEXT("Lobby test"));
			Advance();
		}
		break;
	case 5:
		if (IsMap(Server, TEXT("Lobby_Level")) && Controllers.IsEmpty()
			&& IsMap(RemainingWorld, TEXT("TestMainMenu_Level")) && IsMap(LeavingWorld, TEXT("TestMainMenu_Level")))
		{
			if (!RemainingWorld->GetFirstPlayerController()) return false;
			ULobbyConnectLibrary::ConnectToServer(RemainingWorld, TEXT("127.0.0.1"), FURL::UrlConfig.DefaultPort, FString());
			Advance();
		}
		break;
	case 6:
		if (IsMap(RemainingWorld, TEXT("Lobby_Level")) && Controllers.Num() == 1)
		{
			const auto* Lobby = Server->GetGameState<ALobbyGameState>();
			if (!Lobby || !Lobby->HostPlayerState) return false;
			Test->TestEqual(TEXT("The next lobby has a fresh roster"), Lobby->GetBotCount(), 0);
			return true;
		}
		break;
	}
	return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FRestoreLobbyReturnTest, TSharedPtr<FLobbyReturnTest>, Context);

bool FRestoreLobbyReturnTest::Update()
{
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType == EWorldType::PIE) return false;
	}
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->SetPlayNetMode(Context->PreviousNetMode);
	Settings->SetPlayNumberOfClients(Context->PreviousClientCount);
	Settings->SetRunUnderOneProcess(Context->bPreviousOneProcess);
	Settings->SetServerPort(Context->PreviousServerPort);
	if (!Context->PreviousMap.IsEmpty()) FAutomationEditorCommonUtils::LoadMap(Context->PreviousMap);
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FReleaseLobbyTestWidgets);

bool FReleaseLobbyTestWidgets::Update()
{
	TSet<const UGameInstance*> TestInstances;
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.WorldType == EWorldType::PIE && WorldContext.OwningGameInstance)
		{
			TestInstances.Add(WorldContext.OwningGameInstance);
		}
	}
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (!IsValid(Widget) || Widget->IsTemplate()) continue;
		const UGameInstance* Owner = Widget->GetTypedOuter<UGameInstance>();
		const UWorld* World = Widget->GetWorld();
		if (TestInstances.Contains(Owner) || (IsValid(World) && TestInstances.Contains(World->GetGameInstance())))
		{
			Widget->RemoveFromParent();
			Widget->ReleaseSlateResources(true);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLobbyReturnPIETest, "ShootingArena.PIE.Network.LobbyReturn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLobbyReturnPIETest::RunTest(const FString&)
{
	if (GEditor->PlayWorld)
	{
		AddError(TEXT("End the active PIE session before running the lobby test"));
		return false;
	}
	// The checked-in test map currently reports these unrelated content validation errors on load.
	AddExpectedErrorPlain(TEXT("저장된 Area Graph가 없습니다."), EAutomationExpectedErrorFlags::Contains, -1);
	AddExpectedErrorPlain(TEXT("[PathLink][INVALID] Link=BP_PathLink_C_UAID_D843AE972941CF0503_1721237824"),
		EAutomationExpectedErrorFlags::Contains, -1);
	// These matches are interrupted on purpose. Keep the analyzer's existing forced-close validation outside this test's assertions.
	AddExpectedError(TEXT("Match [0-9A-F]+ validation found: (MISSING_SPAWN_END x[0-9]+, )?dangling references x[0-9]+"),
		EAutomationExpectedErrorFlags::Contains, -1);
	auto Context = MakeShared<FLobbyReturnTest>();
	Context->PreviousMap = GEditor->GetEditorWorldContext().World()->GetOutermost()->GetName();
	ULevelEditorPlaySettings* Settings = GetMutableDefault<ULevelEditorPlaySettings>();
	Settings->GetPlayNetMode(Context->PreviousNetMode);
	Settings->GetPlayNumberOfClients(Context->PreviousClientCount);
	Settings->GetRunUnderOneProcess(Context->bPreviousOneProcess);
	Settings->GetServerPort(Context->PreviousServerPort);
	Settings->SetPlayNetMode(PIE_Client);
	Settings->SetPlayNumberOfClients(2);
	Settings->SetRunUnderOneProcess(true);
	Settings->SetServerPort(FURL::UrlConfig.DefaultPort);
	FAutomationEditorCommonUtils::LoadMap(LobbyMap);
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FVerifyLobbyReturn(Context, this));
	ADD_LATENT_AUTOMATION_COMMAND(FReleaseLobbyTestWidgets());
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FRestoreLobbyReturnTest(Context));
	return true;
}

#endif

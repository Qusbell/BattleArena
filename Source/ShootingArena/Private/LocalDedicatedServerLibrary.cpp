#include "LocalDedicatedServerLibrary.h"

#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "IPAddress.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "SocketSubsystem.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	// 실행 중인 로컬 전용 서버 프로세스 핸들.
	FProcHandle GLocalDedicatedServerHandle;

	// 현재 로컬 서버 실행에 대응하는 Ready 파일의 절대경로.
	// 클라이언트 프로세스에서만 의미 있는 값입니다.
	FString GLocalDedicatedServerReadyFilePath;

	// 현재 로컬 서버 실행에 대응하는 "맵 변경 명령" 파일의 절대경로.
	// 클라이언트가 내용(이동할 맵 이름)을 쓰고, 서버 프로세스가 주기적으로 읽어 처리한 뒤 삭제합니다.
	FString GLocalDedicatedServerCommandFilePath;

	// 이 프로세스에서 OnPreExit(게임 종료 시 서버 프로세스 정리) 핸들러를 한 번만 등록하기 위한 플래그.
	bool GLocalServerExitCleanupRegistered = false;

	constexpr TCHAR LocalServerReadyArgument[] =
		TEXT("LocalServerReadyFile=");

	constexpr TCHAR LocalServerCommandArgument[] =
		TEXT("LocalServerCommandFile=");

	// ------------------------------------------------------------------------
	// 서버 프로세스 측 로직: 맵 변경 명령 감시 + Ready 파일 갱신
	// ------------------------------------------------------------------------

	FTSTicker::FDelegateHandle GServerCommandTickerHandle;

	// 맵 전환(또는 "이미 그 맵" 처리) 이후 Ready 파일을 다시 써야 함을 표시합니다.
	// NetDriver 가 아직 준비되지 않았을 수 있으므로 감시 틱에서 준비될 때까지 재시도합니다.
	bool GServerNeedsReadyWrite = false;

	// 맵 이름 비교용 정규화: URL 옵션(?...)과 경로/오브젝트 접미사(.Name)를 제거하고 짧은 이름만 남깁니다.
	FString NormalizeMapName(const FString& InMapName)
	{
		FString Name = InMapName.TrimStartAndEnd();

		int32 OptionIdx = INDEX_NONE;
		if (Name.FindChar(TEXT('?'), OptionIdx))
		{
			Name.LeftInline(OptionIdx);
		}

		return FPaths::GetBaseFilename(Name);
	}

	UWorld* FindDedicatedServerWorld()
	{
		if (GEngine == nullptr)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (Context.WorldType == EWorldType::Game
				&& IsValid(World)
				&& World->GetNetMode() == NM_DedicatedServer)
			{
				return World;
			}
		}

		return nullptr;
	}

	// NetDriver 가 클라이언트를 받을 수 있는 상태이면 Ready 파일을 씁니다.
	bool TryWriteReadyFile(UWorld* World)
	{
		if (!IsValid(World) || World->GetNetMode() != NM_DedicatedServer)
		{
			return false;
		}

		// 맵 로드/BeginPlay 가 끝나기 전에는 접속을 받으면 안 됩니다.
		if (!World->HasBegunPlay())
		{
			return false;
		}

		UNetDriver* NetDriver = World->GetNetDriver();
		if (!IsValid(NetDriver) || !NetDriver->IsNetResourceValid())
		{
			return false;
		}

		FString ReadyFilePath;
		if (!FParse::Value(FCommandLine::Get(), LocalServerReadyArgument, ReadyFilePath)
			|| ReadyFilePath.IsEmpty())
		{
			return false;
		}

		return FFileHelper::SaveStringToFile(TEXT("READY"), *ReadyFilePath);
	}

	// 서버 프로세스 전용 틱: 명령 파일이 있으면 맵을 전환하고, 전환 후 Ready 를 다시 기록합니다.
	bool TickServerCommandWatcher(float /*DeltaTime*/)
	{
		FString CommandFilePath;
		FString ReadyFilePath;
		FParse::Value(FCommandLine::Get(), LocalServerCommandArgument, CommandFilePath);
		FParse::Value(FCommandLine::Get(), LocalServerReadyArgument, ReadyFilePath);

		UWorld* World = FindDedicatedServerWorld();
		if (World == nullptr)
		{
			return true;
		}

		// 이미 맵 전환이 예약/진행 중이면 이번 틱은 건너뜁니다 (명령 파일은 남겨 둡니다).
		if (!World->NextURL.IsEmpty() || World->IsInSeamlessTravel())
		{
			GServerNeedsReadyWrite = true;
			return true;
		}

		FString TargetMap;
		if (!CommandFilePath.IsEmpty()
			&& FPaths::FileExists(CommandFilePath)
			&& FFileHelper::LoadFileToString(TargetMap, *CommandFilePath))
		{
			TargetMap = TargetMap.TrimStartAndEnd();
			IFileManager::Get().Delete(*CommandFilePath, false, true);

			if (!TargetMap.IsEmpty())
			{
				// 전환 동안은 접속을 받으면 안 되므로 Ready 를 해제합니다.
				if (!ReadyFilePath.IsEmpty())
				{
					IFileManager::Get().Delete(*ReadyFilePath, false, true);
				}

				GServerNeedsReadyWrite = true;

				// URL 옵션(?Difficulty= 등)이 붙은 요청은 게임모드가 옵션을 다시 읽어야 하므로
				// 같은 맵이어도 반드시 재로드(ServerTravel)합니다. 옵션 없는 요청만 "이미 그 맵"으로 건너뜁니다.
				const bool bHasOptions = TargetMap.Contains(TEXT("?"));

				if (!bHasOptions
					&& NormalizeMapName(TargetMap) == FPaths::GetBaseFilename(World->GetMapName()))
				{
					UE_LOG(LogTemp, Log, TEXT("[LocalDedicatedServer] 이미 '%s' 맵입니다. 전환 없이 Ready 만 복구합니다."), *TargetMap);
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[LocalDedicatedServer] 맵 변경 명령 수신 -> ServerTravel('%s')"), *TargetMap);

					// bAbsolute=false: 서버 시작 시 붙은 URL 옵션(?Game= 등)을 그대로 유지합니다.
					World->ServerTravel(TargetMap, false);
				}

				return true;
			}
		}

		// 대기 중인 명령이 없을 때만 Ready 를 (재)기록합니다.
		if (GServerNeedsReadyWrite
			&& (CommandFilePath.IsEmpty() || !FPaths::FileExists(CommandFilePath))
			&& TryWriteReadyFile(World))
		{
			GServerNeedsReadyWrite = false;
		}

		return true;
	}

	// 서버 프로세스에서만 명령 감시 틱을 시작합니다 (여러 번 호출해도 한 번만 등록).
	void EnsureServerCommandWatcher()
	{
		if (GServerCommandTickerHandle.IsValid())
		{
			return;
		}

		FString CommandFilePath;
		if (!FParse::Value(FCommandLine::Get(), LocalServerCommandArgument, CommandFilePath)
			|| CommandFilePath.IsEmpty())
		{
			return;
		}

		// 최초 부팅 후 Ready 도 이 틱이 기록합니다 (게임모드 BP 가 Mark 를 호출하지 않아도 동작).
		GServerNeedsReadyWrite = true;

		GServerCommandTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateStatic(&TickServerCommandWatcher), 0.2f);

		// 맵이 새로 로드될 때마다(= 서버 전환 완료) Ready 를 다시 기록해야 합니다.
		FCoreUObjectDelegates::PostLoadMapWithWorld.AddLambda([](UWorld*)
		{
			GServerNeedsReadyWrite = true;
		});
	}

	// 매치 서버(로비 서버가 스폰하는 별도 프로세스)용 핸들/Ready 파일 경로.
	// 위 캠페인용 상태와는 완전히 독립적으로 추적합니다.
	FProcHandle GMatchServerHandle;
	FString GMatchServerReadyFilePath;

	// 지금 이 프로세스가 StartLocalDedicatedServer / StartMatchServer 가 스폰한
	// 서버 프로세스인지 여부.
	// 스폰 시 항상 -LocalServerReadyFile= 인자를 붙이므로 그 존재 여부로 판별합니다.
	bool IsRunningAsSpawnedLocalServer()
	{
		FString Unused;
		return FParse::Value(
			FCommandLine::Get(), LocalServerReadyArgument, Unused);
	}

	// 패키징된 빌드에서 로컬에 띄울 "진짜" Dedicated Server 실행 파일
	// (<Project>Server.exe)의 절대경로를 해석합니다.
	//
	// 지금 이 프로세스(ShootingArena.exe)는 Game 타겟이라, -server 로 재실행해도
	// FPlatformProperties::IsGameOnly()==true 때문에 IsRunningDedicatedServer()가
	// 항상 false 가 되고 창 달린 클라이언트(리슨서버)로 떠 버립니다. 그래서 별도
	// Server 타겟 산출물을 실행해야 합니다.
	//
	// 스테이징 레이아웃 예:
	//   .../Windows/<Project>/Binaries/Win64/<Project>.exe             (클라이언트: 이 프로세스)
	//   .../WindowsServer/<Project>/Binaries/Win64/<Project>Server.exe (서버: 찾는 대상)
	//   .../Windows/<Project>.exe                                      (플랫 런처 레이아웃)
	//   .../WindowsServer/<Project>Server.exe
	//
	// 찾지 못하면 빈 문자열을 반환합니다(호출부에서 에러 처리).
	// 에디터 빌드에서는 사용되지 않으므로(에디터는 UnrealEditor.exe 재실행) 컴파일 대상에서 제외합니다.
#if !WITH_EDITOR
	FString ResolveLocalDedicatedServerExecutable()
	{
		// 커맨드라인으로 직접 지정할 수 있는 우회로(특수 배치/테스트용).
		FString OverridePath;
		if (FParse::Value(FCommandLine::Get(), TEXT("LocalServerExe="), OverridePath)
			&& !OverridePath.IsEmpty())
		{
			OverridePath = FPaths::ConvertRelativePathToFull(OverridePath);
			if (FPaths::FileExists(OverridePath))
			{
				return OverridePath;
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[LocalDedicatedServer] -LocalServerExe 로 지정된 경로가 존재하지 않습니다: %s"),
				*OverridePath
			);
		}

		const FString ClientExePath =
			FPaths::ConvertRelativePathToFull(FPlatformProcess::ExecutablePath());

		const FString ServerExeName =
			FString(FApp::GetProjectName()) + TEXT("Server.exe");

		TArray<FString> Candidates;

		// 1) 전체 경로에서 마지막 "/Windows/" 스테이징 폴더를 "/WindowsServer/" 로 치환하고
		//    실행 파일 이름을 <Project>Server.exe 로 교체.
		//    (Deep 레이아웃과 Flat 런처 레이아웃 모두 커버)
		{
			const int32 WindowsFolderIdx = ClientExePath.Find(
				TEXT("/Windows/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);

			if (WindowsFolderIdx != INDEX_NONE)
			{
				const FString ServerRoot =
					ClientExePath.Left(WindowsFolderIdx)
					+ TEXT("/WindowsServer/")
					+ ClientExePath.Mid(WindowsFolderIdx + 9 /* len("/Windows/") */);

				Candidates.Add(FPaths::Combine(
					FPaths::GetPath(ServerRoot), ServerExeName));
			}
		}

		// 2) 클라이언트 exe 위치 기준 상대 경로 백업 후보들.
		const FString ClientDir = FPaths::GetPath(ClientExePath);

		//   Deep: <ROOT>/Windows/<Project>/Binaries/Win64 -> <ROOT> 로 4단계 상위
		Candidates.Add(FPaths::Combine(
			ClientDir, TEXT("../../../.."), TEXT("WindowsServer"),
			FApp::GetProjectName(), TEXT("Binaries"), TEXT("Win64"), ServerExeName));

		//   Flat: <ROOT>/Windows -> <ROOT> 로 1단계 상위
		Candidates.Add(FPaths::Combine(
			ClientDir, TEXT(".."), TEXT("WindowsServer"), ServerExeName));

		//   Flat 클라이언트 + Deep 서버
		Candidates.Add(FPaths::Combine(
			ClientDir, TEXT(".."), TEXT("WindowsServer"),
			FApp::GetProjectName(), TEXT("Binaries"), TEXT("Win64"), ServerExeName));

		for (const FString& Candidate : Candidates)
		{
			const FString Full = FPaths::ConvertRelativePathToFull(Candidate);
			if (FPaths::FileExists(Full))
			{
				return Full;
			}
		}

		UE_LOG(
			LogTemp,
			Error,
			TEXT("[LocalDedicatedServer] Server 타겟 실행 파일(%s)을 찾지 못했습니다. "
				"클라이언트 exe=%s / 시도한 후보 %d개"),
			*ServerExeName,
			*ClientExePath,
			Candidates.Num()
		);

		return FString();
	}
#endif // !WITH_EDITOR
}



bool ULocalDedicatedServerLibrary::StartLocalDedicatedServer(
    const FString& MapName,
    int32 Port)
{
    if (IsRunningDedicatedServer())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] Dedicated Server 프로세스에서는 "
                "다른 로컬 Dedicated Server를 시작하지 않습니다.")
        );

        return false;
    }

    // 패키징 빌드에서 IsRunningDedicatedServer()가 -server 를 무시하고 항상 false 인
    // 케이스(Game 타겟이 잘못 재실행된 경우 등)를 대비한 이중 가드.
    // 우리가 스폰한 서버 프로세스는 항상 -LocalServerReadyFile= 인자를 갖습니다.
    if (IsRunningAsSpawnedLocalServer())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] 이 프로세스는 스폰된 서버 프로세스이므로 "
                "또 다른 로컬 서버를 시작하지 않습니다.")
        );

        return false;
    }

    if (IsLocalDedicatedServerRunning())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] 이미 실행 중인 로컬 전용 서버가 있어 새로 시작하지 않습니다.")
        );

        return false;
    }

    // 이전에 유효하지 않게 된 핸들이 남아 있다면 정리합니다.
    if (GLocalDedicatedServerHandle.IsValid())
    {
        FPlatformProcess::CloseProc(GLocalDedicatedServerHandle);
        GLocalDedicatedServerHandle.Reset();
    }

    // --------------------------------------------------------------------
    // Ready 파일 경로 생성
    //
    // Saved/LocalDedicatedServer/Ready_<GUID>.flag
    //
    // 실행마다 GUID를 새로 만들어 이전 실행의 Ready 파일과 절대 충돌하지
    // 않도록 합니다.
    // --------------------------------------------------------------------

    const FString ReadyDirectory = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("LocalDedicatedServer")
        )
    );

    if (!IFileManager::Get().MakeDirectory(*ReadyDirectory, true))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LocalDedicatedServer] Ready 디렉터리를 생성하지 못했습니다: %s"),
            *ReadyDirectory
        );

        return false;
    }

    const FString ReadyFileName = FString::Printf(
        TEXT("Ready_%s.flag"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits)
    );

    GLocalDedicatedServerReadyFilePath = FPaths::Combine(
        ReadyDirectory,
        ReadyFileName
    );

    // 혹시 동일 경로에 파일이 존재한다면 제거합니다.
    IFileManager::Get().Delete(
        *GLocalDedicatedServerReadyFilePath,
        false,
        true
    );

    // 맵 변경 명령 파일 경로 (실행마다 GUID 로 구분, 서버가 읽고 삭제합니다).
    GLocalDedicatedServerCommandFilePath = FPaths::Combine(
        ReadyDirectory,
        FString::Printf(TEXT("Cmd_%s.txt"), *FGuid::NewGuid().ToString(EGuidFormats::Digits))
    );

    IFileManager::Get().Delete(
        *GLocalDedicatedServerCommandFilePath,
        false,
        true
    );

    // 게임이 종료될 때 서버 프로세스가 고아로 남지 않도록 정리 핸들러를 한 번만 등록합니다.
    // (서버는 bLaunchDetached 로 실행되므로 클라이언트 종료와 함께 죽지 않습니다.)
    if (!GLocalServerExitCleanupRegistered)
    {
        GLocalServerExitCleanupRegistered = true;
        FCoreDelegates::OnPreExit.AddStatic(&ULocalDedicatedServerLibrary::StopLocalDedicatedServer);
    }

    // --------------------------------------------------------------------
    // 서버 실행 파일 결정
    //
    // - 에디터: UnrealEditor.exe(지금 이 프로세스)를 그대로 재실행합니다. UnrealEditor 는
    //   커맨드라인 -game -server 를 존중하므로 진짜 헤드리스 Dedicated Server 가 됩니다.
    // - 패키징 빌드: 지금 이 프로세스(ShootingArena.exe, Game 타겟)를 -server 로 재실행하면
    //   IsGameOnly()==true 라서 -server 가 무시되고 창 달린 클라이언트로 떠 버립니다.
    //   그래서 같은 산출물에 함께 스테이징된 별도 Server 타겟 실행 파일
    //   (<Project>Server.exe, WindowsServer 폴더)을 찾아 실행합니다.
    // --------------------------------------------------------------------

#if WITH_EDITOR
    const FString ServerExePath = FPlatformProcess::ExecutablePath();
#else
    const FString ServerExePath = ResolveLocalDedicatedServerExecutable();
#endif

    if (ServerExePath.IsEmpty() || !FPaths::FileExists(ServerExePath))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LocalDedicatedServer] 서버 실행 파일을 찾을 수 없습니다: \"%s\" "
                "(패키징 빌드라면 WindowsServer 산출물이 클라이언트와 같은 위치에 "
                "스테이징되어 있는지 확인하세요.)"),
            *ServerExePath
        );

        GLocalDedicatedServerReadyFilePath.Reset();
        GLocalDedicatedServerCommandFilePath.Reset();
        return false;
    }

#if WITH_EDITOR
    // UnrealEditor.exe는 어떤 프로젝트인지 모르므로 .uproject 경로를 첫 인자로 넘겨줘야 하고,
    // -game으로 게임 모드임을, -server로 데디케이티드 서버임을 명시해야 합니다.
    const FString ProjectFilePath =
        FPaths::ConvertRelativePathToFull(
            FPaths::GetProjectFilePath()
        );

    // Ready 파일의 "절대경로"를 서버 프로세스에 전달합니다.
    const FString Params = FString::Printf(
        TEXT("\"%s\" %s -game -server -log -NoLiveCoding -port=%d -LocalServerReadyFile=\"%s\" -LocalServerCommandFile=\"%s\""),
        *ProjectFilePath,
        *MapName,
        Port,
        *GLocalDedicatedServerReadyFilePath,
        *GLocalDedicatedServerCommandFilePath
    );
#else
    // <Project>Server.exe 는 Server 타겟(UE_SERVER=1)이라 이미 데디케이티드 서버이고,
    // 이 프로젝트 전용으로 빌드된 실행 파일이라 .uproject 경로도 -game 도 필요 없습니다.
    // (-server 는 중복이지만 무해하므로 의도를 드러내기 위해 유지합니다.)
    const FString Params = FString::Printf(
        TEXT("%s -server -log -port=%d -LocalServerReadyFile=\"%s\" -LocalServerCommandFile=\"%s\""),
        *MapName,
        Port,
        *GLocalDedicatedServerReadyFilePath,
        *GLocalDedicatedServerCommandFilePath
    );
#endif

    uint32 OutProcessID = 0;

    GLocalDedicatedServerHandle = FPlatformProcess::CreateProc(
        *ServerExePath,
        *Params,
        true,   // bLaunchDetached
        false,  // bLaunchHidden
        false,  // bLaunchReallyHidden
        &OutProcessID,
        0,
        nullptr,
        nullptr
    );

    if (!GLocalDedicatedServerHandle.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LocalDedicatedServer] 서버 프로세스 실행에 실패했습니다: %s %s"),
            *ServerExePath,
            *Params
        );

        // 서버 실행에 실패했으므로 Ready 상태도 폐기합니다.
        IFileManager::Get().Delete(
            *GLocalDedicatedServerReadyFilePath,
            false,
            true
        );

        GLocalDedicatedServerReadyFilePath.Reset();
        GLocalDedicatedServerCommandFilePath.Reset();

        return false;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT(
            "[LocalDedicatedServer] Spawned. "
            "CallerPID=%u / SpawnedPID=%u / IsDedicated=%d / ReadyFile=%s"
        ),
        FPlatformProcess::GetCurrentProcessId(),
        OutProcessID,
        IsRunningDedicatedServer() ? 1 : 0,
        * GLocalDedicatedServerReadyFilePath
    );

    return true;
}

void ULocalDedicatedServerLibrary::StopLocalDedicatedServer()
{
    if (GLocalDedicatedServerHandle.IsValid())
    {
        FPlatformProcess::TerminateProc(
            GLocalDedicatedServerHandle,
            true
        );

        FPlatformProcess::CloseProc(
            GLocalDedicatedServerHandle
        );

        GLocalDedicatedServerHandle.Reset();
    }

    if (!GLocalDedicatedServerReadyFilePath.IsEmpty())
    {
        IFileManager::Get().Delete(
            *GLocalDedicatedServerReadyFilePath,
            false,
            true
        );

        GLocalDedicatedServerReadyFilePath.Reset();
    }

    if (!GLocalDedicatedServerCommandFilePath.IsEmpty())
    {
        IFileManager::Get().Delete(
            *GLocalDedicatedServerCommandFilePath,
            false,
            true
        );

        GLocalDedicatedServerCommandFilePath.Reset();
    }
}

void ULocalDedicatedServerLibrary::InitServerCommandWatcher()
{
    EnsureServerCommandWatcher();
}

bool ULocalDedicatedServerLibrary::SetLocalDedicatedServerMap(const FString& MapName)
{
    const FString Trimmed = MapName.TrimStartAndEnd();

    if (Trimmed.IsEmpty()
        || GLocalDedicatedServerCommandFilePath.IsEmpty()
        || !IsLocalDedicatedServerRunning())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] 맵 변경 요청 실패: 서버가 실행 중이 아니거나 맵 이름이 비어 있습니다 (%s)"),
            *Trimmed
        );

        return false;
    }

    // 서버가 쓰는 도중의 파일을 읽지 않도록 임시 파일에 쓴 뒤 이동(원자적 교체)합니다.
    const FString TempPath = GLocalDedicatedServerCommandFilePath + TEXT(".tmp");

    if (!FFileHelper::SaveStringToFile(Trimmed, *TempPath)
        || !IFileManager::Get().Move(*GLocalDedicatedServerCommandFilePath, *TempPath, true, true))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LocalDedicatedServer] 맵 변경 명령 파일 기록 실패: %s"),
            *GLocalDedicatedServerCommandFilePath
        );

        IFileManager::Get().Delete(*TempPath, false, true);
        return false;
    }

    // 명령을 먼저 쓴 "뒤에" Ready 를 해제합니다. 서버가 전환을 마치면 Ready 를 다시 만듭니다.
    if (!GLocalDedicatedServerReadyFilePath.IsEmpty())
    {
        IFileManager::Get().Delete(
            *GLocalDedicatedServerReadyFilePath,
            false,
            true
        );
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[LocalDedicatedServer] 서버 맵 변경 요청: %s"),
        *Trimmed
    );

    return true;
}

bool ULocalDedicatedServerLibrary::IsLocalDedicatedServerRunning()
{
	return GLocalDedicatedServerHandle.IsValid() && FPlatformProcess::IsProcRunning(GLocalDedicatedServerHandle);
}



bool ULocalDedicatedServerLibrary::IsLocalDedicatedServerReady()
{
    // Ready 파일이 있어도 서버 프로세스가 죽어 있다면 Ready가 아닙니다.
    if (!IsLocalDedicatedServerRunning())
    {
        return false;
    }

    if (GLocalDedicatedServerReadyFilePath.IsEmpty())
    {
        return false;
    }

    return FPaths::FileExists(
        GLocalDedicatedServerReadyFilePath
    );
}


bool ULocalDedicatedServerLibrary::MarkLocalDedicatedServerReady(
    UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] Ready 처리 실패: WorldContextObject가 유효하지 않습니다.")
        );

        return false;
    }

    UWorld* World = WorldContextObject->GetWorld();

    if (!IsValid(World))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] Ready 처리 실패: World를 찾을 수 없습니다.")
        );

        return false;
    }

    // 실수로 클라이언트나 Standalone에서 호출되는 것을 방지합니다.
    if (World->GetNetMode() != NM_DedicatedServer)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] Ready 처리는 Dedicated Server에서만 가능합니다.")
        );

        return false;
    }

    UNetDriver* NetDriver = World->GetNetDriver();

    if (!IsValid(NetDriver))
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT("[LocalDedicatedServer] 아직 NetDriver가 생성되지 않았습니다.")
        );

        return false;
    }

    if (!NetDriver->IsNetResourceValid())
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT("[LocalDedicatedServer] 아직 NetDriver의 네트워크 리소스가 준비되지 않았습니다.")
        );

        return false;
    }

    // 로컬 싱글 서버(-LocalServerCommandFile 인자가 있는 프로세스)라면 맵 변경 명령 감시를 시작합니다.
    // 이후 맵이 바뀌어도 감시 틱이 Ready 파일을 다시 기록하므로 이 함수가 다시 호출될 필요는 없습니다.
    EnsureServerCommandWatcher();

    // 서버가 부팅하는 사이 클라이언트가 이미 다른 맵을 요청했다면, 그 맵으로 전환이 끝난 뒤
    // 감시 틱이 Ready 를 기록합니다. 지금 Ready 를 쓰면 전환 중에 접속이 들어올 수 있으므로 건너뜁니다.
    {
        FString PendingCommandFile;
        if (FParse::Value(FCommandLine::Get(), LocalServerCommandArgument, PendingCommandFile)
            && !PendingCommandFile.IsEmpty()
            && FPaths::FileExists(PendingCommandFile))
        {
            GServerNeedsReadyWrite = true;
            return false;
        }
    }

    // 클라이언트 프로세스가 CreateProc() 실행 시 넘겨준 경로를 읽습니다.
    FString ReadyFilePath;

    if (!FParse::Value(
        FCommandLine::Get(),
        LocalServerReadyArgument,
        ReadyFilePath))
    {
        UE_LOG(
            LogTemp,
            Verbose,
            TEXT("[LocalDedicatedServer] LocalServerReadyFile 실행 인자를 찾을 수 없습니다.")
        );

        return false;
    }

    if (ReadyFilePath.IsEmpty())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[LocalDedicatedServer] Ready 파일 경로가 비어 있습니다.")
        );

        return false;
    }

    const bool bSaved = FFileHelper::SaveStringToFile(
        TEXT("READY"),
        *ReadyFilePath
    );

    if (!bSaved)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[LocalDedicatedServer] Ready 파일 생성에 실패했습니다: %s"),
            *ReadyFilePath
        );

        return false;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[LocalDedicatedServer] 서버 Ready 완료: %s"),
        *ReadyFilePath
    );

    return true;
}



// ============================================================================
// 매치 서버용 함수들. StartLocalDedicatedServer와 로직은 거의 동일하지만
// Dedicated Server 프로세스(로비 서버) 안에서 호출하는 것을 막지 않고,
// 별도의 핸들(GMatchServerHandle)로 추적합니다.
// Ready 확인/기록은 위 IsLocalDedicatedServerReady / MarkLocalDedicatedServerReady를
// 그대로 재사용합니다 (둘 다 "지금 프로세스가 어떤 Ready 파일을 쓰는지"만 보고,
// 캠페인인지 매치인지는 신경 쓰지 않는 범용 로직이기 때문입니다) — 단, IsMatchServerReady는
// GMatchServerHandle/GMatchServerReadyFilePath 기준으로 별도 구현합니다.
// ============================================================================

bool ULocalDedicatedServerLibrary::StartMatchServer(const FString& MapName, int32 Port)
{
    // 주의: 로비 서버(그 자체가 StartMatchServer 로 스폰됨) -> 매치 서버 스폰이 정상 흐름입니다.
    //
    // "start match" 는 언제나 "새 매치 서버를 원한다" 는 뜻이므로, 이전 매치 서버가 아직
    // 살아 있으면(이전 매치 종료 후 로비 복귀 시 정리 RPC가 누락됐거나 타이밍이 어긋난 경우 등)
    // 재사용하지 않고 확실히 종료한 뒤 새로 띄웁니다. 이렇게 하지 않으면 클라이언트가
    // 이전 매치 상태(누적된 AI 등)가 그대로 남은 낡은 서버로 다시 접속하는 버그가 있었습니다.
    if (IsMatchServerRunning() || GMatchServerHandle.IsValid())
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[MatchServer] 이전 매치 서버가 남아 있어 종료 후 새로 시작합니다.")
        );

        StopMatchServer();
    }

    const FString ReadyDirectory = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(
            FPaths::ProjectSavedDir(),
            TEXT("LocalDedicatedServer")
        )
    );

    if (!IFileManager::Get().MakeDirectory(*ReadyDirectory, true))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MatchServer] Ready 디렉터리를 생성하지 못했습니다: %s"),
            *ReadyDirectory
        );

        return false;
    }

    const FString ReadyFileName = FString::Printf(
        TEXT("MatchReady_%s.flag"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits)
    );

    GMatchServerReadyFilePath = FPaths::Combine(
        ReadyDirectory,
        ReadyFileName
    );

    IFileManager::Get().Delete(
        *GMatchServerReadyFilePath,
        false,
        true
    );

    // StartLocalDedicatedServer와 동일: 에디터는 이 프로세스(UnrealEditor.exe)를 재실행하고,
    // 패키징 빌드는 함께 스테이징된 별도 Server 타겟 실행 파일(<Project>Server.exe)을 찾습니다.
#if WITH_EDITOR
    const FString ServerExePath = FPlatformProcess::ExecutablePath();
#else
    const FString ServerExePath = ResolveLocalDedicatedServerExecutable();
#endif

    if (ServerExePath.IsEmpty() || !FPaths::FileExists(ServerExePath))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MatchServer] 서버 실행 파일을 찾을 수 없습니다: \"%s\" "
                "(패키징 빌드라면 WindowsServer 산출물이 클라이언트와 같은 위치에 "
                "스테이징되어 있는지 확인하세요.)"),
            *ServerExePath
        );

        GMatchServerReadyFilePath.Reset();
        return false;
    }

    // MapName에 "?Game=..." "?AIEasy=1" 같은 URL 옵션이 이미 이어붙어 있어도 그대로 통과시킵니다.
    // (StartLocalDedicatedServer와 달리 원래부터 -game 없이 -server만으로 잘 동작했으므로 그대로 유지합니다.)
#if WITH_EDITOR
    const FString ProjectFilePath =
        FPaths::ConvertRelativePathToFull(
            FPaths::GetProjectFilePath()
        );

    const FString Params = FString::Printf(
        TEXT("\"%s\" %s -server -log -NoLiveCoding -port=%d -LocalServerReadyFile=\"%s\""),
        *ProjectFilePath,
        *MapName,
        Port,
        *GMatchServerReadyFilePath
    );
#else
    const FString Params = FString::Printf(
        TEXT("%s -server -log -port=%d -LocalServerReadyFile=\"%s\""),
        *MapName,
        Port,
        *GMatchServerReadyFilePath
    );
#endif

    uint32 OutProcessID = 0;

    GMatchServerHandle = FPlatformProcess::CreateProc(
        *ServerExePath,
        *Params,
        true,   // bLaunchDetached
        false,  // bLaunchHidden
        false,  // bLaunchReallyHidden
        &OutProcessID,
        0,
        nullptr,
        nullptr
    );

    if (!GMatchServerHandle.IsValid())
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[MatchServer] 서버 프로세스 실행에 실패했습니다: %s %s"),
            *ServerExePath,
            *Params
        );

        IFileManager::Get().Delete(
            *GMatchServerReadyFilePath,
            false,
            true
        );

        GMatchServerReadyFilePath.Reset();
        return false;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[MatchServer] Spawned. CallerPID=%u / SpawnedPID=%u / ReadyFile=%s"),
        FPlatformProcess::GetCurrentProcessId(),
        OutProcessID,
        *GMatchServerReadyFilePath
    );

    return true;
}

void ULocalDedicatedServerLibrary::StopMatchServer()
{
    if (GMatchServerHandle.IsValid())
    {
        FPlatformProcess::TerminateProc(
            GMatchServerHandle,
            true
        );

        FPlatformProcess::CloseProc(
            GMatchServerHandle
        );

        GMatchServerHandle.Reset();
    }

    if (!GMatchServerReadyFilePath.IsEmpty())
    {
        IFileManager::Get().Delete(
            *GMatchServerReadyFilePath,
            false,
            true
        );

        GMatchServerReadyFilePath.Reset();
    }
}

bool ULocalDedicatedServerLibrary::IsMatchServerRunning()
{
    return GMatchServerHandle.IsValid() && FPlatformProcess::IsProcRunning(GMatchServerHandle);
}

bool ULocalDedicatedServerLibrary::IsMatchServerReady()
{
    if (!IsMatchServerRunning())
    {
        return false;
    }

    if (GMatchServerReadyFilePath.IsEmpty())
    {
        return false;
    }

    return FPaths::FileExists(
        GMatchServerReadyFilePath
    );
}

bool ULocalDedicatedServerLibrary::IsLocalIPAddress(const FString& IPAddress)
{
    const FString Trimmed = IPAddress.TrimStartAndEnd();

    if (Trimmed.IsEmpty()
        || Trimmed == TEXT("127.0.0.1")
        || Trimmed.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
    {
        return true;
    }

    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    if (SocketSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[LocalDedicatedServer] IsLocalIPAddress: SocketSubsystem 을 가져오지 못했습니다."));
        return false;
    }

    TArray<TSharedPtr<FInternetAddr>> LocalAddresses;
    SocketSubsystem->GetLocalAdapterAddresses(LocalAddresses);

    for (const TSharedPtr<FInternetAddr>& LocalAddr : LocalAddresses)
    {
        if (LocalAddr.IsValid() && LocalAddr->ToString(false) == Trimmed)
        {
            UE_LOG(LogTemp, Log,
                TEXT("[LocalDedicatedServer] IsLocalIPAddress: '%s' 는 이 PC의 주소입니다 -> 호스트."),
                *Trimmed);
            return true;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("[LocalDedicatedServer] IsLocalIPAddress: '%s' 는 이 PC의 주소가 아닙니다 -> 게스트."),
        *Trimmed);

    return false;
}

bool ULocalDedicatedServerLibrary::IsValidIPAddress(const FString& IPAddress)
{
    const FString Trimmed = IPAddress.TrimStartAndEnd();

    if (Trimmed.Equals(TEXT("localhost"), ESearchCase::IgnoreCase))
    {
        return true;
    }

    TArray<FString> Octets;
    Trimmed.ParseIntoArray(Octets, TEXT("."), false);

    if (Octets.Num() != 4)
    {
        return false;
    }

    for (const FString& Octet : Octets)
    {
        if (Octet.IsEmpty() || Octet.Len() > 3 || !Octet.IsNumeric() || Octet.Contains(TEXT("-")))
        {
            return false;
        }

        if (FCString::Atoi(*Octet) > 255)
        {
            return false;
        }
    }

    return true;
}

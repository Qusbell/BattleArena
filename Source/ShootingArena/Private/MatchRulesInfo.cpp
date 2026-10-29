#include "MatchRulesInfo.h"
#include "MultiRulesLibrary.h"
#include "Loading/LoadingCoordinator.h"
#include "Blueprint/UserWidget.h"
#include "ShootingArenaGameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPtr.h"

namespace
{
	bool SetNumber(UObject* Object, const TCHAR* Name, double Value)
	{
		if (!Object)
		{
			return false;
		}
		FNumericProperty* Property = CastField<FNumericProperty>(Object->GetClass()->FindPropertyByName(Name));
		if (!Property)
		{
			return false;
		}
		void* Ptr = Property->ContainerPtrToValuePtr<void>(Object);
		if (Property->IsFloatingPoint())
		{
			Property->SetFloatingPointPropertyValue(Ptr, Value);
		}
		else
		{
			Property->SetIntPropertyValue(Ptr, static_cast<int64>(Value));
		}
		return true;
	}

	bool ReadBool(const UObject* Object, const TCHAR* Name, bool bDefault)
	{
		if (!Object)
		{
			return bDefault;
		}
		if (const FBoolProperty* Property = CastField<FBoolProperty>(Object->GetClass()->FindPropertyByName(Name)))
		{
			return Property->GetPropertyValue_InContainer(Object);
		}
		return bDefault;
	}

	int32 ReadInt(const UObject* Object, const TCHAR* Name)
	{
		if (!Object)
		{
			return 0;
		}
		if (const FNumericProperty* Property = CastField<FNumericProperty>(Object->GetClass()->FindPropertyByName(Name)))
		{
			return static_cast<int32>(Property->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Object)));
		}
		return 0;
	}

	// DT_MapData(ST_MapData) 의 WinKillCount / GameTimer 컬럼. 유저 정의 구조체라 속성 이름 뒤에 GUID 가 붙어서 접두어로 찾습니다.
	// 싱글플레이는 별도 로컬 서버가 맵을 열기 때문에 서버 쪽 GameInstance 에는 Selected_Map_ID 가 없습니다.
	// 그래서 지금 열린 맵(Map_Asset)이 같은 행을 찾고, 같은 맵을 쓰는 행이 여러 개면 Selected_Map_ID 와 같은 행을 우선합니다.
	void ReadSinglePlayRules(UWorld& World, int32& OutDurationSeconds, int32& OutKills, bool& bOutUnlimitedTime)
	{
		OutDurationSeconds = 0;
		OutKills = 0;
		bOutUnlimitedTime = false;

		const UDataTable* Table = LoadObject<UDataTable>(nullptr, TEXT("/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MapData.DT_MapData"));
		if (!Table)
		{
			return;
		}
		const UStruct* RowStruct = Table->GetRowStruct();

		FName SelectedRow = NAME_None;
		if (UGameInstance* GameInstance = World.GetGameInstance())
		{
			if (const FNameProperty* MapIdProperty = CastField<FNameProperty>(GameInstance->GetClass()->FindPropertyByName(TEXT("Selected_Map_ID"))))
			{
				SelectedRow = MapIdProperty->GetPropertyValue_InContainer(GameInstance);
			}
		}

		const FString CurrentMap = UWorld::RemovePIEPrefix(World.GetOutermost()->GetName());
		const uint8* FoundRow = nullptr;
		for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
		{
			bool bSameMap = false;
			for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
			{
				if (It->GetName().StartsWith(TEXT("Map_Asset")))
				{
					// Map_Asset 은 FSoftWorldReference(구조체) 라서 안쪽의 WorldAsset(소프트 오브젝트)을 꺼냅니다.
					if (const FStructProperty* Wrapper = CastField<FStructProperty>(*It))
					{
						const void* WrapperValue = Wrapper->ContainerPtrToValuePtr<void>(Pair.Value);
						for (TFieldIterator<FSoftObjectProperty> Inner(Wrapper->Struct); Inner; ++Inner)
						{
							const FSoftObjectPtr* Ptr = Inner->ContainerPtrToValuePtr<FSoftObjectPtr>(WrapperValue);
							bSameMap = Ptr && Ptr->ToSoftObjectPath().GetLongPackageName() == CurrentMap;
							break;
						}
					}
					else if (const FSoftObjectProperty* Asset = CastField<FSoftObjectProperty>(*It))
					{
						const FSoftObjectPtr* Ptr = Asset->ContainerPtrToValuePtr<FSoftObjectPtr>(Pair.Value);
						bSameMap = Ptr && Ptr->ToSoftObjectPath().GetLongPackageName() == CurrentMap;
					}
					break;
				}
			}
			if (bSameMap && (!FoundRow || Pair.Key == SelectedRow))
			{
				FoundRow = Pair.Value;
			}
		}
		if (!FoundRow)
		{
			return;
		}

		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			const FString Name = It->GetName();
			if (Name.StartsWith(TEXT("WinKillCount")))
			{
				if (const FNumericProperty* Number = CastField<FNumericProperty>(*It))
				{
					OutKills = FMath::Max(0, static_cast<int32>(Number->GetSignedIntPropertyValue(Number->ContainerPtrToValuePtr<void>(FoundRow))));
				}
			}
			else if (Name.StartsWith(TEXT("GameTimer")))
			{
				const FStructProperty* Struct = CastField<FStructProperty>(*It);
				if (Struct && Struct->Struct == TBaseStructure<FVector2D>::Get())
				{
					const FVector2D Timer = *Struct->ContainerPtrToValuePtr<FVector2D>(FoundRow);
					if (Timer.X >= 0.0)
					{
						OutDurationSeconds = FMath::RoundToInt(Timer.X) * 60 + FMath::Max(0, FMath::RoundToInt(Timer.Y));
					}
					else
					{
						bOutUnlimitedTime = true; // 음수면 시간 제한 없음
					}
				}
			}
		}
	}
}

AMatchRulesInfo::AMatchRulesInfo()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
}

void AMatchRulesInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMatchRulesInfo, MatchDurationSeconds);
	DOREPLIFETIME(AMatchRulesInfo, KillTarget);
	DOREPLIFETIME(AMatchRulesInfo, bUnlimitedTime);
	DOREPLIFETIME(AMatchRulesInfo, MatchStartServerTime);
	DOREPLIFETIME(AMatchRulesInfo, ServerRemainingSeconds);
}

void AMatchRulesInfo::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ApplyTimer, this, &AMatchRulesInfo::TryApplyToGameState, 0.2f, true, 0.0f);
		if (HasAuthority())
		{
			World->GetTimerManager().SetTimer(KillCheckTimer, this, &AMatchRulesInfo::ServerTick, 0.5f, true, 0.5f);
		}
	}
}

void AMatchRulesInfo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ApplyTimer);
		World->GetTimerManager().ClearTimer(KillCheckTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AMatchRulesInfo::TryApplyToGameState()
{
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return; // 클라이언트에서 GameState 가 아직 복제되지 않았으면 다음 틱에 다시 시도합니다.
	}

	if (MatchDurationSeconds <= 0 && KillTarget <= 0)
	{
		// 싱글 등 로비 규칙이 없는 매치: GameConfig 는 그대로 두고 시작 시각 동기화만 합니다.
		bApplied = true;
		GetWorldTimerManager().ClearTimer(ApplyTimer);
		return;
	}

	FObjectPropertyBase* ConfigProperty = CastField<FObjectPropertyBase>(GameState->GetClass()->FindPropertyByName(TEXT("GameConfig")));
	UObject* Config = ConfigProperty ? ConfigProperty->GetObjectPropertyValue_InContainer(GameState) : nullptr;
	if (Config)
	{
		// 공유 데이터 에셋을 바꾸지 않도록 복제본에 값을 씁니다.
		UObject* Copy = DuplicateObject<UObject>(Config, GameState);
		if (MatchDurationSeconds > 0)
		{
			SetNumber(Copy, TEXT("MatchDuration"), MatchDurationSeconds);
		}
		SetNumber(Copy, TEXT("MaxScore"), KillTarget);
		ConfigProperty->SetObjectPropertyValue_InContainer(GameState, Copy);
	}
	SetNumber(GameState, TEXT("EndKillPoint"), KillTarget);

	bApplied = true;
	GetWorldTimerManager().ClearTimer(ApplyTimer);
	UE_LOG(LogTemp, Log, TEXT("[MatchRules] 적용: 제한 시간 %d초, 목표 킬 %d (GameConfig %s)"), MatchDurationSeconds, KillTarget, Config ? TEXT("복제") : TEXT("없음"));
}

void AMatchRulesInfo::ServerTick()
{
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	if (GameState && GameMode && MatchStartServerTime <= 0.0)
	{
		// 전원 접속(LoadingCoordinator: ReadyToPlay)이 끝나는 순간을 매치 시작으로 삼습니다.
		// 로딩 장벽이 없는 월드라면 서버의 bMatchStarted 로 대신합니다.
		bool bStart = false;
		bool bHasCoordinator = false;
		for (TActorIterator<ALoadingCoordinator> It(World); It; ++It)
		{
			bHasCoordinator = true;
			bStart = It->GetPhase() == ELoadingCoordinatorPhase::ReadyToPlay;
			break;
		}
		if (!bHasCoordinator)
		{
			bStart = ReadBool(GameMode, TEXT("bMatchStarted"), false);
		}

		if (bStart)
		{
			// 서버의 종료 판정(ElapsedMatchTime >= MatchDuration)을 이 순간부터 다시 세게 합니다.
			SetNumber(GameState, TEXT("ElapsedMatchTime"), 0.0);
			MatchStartServerTime = FMath::Max(GameState->GetServerWorldTimeSeconds(), 0.001);
			MARK_PROPERTY_DIRTY_FROM_NAME(AMatchRulesInfo, MatchStartServerTime, this);
			UE_LOG(LogTemp, Log, TEXT("[MatchRules] 전원 접속 완료 -> 매치 시작 (서버 시간 %.2f)"), MatchStartServerTime);
			if (GetNetMode() != NM_DedicatedServer)
			{
				// 싱글/리슨 서버는 복제 신호(OnRep)를 받지 않으므로 직접 HUD 타이머를 다시 맞춥니다.
				RestartClientHudTimers();
			}
		}
	}
	if (GameState && MatchStartServerTime > 0.0)
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - LastRemainingSendTime >= 1.0)
		{
			LastRemainingSendTime = Now;
			double Duration = MatchDurationSeconds;
			if (Duration <= 0.0)
			{
				if (const FObjectPropertyBase* ConfigProperty = CastField<FObjectPropertyBase>(GameState->GetClass()->FindPropertyByName(TEXT("GameConfig"))))
				{
					if (UObject* Config = ConfigProperty->GetObjectPropertyValue_InContainer(GameState))
					{
						if (const FNumericProperty* DurationProperty = CastField<FNumericProperty>(Config->GetClass()->FindPropertyByName(TEXT("MatchDuration"))))
						{
							Duration = DurationProperty->GetFloatingPointPropertyValue(DurationProperty->ContainerPtrToValuePtr<void>(Config));
						}
					}
				}
			}
			double Elapsed = 0.0;
			if (const FNumericProperty* ElapsedProperty = CastField<FNumericProperty>(GameState->GetClass()->FindPropertyByName(TEXT("ElapsedMatchTime"))))
			{
				Elapsed = ElapsedProperty->GetFloatingPointPropertyValue(ElapsedProperty->ContainerPtrToValuePtr<void>(GameState));
			}
			ServerRemainingSeconds = static_cast<float>(FMath::Max(Duration - Elapsed, 0.0));
			MARK_PROPERTY_DIRTY_FROM_NAME(AMatchRulesInfo, ServerRemainingSeconds, this);
		}
	}
	if (KillTarget > 0 && MatchStartServerTime > 0.0)
	{
		CheckKillTarget();
	}
}

void AMatchRulesInfo::OnRep_ServerRemainingSeconds()
{
	RemainingAnchorValue = ServerRemainingSeconds;
	RemainingAnchorRealTime = FPlatformTime::Seconds();
	// 첫 값을 받은 순간과 이후 15초마다 HUD 타이머를 서버 기준으로 다시 맞춥니다.
	// HUD 타이머는 월드 시간으로 도는데, 프레임이 끊기면 월드 시간이 실제보다 늦어져서 서로 벌어지기 때문입니다.
	if (ServerRemainingSeconds >= 0.0f && (!bClientHudRestarted || RemainingAnchorRealTime - LastHudRestartRealTime >= 15.0))
	{
		bClientHudRestarted = true;
		LastHudRestartRealTime = RemainingAnchorRealTime;
		RestartClientHudTimers();
	}
}

float AMatchRulesInfo::GetClientRemainingEstimate() const
{
	if (RemainingAnchorValue < 0.0)
	{
		return -1.0f;
	}
	return static_cast<float>(FMath::Max(RemainingAnchorValue - (FPlatformTime::Seconds() - RemainingAnchorRealTime), 0.0));
}

void AMatchRulesInfo::RestartClientHudTimers()
{
	UWorld* World = GetWorld();
	if (!World || MatchStartServerTime <= 0.0)
	{
		return;
	}

	// 1) 이미 돌고 있는 HUD 타이머를 멈춥니다. (다시 시작하면 타이머가 둘이 되어 두 배로 줄어듭니다.)
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (Widget->GetWorld() != World || Widget->IsTemplate() || !Widget->GetClass()->GetName().StartsWith(TEXT("WB_Timer_C")))
		{
			continue;
		}
		if (const FStructProperty* HandleProperty = CastField<FStructProperty>(Widget->GetClass()->FindPropertyByName(TEXT("TimerHandle"))))
		{
			if (HandleProperty->Struct == TBaseStructure<FTimerHandle>::Get())
			{
				FTimerHandle* Handle = HandleProperty->ContainerPtrToValuePtr<FTimerHandle>(Widget);
				World->GetTimerManager().ClearTimer(*Handle);
			}
		}
	}

	// 2) HUD 가 타이머를 새로 만들게 합니다. (남은 시간은 서버 시계 기준으로 계산됩니다.)
	int32 Restarted = 0;
	for (TObjectIterator<UUserWidget> It; It; ++It)
	{
		UUserWidget* Widget = *It;
		if (Widget->GetWorld() != World || Widget->IsTemplate())
		{
			continue;
		}
		if (UFunction* InitFunction = Widget->FindFunction(TEXT("InitCompetitiveMatchTimer")))
		{
			Widget->ProcessEvent(InitFunction, nullptr);
			++Restarted;
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[MatchRules] HUD 타이머 재시작: %d개"), Restarted);
}

void AMatchRulesInfo::CheckKillTarget()
{
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;
	if (!bApplied || !GameState || !GameMode || KillTarget <= 0)
	{
		return;
	}
	if (ReadBool(GameState, TEXT("bIsMatchEnded"), false) || !ReadBool(GameMode, TEXT("bMatchStarted"), true))
	{
		return;
	}

	for (TActorIterator<APlayerState> It(World); It; ++It)
	{
		if (ReadInt(*It, TEXT("KillCount")) >= KillTarget)
		{
			EndMatchOnServer();
			return;
		}
	}
}

void AMatchRulesInfo::EndMatchOnServer()
{
	AGameModeBase* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr;
	UFunction* EndMatchFunction = GameMode ? GameMode->FindFunction(TEXT("EndMatch")) : nullptr;
	if (!EndMatchFunction)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchRules] GameMode 에 EndMatch 함수가 없어 킬 조건으로 매치를 끝내지 못했습니다."));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[MatchRules] 목표 킬 %d 달성 -> 매치 종료"), KillTarget);
	GameMode->ProcessEvent(EndMatchFunction, nullptr);
	if (FBoolProperty* Started = CastField<FBoolProperty>(GameMode->GetClass()->FindPropertyByName(TEXT("bMatchStarted"))))
	{
		Started->SetPropertyValue_InContainer(GameMode, false); // CheckEndCondition 과 같이 종료 후 틱을 멈춥니다.
	}
	GetWorldTimerManager().ClearTimer(KillCheckTimer);
}

bool UMatchRulesLibrary::IsMatchTimeUnlimited(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return false;
	}
	for (TActorIterator<AMatchRulesInfo> It(const_cast<UWorld*>(World)); It; ++It)
	{
		return It->bUnlimitedTime;
	}
	return false;
}

float UMatchRulesLibrary::GetMatchRemainingSeconds(const UObject* WorldContextObject, float DefaultRemaining)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return DefaultRemaining;
	}
	// 서버/싱글은 자기 시계(ElapsedMatchTime)가 곧 기준이라 기본값이 그대로 정확합니다.
	// 클라이언트는 서버가 복제해 준 남은 시간을 받은 순간부터 실제 시간으로 줄여 씁니다.
	for (TActorIterator<AMatchRulesInfo> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const float Estimate = It->GetClientRemainingEstimate();
		if (Estimate >= 0.0f)
		{
			return Estimate;
		}
	}
	return DefaultRemaining;
}

bool UMatchRulesSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld() && Super::ShouldCreateSubsystem(Outer);
}

void UMatchRulesSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}
	const AGameModeBase* GameMode = InWorld.GetAuthGameMode();
	// 매치 시계(ElapsedMatchTime)를 쓰는 게임모드에서만 동작합니다. (로비/메인메뉴 등은 건너뜁니다.)
	if (!GameMode || !GameMode->GameStateClass || !GameMode->GameStateClass->FindPropertyByName(TEXT("ElapsedMatchTime")))
	{
		return;
	}

	int32 DurationSeconds = 0;
	int32 Kills = 0;
	bool bUnlimitedTime = false;
	if (GameMode->GetClass()->GetName().Contains(TEXT("MultiplayerAIGameMode")))
	{
		if (const UShootingArenaGameInstance* GameInstance = Cast<UShootingArenaGameInstance>(InWorld.GetGameInstance()))
		{
			DurationSeconds = UMultiRulesLibrary::GetRuleValue(UMultiRulesLibrary::TimeLimitRowName, GameInstance->PendingTimeIdx) * 60;
			Kills = UMultiRulesLibrary::GetRuleValue(UMultiRulesLibrary::KillTargetRowName, GameInstance->PendingKillIdx);
		}
	}
	else
	{
		// 싱글플레이: 선택한 맵(Selected_Map_ID)의 DT_MapData 행에서 GameTimer(분, 초) / WinKillCount 를 읽습니다. -1 은 "기본값 유지".
		ReadSinglePlayRules(InWorld, DurationSeconds, Kills, bUnlimitedTime);
		if (bUnlimitedTime)
		{
			DurationSeconds = AMatchRulesInfo::UnlimitedDurationSeconds;
		}
	}

	AMatchRulesInfo* Info = InWorld.SpawnActorDeferred<AMatchRulesInfo>(AMatchRulesInfo::StaticClass(), FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Info)
	{
		Info->MatchDurationSeconds = DurationSeconds;
		Info->KillTarget = Kills;
		Info->bUnlimitedTime = bUnlimitedTime;
		Info->FinishSpawning(FTransform::Identity);
		UE_LOG(LogTemp, Log, TEXT("[MatchRules] 매치 규칙 액터 생성: %d초 / %d킬 (0 = 규칙 없음)"), DurationSeconds, Kills);
	}
}

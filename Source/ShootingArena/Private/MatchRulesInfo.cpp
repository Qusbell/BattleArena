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

	int32 Minutes = 0;
	int32 Kills = 0;
	if (GameMode->GetClass()->GetName().Contains(TEXT("MultiplayerAIGameMode")))
	{
		if (const UShootingArenaGameInstance* GameInstance = Cast<UShootingArenaGameInstance>(InWorld.GetGameInstance()))
		{
			Minutes = UMultiRulesLibrary::GetRuleValue(UMultiRulesLibrary::TimeLimitRowName, GameInstance->PendingTimeIdx);
			Kills = UMultiRulesLibrary::GetRuleValue(UMultiRulesLibrary::KillTargetRowName, GameInstance->PendingKillIdx);
		}
	}

	AMatchRulesInfo* Info = InWorld.SpawnActorDeferred<AMatchRulesInfo>(AMatchRulesInfo::StaticClass(), FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Info)
	{
		Info->MatchDurationSeconds = Minutes * 60;
		Info->KillTarget = Kills;
		Info->FinishSpawning(FTransform::Identity);
		UE_LOG(LogTemp, Log, TEXT("[MatchRules] 매치 규칙 액터 생성: %d분 / %d킬 (0 = 규칙 없음)"), Minutes, Kills);
	}
}

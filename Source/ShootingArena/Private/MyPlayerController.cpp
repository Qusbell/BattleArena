#include "MyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Character/CharacterPoolComponent.h"
#include "Animation/WidgetAnimation.h"
#include "Camera/ShooterOutlineComponent.h"
#include "Camera/ShooterNicknameComponent.h"
#include "Camera/ShooterRevengeComponent.h"
#include "HAL/PlatformMisc.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"

AMyPlayerController::AMyPlayerController()
{
	ShooterOutline = CreateDefaultSubobject<UShooterOutlineComponent>(TEXT("ShooterOutline"));
	ShooterNickname = CreateDefaultSubobject<UShooterNicknameComponent>(TEXT("ShooterNickname"));
	ShooterRevenge = CreateDefaultSubobject<UShooterRevengeComponent>(TEXT("ShooterRevenge"));

	// Existing artwork is a replaceable starting point until the five medal designs are assigned.
	static ConstructorHelpers::FClassFinder<UUserWidget> Medal01(TEXT("/Game/CompShooterHUD/UI/Content/Medals/Medals/WB_Medal01"));
	static ConstructorHelpers::FClassFinder<UUserWidget> Medal02(TEXT("/Game/CompShooterHUD/UI/Content/Medals/Medals/WB_Medal02"));
	static ConstructorHelpers::FClassFinder<UUserWidget> Medal03(TEXT("/Game/CompShooterHUD/UI/Content/Medals/Medals/WB_Medal03"));
	static ConstructorHelpers::FClassFinder<UUserWidget> Medal04(TEXT("/Game/CompShooterHUD/UI/Content/Medals/Medals/WB_Medal04"));
	static ConstructorHelpers::FClassFinder<UUserWidget> Medal05(TEXT("/Game/CompShooterHUD/UI/Content/Medals/Medals/WB_Medal05"));
	auto AddStyle = [this](EKillMedalType Type, UClass* Widget, const FText& Label)
	{
		FKillMedalStyle& Style = KillMedalStyles.FindOrAdd(Type);
		Style.MedalWidget = Widget;
		Style.PositionOffset = FVector2D(0.0f, -260.0f);
		Style.Text = Label;
	};
	AddStyle(EKillMedalType::FirstBlood, Medal01.Class, NSLOCTEXT("KillMedal", "FirstBlood", "First Blood"));
	AddStyle(EKillMedalType::DoubleKill, Medal02.Class, NSLOCTEXT("KillMedal", "DoubleKill", "Double Kill"));
	AddStyle(EKillMedalType::TripleKill, Medal03.Class, NSLOCTEXT("KillMedal", "TripleKill", "Triple Kill"));
	AddStyle(EKillMedalType::Rampage, Medal04.Class, NSLOCTEXT("KillMedal", "Rampage", "Rampage"));
	AddStyle(EKillMedalType::RevengeShot, Medal05.Class, NSLOCTEXT("KillMedal", "RevengeShot", "Revenge Shot"));
	KillMedalTierTypes.Add(2, EKillMedalType::DoubleKill);
	KillMedalTierTypes.Add(3, EKillMedalType::TripleKill);
	KillMedalTierTypes.Add(4, EKillMedalType::Rampage);
}

void AMyPlayerController::AwardKillMedal(EKillMedalType MedalType)
{
	if (!HasAuthority() || !GetWorld() || GetWorld()->bIsTearingDown || MedalType == EKillMedalType::None) return;
	if (IsLocalController()) ShowConfiguredKillMedal(MedalType);
	else ClientAwardKillMedal(MedalType);
}

void AMyPlayerController::ClientAwardKillMedal_Implementation(EKillMedalType MedalType)
{
	ShowConfiguredKillMedal(MedalType);
}

void AMyPlayerController::ShowConfiguredKillMedal(EKillMedalType MedalType)
{
	if (const FKillMedalStyle* Style = KillMedalStyles.Find(MedalType))
	{
		QueueKillMedal(*Style, KillMedalDisplayTime);
	}
}

void AMyPlayerController::ClientShowKillMedal_Implementation(TSubclassOf<UUserWidget> MedalWidget,
	const FText& Text, const FText& Value, double DisplayTime, FLinearColor TextColor, FLinearColor HighlightColor)
{
	ShowKillMedalLocally(MedalWidget, Text, Value, DisplayTime, TextColor, HighlightColor);
}

bool AMyPlayerController::ShowKillMedalLocally(TSubclassOf<UUserWidget> MedalWidget,
	const FText& Text, const FText& Value, double DisplayTime, FLinearColor TextColor, FLinearColor HighlightColor)
{
	FKillMedalStyle Style;
	Style.MedalWidget = MedalWidget;
	Style.Text = Text;
	Style.Value = Value;
	Style.TextColor = TextColor;
	Style.HighlightColor = HighlightColor;
	return QueueKillMedal(Style, DisplayTime);
}

bool AMyPlayerController::QueueKillMedal(const FKillMedalStyle& Style, double DisplayTime)
{
	const UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || World->GetNetMode() == NM_DedicatedServer || !IsLocalController()
		|| !Style.MedalWidget || Style.MedalWidget->HasAnyClassFlags(CLASS_Abstract) || !FMath::IsFinite(DisplayTime) || DisplayTime <= 0.0)
	{
		return false;
	}

	AHUD* HUD = GetHUD();
	if (IsValid(HUD) && !HUD->FindFunction(TEXT("AddMedal"))) return false;
	FKillMedalRequest Request;
	Request.Style = Style;
	Request.DisplayTime = DisplayTime;
	PendingKillMedals.Add(MoveTemp(Request));
	if (!bHasActiveKillMedal) UpdateKillMedalQueue();
	return true;
}

UUserWidget* AMyPlayerController::FindKillMedalContainer() const
{
	AHUD* HUD = GetHUD();
	const FObjectPropertyBase* RootProperty = IsValid(HUD)
		? FindFProperty<FObjectPropertyBase>(HUD->GetClass(), TEXT("WB_ShooterHUD")) : nullptr;
	UUserWidget* Root = RootProperty ? Cast<UUserWidget>(RootProperty->GetObjectPropertyValue_InContainer(HUD)) : nullptr;
	return IsValid(Root) ? Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("WB_Medals_Base"))) : nullptr;
}

bool AMyPlayerController::DisplayKillMedalNow(const FKillMedalRequest& Request)
{
	AHUD* HUD = GetHUD();
	UFunction* AddMedal = IsValid(HUD) ? HUD->FindFunction(TEXT("AddMedal")) : nullptr;
	UUserWidget* Medals = FindKillMedalContainer();
	if (!AddMedal || !IsValid(Medals)) return false;
	const FStructProperty* TimerProperty = FindFProperty<FStructProperty>(Medals->GetClass(), TEXT("Timer"));
	if (!TimerProperty || TimerProperty->Struct != FTimerHandle::StaticStruct()) return false;

	// The purchased HUD interface is Blueprint-only; use its named parameters without duplicating its layout.
	FNumericProperty* TimeProperty = FindFProperty<FNumericProperty>(AddMedal, TEXT("DisplayTime"));
	FClassProperty* WidgetProperty = FindFProperty<FClassProperty>(AddMedal, TEXT("MedalWidget"));
	FTextProperty* TextProperty = FindFProperty<FTextProperty>(AddMedal, TEXT("Text"));
	FTextProperty* ValueProperty = FindFProperty<FTextProperty>(AddMedal, TEXT("Value"));
	FStructProperty* TextColorProperty = FindFProperty<FStructProperty>(AddMedal, TEXT("TextColor"));
	FStructProperty* HighlightProperty = FindFProperty<FStructProperty>(AddMedal, TEXT("HighlightColor"));
	if (!TimeProperty || !TimeProperty->IsFloatingPoint() || !WidgetProperty
		|| !Request.Style.MedalWidget->IsChildOf(WidgetProperty->MetaClass) || !TextProperty || !ValueProperty
		|| !TextColorProperty || TextColorProperty->Struct != TBaseStructure<FLinearColor>::Get()
		|| !HighlightProperty || HighlightProperty->Struct != TBaseStructure<FLinearColor>::Get())
	{
		UE_LOG(LogTemp, Warning, TEXT("[KillMedal] HUD AddMedal signature is incompatible: %s"), *GetNameSafe(HUD));
		return false;
	}

	FStructOnScope Parameters(AddMedal);
	void* Data = Parameters.GetStructMemory();
	TimeProperty->SetFloatingPointPropertyValue(TimeProperty->ContainerPtrToValuePtr<void>(Data), Request.DisplayTime);
	WidgetProperty->SetObjectPropertyValue_InContainer(Data, Request.Style.MedalWidget.Get());
	TextProperty->SetPropertyValue_InContainer(Data, Request.Style.Text);
	ValueProperty->SetPropertyValue_InContainer(Data, Request.Style.Value);
	*TextColorProperty->ContainerPtrToValuePtr<FLinearColor>(Data) = Request.Style.TextColor;
	*HighlightProperty->ContainerPtrToValuePtr<FLinearColor>(Data) = Request.Style.HighlightColor;
	HUD->ProcessEvent(AddMedal, Data);
	Medals->SetRenderTranslation(Request.Style.PositionOffset);
	Medals->SetRenderScale(Request.Style.SizeScale);
	ActiveMedalContainer = Medals;
	ActiveMedalHUD = HUD;
	return true;
}

void AMyPlayerController::UpdateKillMedalQueue()
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !IsLocalController()) return;
	if (bHasActiveKillMedal)
	{
		UUserWidget* Medals = ActiveMedalContainer.Get();
		if (!IsValid(Medals) || ActiveMedalHUD.Get() != GetHUD())
		{
			PendingKillMedals.Insert(ActiveKillMedal, 0);
			bHasActiveKillMedal = false;
		}
		else
		{
			const FStructProperty* TimerProperty = FindFProperty<FStructProperty>(Medals->GetClass(), TEXT("Timer"));
			const FObjectPropertyBase* FadeProperty = FindFProperty<FObjectPropertyBase>(Medals->GetClass(), TEXT("Fade"));
			const UWidgetAnimation* Fade = FadeProperty ? Cast<UWidgetAnimation>(FadeProperty->GetObjectPropertyValue_InContainer(Medals)) : nullptr;
			const bool bHolding = TimerProperty && World->GetTimerManager().TimerExists(*TimerProperty->ContainerPtrToValuePtr<FTimerHandle>(Medals));
			const bool bFinished = !bHolding && !Medals->IsAnimationPlaying(Fade);
			// Leave a frame for queued UMG animation work and the HUD's own hide callback.
			if (bFinished && bKillMedalFinishedLastPoll) bHasActiveKillMedal = false;
			bKillMedalFinishedLastPoll = bFinished;
		}
	}
	if (!bHasActiveKillMedal)
	{
		ActiveKillMedal = FKillMedalRequest();
		ActiveMedalContainer.Reset();
		ActiveMedalHUD.Reset();
		bKillMedalFinishedLastPoll = false;
		if (!PendingKillMedals.IsEmpty() && DisplayKillMedalNow(PendingKillMedals[0]))
		{
			ActiveKillMedal = MoveTemp(PendingKillMedals[0]);
			PendingKillMedals.RemoveAt(0);
			bHasActiveKillMedal = true;
		}
	}
	if (bHasActiveKillMedal || !PendingKillMedals.IsEmpty())
	{
		World->GetTimerManager().SetTimer(KillMedalQueueTimer, this, &ThisClass::UpdateKillMedalQueue, 0.05f, false);
	}
	else World->GetTimerManager().ClearTimer(KillMedalQueueTimer);
}

void AMyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(KillMedalQueueTimer);
	PendingKillMedals.Reset();
	ActiveKillMedal = FKillMedalRequest();
	ActiveMedalContainer.Reset();
	ActiveMedalHUD.Reset();
	bHasActiveKillMedal = false;
	Super::EndPlay(EndPlayReason);
}

void AMyPlayerController::OnUnPossess()
{
	if (APawn* PreviousPawn = GetPawn())
		if (const auto* Pool = PreviousPawn->FindComponentByClass<UCharacterPoolComponent>(); Pool && Pool->IsManaged())
			UCharacterPoolComponent::DetachControllerLife(this, PreviousPawn);
	Super::OnUnPossess();
}

void AMyPlayerController::PawnLeavingGame()
{
	// ������ destroy�� ������.
}



// �������� ��¥�� ����Ǵ� �κ�.
// bShouldPause�� true�� ������ ���߰�, false�� �ٽ� Ǯ����.
void AMyPlayerController::ServerSetGamePaused_Implementation(bool bShouldPause)
{

    int32 PlayerCount = 0;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (IsValid(PC))
        {
            ++PlayerCount;
        }
    }

    if (PlayerCount >= 2)
    {
        // 2�� �̻��̸� ���� ��ü pause�� ���� ����
        return;
    }


	SetPause(bShouldPause);
}

// GameMode가 서버 권한으로 직접 호출하는 강제 일시정지입니다. 인원수 제한 없이 항상 적용됩니다.
void AMyPlayerController::SetGamePausedByGameMode(bool bShouldPause)
{
	SetPause(bShouldPause);
}

// 결과창의 "로비로 돌아가기" 버튼에서 호출합니다. 누가 눌러도 서버 전체가 Lobby_Level로 travel합니다.
// bAbsolute=true를 반드시 넘겨야 합니다 — 안 그러면 매치 시작 때 붙였던 "?Game=BP_MultiplayerAIGameMode"
// 같은 이전 레벨의 URL 옵션이 그대로 이어져서, Lobby_Level이 엉뚱한 GameMode(그리고 그에 딸린
// BP_QuakePlayerController)로 열려버립니다 — 그러면 BP_LobbyPlayerController가 안 쓰여서
// WBP_Lobby가 아예 생성되지 않고 화면이 검게 보이는 버그로 이어집니다.
void AMyPlayerController::ServerReturnToLobby_Implementation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 여러 클라이언트가 결과창에서 동시에 "로비로"를 눌러도 ServerTravel 이 한 번만
	// 실행되도록 가드합니다. ServerTravel 이 예약되면 World->NextURL 이 채워집니다.
	if (!World->NextURL.IsEmpty() || World->IsInSeamlessTravel())
	{
		return;
	}

	// bAbsolute=true 필수 — 매치 시작 때 붙였던 "?Game=BP_MultiplayerAIGameMode" 같은
	// 이전 레벨의 URL 옵션이 이어지면 Lobby_Level 이 엉뚱한 GameMode 로 열립니다.
	// (Lobby_Level 의 World Settings > GameMode Override 가 BP_LobbyGameMode 여야 함)
	World->ServerTravel(TEXT("/Game/QuakeLike_1_0/GameFlow/Lobby/Lobby_Level"), true);
}

// GetNetMode()는 블루프린트에 노출되어 있지 않아서 감싸둔 함수입니다.
bool AMyPlayerController::IsStandaloneGame() const
{
	return GetNetMode() == NM_Standalone;
}

// 결과창에서 매치 서버를 떠날 때 호출됩니다. 이 프로세스 자신을 정상 종료시킵니다.
// 단, "일회용으로 스폰된 매치 서버 프로세스" 일 때만 종료합니다.
// 학교 배포처럼 RunServer.bat 으로 상주시키는 통일 서버(-LocalServerReadyFile 인자 없음)나
// 에디터 리슨 서버에서는 절대 종료하지 않습니다 (누가 "메인메뉴로"를 눌러도 서버가 살아있어야 함).
void AMyPlayerController::ServerShutdownMatchServer_Implementation()
{
	if (!IsRunningDedicatedServer())
	{
		return;
	}

	// 스폰된 로컬/매치 서버는 항상 "-LocalServerReadyFile=..." 인자를 갖고 실행됩니다.
	FString UnusedReadyFile;
	const bool bIsSpawnedDisposableServer = FParse::Value(FCommandLine::Get(), TEXT("LocalServerReadyFile="), UnusedReadyFile);
	if (!bIsSpawnedDisposableServer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MatchServer] ServerShutdownMatchServer 무시: 상주(통일) 서버는 종료하지 않습니다."));
		return;
	}

	FPlatformMisc::RequestExit(false);
}

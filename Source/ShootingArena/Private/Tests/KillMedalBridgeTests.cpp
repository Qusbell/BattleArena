#if WITH_DEV_AUTOMATION_TESTS

#include "MyPlayerController.h"
#include "UI/CompetitiveRankingHUD.h"
#include "UI/KillMedalUISettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Audio/KillStreakVoiceComponent.h"
#include "Audio/KillStreakSettingsDataAsset.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include <limits>
#if WITH_EDITOR
#include "Editor.h"
#include "Tests/AutomationEditorCommon.h"
#include "Widgets/SWindow.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillMedalBridgeTest, "ShootingArena.UI.KillMedalBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKillMedalBridgeTest::RunTest(const FString& Parameters)
{
	UFunction* RPC = AMyPlayerController::StaticClass()->FindFunctionByName(TEXT("ClientShowKillMedal"));
	if (!TestNotNull(TEXT("Medal RPC"), RPC)) return false;
	TestTrue(TEXT("Delivery is reliable and owning-client only"),
		RPC->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable) && !RPC->HasAnyFunctionFlags(FUNC_NetMulticast));
	UFunction* AwardRPC = AMyPlayerController::StaticClass()->FindFunctionByName(TEXT("ClientAwardKillMedal"));
	if (!TestNotNull(TEXT("Automatic medal RPC"), AwardRPC)) return false;
	TestTrue(TEXT("Automatic awards also target only the owner"), AwardRPC->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable)
		&& !AwardRPC->HasAnyFunctionFlags(FUNC_NetMulticast));

	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	UGameInstance* GI = World->GetGameInstance();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
	ON_SCOPE_EXIT { GI->RemoveLocalPlayer(Player); };
	AMyPlayerController* PC = World->SpawnActor<AMyPlayerController>();
	if (!TestNotNull(TEXT("Local controller"), PC)) return false;
	PC->SetPlayer(Player);
	World->InitializeActorsForPlay(FURL());
	TestTrue(TEXT("Test uses an owning local controller"), PC->IsLocalController());
	UClass* MedalClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/CompShooterHUD/UI/Content/Medals/Medals/WB_Medal01.WB_Medal01_C"));
	UClass* HUDClass = LoadClass<AHUD>(nullptr,
		TEXT("/Game/CompShooterHUD/Blueprints/HUD_CompShooter.HUD_CompShooter_C"));
	if (!TestNotNull(TEXT("Existing medal widget"), MedalClass) || !TestNotNull(TEXT("Existing HUD"), HUDClass)) return false;
	const FText Label = FText::FromString(TEXT("Medal bridge test"));
	const FText Value = FText::FromString(TEXT("2"));
	const FLinearColor TextColor(0.2f, 0.6f, 0.9f, 1.0f);
	TestTrue(TEXT("An early award waits for HUD initialization"), PC->ShowKillMedalLocally(MedalClass, Label, Value, 0.8, TextColor));
	TestEqual(TEXT("The early award is retained"), PC->PendingKillMedals.Num(), 1);
	PC->ClientSetHUD_Implementation(HUDClass);
	AHUD* HUD = PC->GetHUD();
	if (!TestNotNull(TEXT("HUD instance"), HUD)) return false;
	UFunction* CreateHUD = HUD->FindFunction(TEXT("CreateCSHUD"));
	if (!TestNotNull(TEXT("Existing HUD initialization"), CreateHUD)) return false;
	HUD->ProcessEvent(CreateHUD, nullptr);
	FObjectPropertyBase* RootProperty = FindFProperty<FObjectPropertyBase>(HUDClass, TEXT("WB_ShooterHUD"));
	UUserWidget* Root = RootProperty ? Cast<UUserWidget>(RootProperty->GetObjectPropertyValue_InContainer(HUD)) : nullptr;
	if (!TestNotNull(TEXT("Existing HUD root"), Root)) return false;
	const TSharedRef<SWidget> RootSlate = Root->TakeWidget();
	PC->UpdateKillMedalQueue();
	UUserWidget* Medals = Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("WB_Medals_Base")));
	if (!TestNotNull(TEXT("Existing medal container"), Medals)) return false;
	FNumericProperty* Time = FindFProperty<FNumericProperty>(Medals->GetClass(), TEXT("DisplayTime"));
	UUserWidget* Content = Cast<UUserWidget>(Medals->GetWidgetFromName(TEXT("WB_MedalContent")));
	if (!TestNotNull(TEXT("Existing display duration"), Time) || !TestNotNull(TEXT("Existing medal content"), Content)) return false;
	UTextBlock* LabelWidget = Cast<UTextBlock>(Content->GetWidgetFromName(TEXT("Text_Medal")));
	UTextBlock* ValueWidget = Cast<UTextBlock>(Content->GetWidgetFromName(TEXT("Text_Value")));
	UOverlay* Icons = Cast<UOverlay>(Content->GetWidgetFromName(TEXT("OV_Medal")));
	if (!TestNotNull(TEXT("Medal label"), LabelWidget) || !TestNotNull(TEXT("Medal value"), ValueWidget)
		|| !TestNotNull(TEXT("Medal icon slot"), Icons)) return false;

	TestTrue(TEXT("Native bridge invokes the real Blueprint HUD"),
		PC->bHasActiveKillMedal && PC->PendingKillMedals.IsEmpty());
	TestTrue(TEXT("Duration reaches the medal container"),
		FMath::IsNearlyEqual(Time->GetFloatingPointPropertyValue(Time->ContainerPtrToValuePtr<void>(Medals)), 0.8, 1.e-6));
	TestTrue(TEXT("Label is preserved"), LabelWidget->GetText().EqualTo(Label));
	TestTrue(TEXT("Value is preserved"), ValueWidget->GetText().EqualTo(Value));
	TestTrue(TEXT("Text color is preserved"), LabelWidget->GetColorAndOpacity().GetSpecifiedColor().Equals(TextColor));
	TestTrue(TEXT("Requested widget class is used"), Icons->GetChildrenCount() == 1 && Icons->GetChildAt(0)->IsA(MedalClass));
	const FText Second = FText::FromString(TEXT("Second medal"));
	const FText Third = FText::FromString(TEXT("Third medal"));
	PC->ClientShowKillMedal_Implementation(MedalClass, Second, Value, 1.6, FLinearColor::White, FLinearColor::White);
	PC->ShowKillMedalLocally(MedalClass, Third, Value, 0.6);
	TestTrue(TEXT("Later requests cannot overwrite the current medal"), LabelWidget->GetText().EqualTo(Label));
	TestEqual(TEXT("Both later requests remain in FIFO"), PC->PendingKillMedals.Num(), 2);
	PC->UpdateKillMedalQueue();
	TestTrue(TEXT("The existing hold timer blocks advancement"), LabelWidget->GetText().EqualTo(Label));
	FStructProperty* Timer = FindFProperty<FStructProperty>(Medals->GetClass(), TEXT("Timer"));
	FObjectPropertyBase* FadeProperty = FindFProperty<FObjectPropertyBase>(Medals->GetClass(), TEXT("Fade"));
	UWidgetAnimation* Fade = FadeProperty ? Cast<UWidgetAnimation>(FadeProperty->GetObjectPropertyValue_InContainer(Medals)) : nullptr;
	if (!TestNotNull(TEXT("HUD timer"), Timer) || !TestNotNull(TEXT("Existing fade animation"), Fade)) return false;
	auto CompleteCurrent = [&]()
	{
		World->GetTimerManager().ClearTimer(*Timer->ContainerPtrToValuePtr<FTimerHandle>(Medals));
		Medals->StopAllAnimations();
		PC->UpdateKillMedalQueue();
		PC->UpdateKillMedalQueue();
	};
	CompleteCurrent();
	TestTrue(TEXT("Second medal follows the first"), LabelWidget->GetText().EqualTo(Second));
	TestTrue(TEXT("Each queued request preserves its own duration"),
		FMath::IsNearlyEqual(Time->GetFloatingPointPropertyValue(Time->ContainerPtrToValuePtr<void>(Medals)), 1.6, 1.e-6));
	TestTrue(TEXT("The purchased HUD contains a usable fade"), Fade->GetEndTime() > Fade->GetStartTime());
	CompleteCurrent();
	TestTrue(TEXT("Third medal remains in acquisition order"), LabelWidget->GetText().EqualTo(Third));
	CompleteCurrent();
	TestFalse(TEXT("Queue becomes idle after the final medal"), PC->bHasActiveKillMedal);
	TestFalse(TEXT("Idle queue stops polling"), World->GetTimerManager().TimerExists(PC->KillMedalQueueTimer));
	PC->ShowConfiguredKillMedal(EKillMedalType::DoubleKill, true);
	PC->ShowConfiguredKillMedal(EKillMedalType::TripleKill, true);
	TestTrue(TEXT("A new voice replaces the visible medal without a second UI delay"), LabelWidget->GetText().EqualTo(PC->KillMedalStyles[EKillMedalType::TripleKill].Text));
	TestTrue(TEXT("A superseded medal cannot remain queued"), PC->PendingKillMedals.IsEmpty());
	TestFalse(TEXT("Missing widget is rejected"), PC->ShowKillMedalLocally(nullptr, Label, Value));
	TestFalse(TEXT("Zero duration is rejected"), PC->ShowKillMedalLocally(MedalClass, Label, Value, 0.0));
	TestFalse(TEXT("Nonfinite duration is rejected"), PC->ShowKillMedalLocally(MedalClass, Label, Value, std::numeric_limits<double>::infinity()));
	World->bIsTearingDown = true;
	TestFalse(TEXT("World teardown cannot dispatch UI"), PC->ShowKillMedalLocally(MedalClass, Label, Value));
	World->bIsTearingDown = false;
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillMedalAutomaticAwardsTest, "ShootingArena.UI.KillMedalAutomaticAwards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKillMedalAutomaticAwardsTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	UGameInstance* GI = World->GetGameInstance();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
	ON_SCOPE_EXIT { GI->RemoveLocalPlayer(Player); };
	AMyPlayerController* PC = World->SpawnActor<AMyPlayerController>();
	PC->SetPlayer(Player);
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Spawn;
	Spawn.Owner = PC;
	APlayerState* State = World->SpawnActor<APlayerState>(Spawn);
	UKillStreakVoiceComponent* Voice = NewObject<UKillStreakVoiceComponent>(State);
	State->AddInstanceComponent(Voice);
	Voice->RegisterComponent();
	UKillStreakSettingsDataAsset* Existing = LoadObject<UKillStreakSettingsDataAsset>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Data/Player/DA_KillStreakSettings.DA_KillStreakSettings"));
	if (!TestNotNull(TEXT("Existing kill settings"), Existing)) return false;
	Voice->Settings = DuplicateObject<UKillStreakSettingsDataAsset>(Existing, State);
	Voice->Settings->KillEventBatchDelay = 10.0f;
	Voice->Settings->FirstBloodSounds.Reset();
	Voice->Settings->RevengeSound = nullptr;
	for (auto& Tier : Voice->Settings->KillStreakTiers) Tier.Sounds.Reset();
	PC->KillMedalDisplayTime = 0.8;
	Voice->PlayFirstBloodVoice();
	Voice->QueueKillEvent();
	Voice->QueueKillEvent();
	Voice->QueueKillEvent(true);
	Voice->QueueKillEvent();
	TestEqual(TEXT("Each credited kill immediately updates the streak"), Voice->GetCurrentStreakCount(), 4);
	TestTrue(TEXT("Voice events no longer queue the old tier medals"), PC->PendingKillMedals.IsEmpty());
	Voice->ProcessKillBatch();
	TestEqual(TEXT("Voice batching cannot double count kills"), Voice->GetCurrentStreakCount(), 4);
	Voice->Settings->RevengeSound = Existing->RevengeSound;
	if (!TestNotNull(TEXT("Existing revenge sound is configured"), Existing->RevengeSound.Get())) return false;
	Voice->QueueKillEvent(true);
	Voice->ProcessKillBatch();
	TestEqual(TEXT("Revenge is still one credited kill"), Voice->GetCurrentStreakCount(), 5);
	TestTrue(TEXT("Revenge audio cannot replace the accumulated medals"), PC->PendingKillMedals.IsEmpty());
	UCompetitiveRankingHUD* StreakHUD = CreateWidget<UCompetitiveRankingHUD>(PC);
	if (!TestNotNull(TEXT("Streak HUD"), StreakHUD)) return false;
	StreakHUD->WidgetTree = NewObject<UWidgetTree>(StreakHUD);
	StreakHUD->WidgetTree->RootWidget = StreakHUD->WidgetTree->ConstructWidget<UCanvasPanel>();
	UKillMedalUISettings* MedalSettings = StreakHUD->KillMedalUISettings.LoadSynchronous();
	if (!TestNotNull(TEXT("Designer medal settings asset is connected"), MedalSettings)) return false;
	if (!TestNotNull(TEXT("Existing first-kill artwork"), MedalSettings->MedalTexture.Get())) return false;
	TestEqual(TEXT("Count uses the unified HUD font"), GetPathNameSafe(MedalSettings->CountFont.FontObject),
		FString(TEXT("/Game/QuakeLike_Base/SDM/F_BITCORE1.F_BITCORE1")));
	UKillMedalUISettings* CustomSettings = DuplicateObject<UKillMedalUISettings>(MedalSettings, StreakHUD);
	CustomSettings->FirstMedalSize = FVector2D(90, 70);
	CustomSettings->AdditionalMedalSize = FVector2D(30, 25);
	CustomSettings->LargeMedalSize = FVector2D(110, 85);
	CustomSettings->MedalAnchor = FVector2D(0.4, 0.2);
	CustomSettings->MedalPosition = FVector2D(15, -20);
	CustomSettings->CountPosition = FVector2D(-6, 8);
	CustomSettings->CountFont.Size = 31;
	StreakHUD->ActiveStreakSettings = CustomSettings;
	for (int32 Count : {1, 2, 10, 11, 25})
	{
		StreakHUD->RefreshStreakMedals(Count);
		TestEqual(TEXT("Ten individual medals then one large medal"), StreakHUD->StreakIcons->GetChildrenCount(), Count > 10 ? 1 : Count);
		TestEqual(TEXT("Numeric count is visible only after ten"), StreakHUD->StreakCountLabel->GetVisibility(),
			Count > 10 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (Count > 10) TestTrue(TEXT("Large medal shows the actual total"), StreakHUD->StreakCountLabel->GetText().EqualTo(FText::AsNumber(Count)));
		USizeBox* FirstBox = Cast<USizeBox>(StreakHUD->StreakIcons->GetChildAt(0));
		const FVector2D ExpectedSize = Count > 10 ? CustomSettings->LargeMedalSize : CustomSettings->FirstMedalSize;
		TestTrue(TEXT("Medal size comes from its data asset"), FirstBox &&
			FMath::IsNearlyEqual(FirstBox->GetWidthOverride(), float(ExpectedSize.X)) &&
			FMath::IsNearlyEqual(FirstBox->GetHeightOverride(), float(ExpectedSize.Y)));
		if (Count > 1 && Count <= 10)
		{
			const USizeBox* SmallBox = Cast<USizeBox>(StreakHUD->StreakIcons->GetChildAt(1));
			TestTrue(TEXT("Additional medals use their own configured size"), SmallBox &&
				SmallBox->GetWidthOverride() == 30.0f && SmallBox->GetHeightOverride() == 25.0f);
		}
	}
	const UCanvasPanelSlot* StreakSlot = Cast<UCanvasPanelSlot>(StreakHUD->StreakPanel->Slot);
	TestTrue(TEXT("Medal anchor comes from the data asset"), StreakSlot && StreakSlot->GetAnchors().Minimum.Equals(CustomSettings->MedalAnchor));
	TestTrue(TEXT("Medal position comes from the data asset"), StreakSlot && StreakSlot->GetPosition().Equals(CustomSettings->MedalPosition));
	TestEqual(TEXT("Text size comes from the data asset"), StreakHUD->StreakCountLabel->GetFont().Size, 31.0f);
	TestTrue(TEXT("Text position comes from the data asset"), StreakHUD->StreakCountLabel->GetRenderTransform().Translation.Equals(CustomSettings->CountPosition));
	Voice->NotifyOwnerDeath();
	StreakHUD->RefreshStreakMedals(Voice->GetCurrentStreakCount());
	TestEqual(TEXT("Death resets the streak"), Voice->GetCurrentStreakCount(), 0);
	TestEqual(TEXT("Death hides the accumulated medals"), StreakHUD->StreakPanel->GetVisibility(), ESlateVisibility::Collapsed);
	Voice->QueueKillEvent();
	Voice->ProcessKillBatch();
	TestEqual(TEXT("The next life starts at one"), Voice->GetCurrentStreakCount(), 1);
	return !HasAnyErrors();
}

#if WITH_EDITOR
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FPollKillMedalNetwork, TFunction<bool()>, Poll);
bool FPollKillMedalNetwork::Update() { return Poll(); }

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKillMedalNetworkTest, "ShootingArena.UI.KillMedalNetwork",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FKillMedalNetworkTest::RunTest(const FString& Parameters)
{
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld || EditorWorld->GetOutermost()->IsDirty()) { AddError(TEXT("Current map must be saved before the network test")); return false; }
	const FString OriginalMap = EditorWorld->GetOutermost()->GetName();
	const FString Fixture = TEXT("/Game/QuakeLike_Base/Lvl_QuakeMap_00");
	AddExpectedError(TEXT("저장된 Area Graph가 없습니다"), EAutomationExpectedErrorFlags::Contains, 0);
	// The legacy HUD can report this unrelated initialization error, but need not report it.
	AddExpectedError(TEXT("프로퍼티 WB_Crosshair_Base"), EAutomationExpectedErrorFlags::Contains, -1);
	if (OriginalMap != Fixture) ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	const double ReadyDeadline = FPlatformTime::Seconds() + 30.0;
	ADD_LATENT_AUTOMATION_COMMAND(FPollKillMedalNetwork([this, OriginalMap, Fixture, ReadyDeadline]()
	{
		auto FinishPIE = [OriginalMap, Fixture]()
		{
			ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
			if (OriginalMap != Fixture) ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(OriginalMap));
		};
		AMyPlayerController* Sender = nullptr;
		TArray<AMyPlayerController*> Clients;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || Context.WorldType != EWorldType::PIE) continue;
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				if (AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get()))
				{
					if (World->GetNetMode() == NM_Client && PC->IsLocalController()) Clients.Add(PC);
					else if (World->GetNetMode() != NM_Client) Sender = PC;
				}
		}
		if (!Sender || Clients.Num() != 2)
		{
			if (FPlatformTime::Seconds() < ReadyDeadline) return false;
			AddError(FString::Printf(TEXT("Dedicated server/client controllers did not become ready: server=%d clients=%d"), Sender != nullptr, Clients.Num()));
			FinishPIE();
			return true;
		}
		AMyPlayerController* Owner = nullptr;
		AMyPlayerController* Observer = nullptr;
		for (AMyPlayerController* PC : Clients)
		{
			if (PC->PlayerState && Sender->PlayerState && PC->PlayerState->GetPlayerId() == Sender->PlayerState->GetPlayerId()) Owner = PC;
			else Observer = PC;
		}
		if (!Owner || !Observer)
		{
			if (FPlatformTime::Seconds() < ReadyDeadline) return false;
			AddError(TEXT("Client PlayerStates did not become ready"));
			FinishPIE();
			return true;
		}
		auto MedalUI = [](AMyPlayerController* PC) -> UUserWidget*
		{
			AHUD* HUD = PC ? PC->GetHUD() : nullptr;
			FObjectPropertyBase* RootProperty = HUD ? FindFProperty<FObjectPropertyBase>(HUD->GetClass(), TEXT("WB_ShooterHUD")) : nullptr;
			UUserWidget* Root = RootProperty ? Cast<UUserWidget>(RootProperty->GetObjectPropertyValue_InContainer(HUD)) : nullptr;
			return Root ? Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("WB_Medals_Base"))) : nullptr;
		};
		auto Label = [](UUserWidget* Medals) -> FString
		{
			UUserWidget* Content = Medals ? Cast<UUserWidget>(Medals->GetWidgetFromName(TEXT("WB_MedalContent"))) : nullptr;
			UTextBlock* Text = Content ? Cast<UTextBlock>(Content->GetWidgetFromName(TEXT("Text_Medal"))) : nullptr;
			return Text ? Text->GetText().ToString() : FString();
		};
		if (!MedalUI(Owner) || !MedalUI(Observer))
		{
			if (FPlatformTime::Seconds() < ReadyDeadline) return false;
			AddError(TEXT("Client HUDs did not become ready"));
			FinishPIE();
			return true;
		}
		ON_SCOPE_EXIT { FinishPIE(); };
		// Keep unrelated legacy AI combat from interrupting this disposable network fixture.
		for (APawn* Pawn : TActorRange<APawn>(Sender->GetWorld())) Pawn->SetCanBeDamaged(false);
		// Use the second client's window: editor error tabs can occlude the embedded first viewport.
		if (ULocalPlayer* LocalPlayer = Owner->GetLocalPlayer())
			if (UGameViewportClient* Viewport = LocalPlayer->ViewportClient)
				if (TSharedPtr<SWindow> Window = Viewport->GetWindow()) Window->BringToFront(true);
		struct FObservation
		{
			TArray<FString> Seen;
			TArray<double> Times;
			bool bLastFading = false;
			bool bSawFade = false;
			bool bObserverChanged = false;
			bool bCheckedFIFO = false;
		};
		const auto Observation = MakeShared<FObservation>();
		const FString ObserverLabel = Label(MedalUI(Observer));
		const double Start = FPlatformTime::Seconds();
		const TWeakObjectPtr<AMyPlayerController> OwnerWeak(Owner), ObserverWeak(Observer), SenderWeak(Sender);
		const TArray<FString> Expected{TEXT("First Blood"), TEXT("Double Kill"), TEXT("Triple Kill"), TEXT("Revenge Shot")};
		{
			// Python's editor script guard forces RPCs to local callspace; exercise real network routing here.
			TGuardValue<bool> NormalCallspace(GAllowActorScriptExecutionInEditor, false);
			for (EKillMedalType Type : {EKillMedalType::FirstBlood, EKillMedalType::DoubleKill, EKillMedalType::TripleKill, EKillMedalType::RevengeShot})
				Sender->AwardKillMedal(Type);
		}
		ADD_LATENT_AUTOMATION_COMMAND(FPollKillMedalNetwork([this, Observation, OwnerWeak, ObserverWeak, SenderWeak, ObserverLabel, Start, Expected, MedalUI, Label]()
		{
			if (!OwnerWeak.IsValid() || !ObserverWeak.IsValid()) { AddError(TEXT("PIE clients disappeared during the test")); return true; }
			UUserWidget* Medals = MedalUI(OwnerWeak.Get());
			FObjectPropertyBase* FadeProperty = Medals ? FindFProperty<FObjectPropertyBase>(Medals->GetClass(), TEXT("Fade")) : nullptr;
			UWidgetAnimation* Fade = FadeProperty ? Cast<UWidgetAnimation>(FadeProperty->GetObjectPropertyValue_InContainer(Medals)) : nullptr;
			const bool bFading = Medals && Medals->IsAnimationPlaying(Fade);
			const FString Current = Label(Medals);
			const double Elapsed = FPlatformTime::Seconds() - Start;
			Observation->bSawFade |= bFading;
			Observation->bObserverChanged |= Label(MedalUI(ObserverWeak.Get())) != ObserverLabel;
			if (Expected.Contains(Current) && (Observation->Seen.IsEmpty() || Observation->Seen.Last() != Current))
			{
				if (!Observation->Seen.IsEmpty()) TestFalse(TEXT("Next medal waits until the previous fade ends"), Observation->bLastFading);
				Observation->Seen.Add(Current);
				Observation->Times.Add(Elapsed);
			}
			Observation->bLastFading = bFading;
			if (Elapsed < 8.0) return false;
			if (Observation->bCheckedFIFO)
			{
				if (Elapsed < 9.0) return false;
				const auto* OwnerStreak = OwnerWeak->PlayerState->FindComponentByClass<UKillStreakVoiceComponent>();
				TestTrue(TEXT("Streak count replicates to its owning client"), OwnerStreak && OwnerStreak->GetCurrentStreakCount() == 4);
				APlayerState* RemoteState = OwnerWeak->PlayerState;
				for (APlayerState* State : TActorRange<APlayerState>(ObserverWeak->GetWorld()))
					if (State->GetPlayerId() == RemoteState->GetPlayerId())
					{
						const auto* RemoteStreak = State->FindComponentByClass<UKillStreakVoiceComponent>();
						TestTrue(TEXT("Other clients do not receive the owner's streak count"), RemoteStreak && RemoteStreak->GetCurrentStreakCount() == 0);
					}
				TestFalse(TEXT("The unified voice and medal cue remains owner-only"), Observation->bObserverChanged);
				return true;
			}
			Observation->bCheckedFIFO = true;
			TestEqual(TEXT("All remote awards reached the owner"), Observation->Seen.Num(), Expected.Num());
			for (int32 Index = 0; Index < FMath::Min(Observation->Seen.Num(), Expected.Num()); ++Index)
			{
				TestEqual(TEXT("Real client FIFO order"), Observation->Seen[Index], Expected[Index]);
				if (Index > 0) TestTrue(TEXT("Each medal keeps its full hold time"),
					Observation->Times[Index] - Observation->Times[Index - 1] >= OwnerWeak->KillMedalDisplayTime - 0.05);
			}
			TestTrue(TEXT("Existing fade actually played in PIE"), Observation->bSawFade);
			TestFalse(TEXT("Other client never received these awards"), Observation->bObserverChanged);
			AddInfo(FString::Printf(TEXT("Owner-only FIFO: %s"), *FString::Join(Observation->Seen, TEXT(" -> "))));
			UKillStreakVoiceComponent* Voice = SenderWeak.IsValid() && SenderWeak->PlayerState
				? SenderWeak->PlayerState->FindComponentByClass<UKillStreakVoiceComponent>() : nullptr;
			if (!TestNotNull(TEXT("Existing server voice component"), Voice)) return true;
			Voice->NotifyOwnerDeath();
			for (int32 Index = 0; Index < 4; ++Index) Voice->QueueKillEvent(Index == 3);
			return false;
		}));
		return true;
	}));
	return true;
}
#endif

#endif

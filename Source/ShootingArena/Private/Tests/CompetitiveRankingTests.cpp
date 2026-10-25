#if WITH_DEV_AUTOMATION_TESTS
#include "UI/CompetitiveRankingOrder.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "UI/CompetitiveRankingHUD.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Net/NetPushModelHelpers.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "Widgets/SViewport.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Tests/AutomationEditorCommon.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompetitiveRankingTest, "ShootingArena.UI.CompactRanking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompetitiveRankingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Ranking test world"), World)) return false;
	APlayerState* Leader = World->SpawnActor<APlayerState>();
	APlayerState* Second = World->SpawnActor<APlayerState>();
	APlayerState* Local = World->SpawnActor<APlayerState>();
	TArray<FCompetitiveRankEntry> Players{{Leader, 20}, {Second, 12}, {Local, 5}};
	auto Rows = BuildCompactRanking(Players, Local);
	TestEqual(TEXT("Only leader and local player are shown"), Rows.Num(), 2);
	TestTrue(TEXT("First row is the leader"), Rows[0].Player == Leader && Rows[0].Rank == 1);
	TestTrue(TEXT("Local player retains actual rank outside top two"), Rows[1].Player == Local && Rows[1].Rank == 3);
	Players[2].Kills = 25;
	Rows = BuildCompactRanking(Players, Local);
	TestTrue(TEXT("Overtaking puts local player first"), Rows[0].Player == Local && Rows[0].Rank == 1);
	TestTrue(TEXT("Leader sees runner-up, not a duplicate self row"), Rows[1].Player == Leader && Rows[1].Rank == 2);
	Players[2].Kills = 12;
	Rows = BuildCompactRanking(Players, Local);
	TestEqual(TEXT("Ties preserve Tab scoreboard ordering"), Rows[1].Rank, 3);
	Rows = BuildCompactRanking({{Local, -1}}, Local);
	TestTrue(TEXT("Solo/negative score is supported"), Rows.Num() == 1 && Rows[0].Kills == -1 && Rows[0].Rank == 1);
	TestTrue(TEXT("Missing local replication hides the panel"), BuildCompactRanking(Players, nullptr).IsEmpty());
	TestTrue(TEXT("Departed local player hides the panel"), BuildCompactRanking({{Leader, 1}}, Local).IsEmpty());
	World->DestroyWorld(false);
	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompetitiveRankingPIETest, "ShootingArena.UI.NetworkRanking",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCompetitiveRankingPIETest::RunTest(const FString& Parameters)
{
	// The existing lightweight arena fixture has no saved AI area graph.
	AddExpectedError(TEXT("저장된 Area Graph가 없습니다"), EAutomationExpectedErrorFlags::Contains, 0);
	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld || EditorWorld->GetOutermost()->IsDirty())
	{
		AddError(TEXT("Save the current map before running the interactive HUD test."));
		return false;
	}
	const FString OriginalMap = EditorWorld->GetOutermost()->GetName();
	const FString Fixture = TEXT("/Game/QuakeLike_Base/Lvl_QuakeMap_00");
	if (OriginalMap != Fixture) ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(Fixture));
	const auto Scores = MakeShared<TArray<TPair<TWeakObjectPtr<APlayerState>, int32>>>();
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Scores]()
	{
	auto& OriginalScores = *Scores;
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		UWorld* World = Context.World();
		if (!World || World->WorldType != EWorldType::PIE || World->GetNetMode() == NM_Client) continue;
		if (AGameStateBase* State = World->GetGameState())
			for (APlayerState* Player : State->PlayerArray)
				if (auto* Kills = FindFProperty<FIntProperty>(Player->GetClass(), TEXT("KillCount")))
				{
					OriginalScores.Emplace(Player, Kills->GetPropertyValue_InContainer(Player));
					Kills->SetPropertyValue_InContainer(Player, 20 - OriginalScores.Num() * 3);
					UNetPushModelHelpers::MarkPropertyDirty(Player, TEXT("KillCount"));
					Player->ForceNetUpdate();
				}
	}
	TestFalse(TEXT("Gameplay PIE created score-bearing PlayerStates"), OriginalScores.IsEmpty());
	}, 4.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FDelayedFunctionLatentCommand([this, Scores]()
	{
		const auto& OriginalScores = *Scores;
		int32 ClientCount = 0;
		int32 VisibleClientCount = 0;
		for (TObjectIterator<UCompetitiveRankingHUD> It; It; ++It)
		{
			UCompetitiveRankingHUD* HUD = *It;
			UWorld* World = HUD->GetWorld();
			if (!World || World->WorldType != EWorldType::PIE || !HUD->IsInViewport()) continue;
			APlayerController* PC = HUD->GetOwningPlayer();
			AGameStateBase* State = World->GetGameState();
			if (!PC || !PC->IsLocalController() || !State) continue;
			++ClientCount;
			if (!TestNotNull(TEXT("CompShooter root constructed the ranking panel"), HUD->RankingPanel.Get())) continue;
			TestTrue(TEXT("Ranking listens to the local client's GameState"), HUD->BoundGameState.Get() == State);
			TArray<FCompetitiveRankEntry> Players;
			for (APlayerState* Player : State->PlayerArray)
				if (const auto* Kills = FindFProperty<FIntProperty>(Player->GetClass(), TEXT("KillCount")))
				{
					const int32 Value = Kills->GetPropertyValue_InContainer(Player);
					Players.Add({Player, Value});
					const auto* Original = OriginalScores.FindByPredicate([Player](const auto& Pair)
						{ return Pair.Key.IsValid() && Pair.Key->GetPlayerId() == Player->GetPlayerId(); });
					if (Original)
						TestEqual(TEXT("Seeded server score replicated to the client"), Value,
							20 - int32(Original - OriginalScores.GetData() + 1) * 3);
				}
			const auto Expected = BuildCompactRanking(Players, PC->PlayerState);
			for (int32 Index = 0; Index < Expected.Num(); ++Index)
			{
				TestEqual(TEXT("Displayed actual rank"), HUD->RankLabels[Index]->GetText().ToString(), FString::FromInt(Expected[Index].Rank));
				TestEqual(TEXT("Displayed kill score"), HUD->ScoreLabels[Index]->GetText().ToString(), FString::FromInt(Expected[Index].Kills));
				TestTrue(TEXT("Uses the existing Blueprint name resolver"), HUD->NameLabels[Index]->GetText().EqualTo(UShooterDisplayLibrary::GetPlayerStateDisplayName(Expected[Index].Player)));
			}
			if (HUD->GetCachedGeometry().GetLocalSize().X > 0)
			{
				++VisibleClientCount;
				TestTrue(TEXT("Ranking text is not enlarged by a render transform"),
					HUD->RankingPanel->GetRenderTransform().Scale.Equals(FVector2D(1.0f)));
				TestEqual(TEXT("Player names have no blurring drop shadow"),
					HUD->NameLabels[0]->GetShadowOffset(), FVector2D::ZeroVector);
				TestTrue(TEXT("Panel has a visible position alongside the timer"), HUD->RankingPanel->IsVisible()
					&& HUD->RankingPanel->GetRenderOpacity() > 0 && HUD->RankingSlot->GetPosition().X > 0);
				UUserWidget* Timer = Cast<UUserWidget>(HUD->GetWidgetFromName(TEXT("WB_Timer")));
				if (UWidget* Container = HUD->GetWidgetFromName(TEXT("SB_Timer")))
				{
					HUD->PositionBesideTimer(HUD->GetCachedGeometry());
					const FVector2D Stable = Container->GetRenderTransform().Translation;
					const FVector2D PanelPosition = HUD->RankingSlot->GetPosition();
					Container->SetRenderTranslation(Stable + FVector2D(100, 0));
					HUD->PositionBesideTimer(HUD->GetCachedGeometry());
					TestTrue(TEXT("Centering does not accumulate previous render offsets"), Container->GetRenderTransform().Translation.Equals(Stable));
					TestTrue(TEXT("Ranking position is independent of previous render offsets"), HUD->RankingSlot->GetPosition().Equals(PanelPosition));
				}
				if (UWidget* TimerFrame = Timer ? Timer->GetWidgetFromName(TEXT("b_Timer")) : nullptr)
				{
					for (const TCHAR* Name : {TEXT("Text_Minutes"), TEXT("Text_Seconds")})
						if (UTextBlock* Digits = Cast<UTextBlock>(Timer->GetWidgetFromName(Name)))
						{
							TestTrue(TEXT("Timer digits use uniform character advances"), Digits->GetFont().bForceMonospaced);
							const FText Original = Digits->GetText();
							Digits->SetText(FText::FromString(TEXT("11"))); HUD->ForceLayoutPrepass();
							const FVector2D Narrow = Digits->GetDesiredSize();
							Digits->SetText(FText::FromString(TEXT("88"))); HUD->ForceLayoutPrepass();
							TestTrue(TEXT("Different timer digits keep the same layout width"), FMath::IsNearlyEqual(Narrow.X, Digits->GetDesiredSize().X));
							Digits->SetText(Original);
						}
					const FGeometry& Root = HUD->GetCachedGeometry();
					const FGeometry& Panel = HUD->RankingPanel->GetCachedGeometry();
					const FGeometry& Frame = TimerFrame->GetCachedGeometry();
					const double Left = Root.AbsoluteToLocal(Frame.LocalToAbsolute(FVector2D::ZeroVector)).X;
					const double TimerRight = Root.AbsoluteToLocal(Frame.LocalToAbsolute(FVector2D(Frame.GetLocalSize().X, 0))).X;
					TestTrue(TEXT("Ranking uses the fixed timer frame with a 5-unit gap"),
						FMath::Abs(HUD->RankingSlot->GetPosition().X - TimerRight - 5.0) < 2.0);
					const double Right = Root.AbsoluteToLocal(Panel.LocalToAbsolute(FVector2D(Panel.GetLocalSize().X, 0))).X;
					TestTrue(TEXT("Timer and ranking form one centered group"), FMath::Abs((Left + Right - Root.GetLocalSize().X) * 0.5) < 2.0);
				}
			}
			for (TObjectIterator<UUserWidget> Board; Board; ++Board)
				if (Board->GetWorld() == World && Board->GetClass()->GetName() == TEXT("WBP_ScoreBoard_C"))
					if (UFunction* Update = Board->FindFunction(TEXT("UpdateScoreboard")))
					{
						Board->ProcessEvent(Update, nullptr);
						const FArrayProperty* Sorted = FindFProperty<FArrayProperty>(Board->GetClass(), TEXT("SortedResult"));
						if (!TestNotNull(TEXT("Existing Tab sorted result"), Sorted)) continue;
						FScriptArrayHelper Array(Sorted, Sorted->ContainerPtrToValuePtr<void>(*Board));
						const auto* Inner = CastFieldChecked<FObjectPropertyBase>(Sorted->Inner);
						for (const auto& Entry : Expected)
							TestTrue(TEXT("Compact rank matches Tab scoreboard order"), Array.IsValidIndex(Entry.Rank - 1)
								&& Inner->GetObjectPropertyValue(Array.GetRawPtr(Entry.Rank - 1)) == Entry.Player);
					}
			if (UGameViewportClient* Viewport = World->GetGameViewport())
				if (TSharedPtr<SViewport> Widget = Viewport->GetGameViewportWidget())
				{
					TArray<FColor> Pixels;
					FIntVector Size;
					if (FSlateApplication::Get().TakeScreenshot(Widget.ToSharedRef(), Pixels, Size))
					{
						for (FColor& Pixel : Pixels) Pixel.A = 255;
						TArray64<uint8> PNG;
						FImageUtils::PNGCompressImageArray(Size.X, Size.Y, Pixels, PNG);
						const FString File = FPaths::ProjectSavedDir() / FString::Printf(TEXT("CompetitiveRankingClient%d.png"), ClientCount);
						TestTrue(TEXT("Saved client HUD screenshot"), FFileHelper::SaveArrayToFile(PNG, *File));
					}
				}
		}
		TestTrue(TEXT("At least one actual client HUD was checked"), ClientCount > 0);
		TestTrue(TEXT("At least one visible client layout was checked"), VisibleClientCount > 0);
		for (const auto& Original : OriginalScores)
			if (APlayerState* Player = Original.Key.Get())
				if (auto* Kills = FindFProperty<FIntProperty>(Player->GetClass(), TEXT("KillCount")))
				{
					Kills->SetPropertyValue_InContainer(Player, Original.Value);
					UNetPushModelHelpers::MarkPropertyDirty(Player, TEXT("KillCount"));
					Player->ForceNetUpdate();
				}
	}, 1.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	if (OriginalMap != Fixture) ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(OriginalMap));
	return true;
}
#endif
#endif

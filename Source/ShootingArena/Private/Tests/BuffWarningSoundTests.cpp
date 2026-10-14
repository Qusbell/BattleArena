#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Buff/BuffPresentationComponent.h"
#include "Components/AudioComponent.h"
#include "DataTableUtils.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundWave.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffWarningSoundTest, "ShootingArena.Buff.WarningSound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FBuffWarningSoundTest::RunTest(const FString& Parameters)
{
	UBuffPresentationComponent* Clock = NewObject<UBuffPresentationComponent>();
	TestEqual(TEXT("Outside warning window"), Clock->ConsumeWarningSecond(30.0), 0);
	TestEqual(TEXT("Before five seconds"), Clock->ConsumeWarningSecond(5.001), 0);
	for (int32 Second = 5; Second >= 1; --Second)
	{
		TestEqual(TEXT("Each threshold emits once"), Clock->ConsumeWarningSecond(static_cast<double>(Second)), Second);
		TestEqual(TEXT("Repeated calls in an interval are silent"), Clock->ConsumeWarningSecond(Second - 0.2), 0);
	}
	TestEqual(TEXT("Zero never emits"), Clock->ConsumeWarningSecond(0.0), 0);
	TestEqual(TEXT("Expired clocks cannot be revived by a correction"), Clock->ConsumeWarningSecond(2.0), 0);
	Clock->NextWarningSecond = 6;
	TestEqual(TEXT("Late replication starts with current interval only"), Clock->ConsumeWarningSecond(2.8), 3);
	TestEqual(TEXT("A backward clock correction does not replay skipped intervals"), Clock->ConsumeWarningSecond(4.8), 0);
	TestEqual(TEXT("A hitch skips to the current interval"), Clock->ConsumeWarningSecond(0.8), 1);
	TestEqual(TEXT("A repeated final interval is silent"), Clock->ConsumeWarningSecond(0.5), 0);
	Clock->NextWarningSecond = 6;
	TestEqual(TEXT("Nonfinite times are silent"), Clock->ConsumeWarningSecond(std::numeric_limits<double>::quiet_NaN()), 0);
	Clock->NextWarningSecond = 6;
	TestEqual(TEXT("A hitch past expiry cannot burst old warnings"), Clock->ConsumeWarningSecond(-2.0), 0);

	IConsoleVariable* Mode = IConsoleManager::Get().FindConsoleVariable(TEXT("Buff.WarningSound.Mode"));
	if (!TestNotNull(TEXT("Runtime warning mode is available"), Mode)) return false;
	const int32 OriginalMode = Mode->GetInt();
	ON_SCOPE_EXIT { Mode->Set(OriginalMode, ECVF_SetByConsole); };
	Mode->Set(-1, ECVF_SetByConsole);
	for (int32 Mask = 0; Mask < 4; ++Mask)
	{
		Clock->Presentation.bWarningLocalEnabled = (Mask & 1) != 0;
		Clock->Presentation.bWarningWorldEnabled = (Mask & 2) != 0;
		TestEqual(TEXT("DT allows None, Local, World and Both"), Clock->GetWarningOutputMask(), Mask);
	}
	for (int32 Mask = 0; Mask < 4; ++Mask)
	{
		Mode->Set(Mask, ECVF_SetByConsole);
		TestEqual(TEXT("Runtime mode overrides the DT checkboxes"), Clock->GetWarningOutputMask(), Mask);
	}
	Mode->Set(0, ECVF_SetByConsole);

	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	AddExpectedError(TEXT("저장된 Area Graph가 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);
	if (!Context.BeginPlayInTestWorld()) return false;
	ACharacter* Owner = World->SpawnActor<ACharacter>();
	UClass* BuffClass = LoadClass<UBuffPresentationComponent>(nullptr, TEXT("/Game/QuakeLike_1_0/Buff/BPC_Buff.BPC_Buff_C"));
	UDataTable* Source = LoadObject<UDataTable>(nullptr, TEXT("/Game/QuakeLike_1_0/Data/Item/DropTable/DT_BuffTable.DT_BuffTable"));
	if (!TestNotNull(TEXT("Buff class"), BuffClass) || !TestNotNull(TEXT("Buff DT"), Source)) return false;
	UDataTable* Table = DuplicateObject<UDataTable>(Source, GetTransientPackage());
	UBuffPresentationComponent* Buff = NewObject<UBuffPresentationComponent>(Owner, BuffClass);
	Owner->AddInstanceComponent(Buff);
	Buff->BuffDataTable = Table;
	Buff->RegisterComponent();
	const FName RowName(TEXT("Buff.Multi.Atk"));
	uint8* Row = Table->FindRowUnchecked(RowName);
	FNameProperty* CurrentRow = FindFProperty<FNameProperty>(BuffClass, TEXT("currentBuffRowName"));
	if (!TestNotNull(TEXT("Active DT row"), Row) || !TestNotNull(TEXT("Existing row variable"), CurrentRow)) return false;
	CurrentRow->SetPropertyValue_InContainer(Buff, RowName);
	USoundWave* Sound = NewObject<USoundWave>(Table);
	for (TFieldIterator<FProperty> It(Table->GetRowStruct()); It; ++It)
	{
		const FString Name = DataTableUtils::GetPropertyExportName(*It);
		if (FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(*It))
		{
			if (Name == TEXT("WarningSound")) Property->SetObjectPropertyValue_InContainer(Row, Sound);
			if (Name == TEXT("NiagaraVFX") || Name == TEXT("ParticleVFX")) Property->SetObjectPropertyValue_InContainer(Row, nullptr);
		}
		if (FBoolProperty* Property = CastField<FBoolProperty>(*It))
		{
			if (Name == TEXT("WarningLocalEnabled") || Name == TEXT("WarningWorldEnabled")) Property->SetPropertyValue_InContainer(Row, true);
		}
		if (FNumericProperty* Property = CastField<FNumericProperty>(*It))
		{
			if (Name == TEXT("WarningLocalVolume")) Property->SetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Row), 0.5);
			if (Name == TEXT("WarningWorldVolume")) Property->SetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Row), 0.75);
		}
	}
	const FGameplayTag Quad = FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"));
	const double Now = World->GetGameState()->GetServerWorldTimeSeconds();
	Buff->SetBuffPresentation(Quad, Now + 8.0);
	TestTrue(TEXT("Sound comes from the active row"), Buff->Presentation.WarningSound == Sound);
	TestTrue(TEXT("Independent enable flags come from the row"), Buff->Presentation.bWarningLocalEnabled && Buff->Presentation.bWarningWorldEnabled);
	TestEqual(TEXT("Local volume comes from the row"), Buff->Presentation.WarningLocalVolume, 0.5f);
	TestEqual(TEXT("World volume comes from the row"), Buff->Presentation.WarningWorldVolume, 0.75f);
	const uint32 Revision = Buff->Presentation.Revision;
	Buff->ConsumeWarningSecond(4.8);
	Buff->SetBuffPresentation(Quad, Now + 8.0);
	TestEqual(TEXT("Duplicate grant does not create a new generation"), Buff->Presentation.Revision, Revision);
	TestEqual(TEXT("Duplicate grant keeps consumed intervals"), Buff->NextWarningSecond, 5);
	Buff->SetBuffPresentation(Quad, Now + 12.0);
	TestTrue(TEXT("A changed deadline starts a new generation"), Buff->Presentation.Revision != Revision);
	TestEqual(TEXT("Refresh resets the interval cursor"), Buff->NextWarningSecond, 6);
	Buff->SetBuffPresentation(Quad, Now + 3.0);
	TestEqual(TEXT("Shortened duration consumes only the current interval"), Buff->NextWarningSecond, 3);
	Buff->PlayBuffWarningSoundFromDataTable();
	TestEqual(TEXT("Manual calls cannot duplicate automatic playback"), Buff->NextWarningSecond, 3);
	Buff->SetBuffPresentation(Quad, Now + 4.5);
	TestEqual(TEXT("A transferred deadline skips the already passed five-second threshold"), Buff->Presentation.WarningStartSecond, 5);
	TestEqual(TEXT("Transfer cannot repeat the current partial interval"), Buff->ConsumeWarningSecond(4.49), 0);
	TestEqual(TEXT("Transfer can emit the next future threshold"), Buff->ConsumeWarningSecond(4.0), 4);
	Buff->WarningLocalAudio = NewObject<UAudioComponent>(Owner);
	Buff->WarningWorldAudio = NewObject<UAudioComponent>(Owner);
	Buff->OnOwnerDeath(nullptr, nullptr);
	TestTrue(TEXT("Death clears both audio handles"), !Buff->WarningLocalAudio && !Buff->WarningWorldAudio);
	TestEqual(TEXT("Death disables future warning intervals"), Buff->NextWarningSecond, 0);
	TestEqual(TEXT("Death clears the deadline"), Buff->GetPresentationRemainingTime(), 0.0);
	Buff->SetBuffPresentation(Quad, Now + 7.0);
	TestEqual(TEXT("Reacquisition starts fresh"), Buff->NextWarningSecond, 6);
	Buff->WarningWorldAudio = NewObject<UAudioComponent>(Owner);
	Buff->ClearBuffPresentation();
	TestTrue(TEXT("Early removal clears audio"), !Buff->WarningWorldAudio);
	TestEqual(TEXT("Early removal disables warnings"), Buff->NextWarningSecond, 0);
	Context.DestroyTestWorld(false);
	return !HasAnyErrors();
}

#endif

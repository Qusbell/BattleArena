#if WITH_DEV_AUTOMATION_TESTS

#include "DataTableUtils.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "K2Node_CustomEvent.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemSoundTest, "ShootingArena.Items.SpawnAndPickupSound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemSoundTest::RunTest(const FString& Parameters)
{
	USoundBase* SpawnCue = LoadObject<USoundBase>(nullptr, TEXT("/Game/QuakeLike_Base/SDM/SoundPack_SDM/Cue/QuadeDamage.QuadeDamage"));
	USoundBase* PickupCue = LoadObject<USoundBase>(nullptr, TEXT("/Game/QuakeLike_Base/SDM/SoundPack_SDM/Cue/HealthBoost.HealthBoost"));
	if (!TestNotNull(TEXT("Distinct spawn test sound"), SpawnCue) || !TestNotNull(TEXT("Distinct pickup test sound"), PickupCue)) return false;
	auto Field = [](const UStruct* S, const TCHAR* Name) -> FProperty*
	{
		for (TFieldIterator<FProperty> It(S); It; ++It)
			if (DataTableUtils::GetPropertyExportName(*It).Equals(Name, ESearchCase::IgnoreCase)) return *It;
		return nullptr;
	};
	auto SetNumber = [](FProperty* P, void* Data, double Value)
	{
		auto* Number = CastFieldChecked<FNumericProperty>(P);
		Number->SetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(Data), Value);
	};
	auto Number = [](FProperty* P, const void* Data)
	{
		return CastFieldChecked<FNumericProperty>(P)->GetFloatingPointPropertyValue(P->ContainerPtrToValuePtr<void>(Data));
	};
	const TCHAR* Kinds[] = {TEXT("HeldItem"), TEXT("Buff"), TEXT("LifeItem"), TEXT("AmmoItem")};
	const TCHAR* Drops[] = {TEXT("HoldableItem"), TEXT("BuffItem"), TEXT("LifeItem"), TEXT("AmmoItem")};
	for (int32 I = 0; I < 4; ++I)
	{
		const FString Kind(Kinds[I]);
		UDataTable* Table = LoadObject<UDataTable>(nullptr, *(TEXT("/Game/QuakeLike_1_0/Data/Item/DropTable/DT_") + Kind + TEXT("Table")));
		UClass* Spawner = LoadClass<UObject>(nullptr, *(TEXT("/Game/QuakeLike_1_0/Spawner/Component/BPC_") + Kind + TEXT("Spawner.BPC_") + Kind + TEXT("Spawner_C")));
		const FString DropName(Drops[I]);
		UClass* Drop = LoadClass<UObject>(nullptr, *(TEXT("/Game/QuakeLike_1_0/DropItem/BP_") + DropName + TEXT(".BP_") + DropName + TEXT("_C")));
		if (!TestNotNull(Kind + TEXT(" table"), Table) || !TestNotNull(Kind + TEXT(" spawner"), Spawner) || !TestNotNull(Kind + TEXT(" pickup"), Drop)) return false;
		auto* SpawnSound = CastField<FObjectPropertyBase>(Field(Table->GetRowStruct(), TEXT("SpawnSound")));
		auto* PickupSound = CastField<FObjectPropertyBase>(Field(Table->GetRowStruct(), TEXT("PickupSound")));
		FProperty* SpawnVolume = Field(Table->GetRowStruct(), TEXT("SpawnVolume"));
		FProperty* PickupVolume = Field(Table->GetRowStruct(), TEXT("PickupVolume"));
		if (!TestNotNull(Kind + TEXT(" SpawnSound field"), SpawnSound) || !TestNotNull(Kind + TEXT(" PickupSound field"), PickupSound)
			|| !TestNotNull(Kind + TEXT(" SpawnVolume field"), SpawnVolume) || !TestNotNull(Kind + TEXT(" PickupVolume field"), PickupVolume)) return false;
		UObject* SpawnerCDO = Spawner->GetDefaultObject();
		UObject* DropCDO = Drop->GetDefaultObject();
		UFunction* Feedback = SpawnerCDO->FindFunction(TEXT("GetSpawnFeedback"));
		UFunction* Pickup = DropCDO->FindFunction(TEXT("GetPickSound"));
		auto* Handle = FindFProperty<FStructProperty>(Drop, TEXT("DataTable Handle"));
		if (!TestNotNull(TEXT("BPI feedback implementation"), Feedback) || !TestNotNull(TEXT("Pickup override"), Pickup)
			|| !TestNotNull(TEXT("Pickup row handle"), Handle)) return false;
		const FDataTableRowHandle OriginalHandle = *Handle->ContainerPtrToValuePtr<FDataTableRowHandle>(DropCDO);
		ON_SCOPE_EXIT { *Handle->ContainerPtrToValuePtr<FDataTableRowHandle>(DropCDO) = OriginalHandle; };
		for (const FName RowName : Table->GetRowNames())
		{
			uint8* Row = Table->FindRowUnchecked(RowName);
			const auto OriginalSpawn = SpawnSound->GetObjectPropertyValue_InContainer(Row);
			const auto OriginalPickup = PickupSound->GetObjectPropertyValue_InContainer(Row);
			const double OriginalSpawnVolume = Number(SpawnVolume, Row), OriginalPickupVolume = Number(PickupVolume, Row);
			ON_SCOPE_EXIT
			{
				SpawnSound->SetObjectPropertyValue_InContainer(Row, OriginalSpawn);
				PickupSound->SetObjectPropertyValue_InContainer(Row, OriginalPickup);
				SetNumber(SpawnVolume, Row, OriginalSpawnVolume); SetNumber(PickupVolume, Row, OriginalPickupVolume);
			}; // Diagnostic changes are memory-only: never save art/data/map assets.
			FDataTableRowHandle Selected; Selected.DataTable = Table; Selected.RowName = RowName;
			*Handle->ContainerPtrToValuePtr<FDataTableRowHandle>(DropCDO) = Selected;
			for (int32 Mask = 0; Mask < 4; ++Mask)
			{
				SpawnSound->SetObjectPropertyValue_InContainer(Row, (Mask & 1) ? SpawnCue : nullptr);
				PickupSound->SetObjectPropertyValue_InContainer(Row, (Mask & 2) ? PickupCue : nullptr);
				SetNumber(SpawnVolume, Row, 1.25); SetNumber(PickupVolume, Row, 0.35);
				FStructOnScope FeedbackArgs(Feedback), PickupArgs(Pickup);
				FindFProperty<FNameProperty>(Feedback, TEXT("Row Name"))->SetPropertyValue_InContainer(FeedbackArgs.GetStructMemory(), RowName);
				SpawnerCDO->ProcessEvent(Feedback, FeedbackArgs.GetStructMemory());
				DropCDO->ProcessEvent(Pickup, PickupArgs.GetStructMemory());
				TestTrue(Kind + TEXT(" selects only SpawnSound"), FindFProperty<FObjectPropertyBase>(Feedback, TEXT("Spawn Sound"))->GetObjectPropertyValue_InContainer(FeedbackArgs.GetStructMemory()) == ((Mask & 1) ? SpawnCue : nullptr));
				TestTrue(Kind + TEXT(" selects only PickupSound"), FindFProperty<FObjectPropertyBase>(Pickup, TEXT("sound"))->GetObjectPropertyValue_InContainer(PickupArgs.GetStructMemory()) == ((Mask & 2) ? PickupCue : nullptr));
				FProperty* FeedbackVolume = FindFProperty<FProperty>(Feedback, TEXT("Spawn Volume"));
				if (!TestNotNull(TEXT("BPI returns spawn volume"), FeedbackVolume)) return false;
				TestTrue(TEXT("Independent spawn volume"), FMath::IsNearlyEqual(Number(FeedbackVolume, FeedbackArgs.GetStructMemory()), 1.25));
				TestTrue(TEXT("Independent pickup volume"), FMath::IsNearlyEqual(Number(FindFProperty<FProperty>(Pickup, TEXT("pickupVolume")), PickupArgs.GetStructMemory()), 0.35, 1.e-6));
			}
			SetNumber(SpawnVolume, Row, 0); SetNumber(PickupVolume, Row, 0);
			FStructOnScope Silent(Pickup);
			DropCDO->ProcessEvent(Pickup, Silent.GetStructMemory());
			TestEqual(TEXT("Zero pickup volume remains an explicit mute"), Number(FindFProperty<FProperty>(Pickup, TEXT("pickupVolume")), Silent.GetStructMemory()), 0.0);
		}
		FDataTableRowHandle Missing; Missing.DataTable = Table; Missing.RowName = TEXT("Missing_Sound_Test_Row");
		*Handle->ContainerPtrToValuePtr<FDataTableRowHandle>(DropCDO) = Missing;
		FStructOnScope MissingArgs(Pickup);
		DropCDO->ProcessEvent(Pickup, MissingArgs.GetStructMemory());
		TestNull(TEXT("Missing row does not produce a pickup sound"), FindFProperty<FObjectPropertyBase>(Pickup, TEXT("sound"))->GetObjectPropertyValue_InContainer(MissingArgs.GetStructMemory()));
	}
	UClass* Controller = LoadClass<UObject>(nullptr, TEXT("/Game/QuakeLike_1_0/Controller/BP_QuakePlayerController.BP_QuakePlayerController_C"));
	UFunction* RPC = Controller ? Controller->FindFunctionByName(TEXT("Client_PlayPickupSound")) : nullptr;
	if (!TestNotNull(TEXT("Existing pickup RPC"), RPC)) return false;
	TestTrue(TEXT("Pickup remains reliable owning-client only"), RPC->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient | FUNC_NetReliable) && !RPC->HasAnyFunctionFlags(FUNC_NetMulticast));
	UClass* SpawnPoint = LoadClass<UObject>(nullptr, TEXT("/Game/QuakeLike_1_0/Spawner/SpawnPoint/BP_ItemSpawnPoint.BP_ItemSpawnPoint_C"));
	UFunction* SpawnRPC = SpawnPoint ? SpawnPoint->FindFunctionByName(TEXT("Multicast_PlaySpawnFeedback")) : nullptr;
	if (!TestNotNull(TEXT("Existing location feedback RPC"), SpawnRPC)) return false;
	TestTrue(TEXT("Spawn feedback remains multicast"), SpawnRPC->HasAllFunctionFlags(FUNC_Net | FUNC_NetMulticast));
	TestNotNull(TEXT("Spawn RPC carries selected row volume"), FindFProperty<FProperty>(SpawnRPC, TEXT("SpawnVolume")));
	return !HasAnyErrors();
}

#endif

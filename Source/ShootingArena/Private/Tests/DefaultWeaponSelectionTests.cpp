#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/InitialWeaponLibrary.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "Weapon/InitialWeaponLoadoutDataAsset.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDefaultWeaponSelectionTest, "ShootingArena.Weapon.DefaultWeaponSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDefaultWeaponSelectionTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	AActor* Owner = World->SpawnActor<AActor>();
	auto* Loadout = NewObject<UInitialWeaponLoadoutComponent>(Owner);
	Owner->AddInstanceComponent(Loadout);
	Loadout->LoadoutData = DuplicateObject(LoadObject<UInitialWeaponLoadoutDataAsset>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Data/Item/DA_InitialWeaponLoadout.DA_InitialWeaponLoadout")), Owner);
	if (!TestNotNull(TEXT("Existing loadout asset"), Loadout->LoadoutData.Get())) return false;
	Loadout->LoadoutData->bRestrictDefaultWeaponSelection = true;
	UClass* InventoryClass = LoadClass<UActorComponent>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Inventory/BPC_Inventory.BPC_Inventory_C"));
	if (!TestNotNull(TEXT("Existing inventory"), InventoryClass)) return false;
	auto* Inventory = NewObject<UActorComponent>(Owner, InventoryClass);
	Owner->AddInstanceComponent(Inventory);
	FInitialWeaponEntry Default, Regular;
	FInitialWeaponEntry Pistol, MachineGun;
	Pistol.WeaponName = TEXT("ID.Pistol");
	MachineGun.WeaponName = TEXT("ID.MachineGun");
	if (!TestTrue(TEXT("Pistol catalog entry"), Loadout->LoadoutData->ResolveWeaponEntry(Pistol, Default))
		|| !TestTrue(TEXT("Machinegun catalog entry"), Loadout->LoadoutData->ResolveWeaponEntry(MachineGun, Regular))) return false;
	Loadout->LoadoutData->DefaultWeapons = { Pistol };
	FActorSpawnParameters Spawn;
	Spawn.Owner = Owner;
	AActor* Base = World->SpawnActor<AActor>(Default.WeaponClass.LoadSynchronous(), Spawn);
	AActor* Gun = World->SpawnActor<AActor>(Regular.WeaponClass.LoadSynchronous(), Spawn);
	if (!TestNotNull(TEXT("Default gun"), Base) || !TestNotNull(TEXT("Regular gun"), Gun)) return false;
	for (auto Pair : { TPair<AActor*, FInitialWeaponEntry>(Base, Default), TPair<AActor*, FInitialWeaponEntry>(Gun, Regular) })
	{
		auto* Data = FindFProperty<FObjectPropertyBase>(Pair.Key->GetClass(), TEXT("weaponData"));
		if (!TestNotNull(TEXT("Weapon data contract"), Data)) return false;
		Data->SetObjectPropertyValue_InContainer(Pair.Key, Pair.Value.WeaponData.LoadSynchronous());
		auto* ItemInfo = FindFProperty<FStructProperty>(Pair.Key->GetClass(), TEXT("itemInfo"));
		if (!TestNotNull(TEXT("Held item initialization contract"), ItemInfo)) return false;
		for (TFieldIterator<FObjectPropertyBase> It(ItemInfo->Struct); It; ++It)
		{
			void* InfoData = ItemInfo->ContainerPtrToValuePtr<void>(Pair.Key);
			if (It->GetName().StartsWith(TEXT("ItemData"))) It->SetObjectPropertyValue_InContainer(InfoData, Pair.Value.WeaponData.Get());
			if (It->GetName().StartsWith(TEXT("ItemOwner"))) It->SetObjectPropertyValue_InContainer(InfoData, Owner);
		}
	}
	auto* Items = FindFProperty<FArrayProperty>(InventoryClass, TEXT("All_Items"));
	if (!TestNotNull(TEXT("Inventory items contract"), Items)) return false;
	auto* Item = CastField<FObjectPropertyBase>(Items->Inner);
	if (!TestNotNull(TEXT("Inventory item type"), Item)) return false;
	FScriptArrayHelper Array(Items, Items->ContainerPtrToValuePtr<void>(Inventory));
	Array.Resize(4);
	Item->SetObjectPropertyValue(Array.GetRawPtr(0), nullptr);
	Item->SetObjectPropertyValue(Array.GetRawPtr(1), Base);
	Item->SetObjectPropertyValue(Array.GetRawPtr(2), Gun);
	Item->SetObjectPropertyValue(Array.GetRawPtr(3), nullptr);
	auto SetAmmo = [](AActor* Weapon, int32 Count)
	{
		auto* Usage = FindFProperty<FStructProperty>(Weapon->GetClass(), TEXT("currentUsageCount"));
		if (!Usage) return false;
		for (TFieldIterator<FIntProperty> It(Usage->Struct); It; ++It)
			if (It->GetName().StartsWith(TEXT("currentUsageCount")))
			{
				It->SetPropertyValue_InContainer(Usage->ContainerPtrToValuePtr<void>(Weapon), Count);
				return true;
			}
		return false;
	};
	if (!TestTrue(TEXT("Default ammo contract"), SetAmmo(Base, 100))
		|| !TestTrue(TEXT("Regular ammo contract"), SetAmmo(Gun, 2))) return false;
	UClass* HUDClass = LoadClass<UObject>(nullptr,
		TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_PlayerHUD_sdm.WBP_PlayerHUD_sdm_C"));
	if (!TestNotNull(TEXT("Combat HUD handler contract"), HUDClass)) return false;
	UObject* HUD = NewObject<UObject>(GetTransientPackage(), HUDClass);
	const auto* ConsumedEvent = FindFProperty<FMulticastDelegateProperty>(Gun->GetClass(), TEXT("OnUsableConsum"));
	if (!TestNotNull(TEXT("Ammo consumption delegate"), ConsumedEvent)) return false;
	const auto* Callbacks = ConsumedEvent->GetMulticastDelegate(ConsumedEvent->ContainerPtrToValuePtr<void>(Gun));
	const int32 ExistingCallbacks = Callbacks->GetAllObjects().Num();
	UInitialWeaponLibrary::SetWeaponSlotConsumptionBinding(HUD, Gun, true);
	UInitialWeaponLibrary::SetWeaponSlotConsumptionBinding(HUD, Gun, true);
	TestTrue(TEXT("Slot updates keep listening to every owned weapon"), Callbacks->Contains(HUD, FName(TEXT("RF_HandleAmmoConsumed"))));
	TestEqual(TEXT("Rebinding after a swap cannot duplicate callbacks"), Callbacks->GetAllObjects().Num(), ExistingCallbacks + 1);
	UInitialWeaponLibrary::SetWeaponSlotConsumptionBinding(HUD, Gun, false);
	TestEqual(TEXT("Changing the displayed pawn releases its weapon callback"), Callbacks->GetAllObjects().Num(), ExistingCallbacks);
	TestTrue(TEXT("Catalog identifies default weapon"), UInitialWeaponLibrary::IsDefaultWeapon(Base));
	TestFalse(TEXT("Regular ammo blocks default selection"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 1));
	TestTrue(TEXT("Regular weapon remains selectable"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 2));
	TestEqual(TEXT("Restricted pistol is absent from action slots"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 1), 0);
	TestEqual(TEXT("Machinegun moves from shortcut two to one"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 2), 1);
	TestEqual(TEXT("Key one selects the displayed machinegun"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 1), 2);
	TestEqual(TEXT("Empty numeric shortcut does nothing"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 2), INDEX_NONE);
	TestFalse(TEXT("Empty slot is rejected"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 3));
	TestFalse(TEXT("Invalid index is rejected"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, -1));
	TestEqual(TEXT("Direct default request resolves to regular weapon"), UInitialWeaponLibrary::ResolveInventoryWeaponSelection(Inventory, 1), 2);
	SetAmmo(Gun, 0);
	TestTrue(TEXT("Default becomes available after all regular ammo is exhausted"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 1));
	TestFalse(TEXT("Empty regular gun is skipped"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 2));
	TestEqual(TEXT("Pistol reappears as shortcut one after depletion"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 1), 1);
	TestEqual(TEXT("Depleted regular gun keeps its action slot"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 2), 2);
	TestEqual(TEXT("Visible empty gun keeps its hotkey mapping while selection remains blocked"),
		UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 2), 2);
	TestEqual(TEXT("Key one falls back to the pistol"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 1), 1);
	FInitialWeaponEntry ShotgunSelection, ShotgunEntry;
	ShotgunSelection.WeaponName = TEXT("ID.ShotGun");
	if (!TestTrue(TEXT("Second regular weapon catalog entry"), Loadout->LoadoutData->ResolveWeaponEntry(ShotgunSelection, ShotgunEntry))) return false;
	AActor* Shotgun = World->SpawnActor<AActor>(ShotgunEntry.WeaponClass.LoadSynchronous(), Spawn);
	if (!TestNotNull(TEXT("Second regular gun"), Shotgun)) return false;
	auto* ShotgunData = FindFProperty<FObjectPropertyBase>(Shotgun->GetClass(), TEXT("weaponData"));
	auto* ShotgunInfo = FindFProperty<FStructProperty>(Shotgun->GetClass(), TEXT("itemInfo"));
	if (!TestNotNull(TEXT("Second weapon data contract"), ShotgunData) || !TestNotNull(TEXT("Second held item contract"), ShotgunInfo)) return false;
	ShotgunData->SetObjectPropertyValue_InContainer(Shotgun, ShotgunEntry.WeaponData.LoadSynchronous());
	for (TFieldIterator<FObjectPropertyBase> It(ShotgunInfo->Struct); It; ++It)
	{
		void* InfoData = ShotgunInfo->ContainerPtrToValuePtr<void>(Shotgun);
		if (It->GetName().StartsWith(TEXT("ItemData"))) It->SetObjectPropertyValue_InContainer(InfoData, ShotgunEntry.WeaponData.Get());
		if (It->GetName().StartsWith(TEXT("ItemOwner"))) It->SetObjectPropertyValue_InContainer(InfoData, Owner);
	}
	if (!TestTrue(TEXT("Second regular ammo contract"), SetAmmo(Shotgun, 1))) return false;
	Item->SetObjectPropertyValue(Array.GetRawPtr(3), Shotgun);
	TestEqual(TEXT("Pistol stays hidden while another regular weapon has ammo"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 1), 0);
	TestEqual(TEXT("Empty machinegun keeps its shifted slot beside a usable shotgun"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 2), 1);
	TestEqual(TEXT("Usable shotgun keeps its shifted shortcut"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 2), 3);
	TestFalse(TEXT("Keeping an empty slot never enables selecting it"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 2));
	Item->SetObjectPropertyValue(Array.GetRawPtr(3), nullptr);
	SetAmmo(Gun, 1);
	TestFalse(TEXT("Recovered ammo locks default again"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 1));
	TestEqual(TEXT("Recovered ammo hides the pistol again"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 1), 0);
	TestEqual(TEXT("Recovered gun takes shortcut one again"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 1), 2);
	auto* Info = FindFProperty<FStructProperty>(InventoryClass, TEXT("IndexInfo"));
	if (!TestNotNull(TEXT("Selection state contract"), Info)) return false;
	FIntProperty* Selected = nullptr;
	for (TFieldIterator<FIntProperty> It(Info->Struct); It; ++It)
		if (It->GetName().StartsWith(TEXT("SelectedIndex"))) Selected = *It;
	if (!TestNotNull(TEXT("Selected index contract"), Selected)) return false;
	Selected->SetPropertyValue_InContainer(Info->ContainerPtrToValuePtr<void>(Inventory), 1);
	UInitialWeaponLibrary::RefreshDefaultWeaponSelection(Gun);
	TestEqual(TEXT("Recovery switches from default to recovered regular weapon"),
		Selected->GetPropertyValue_InContainer(Info->ContainerPtrToValuePtr<void>(Inventory)), 2);
	SetAmmo(Gun, 0);
	UFunction* Next = Inventory->FindFunction(TEXT("Get Next Valid Index"));
	if (!TestNotNull(TEXT("Existing wheel/fallback selection function"), Next)) return false;
	FStructOnScope NextParams(Next);
	auto* Direction = FindFProperty<FBoolProperty>(Next, TEXT("Is Next"));
	auto* ResultIndex = FindFProperty<FIntProperty>(Next, TEXT("Result"));
	if (!TestNotNull(TEXT("Wheel direction contract"), Direction) || !TestNotNull(TEXT("Wheel result contract"), ResultIndex)) return false;
	Direction->SetPropertyValue_InContainer(NextParams.GetStructMemory(), true);
	Inventory->ProcessEvent(Next, NextParams.GetStructMemory());
	TestEqual(TEXT("Existing depletion route selects default after last regular ammo"),
		ResultIndex->GetPropertyValue_InContainer(NextParams.GetStructMemory()), 1);
	SetAmmo(Gun, 1);
	Loadout->LoadoutData->bRestrictDefaultWeaponSelection = false;
	TestTrue(TEXT("DA switch restores default selection"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 1));
	TestEqual(TEXT("DA off keeps the pistol's original slot number"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 1), 1);
	TestEqual(TEXT("DA off keeps the regular gun's original slot number"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, 2), 2);
	TestEqual(TEXT("DA off leaves numeric-key mapping unchanged"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 2), 2);
	TestEqual(TEXT("Reserved slot zero is not a shortcut"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, 0), INDEX_NONE);
	TestEqual(TEXT("Invalid slot is hidden"), UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(Inventory, -1), 0);
	TestEqual(TEXT("Negative shortcut is rejected"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, -1), INDEX_NONE);
	TestEqual(TEXT("Out-of-range shortcut is rejected"), UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(Inventory, MAX_int32), INDEX_NONE);
	TestEqual(TEXT("DA switch preserves explicit default request"), UInitialWeaponLibrary::ResolveInventoryWeaponSelection(Inventory, 1), 1);
	TestFalse(TEXT("Null inventory is rejected"), UInitialWeaponLibrary::CanSelectInventoryWeapon(nullptr, 0));
	return !HasAnyErrors();
}

#endif

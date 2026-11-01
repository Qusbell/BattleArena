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
	Array.Resize(3);
	Item->SetObjectPropertyValue(Array.GetRawPtr(0), Base);
	Item->SetObjectPropertyValue(Array.GetRawPtr(1), Gun);
	Item->SetObjectPropertyValue(Array.GetRawPtr(2), nullptr);
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
	TestTrue(TEXT("Catalog identifies default weapon"), UInitialWeaponLibrary::IsDefaultWeapon(Base));
	TestFalse(TEXT("Regular ammo blocks default selection"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 0));
	TestTrue(TEXT("Regular weapon remains selectable"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 1));
	TestFalse(TEXT("Empty slot is rejected"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 2));
	TestFalse(TEXT("Invalid index is rejected"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, -1));
	TestEqual(TEXT("Direct default request resolves to regular weapon"), UInitialWeaponLibrary::ResolveInventoryWeaponSelection(Inventory, 0), 1);
	SetAmmo(Gun, 0);
	TestTrue(TEXT("Default becomes available after all regular ammo is exhausted"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 0));
	TestFalse(TEXT("Empty regular gun is skipped"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 1));
	SetAmmo(Gun, 1);
	TestFalse(TEXT("Recovered ammo locks default again"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 0));
	UInitialWeaponLibrary::RefreshDefaultWeaponSelection(Gun);
	auto* Info = FindFProperty<FStructProperty>(InventoryClass, TEXT("IndexInfo"));
	if (!TestNotNull(TEXT("Selection state contract"), Info)) return false;
	FIntProperty* Selected = nullptr;
	for (TFieldIterator<FIntProperty> It(Info->Struct); It; ++It)
		if (It->GetName().StartsWith(TEXT("SelectedIndex"))) Selected = *It;
	if (!TestNotNull(TEXT("Selected index contract"), Selected)) return false;
	TestEqual(TEXT("Recovery switches from default to recovered regular weapon"),
		Selected->GetPropertyValue_InContainer(Info->ContainerPtrToValuePtr<void>(Inventory)), 1);
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
		ResultIndex->GetPropertyValue_InContainer(NextParams.GetStructMemory()), 0);
	SetAmmo(Gun, 1);
	Loadout->LoadoutData->bRestrictDefaultWeaponSelection = false;
	TestTrue(TEXT("DA switch restores default selection"), UInitialWeaponLibrary::CanSelectInventoryWeapon(Inventory, 0));
	TestEqual(TEXT("DA switch preserves explicit default request"), UInitialWeaponLibrary::ResolveInventoryWeaponSelection(Inventory, 0), 0);
	TestFalse(TEXT("Null inventory is rejected"), UInitialWeaponLibrary::CanSelectInventoryWeapon(nullptr, 0));
	return !HasAnyErrors();
}

#endif

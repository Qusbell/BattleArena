#include "Weapon/InitialWeaponLibrary.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "Weapon/InitialWeaponLoadoutDataAsset.h"
#include "GameFramework/Actor.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
	TArray<AActor*> InventoryWeapons(UActorComponent* Inventory)
	{
		TArray<AActor*> Result;
		if (!IsValid(Inventory)) return Result;
		const FArrayProperty* Items = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("All_Items"));
		const FObjectPropertyBase* Item = Items ? CastField<FObjectPropertyBase>(Items->Inner) : nullptr;
		if (!Item) return Result;
		FScriptArrayHelper Array(Items, Items->ContainerPtrToValuePtr<void>(Inventory));
		for (int32 Index = 0; Index < Array.Num(); ++Index)
			Result.Add(Cast<AActor>(Item->GetObjectPropertyValue(Array.GetRawPtr(Index))));
		return Result;
	}

	bool HasAmmo(AActor* Weapon)
	{
		if (!IsValid(Weapon)) return false;
		const FStructProperty* Usage = FindFProperty<FStructProperty>(Weapon->GetClass(), TEXT("currentUsageCount"));
		if (!Usage) return false;
		for (TFieldIterator<FIntProperty> It(Usage->Struct); It; ++It)
			if (It->GetName().StartsWith(TEXT("currentUsageCount")))
				return It->GetPropertyValue_InContainer(Usage->ContainerPtrToValuePtr<void>(Weapon)) > 0;
		return false;
	}

	const UInitialWeaponLoadoutComponent* RestrictedLoadout(UActorComponent* Inventory)
	{
		AActor* Owner = IsValid(Inventory) ? Inventory->GetOwner() : nullptr;
		const auto* Loadout = IsValid(Owner) ? Owner->FindComponentByClass<UInitialWeaponLoadoutComponent>() : nullptr;
		return Loadout && Loadout->LoadoutData && Loadout->LoadoutData->bRestrictDefaultWeaponSelection ? Loadout : nullptr;
	}

	bool IsRegularWeapon(AActor* Weapon, const UInitialWeaponLoadoutComponent* Loadout)
	{
		return IsValid(Weapon) && FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData"))
			&& !Loadout->IsDefaultWeapon(Weapon) && HasAmmo(Weapon);
	}

	int32 SelectedIndex(UActorComponent* Inventory)
	{
		const FStructProperty* Info = FindFProperty<FStructProperty>(Inventory->GetClass(), TEXT("IndexInfo"));
		if (!Info) return INDEX_NONE;
		for (TFieldIterator<FIntProperty> It(Info->Struct); It; ++It)
			if (It->GetName().StartsWith(TEXT("SelectedIndex")))
				return It->GetPropertyValue_InContainer(Info->ContainerPtrToValuePtr<void>(Inventory));
		return INDEX_NONE;
	}
}

bool UInitialWeaponLibrary::IsDefaultWeapon(const AActor* Weapon)
{
	if (!IsValid(Weapon))
	{
		return false;
	}

	const AActor* Owner = Weapon->GetOwner();
	const UInitialWeaponLoadoutComponent* Loadout = IsValid(Owner)
		? Owner->FindComponentByClass<UInitialWeaponLoadoutComponent>()
		: nullptr;
	return Loadout && Loadout->IsDefaultWeapon(Weapon);
}

bool UInitialWeaponLibrary::CanSelectInventoryWeapon(UActorComponent* Inventory, int32 Index)
{
	const TArray<AActor*> Weapons = InventoryWeapons(Inventory);
	if (!Weapons.IsValidIndex(Index) || !HasAmmo(Weapons[Index])) return false;
	const auto* Loadout = RestrictedLoadout(Inventory);
	if (!Loadout || !Loadout->IsDefaultWeapon(Weapons[Index])) return true;
	return !Weapons.ContainsByPredicate([Loadout](AActor* Weapon) { return IsRegularWeapon(Weapon, Loadout); });
}

int32 UInitialWeaponLibrary::ResolveInventoryWeaponSelection(UActorComponent* Inventory, int32 RequestedIndex)
{
	const auto* Loadout = RestrictedLoadout(Inventory);
	if (!Loadout) return RequestedIndex;
	const TArray<AActor*> Weapons = InventoryWeapons(Inventory);
	if (!Weapons.IsValidIndex(RequestedIndex) || !Loadout->IsDefaultWeapon(Weapons[RequestedIndex])) return RequestedIndex;
	const int32 Current = SelectedIndex(Inventory);
	if (Weapons.IsValidIndex(Current) && IsRegularWeapon(Weapons[Current], Loadout)) return Current;
	const int32 Regular = Weapons.IndexOfByPredicate([Loadout](AActor* Weapon) { return IsRegularWeapon(Weapon, Loadout); });
	return Regular != INDEX_NONE ? Regular : RequestedIndex;
}

void UInitialWeaponLibrary::RefreshDefaultWeaponSelection(AActor* ChangedWeapon)
{
	AActor* Owner = IsValid(ChangedWeapon) ? ChangedWeapon->GetOwner() : nullptr;
	if (!IsValid(Owner) || !Owner->HasAuthority() || !HasAmmo(ChangedWeapon)) return;
	TInlineComponentArray<UActorComponent*> Components(Owner);
	for (UActorComponent* Inventory : Components)
	{
		if (!RestrictedLoadout(Inventory)) continue;
		const TArray<AActor*> Weapons = InventoryWeapons(Inventory);
		if (!Weapons.Contains(ChangedWeapon)) continue;
		const int32 Current = SelectedIndex(Inventory);
		const int32 Next = ResolveInventoryWeaponSelection(Inventory, Current);
		if (Current == Next) return;
		UFunction* SetIndex = Inventory->FindFunction(TEXT("SetIndex"));
		FIntProperty* Index = SetIndex ? FindFProperty<FIntProperty>(SetIndex, TEXT("new Index")) : nullptr;
		if (!Index || !Index->HasAnyPropertyFlags(CPF_Parm)) return;
		FStructOnScope Parameters(SetIndex);
		Index->SetPropertyValue_InContainer(Parameters.GetStructMemory(), Next);
		Inventory->ProcessEvent(SetIndex, Parameters.GetStructMemory());
		return;
	}
}

#include "Weapon/InitialWeaponLoadoutDataAsset.h"

#include "DataTableUtils.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace
{
	bool ResolveCatalogRow(const UDataTable* Table, FName Name, FInitialWeaponEntry& OutEntry)
	{
		if (!IsValid(Table) || !Table->GetRowStruct() || Name.IsNone()) return false;
		const uint8* Row = Table->FindRowUnchecked(Name);
		if (!Row) return false;

		UClass* Class = nullptr;
		UPrimaryDataAsset* Data = nullptr;
		UObject* ItemData = nullptr;
		for (TFieldIterator<FProperty> It(Table->GetRowStruct()); It; ++It)
		{
			// Friendly editor names and cooked authored names may differ in spacing/case.
			const FString Column = DataTableUtils::GetPropertyExportName(*It).Replace(TEXT(" "), TEXT("")).Replace(TEXT("_"), TEXT(""));
			if (Column.Equals(TEXT("item"), ESearchCase::IgnoreCase))
			{
				if (const FClassProperty* Property = CastField<FClassProperty>(*It))
					Class = Cast<UClass>(Property->GetObjectPropertyValue_InContainer(Row));
			}
			else if (const FObjectPropertyBase* Property = CastField<FObjectPropertyBase>(*It))
			{
				if (Column.Equals(TEXT("WeaponDA"), ESearchCase::IgnoreCase)) Data = Cast<UPrimaryDataAsset>(Property->GetObjectPropertyValue_InContainer(Row));
				else if (Column.Equals(TEXT("ItemDA"), ESearchCase::IgnoreCase)) ItemData = Property->GetObjectPropertyValue_InContainer(Row);
			}
		}
		if (!IsValid(Class) || !Class->IsChildOf(AActor::StaticClass())
			|| Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
			|| !IsValid(Data) || (ItemData && ItemData != Data)) return false;
		const FObjectPropertyBase* DataProperty = FindFProperty<FObjectPropertyBase>(Class, TEXT("weaponData"));
		if (!DataProperty || !DataProperty->PropertyClass || !Data->IsA(DataProperty->PropertyClass)) return false;

		OutEntry.WeaponName = Name;
		OutEntry.WeaponClass = Class;
		OutEntry.WeaponData = Data;
		return true;
	}
}

UInitialWeaponLoadoutDataAsset::UInitialWeaponLoadoutDataAsset()
{
	WeaponTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/Data/Item/DropTable/DT_HeldItemTable.DT_HeldItemTable")));
	DefaultWeapons.AddDefaulted_GetRef().WeaponName = TEXT("ID.Pistol");
	GrantedWeapons.AddDefaulted_GetRef().WeaponName = TEXT("ID.MachineGun");
}

bool UInitialWeaponLoadoutDataAsset::ResolveWeaponEntry(const FInitialWeaponEntry& Entry, FInitialWeaponEntry& OutEntry) const
{
	// Copy first so callers may safely resolve in place.
	const FInitialWeaponEntry Input = Entry;
	OutEntry = FInitialWeaponEntry();
	if (!Input.WeaponName.IsNone()) return ResolveCatalogRow(WeaponTable.LoadSynchronous(), Input.WeaponName, OutEntry);
	if (Input.WeaponClass.IsNull() || Input.WeaponData.IsNull()) return false;
	OutEntry = Input;
	return true;
}

TArray<FString> UInitialWeaponLoadoutDataAsset::GetWeaponOptions() const
{
	TArray<FString> Options;
	if (const UDataTable* Table = WeaponTable.LoadSynchronous(); IsValid(Table))
	{
		for (const FName Name : Table->GetRowNames())
		{
			FInitialWeaponEntry Entry;
			if (ResolveCatalogRow(Table, Name, Entry)) Options.Add(Name.ToString());
		}
	}
	Options.Sort();
	Options.Insert(TEXT("None"), 0);
	return Options;
}

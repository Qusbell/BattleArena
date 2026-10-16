#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "Weapon/InitialWeaponLoadoutDataAsset.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr TCHAR InventoryInterfacePath[] = TEXT("/Game/QuakeLike_1_0/Inventory/BPI_Inventory.BPI_Inventory_C");

	FObjectPropertyBase* FindItemDataProperty(FStructProperty* ItemInfoProperty)
	{
		if (!ItemInfoProperty)
		{
			return nullptr;
		}

		for (TFieldIterator<FProperty> It(ItemInfoProperty->Struct); It; ++It)
		{
			if (It->GetName().StartsWith(TEXT("ItemData")))
			{
				if (FObjectPropertyBase* ItemData = CastField<FObjectPropertyBase>(*It))
				{
					return ItemData;
				}
			}
		}
		return nullptr;
	}

	UActorComponent* FindInventory(AActor* Owner)
	{
		UClass* InventoryInterface = LoadObject<UClass>(nullptr, InventoryInterfacePath);
		if (!InventoryInterface)
		{
			return nullptr;
		}

		TArray<UActorComponent*> Components;
		Owner->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (IsValid(Component) && Component->GetClass()->ImplementsInterface(InventoryInterface))
			{
				return Component;
			}
		}
		return nullptr;
	}

	bool PickupWeapon(UActorComponent* Inventory, AActor* Weapon)
	{
		if (!IsValid(Inventory) || !IsValid(Weapon))
		{
			return false;
		}
		UFunction* Function = Inventory->FindFunction(TEXT("Pickup"));
		if (!Function)
		{
			return false;
		}

		FObjectPropertyBase* ItemParameter = nullptr;
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm)
				&& It->GetName().Equals(TEXT("item"), ESearchCase::IgnoreCase))
			{
				ItemParameter = CastField<FObjectPropertyBase>(*It);
				break;
			}
		}
		if (!ItemParameter || !Weapon->IsA(ItemParameter->PropertyClass))
		{
			return false;
		}

		FStructOnScope Parameters(Function);
		ItemParameter->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), Weapon);
		Inventory->ProcessEvent(Function, Parameters.GetStructMemory());
		return true;
	}

	bool GrantOne(AActor* Owner, UActorComponent* Inventory, const FInitialWeaponEntry& Entry, bool bDefault)
	{
		if (!IsValid(Owner) || !IsValid(Inventory))
		{
			return false;
		}
		UWorld* World = Owner->GetWorld();
		if (!IsValid(World))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon on %s cannot spawn without a valid world."), *Owner->GetName());
			return false;
		}

		UClass* WeaponClass = Entry.WeaponClass.LoadSynchronous();
		UPrimaryDataAsset* Data = Entry.WeaponData.LoadSynchronous();
		if (!IsValid(WeaponClass) || !IsValid(Data) || !WeaponClass->IsChildOf(AActor::StaticClass())
			|| WeaponClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon entry on %s has an invalid class (%s) or data asset (%s)."),
				*Owner->GetName(), *Entry.WeaponClass.ToString(), *Entry.WeaponData.ToString());
			return false;
		}

		FObjectPropertyBase* WeaponDataProperty = FindFProperty<FObjectPropertyBase>(WeaponClass, TEXT("weaponData"));
		FStructProperty* ItemInfoProperty = FindFProperty<FStructProperty>(WeaponClass, TEXT("itemInfo"));
		FObjectPropertyBase* ItemDataProperty = FindItemDataProperty(ItemInfoProperty);
		if (!WeaponDataProperty || !ItemDataProperty
			|| !WeaponDataProperty->PropertyClass || !ItemDataProperty->PropertyClass
			|| !Data->IsA(WeaponDataProperty->PropertyClass)
			|| !Data->IsA(ItemDataProperty->PropertyClass))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon %s does not accept data asset %s."), *WeaponClass->GetName(), *Data->GetName());
			return false;
		}

		UFunction* RecoverFunction = nullptr;
		FIntProperty* RecoverCountParameter = nullptr;
		int32 RecoverAmount = 0;
		if (bDefault)
		{
			const FIntProperty* MaxCountProperty = FindFProperty<FIntProperty>(Data->GetClass(), TEXT("maxUsageCount"));
			const FIntProperty* StartCountProperty = FindFProperty<FIntProperty>(Data->GetClass(), TEXT("startUsageCount"));
			RecoverFunction = WeaponClass->FindFunctionByName(TEXT("Recover"));
			RecoverCountParameter = RecoverFunction
				? FindFProperty<FIntProperty>(RecoverFunction, TEXT("recoverCount"))
				: nullptr;
			if (!MaxCountProperty || !StartCountProperty || !RecoverCountParameter
				|| !RecoverCountParameter->HasAnyPropertyFlags(CPF_Parm))
			{
				UE_LOG(LogTemp, Warning, TEXT("Default weapon %s is missing ammo count data or Recover(recoverCount)."), *WeaponClass->GetName());
				return false;
			}
			const int32 MaxCount = MaxCountProperty->GetPropertyValue_InContainer(Data);
			const int32 StartCount = StartCountProperty->GetPropertyValue_InContainer(Data);
			if (MaxCount < 0 || StartCount < 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("Default weapon %s has a negative start or max ammo count."), *WeaponClass->GetName());
				return false;
			}
			RecoverAmount = FMath::Max(0, MaxCount - StartCount);
		}

		const FTransform SpawnTransform = Owner->GetActorTransform();
		AActor* Weapon = World->SpawnActorDeferred<AActor>(
			WeaponClass, SpawnTransform, Owner, Cast<APawn>(Owner),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!IsValid(Weapon))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon %s could not be spawned for %s."), *WeaponClass->GetName(), *Owner->GetName());
			return false;
		}

		WeaponDataProperty->SetObjectPropertyValue_InContainer(Weapon, Data);
		void* ItemInfo = ItemInfoProperty->ContainerPtrToValuePtr<void>(Weapon);
		ItemDataProperty->SetObjectPropertyValue_InContainer(ItemInfo, Data);
		Weapon->FinishSpawning(SpawnTransform);
		if (!IsValid(Weapon))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon %s was destroyed during spawning."), *WeaponClass->GetName());
			return false;
		}

		if (bDefault)
		{
			// BP_HeldItem initializes ammo from startUsageCount before Recover runs.
			FStructOnScope Parameters(RecoverFunction);
			RecoverCountParameter->SetPropertyValue_InContainer(
				Parameters.GetStructMemory(), RecoverAmount);
			Weapon->ProcessEvent(RecoverFunction, Parameters.GetStructMemory());
			if (!IsValid(Weapon))
			{
				UE_LOG(LogTemp, Warning, TEXT("Default weapon %s was destroyed while setting ammo."), *WeaponClass->GetName());
				return false;
			}
		}

		if (!PickupWeapon(Inventory, Weapon))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon %s could not be picked up by %s."),
				*GetNameSafe(Weapon), *GetNameSafe(Inventory));
			if (IsValid(Weapon))
			{
				Weapon->Destroy();
			}
			return false;
		}
		return true;
	}
}

UInitialWeaponLoadoutComponent::UInitialWeaponLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInitialWeaponLoadoutComponent::GrantLoadout()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority() || bGrantedThisSpawn)
	{
		return;
	}
	if (!IsValid(Owner->GetWorld()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Initial weapon loadout on %s has no valid world."), *Owner->GetName());
		return;
	}
	UInitialWeaponLoadoutDataAsset* Data = LoadoutData.Get();
	if (!IsValid(Data))
	{
		UE_LOG(LogTemp, Warning, TEXT("Initial weapon loadout on %s has no data asset."), *Owner->GetName());
		return;
	}

	UActorComponent* Inventory = FindInventory(Owner);
	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("Initial weapon loadout on %s could not find its inventory."), *Owner->GetName());
		return;
	}

	bGrantedThisSpawn = true;
	TSet<FSoftObjectPath> GrantedClasses;
	for (const FInitialWeaponEntry& Entry : Data->DefaultWeapons)
	{
		if (GrantedClasses.Contains(Entry.WeaponClass.ToSoftObjectPath()))
		{
			continue;
		}
		if (GrantOne(Owner, Inventory, Entry, true))
		{
			GrantedClasses.Add(Entry.WeaponClass.ToSoftObjectPath());
		}
	}
	for (const FInitialWeaponEntry& Entry : Data->GrantedWeapons)
	{
		if (GrantedClasses.Contains(Entry.WeaponClass.ToSoftObjectPath()))
		{
			continue;
		}
		if (GrantOne(Owner, Inventory, Entry, false))
		{
			GrantedClasses.Add(Entry.WeaponClass.ToSoftObjectPath());
		}
	}
}

bool UInitialWeaponLoadoutComponent::IsDefaultWeapon(const AActor* Weapon) const
{
	if (!IsValid(Weapon))
	{
		return false;
	}
	const UInitialWeaponLoadoutDataAsset* Loadout = LoadoutData.Get();
	if (!IsValid(Loadout))
	{
		return false;
	}

	const FObjectPropertyBase* WeaponDataProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData"));
	const UObject* Data = WeaponDataProperty
		? WeaponDataProperty->GetObjectPropertyValue_InContainer(Weapon)
		: nullptr;
	for (const FInitialWeaponEntry& Entry : Loadout->DefaultWeapons)
	{
		if ((Data && Entry.WeaponData.Get() == Data)
			|| Entry.WeaponClass.Get() == Weapon->GetClass())
		{
			return true;
		}
	}
	return false;
}

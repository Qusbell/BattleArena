#include "Weapon/InitialWeaponLoadoutComponent.h"

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
				return CastField<FObjectPropertyBase>(*It);
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
		UClass* WeaponClass = Entry.WeaponClass.LoadSynchronous();
		UPrimaryDataAsset* Data = Entry.WeaponData.LoadSynchronous();
		if (!WeaponClass || !Data || !WeaponClass->IsChildOf(AActor::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon entry on %s has no valid class or data asset."), *Owner->GetName());
			return false;
		}

		FObjectPropertyBase* WeaponDataProperty = FindFProperty<FObjectPropertyBase>(WeaponClass, TEXT("weaponData"));
		FStructProperty* ItemInfoProperty = FindFProperty<FStructProperty>(WeaponClass, TEXT("itemInfo"));
		FObjectPropertyBase* ItemDataProperty = FindItemDataProperty(ItemInfoProperty);
		if (!WeaponDataProperty || !ItemDataProperty
			|| !Data->IsA(WeaponDataProperty->PropertyClass)
			|| !Data->IsA(ItemDataProperty->PropertyClass))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon %s does not accept data asset %s."), *WeaponClass->GetName(), *Data->GetName());
			return false;
		}

		const FTransform SpawnTransform = Owner->GetActorTransform();
		AActor* Weapon = Owner->GetWorld()->SpawnActorDeferred<AActor>(
			WeaponClass, SpawnTransform, Owner, Cast<APawn>(Owner),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Weapon)
		{
			return false;
		}

		WeaponDataProperty->SetObjectPropertyValue_InContainer(Weapon, Data);
		void* ItemInfo = ItemInfoProperty->ContainerPtrToValuePtr<void>(Weapon);
		ItemDataProperty->SetObjectPropertyValue_InContainer(ItemInfo, Data);
		Weapon->FinishSpawning(SpawnTransform);

		if (bDefault)
		{
			// Recover adds this amount to the starting count and clamps it to maxUsageCount.
			const FIntProperty* MaxCountProperty = FindFProperty<FIntProperty>(Data->GetClass(), TEXT("maxUsageCount"));
			UFunction* RecoverFunction = Weapon->FindFunction(TEXT("Recover"));
			FIntProperty* RecoverCountParameter = RecoverFunction
				? FindFProperty<FIntProperty>(RecoverFunction, TEXT("recoverCount"))
				: nullptr;
			if (!MaxCountProperty || !RecoverCountParameter)
			{
				UE_LOG(LogTemp, Warning, TEXT("Default weapon %s has no maxUsageCount or Recover(recoverCount)."), *Weapon->GetName());
				Weapon->Destroy();
				return false;
			}

			FStructOnScope Parameters(RecoverFunction);
			RecoverCountParameter->SetPropertyValue_InContainer(
				Parameters.GetStructMemory(), MaxCountProperty->GetPropertyValue_InContainer(Data));
			Weapon->ProcessEvent(RecoverFunction, Parameters.GetStructMemory());
		}

		if (!PickupWeapon(Inventory, Weapon))
		{
			UE_LOG(LogTemp, Warning, TEXT("Initial weapon %s could not be picked up by %s."), *Weapon->GetName(), *Inventory->GetName());
			Weapon->Destroy();
			return false;
		}
		return true;
	}
}

UInitialWeaponLoadoutComponent::UInitialWeaponLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	FInitialWeaponEntry& Pistol = DefaultWeapons.AddDefaulted_GetRef();
	Pistol.WeaponClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_Pistol.BP_Weapon_Pistol_C")));
	Pistol.WeaponData = TSoftObjectPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/Data/Item/DA_Pistol.DA_Pistol")));

	FInitialWeaponEntry& MachineGun = GrantedWeapons.AddDefaulted_GetRef();
	MachineGun.WeaponClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_MachineGun.BP_Weapon_MachineGun_C")));
	MachineGun.WeaponData = TSoftObjectPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/Data/Item/DA_MachineGun.DA_MachineGun")));
}

void UInitialWeaponLoadoutComponent::GrantLoadout()
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner) || !Owner->HasAuthority() || bGrantedThisSpawn)
	{
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
	for (const FInitialWeaponEntry& Entry : DefaultWeapons)
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
	for (const FInitialWeaponEntry& Entry : GrantedWeapons)
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

	const FObjectPropertyBase* WeaponDataProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData"));
	const UObject* Data = WeaponDataProperty
		? WeaponDataProperty->GetObjectPropertyValue_InContainer(Weapon)
		: nullptr;
	for (const FInitialWeaponEntry& Entry : DefaultWeapons)
	{
		if ((Data && Entry.WeaponData.Get() == Data)
			|| Entry.WeaponClass.Get() == Weapon->GetClass())
		{
			return true;
		}
	}
	return false;
}

#include "Weapon/InitialWeaponLibrary.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "Weapon/InitialWeaponLoadoutDataAsset.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
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

	UObject* ReadWeaponHUDObject(const UObject* Object, FName Name)
	{
		const auto* Property = IsValid(Object) ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
		return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
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

int32 UInitialWeaponLibrary::GetInventoryWeaponSlotNumber(UActorComponent* Inventory, int32 Index)
{
	const TArray<AActor*> Weapons = InventoryWeapons(Inventory);
	if (Index <= 0 || !Weapons.IsValidIndex(Index) || !IsValid(Weapons[Index])) return 0;
	int32 Number = Index;
	if (const auto* Loadout = RestrictedLoadout(Inventory))
	{
		if (Loadout->IsDefaultWeapon(Weapons[Index]) && !CanSelectInventoryWeapon(Inventory, Index)) return 0;
		if (!Loadout->IsDefaultWeapon(Weapons[Index])
			&& !Weapons.ContainsByPredicate([Loadout](AActor* Weapon) { return IsRegularWeapon(Weapon, Loadout); })) return 0;
		for (int32 Before = 0; Before < Index; ++Before)
			if (IsValid(Weapons[Before]) && Loadout->IsDefaultWeapon(Weapons[Before])
				&& !CanSelectInventoryWeapon(Inventory, Before)) --Number;
	}
	return Number;
}

int32 UInitialWeaponLibrary::ResolveInventoryWeaponShortcut(UActorComponent* Inventory, int32 ShortcutIndex)
{
	const TArray<AActor*> Weapons = InventoryWeapons(Inventory);
	if (ShortcutIndex <= 0 || ShortcutIndex >= Weapons.Num()) return INDEX_NONE;
	for (int32 Index = 1; Index < Weapons.Num(); ++Index)
		if (GetInventoryWeaponSlotNumber(Inventory, Index) == ShortcutIndex) return Index;
	return INDEX_NONE;
}

void UInitialWeaponLibrary::SetWeaponSlotConsumptionBinding(UObject* HUD, AActor* Weapon, bool bBind)
{
	if (!IsValid(HUD) || !IsValid(Weapon) || !HUD->FindFunction(TEXT("RF_HandleAmmoConsumed"))) return;
	const auto* Event = FindFProperty<FMulticastDelegateProperty>(Weapon->GetClass(), TEXT("OnUsableConsum"));
	if (!Event) return;
	FScriptDelegate Callback;
	Callback.BindUFunction(HUD, TEXT("RF_HandleAmmoConsumed"));
	if (bBind) Event->AddDelegate(Callback, Weapon);
	else Event->RemoveDelegate(Callback, Weapon);
}

void UInitialWeaponLibrary::UpdateDisplayedWeaponSlotAmmo(UUserWidget* HUD, int32 SlotNumber, double AmmoPercent)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WeaponHUD_UpdateSlotAmmo);
	if (!IsValid(HUD) || SlotNumber <= 0) return;
	const APlayerController* Player = HUD->GetOwningPlayer();
	const auto* Root = Player ? Cast<UUserWidget>(ReadWeaponHUDObject(Player->GetHUD(), TEXT("WB_ShooterHUD"))) : nullptr;
	const auto* Bar = Root ? Cast<UUserWidget>(Root->GetWidgetFromName(TEXT("WB_ActionBar_Base"))) : nullptr;
	const auto* Mapping = Bar ? FindFProperty<FMapProperty>(Bar->GetClass(), TEXT("SlotMapping")) : nullptr;
	const auto* Key = Mapping ? CastField<FObjectPropertyBase>(Mapping->KeyProp) : nullptr;
	if (!Key) return;

	const FText SlotText = FText::AsNumber(SlotNumber);
	FScriptMapHelper Slots(Mapping, Mapping->ContainerPtrToValuePtr<void>(Bar));
	for (FScriptMapHelper::FIterator It(Slots); It; ++It)
	{
		const auto* Slot = Cast<UUserWidget>(Key->GetObjectPropertyValue(Slots.GetKeyPtr(It)));
		if (!IsValid(Slot) || !Slot->GetParent() || Slot->GetOwningPlayer() != Player) continue;
		const auto* KeyText = Cast<UTextBlock>(Slot->GetWidgetFromName(TEXT("Text_SlotKey")));
		if (!KeyText || !KeyText->GetText().EqualTo(SlotText)) continue;
		auto* Progress = Slot->GetWidgetFromName(TEXT("WB_AmmoProgress_ActionSlot"));
		UFunction* Update = IsValid(Progress) ? Progress->FindFunction(TEXT("UpdateProgress")) : nullptr;
		const auto* Percent = Update ? FindFProperty<FNumericProperty>(Update, TEXT("InPercent")) : nullptr;
		if (!Percent || !Percent->IsFloatingPoint()) continue;
		FStructOnScope Parameters(Update);
		Percent->SetFloatingPointPropertyValue(Percent->ContainerPtrToValuePtr<void>(Parameters.GetStructMemory()), AmmoPercent);
		Progress->ProcessEvent(Update, Parameters.GetStructMemory());
		return;
	}
}

void UInitialWeaponLibrary::QueueDisplayedWeaponSlotRefresh(UUserWidget* HUD)
{
	if (!IsValid(HUD) || !HUD->GetWorld()) return;
	FTimerDynamicDelegate Refresh;
	Refresh.BindUFunction(HUD, TEXT("RefreshCompetitveWeaponSlots"));
	FTimerManager& Timers = HUD->GetWorld()->GetTimerManager();
	if (Refresh.IsBound() && !Timers.K2_FindDynamicTimerHandle(Refresh).IsValid())
	{
		Timers.SetTimerForNextTick(Refresh);
	}
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

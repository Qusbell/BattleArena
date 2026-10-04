#include "Weapon/WeaponPresentationLibrary.h"

#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace
{
template<typename T>
const T* StructField(const FStructProperty* Struct, const TCHAR* Prefix)
{
	if (!Struct) return nullptr;
	for (TFieldIterator<T> It(Struct->Struct); It; ++It)
		if (It->GetName().StartsWith(Prefix)) return *It;
	return nullptr;
}

bool ReadEquipped(const AActor* Item, bool& bEquipped)
{
	const FStructProperty* Info = IsValid(Item) ? FindFProperty<FStructProperty>(Item->GetClass(), TEXT("itemInfo")) : nullptr;
	const FBoolProperty* Equip = StructField<FBoolProperty>(Info, TEXT("IsEquip_"));
	if (!Equip) return false;
	bEquipped = Equip->GetPropertyValue_InContainer(Info->ContainerPtrToValuePtr<void>(Item));
	return true;
}
}

void UWeaponPresentationLibrary::ReconcileStandaloneEquip(UActorComponent* Inventory)
{
	// An authority check would also affect dedicated/listen servers. This fallback
	// belongs strictly to local Standalone and never executes on network clients.
	if (!IsValid(Inventory) || !Inventory->GetWorld() || Inventory->GetWorld()->GetNetMode() != NM_Standalone) return;
	const APawn* Pawn = Cast<APawn>(Inventory->GetOwner());
	if (!IsValid(Pawn) || !Pawn->IsLocallyControlled()) return;
	const FStructProperty* IndexInfo = FindFProperty<FStructProperty>(Inventory->GetClass(), TEXT("IndexInfo"));
	const FIntProperty* Selected = StructField<FIntProperty>(IndexInfo, TEXT("SelectedIndex_"));
	const FIntProperty* Before = StructField<FIntProperty>(IndexInfo, TEXT("BeforeIndex_"));
	const FArrayProperty* Items = FindFProperty<FArrayProperty>(Inventory->GetClass(), TEXT("All_Items"));
	const FObjectPropertyBase* ItemProperty = Items ? CastField<FObjectPropertyBase>(Items->Inner) : nullptr;
	if (!Selected || !Before || !ItemProperty) return;
	const void* IndexMemory = IndexInfo->ContainerPtrToValuePtr<void>(Inventory);
	const int32 SelectedIndex = Selected->GetPropertyValue_InContainer(IndexMemory);
	const int32 BeforeIndex = Before->GetPropertyValue_InContainer(IndexMemory);
	FScriptArrayHelper Array(Items, Items->ContainerPtrToValuePtr<void>(Inventory));
	if (!Array.IsValidIndex(SelectedIndex)) return;
	AActor* SelectedItem = Cast<AActor>(ItemProperty->GetObjectPropertyValue(Array.GetRawPtr(SelectedIndex)));
	if (!IsValid(SelectedItem) || SelectedItem->GetOwner() != Pawn) return;
	bool bSelectedEquipped = false;
	if (!ReadEquipped(SelectedItem, bSelectedEquipped)) return;
	bool bPreviousStillEquipped = false;
	if (BeforeIndex != SelectedIndex && Array.IsValidIndex(BeforeIndex))
	{
		const AActor* PreviousItem = Cast<AActor>(ItemProperty->GetObjectPropertyValue(Array.GetRawPtr(BeforeIndex)));
		if (IsValid(PreviousItem) && PreviousItem->GetOwner() == Pawn) ReadEquipped(PreviousItem, bPreviousStillEquipped);
	}
	if (bSelectedEquipped && !bPreviousStillEquipped) return;
	// Reuse the existing Swap handler -> Unequip/Equip -> ChangeAnim. Calling
	// the handler directly also covers an absent local OnSwapItem binding, without
	// replaying pickup notifications or implementing a second equip path.
	UFunction* Swap = Inventory->FindFunction(TEXT("Swap"));
	// Swap is bound to OnSwapItem(New Item), even though the graph reads the
	// selected actor from the inventory instead of wiring this input onward.
	const FObjectPropertyBase* NewItem = Swap ? FindFProperty<FObjectPropertyBase>(Swap, TEXT("New Item")) : nullptr;
	if (!Swap || Swap->HasAnyFunctionFlags(FUNC_Net) || Swap->NumParms != 1 || !NewItem
		|| !NewItem->HasAnyPropertyFlags(CPF_Parm) || NewItem->HasAnyPropertyFlags(CPF_ReturnParm)
		|| !SelectedItem->IsA(NewItem->PropertyClass)) return;
	FStructOnScope Parameters(Swap);
	NewItem->SetObjectPropertyValue_InContainer(Parameters.GetStructMemory(), SelectedItem);
	Inventory->ProcessEvent(Swap, Parameters.GetStructMemory());
}

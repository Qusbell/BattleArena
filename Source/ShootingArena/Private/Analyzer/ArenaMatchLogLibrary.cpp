#include "Analyzer/ArenaMatchLogLibrary.h"
#include "Analyzer/ArenaMatchLogSubsystem.h"

#include "GameFramework/Pawn.h"

namespace
{
	FString NameText(FName Name)
	{
		return Name.IsNone() ? FString() : Name.ToString();
	}
}

void UArenaMatchLogLibrary::RecordWeaponEquip(APawn* Pawn, FName WeaponBefore, FName WeaponAfter, FName Reason)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Pawn))
	{
		Log->RecordWeaponEquip(Pawn, NameText(WeaponBefore), NameText(WeaponAfter), Reason.ToString());
	}
}

int32 UArenaMatchLogLibrary::RecordItemSpawn(AActor* Item, FName Category, FName ItemRowName, FName Reason)
{
	UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Item);
	return Log ? Log->RecordItemSpawn(Item, Category.ToString(), ItemRowName.ToString(), Reason.ToString()) : INDEX_NONE;
}

void UArenaMatchLogLibrary::RecordItemPickup(APawn* Pawn, AActor* Item, bool bHasAmounts, float ItemAmount, float AppliedAmount)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Pawn))
	{
		Log->RecordItemPickup(Pawn, Item, ItemAmount, AppliedAmount, bHasAmounts);
	}
}

void UArenaMatchLogLibrary::RecordItemRemove(AActor* Item, FName Reason)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Item))
	{
		Log->RecordItemRemove(Item, Reason.ToString());
	}
}

void UArenaMatchLogLibrary::RecordBuffChange(APawn* Pawn, FName BuffAfter, FName ChangeType, bool bHasExpiry, float ExpiresAtSeconds)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Pawn))
	{
		Log->RecordBuff(Pawn, NameText(BuffAfter), ChangeType.ToString(), ExpiresAtSeconds, bHasExpiry);
	}
}

void UArenaMatchLogLibrary::RecordDropItemSpawned(AActor* Item)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Item)) Log->RecordDropItemSpawned(Item);
}

void UArenaMatchLogLibrary::BeginItemPickup(AActor* Item, AActor* Picker)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Item)) Log->BeginItemPickup(Item, Picker);
}

void UArenaMatchLogLibrary::CommitItemPickup(AActor* Item, AActor* Picker)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Item)) Log->CommitItemPickup(Item, Picker);
}

void UArenaMatchLogLibrary::RecordWeaponSwap(AActor* Owner, AActor* BeforeItem, AActor* AfterItem)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Owner)) Log->RecordWeaponSwap(Owner, BeforeItem, AfterItem);
}

void UArenaMatchLogLibrary::RecordBuffApplied(UActorComponent* BuffComponent)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(BuffComponent)) Log->RecordBuffApplied(BuffComponent);
}

void UArenaMatchLogLibrary::RecordBuffExpired(UActorComponent* BuffComponent)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(BuffComponent)) Log->RecordBuffExpired(BuffComponent);
}

void UArenaMatchLogLibrary::RecordDamageInput(AActor* Victim, float InputDamage)
{
	if (UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(Victim)) Log->RecordDamageInput(Victim, InputDamage);
}

float UArenaMatchLogLibrary::GetMatchElapsedSeconds(const UObject* WorldContextObject)
{
	const UArenaMatchLogSubsystem* Log = UArenaMatchLogSubsystem::Get(WorldContextObject);
	return Log ? float(Log->ElapsedSeconds()) : 0.0f;
}

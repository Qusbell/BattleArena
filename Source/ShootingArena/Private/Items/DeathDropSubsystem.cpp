#include "Items/DeathDropSubsystem.h"

#include "Items/DeathDropCarrier.h"
#include "Items/DeathDropSettings.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "Weapon/InitialWeaponLoadoutDataAsset.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogDeathDrop, Log, All);

namespace
{
	struct FCandidate
	{
		UClass* PickupClass = nullptr;
		FDataTableRowHandle Item;
		double BuffEndTime = 0.0;
		bool bBuff = false;
	};

	UActorComponent* FindComponent(AActor* Owner, const TCHAR* ClassName)
	{
		TArray<UActorComponent*> Components;
		Owner->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (!IsValid(Component)) continue;
			for (UClass* Class = Component->GetClass(); Class; Class = Class->GetSuperClass())
				if (Class->GetName() == ClassName) return Component;
		}
		return nullptr;
	}

	bool ValidItem(const FDataTableRowHandle& Item)
	{
		return IsValid(Item.DataTable) && !Item.RowName.IsNone() && Item.DataTable->FindRowUnchecked(Item.RowName);
	}

	AActor* SelectedWeapon(AActor* Character)
	{
		UActorComponent* Inventory = FindComponent(Character, TEXT("BPC_Inventory_C"));
		UFunction* GetSelected = Inventory ? Inventory->FindFunction(TEXT("GetSelectedItem")) : nullptr;
		if (!GetSelected) return nullptr;
		FStructOnScope Parameters(GetSelected);
		Inventory->ProcessEvent(GetSelected, Parameters.GetStructMemory());
		for (TFieldIterator<FProperty> It(GetSelected); It; ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_OutParm)) continue;
			if (FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(*It))
				return Cast<AActor>(Object->GetObjectPropertyValue_InContainer(Parameters.GetStructMemory()));
		}
		return nullptr;
	}

	FDataTableRowHandle FindWeaponRow(UDataTable* Table, UClass* WeaponClass, UObject* WeaponData)
	{
		FDataTableRowHandle Result;
		if (!Table || !Table->GetRowStruct() || (!WeaponClass && !WeaponData)) return Result;
		FName ClassMatch;
		for (const TPair<FName, uint8*>& Row : Table->GetRowMap())
		{
			for (TFieldIterator<FProperty> It(Table->GetRowStruct()); It; ++It)
			{
				FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(*It);
				if (!Object) continue;
				UObject* Value = Object->GetObjectPropertyValue_InContainer(Row.Value);
				FString Name = It->GetName().Replace(TEXT(" "), TEXT(""));
				if (WeaponData && Value == WeaponData &&
					(Name.StartsWith(TEXT("WeaponDA")) || Name.StartsWith(TEXT("ItemDA"))))
				{
					Result.DataTable = Table; Result.RowName = Row.Key; return Result;
				}
				if (WeaponClass && Value == WeaponClass) ClassMatch = Row.Key;
			}
		}
		if (!ClassMatch.IsNone()) { Result.DataTable = Table; Result.RowName = ClassMatch; }
		return Result;
	}

	FDataTableRowHandle FirstGrantedWeaponRow(UDataTable* Table,
		const UInitialWeaponLoadoutComponent* Loadout, const AActor* Character)
	{
		const UInitialWeaponLoadoutDataAsset* Data = Loadout ? Loadout->LoadoutData.Get() : nullptr;
		if (!IsValid(Data) || Data->GrantedWeapons.IsEmpty())
		{
			UE_LOG(LogDeathDrop, Warning,
				TEXT("Skipping base weapon death drop for %s: initial loadout has no GrantedWeapons[0]."),
				*GetNameSafe(Character));
			return {};
		}
		// Multiple granted weapons will be discussed separately; use index 0 for now.
		const FInitialWeaponEntry& Entry = Data->GrantedWeapons[0];
		const FDataTableRowHandle Row = FindWeaponRow(Table,
			Entry.WeaponClass.LoadSynchronous(), Entry.WeaponData.LoadSynchronous());
		if (!ValidItem(Row))
			UE_LOG(LogDeathDrop, Warning,
				TEXT("Skipping base weapon death drop for %s: GrantedWeapons[0] has no matching pickup row in %s."),
				*GetNameSafe(Character), *GetNameSafe(Table));
		return Row;
	}
}

void UDeathDropSubsystem::HandleFatalDamage(AActor* Character, double NewHealth, FDataTableRowHandle Settings)
{
	if (!IsValid(Character) || !Character->HasAuthority() || NewHealth > 0.0 || !FMath::IsFinite(NewHealth)) return;
	UWorld* World = Character->GetWorld();
	UDeathDropSubsystem* Subsystem = World ? World->GetSubsystem<UDeathDropSubsystem>() : nullptr;
	if (!Subsystem || Subsystem->ProcessedDeaths.Contains(Character)) return;
	const FDeathDropSettings* Config = Settings.GetRow<FDeathDropSettings>(TEXT("DeathDrop"));
	if (!Config)
	{
		UE_LOG(LogDeathDrop, Error, TEXT("Death drop settings are missing on %s"), *GetNameSafe(Character));
		return;
	}
	for (auto It = Subsystem->ProcessedDeaths.CreateIterator(); It; ++It)
		if (!It->IsValid()) It.RemoveCurrent();
	Subsystem->ProcessedDeaths.Add(Character);
	const FDeathDropSettings Drop = *Config;
	const FVector DeathCenter = Character->GetActorLocation();
	const double Now = World->GetGameState() ? World->GetGameState()->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	TArray<FCandidate> Candidates;
	auto Add = [&Candidates](UClass* Class, const FDataTableRowHandle& Item, bool bBuff = false, double EndTime = 0.0)
	{
		if (Class && Class->IsChildOf(AActor::StaticClass()) && ValidItem(Item))
			Candidates.Add({Class, Item, EndTime, bBuff});
	};

	if (Drop.WeaponDropEnabled)
	{
		AActor* Weapon = SelectedWeapon(Character);
		if (IsValid(Weapon))
		{
			const FObjectPropertyBase* DataProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData"));
			UObject* Data = DataProperty ? DataProperty->GetObjectPropertyValue_InContainer(Weapon) : nullptr;
			const FBoolProperty* Base = Data ? FindFProperty<FBoolProperty>(Data->GetClass(), TEXT("BaseWeapon")) : nullptr;
			const UInitialWeaponLoadoutComponent* Loadout =
				Character->FindComponentByClass<UInitialWeaponLoadoutComponent>();
			// Keep death drops aligned with the character's actual default-weapon configuration.
			const bool bBase = Loadout && IsValid(Loadout->LoadoutData.Get())
				? Loadout->IsDefaultWeapon(Weapon) : Base && Base->GetPropertyValue_InContainer(Data);
			UDataTable* WeaponTable = Drop.WeaponTable.LoadSynchronous();
			const FDataTableRowHandle Row = bBase ? FirstGrantedWeaponRow(WeaponTable, Loadout, Character) :
				FindWeaponRow(WeaponTable, Weapon->GetClass(), Data);
			Add(Drop.WeaponPickupClass.LoadSynchronous(), Row);
		}
	}
	if (Drop.BuffDropEnabled)
	{
		UActorComponent* Buff = FindComponent(Character, TEXT("BPC_Buff_C"));
		const FDoubleProperty* End = Buff ? FindFProperty<FDoubleProperty>(Buff->GetClass(), TEXT("buffEndServerTime")) : nullptr;
		const FNameProperty* Row = Buff ? FindFProperty<FNameProperty>(Buff->GetClass(), TEXT("currentBuffRowName")) : nullptr;
		if (End && Row && End->GetPropertyValue_InContainer(Buff) > Now)
		{
			FDataTableRowHandle Item;
			Item.DataTable = Drop.BuffTable.LoadSynchronous();
			Item.RowName = Row->GetPropertyValue_InContainer(Buff);
			Add(Drop.BuffPickupClass.LoadSynchronous(), Item, true, End->GetPropertyValue_InContainer(Buff));
		}
	}
	if (Drop.SmallHealthDropEnabled) Add(Drop.LifePickupClass.LoadSynchronous(), Drop.SmallHealthItem);
	if (Drop.SmallArmorDropEnabled) Add(Drop.LifePickupClass.LoadSynchronous(), Drop.SmallArmorItem);

	// Partial Fisher-Yates: every candidate has the same chance, without replacement.
	const int32 Count = FMath::Clamp(Drop.MaxDropCount, 0, Candidates.Num());
	if (Count < Candidates.Num())
		for (int32 Index = 0; Index < Count; ++Index)
			Candidates.Swap(Index, FMath::RandRange(Index, Candidates.Num() - 1));
	Candidates.SetNum(Count);
	if (Count == 0) return;

	// Spawn successfully first so direction spacing uses the number that actually launches.
	TArray<ADeathDropCarrier*> Carriers;
	for (const FCandidate& Candidate : Candidates)
	{
		const FTransform Transform(FRotator::ZeroRotator, DeathCenter);
		ADeathDropCarrier* Carrier = World->SpawnActorDeferred<ADeathDropCarrier>(ADeathDropCarrier::StaticClass(),
			Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Carrier) continue;
		Carrier->Initialize(Drop, Candidate.PickupClass, Candidate.Item, Candidate.BuffEndTime, Candidate.bBuff);
		Carrier->FinishSpawning(Transform);
		if (IsValid(Carrier)) Carriers.Add(Carrier);
	}
	if (Carriers.IsEmpty()) return;
	const float StartYaw = FMath::FRand() * 360.0f;
	const float Error = float(FMath::Clamp(Drop.DirectionError, 0, 180));
	const float LowPitch = float(FMath::Clamp(FMath::Min(Drop.MinUpAngle, Drop.MaxUpAngle), 0, 90));
	const float HighPitch = float(FMath::Clamp(FMath::Max(Drop.MinUpAngle, Drop.MaxUpAngle), 0, 90));
	const float LowForce = FMath::Max(0.0f, FMath::Min(Drop.MinLaunchForce, Drop.MaxLaunchForce));
	const float HighForce = FMath::Max(LowForce, FMath::Max(Drop.MinLaunchForce, Drop.MaxLaunchForce));
	for (int32 Index = 0; Index < Carriers.Num(); ++Index)
	{
		const float Yaw = StartYaw + Index * (360.0f / Carriers.Num()) +
			(Carriers.Num() > 1 ? FMath::FRandRange(-Error, Error) : 0.0f);
		const float Pitch = FMath::FRandRange(LowPitch, HighPitch);
		Carriers[Index]->Launch(FRotator(Pitch, Yaw, 0.0f).Vector() * FMath::FRandRange(LowForce, HighForce));
	}
	UE_LOG(LogDeathDrop, Log, TEXT("Spawned %d death pickups for %s at %s"),
		Carriers.Num(), *GetNameSafe(Character), *DeathCenter.ToString());
}

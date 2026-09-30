#include "Buff/BuffSpawnCycleSubsystem.h"

#include "Engine/DataTable.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace BuffSpawnCycle
{
	constexpr float ScanIntervalSeconds = 0.1f;
	const FName SpawnItemTablePropertyName(TEXT("SpawnItemTable"));
	// This Blueprint variable's authored name contains a space. Reflection uses
	// the authored name here (unlike SpawnItemTable), so "HavingItem" never
	// resolves and the cycle watcher would silently skip every spawn point.
	const FName HavingItemPropertyName(TEXT("Having Item"));
	const FString BuffTableName(TEXT("DT_BuffTable"));

	bool TryGetHoldingItem(const AActor* SpawnPoint, const FProperty* Property, bool& bOutHoldingItem)
	{
		if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
		{
			bOutHoldingItem = BoolProperty->GetPropertyValue_InContainer(SpawnPoint);
			return true;
		}

		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			bOutHoldingItem = IsValid(ObjectProperty->GetObjectPropertyValue_InContainer(SpawnPoint));
			return true;
		}

		return false;
	}
}

void UBuffSpawnCycleSubsystem::Tick(const float DeltaTime)
{
	TimeUntilNextScan -= DeltaTime;
	if (TimeUntilNextScan > 0.0f)
	{
		return;
	}

	TimeUntilNextScan = BuffSpawnCycle::ScanIntervalSeconds;

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->GetClass()->GetName().StartsWith(TEXT("BP_BuffItem_C")))
		{
			UpdateReplicatedBuffItem(Actor);
		}

		if (Actor->HasAuthority()
			&& Actor->GetClass()->GetName().StartsWith(TEXT("BP_ItemSpawnPoint_C")))
		{
			UpdateSpawnPoint(Actor);
		}
	}

	for (auto It = SpawnPointStates.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = InitializedBuffItems.CreateIterator(); It; ++It)
	{
		if (!It->IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

TStatId UBuffSpawnCycleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBuffSpawnCycleSubsystem, STATGROUP_Tickables);
}

bool UBuffSpawnCycleSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return IsValid(World) && World->IsGameWorld();
}

void UBuffSpawnCycleSubsystem::UpdateReplicatedBuffItem(AActor* BuffItem)
{
	if (InitializedBuffItems.Contains(BuffItem))
	{
		return;
	}

	const FStructProperty* RowHandleProperty = FindFProperty<FStructProperty>(
		BuffItem->GetClass(), TEXT("Data Table Handle"));
	if (!RowHandleProperty || RowHandleProperty->Struct != FDataTableRowHandle::StaticStruct())
	{
		return;
	}

	const FDataTableRowHandle* RowHandle =
		RowHandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(BuffItem);
	if (!RowHandle || !IsValid(RowHandle->DataTable) || RowHandle->RowName.IsNone())
	{
		// On clients the actor can BeginPlay before its exposed-on-spawn data is
		// replicated. Keep polling until the handle arrives.
		return;
	}

	if (!RowHandle->DataTable->FindRowUnchecked(RowHandle->RowName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Buff item %s received invalid row %s from %s"),
			*GetNameSafe(BuffItem), *RowHandle->RowName.ToString(),
			*GetNameSafe(RowHandle->DataTable));
		return;
	}

	if (UFunction* SetupMeshFunction = BuffItem->FindFunction(TEXT("Setup Mesh")))
	{
		BuffItem->ProcessEvent(SetupMeshFunction, nullptr);
		InitializedBuffItems.Add(BuffItem);
		UE_LOG(LogTemp, Log, TEXT("Initialized replicated buff item %s with %s:%s"),
			*GetNameSafe(BuffItem), *GetNameSafe(RowHandle->DataTable),
			*RowHandle->RowName.ToString());
	}
}

void UBuffSpawnCycleSubsystem::UpdateSpawnPoint(AActor* SpawnPoint)
{
	const FStructProperty* RowHandleProperty = FindFProperty<FStructProperty>(
		SpawnPoint->GetClass(), BuffSpawnCycle::SpawnItemTablePropertyName);
	const FProperty* HavingItemProperty = FindFProperty<FProperty>(
		SpawnPoint->GetClass(), BuffSpawnCycle::HavingItemPropertyName);

	if (!RowHandleProperty
		|| RowHandleProperty->Struct != FDataTableRowHandle::StaticStruct()
		|| !HavingItemProperty)
	{
		return;
	}

	FDataTableRowHandle* RowHandle = RowHandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(SpawnPoint);
	if (!RowHandle || !IsValid(RowHandle->DataTable)
		|| RowHandle->DataTable->GetName() != BuffSpawnCycle::BuffTableName)
	{
		SpawnPointStates.Remove(SpawnPoint);
		return;
	}

	const FName NextRow = GetNextBuffRow(RowHandle->RowName);
	if (NextRow.IsNone())
	{
		// Rows outside the supported ATK/DEF pairs remain under Blueprint control.
		SpawnPointStates.Remove(SpawnPoint);
		return;
	}

	bool bIsHoldingItem = false;
	if (!BuffSpawnCycle::TryGetHoldingItem(SpawnPoint, HavingItemProperty, bIsHoldingItem))
	{
		return;
	}
	FSpawnPointState& State = SpawnPointStates.FindOrAdd(SpawnPoint);
	if (!State.bWasInitialized)
	{
		State.bWasInitialized = true;
		State.bWasHoldingItem = bIsHoldingItem;
		return;
	}

	// The Blueprint clears HavingItem immediately when the pickup disappears,
	// before its respawn timer fires. Updating here makes the next spawn use the
	// opposite buff without replacing the Blueprint's spawn implementation.
	if (State.bWasHoldingItem && !bIsHoldingItem)
	{
		RowHandle->RowName = NextRow;
		SpawnPoint->ForceNetUpdate();

		UE_LOG(LogTemp, Log, TEXT("Buff cycle at %s: next row is %s"),
			*GetNameSafe(SpawnPoint), *NextRow.ToString());
	}

	State.bWasHoldingItem = bIsHoldingItem;
}

FName UBuffSpawnCycleSubsystem::GetNextBuffRow(const FName CurrentRow)
{
	static const FName MultiAtk(TEXT("Buff.Multi.Atk"));
	static const FName MultiDef(TEXT("Buff.Multi.Def"));
	static const FName AddAtk(TEXT("Buff.Add.Atk"));
	static const FName AddDef(TEXT("Buff.Add.Def"));

	if (CurrentRow == MultiAtk)
	{
		return MultiDef;
	}
	if (CurrentRow == MultiDef)
	{
		return MultiAtk;
	}
	if (CurrentRow == AddAtk)
	{
		return AddDef;
	}
	if (CurrentRow == AddDef)
	{
		return AddAtk;
	}

	return NAME_None;
}

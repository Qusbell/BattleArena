#include "Buff/BuffSpawnCycleSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "UObject/UnrealType.h"

namespace BuffSpawnCycle
{
	constexpr float TrackedActorUpdateIntervalSeconds = 0.1f;
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

void UBuffSpawnCycleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
			FOnActorSpawned::FDelegate::CreateUObject(this, &UBuffSpawnCycleSubsystem::RegisterActor));
	}
	LevelAddedHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(
		this, &UBuffSpawnCycleSubsystem::RegisterLevelActors);
	LevelRemovedHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(
		this, &UBuffSpawnCycleSubsystem::UnregisterLevelActors);
}

void UBuffSpawnCycleSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
	{
		World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
	}
	ActorSpawnedHandle.Reset();
	FWorldDelegates::LevelAddedToWorld.Remove(LevelAddedHandle);
	FWorldDelegates::LevelRemovedFromWorld.Remove(LevelRemovedHandle);
	LevelAddedHandle.Reset();
	LevelRemovedHandle.Reset();
	Super::Deinitialize();
}

void UBuffSpawnCycleSubsystem::Tick(const float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	if (bInitialActorRegistrationPending)
	{
		bInitialActorRegistrationPending = false;
		// Level-placed actors predate the spawn delegate, so seed the registry once.
		TRACE_CPUPROFILER_EVENT_SCOPE(BuffSpawnCycleSubsystem_InitialRegistration);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			RegisterActor(*It);
		}
	}

	TimeUntilNextUpdate -= DeltaTime;
	if (TimeUntilNextUpdate > 0.0f)
	{
		return;
	}

	TimeUntilNextUpdate = BuffSpawnCycle::TrackedActorUpdateIntervalSeconds;

	{
		TRACE_CPUPROFILER_EVENT_SCOPE(BuffSpawnCycleSubsystem_TrackedActorUpdate);
		for (auto It = PendingBuffItems.CreateIterator(); It; ++It)
		{
			AActor* BuffItem = It->Get();
			if (!IsValid(BuffItem))
			{
				It.RemoveCurrent();
				continue;
			}

			if (UpdateReplicatedBuffItem(BuffItem))
			{
				It.RemoveCurrent();
			}
		}

		for (auto It = TrackedSpawnPoints.CreateIterator(); It; ++It)
		{
			AActor* SpawnPoint = It->Get();
			if (!IsValid(SpawnPoint))
			{
				SpawnPointStates.Remove(*It);
				It.RemoveCurrent();
				continue;
			}

			if (SpawnPoint->HasAuthority())
			{
				UpdateSpawnPoint(SpawnPoint);
			}
		}
	}

	for (auto It = SpawnPointStates.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void UBuffSpawnCycleSubsystem::RegisterActor(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return;
	}

	const FString ClassName = Actor->GetClass()->GetName();
	if (ClassName.StartsWith(TEXT("BP_BuffItem_C")))
	{
		PendingBuffItems.Add(Actor);
	}
	else if (ClassName.StartsWith(TEXT("BP_ItemSpawnPoint_C")))
	{
		TrackedSpawnPoints.Add(Actor);
	}
}

void UBuffSpawnCycleSubsystem::RegisterLevelActors(ULevel* Level, UWorld* World)
{
	if (!IsValid(Level) || World != GetWorld())
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		RegisterActor(Actor);
	}
}

void UBuffSpawnCycleSubsystem::UnregisterLevelActors(ULevel* Level, UWorld* World)
{
	if (!IsValid(Level) || World != GetWorld())
	{
		return;
	}

	for (AActor* Actor : Level->Actors)
	{
		const TWeakObjectPtr<AActor> ActorKey(Actor);
		PendingBuffItems.Remove(ActorKey);
		TrackedSpawnPoints.Remove(ActorKey);
		SpawnPointStates.Remove(ActorKey);
	}
}

TStatId UBuffSpawnCycleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UBuffSpawnCycleSubsystem, STATGROUP_Tickables);
}

bool UBuffSpawnCycleSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return IsValid(World) && World->IsGameWorld()
		&& (bInitialActorRegistrationPending || !TrackedSpawnPoints.IsEmpty() || !PendingBuffItems.IsEmpty());
}

bool UBuffSpawnCycleSubsystem::UpdateReplicatedBuffItem(AActor* BuffItem)
{
	const FStructProperty* RowHandleProperty = FindFProperty<FStructProperty>(
		BuffItem->GetClass(), TEXT("Data Table Handle"));
	if (!RowHandleProperty || RowHandleProperty->Struct != FDataTableRowHandle::StaticStruct())
	{
		return false;
	}

	const FDataTableRowHandle* RowHandle =
		RowHandleProperty->ContainerPtrToValuePtr<FDataTableRowHandle>(BuffItem);
	if (!RowHandle || !IsValid(RowHandle->DataTable) || RowHandle->RowName.IsNone())
	{
		// On clients the actor can BeginPlay before its exposed-on-spawn data is
		// replicated. Keep polling until the handle arrives.
		return false;
	}

	if (!RowHandle->DataTable->FindRowUnchecked(RowHandle->RowName))
	{
		UE_LOG(LogTemp, Warning, TEXT("Buff item %s received invalid row %s from %s"),
			*GetNameSafe(BuffItem), *RowHandle->RowName.ToString(),
			*GetNameSafe(RowHandle->DataTable));
		return false;
	}

	if (UFunction* SetupMeshFunction = BuffItem->FindFunction(TEXT("Setup Mesh")))
	{
		BuffItem->ProcessEvent(SetupMeshFunction, nullptr);
		UE_LOG(LogTemp, Log, TEXT("Initialized replicated buff item %s with %s:%s"),
			*GetNameSafe(BuffItem), *GetNameSafe(RowHandle->DataTable),
			*RowHandle->RowName.ToString());
		return true;
	}

	return false;
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

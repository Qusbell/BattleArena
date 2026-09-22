#include "Spawner/ItemSpawnSelectionLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

bool UItemSpawnSelectionLibrary::SelectNextSpawnItem(
	const TArray<FDataTableRowHandle>& SpawnItemTable,
	const E_ItemSpawnType SpawnType,
	const int32 CurrentSpawnIndex,
	const int32 SpawnDirection,
	int32& NextSpawnIndex,
	int32& NextSpawnDirection,
	FDataTableRowHandle& CurrentSpawnItem)
{
	CurrentSpawnItem = FDataTableRowHandle();
	NextSpawnIndex = CurrentSpawnIndex;
	NextSpawnDirection = SpawnDirection;

	const int32 ItemCount = SpawnItemTable.Num();
	if (ItemCount == 0)
	{
		NextSpawnIndex = INDEX_NONE;
		NextSpawnDirection = 1;
		return false;
	}

	if (ItemCount == 1)
	{
		NextSpawnIndex = 0;
		NextSpawnDirection = 1;
	}
	else
	{
		switch (SpawnType)
		{
		case E_ItemSpawnType::Sequential:
			NextSpawnIndex = (CurrentSpawnIndex + 1) % ItemCount;
			NextSpawnDirection = 1;
			break;

		case E_ItemSpawnType::PingPong:
			if (!SpawnItemTable.IsValidIndex(CurrentSpawnIndex))
			{
				NextSpawnIndex = 0;
				NextSpawnDirection = 1;
			}
			else
			{
				NextSpawnDirection = SpawnDirection < 0 ? -1 : 1;
				int32 NextIndex = CurrentSpawnIndex + NextSpawnDirection;
				if (NextIndex >= ItemCount)
				{
					NextSpawnDirection = -1;
					NextIndex = ItemCount - 2;
				}
				else if (NextIndex < 0)
				{
					NextSpawnDirection = 1;
					NextIndex = 1;
				}
				NextSpawnIndex = NextIndex;
			}
			break;

		case E_ItemSpawnType::Random:
			NextSpawnIndex = FMath::RandRange(0, ItemCount - 1);
			NextSpawnDirection = 1;
			break;
		}
	}

	if (!SpawnItemTable.IsValidIndex(NextSpawnIndex))
	{
		NextSpawnIndex = INDEX_NONE;
		return false;
	}

	CurrentSpawnItem = SpawnItemTable[NextSpawnIndex];
	return IsValid(CurrentSpawnItem.DataTable)
		&& !CurrentSpawnItem.RowName.IsNone()
		&& CurrentSpawnItem.DataTable->FindRowUnchecked(CurrentSpawnItem.RowName) != nullptr;
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemSpawnSelectionTest,
	"ShootingArena.Spawner.SelectionModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemSpawnSelectionTest::RunTest(const FString& Parameters)
{
	UDataTable* Table = NewObject<UDataTable>();
	Table->RowStruct = FTableRowBase::StaticStruct();
	FTableRowBase EmptyRow;
	for (const FName Name : {FName(TEXT("Item1")), FName(TEXT("Item2")), FName(TEXT("Item3"))}) Table->AddRow(Name, EmptyRow);
	TArray<FDataTableRowHandle> Items;
	for (const FName Name : {FName(TEXT("Item1")), FName(TEXT("Item2")), FName(TEXT("Item3"))})
	{
		FDataTableRowHandle Handle;
		Handle.DataTable = Table;
		Handle.RowName = Name;
		Items.Add(Handle);
	}

	auto Run = [&](E_ItemSpawnType Type, int32 Count)
	{
		TArray<FName> Result;
		int32 Index = INDEX_NONE;
		int32 Direction = 1;
		for (int32 Iteration = 0; Iteration < Count; ++Iteration)
		{
			int32 NextIndex = INDEX_NONE;
			int32 NextDirection = 1;
			FDataTableRowHandle Item;
			TestTrue(TEXT("Selection succeeds"), UItemSpawnSelectionLibrary::SelectNextSpawnItem(Items, Type, Index, Direction, NextIndex, NextDirection, Item));
			Index = NextIndex;
			Direction = NextDirection;
			Result.Add(Item.RowName);
		}
		return Result;
	};

	TestEqual(TEXT("Sequential"), Run(E_ItemSpawnType::Sequential, 6),
		TArray<FName>({TEXT("Item1"), TEXT("Item2"), TEXT("Item3"), TEXT("Item1"), TEXT("Item2"), TEXT("Item3")}));
	TestEqual(TEXT("PingPong"), Run(E_ItemSpawnType::PingPong, 6),
		TArray<FName>({TEXT("Item1"), TEXT("Item2"), TEXT("Item3"), TEXT("Item2"), TEXT("Item1"), TEXT("Item2")}));

	int32 Index = INDEX_NONE;
	int32 Direction = 1;
	for (int32 Iteration = 0; Iteration < 100; ++Iteration)
	{
		int32 NextIndex = INDEX_NONE;
		int32 NextDirection = 1;
		FDataTableRowHandle Item;
		TestTrue(TEXT("Random succeeds"), UItemSpawnSelectionLibrary::SelectNextSpawnItem(Items, E_ItemSpawnType::Random, Index, Direction, NextIndex, NextDirection, Item));
		TestTrue(TEXT("Random remains in range"), Items.IsValidIndex(NextIndex));
		Index = NextIndex;
		Direction = NextDirection;
	}

	TArray<FDataTableRowHandle> Empty;
	FDataTableRowHandle Item;
	int32 NextIndex = 99;
	int32 NextDirection = 99;
	TestFalse(TEXT("Empty array safely stops"), UItemSpawnSelectionLibrary::SelectNextSpawnItem(Empty, E_ItemSpawnType::Sequential, INDEX_NONE, 1, NextIndex, NextDirection, Item));
	TestEqual(TEXT("Empty index reset"), NextIndex, INDEX_NONE);

	TArray<FDataTableRowHandle> One = {Items[0]};
	Index = INDEX_NONE;
	for (int32 Iteration = 0; Iteration < 3; ++Iteration)
	{
		TestTrue(TEXT("Single item repeats"), UItemSpawnSelectionLibrary::SelectNextSpawnItem(One, E_ItemSpawnType::PingPong, Index, Direction, NextIndex, NextDirection, Item));
		TestEqual(TEXT("Single item index"), NextIndex, 0);
		Index = NextIndex;
		Direction = NextDirection;
	}
	return true;
}
#endif

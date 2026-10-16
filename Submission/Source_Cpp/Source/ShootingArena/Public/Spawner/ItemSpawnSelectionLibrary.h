#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemSpawnSelectionLibrary.generated.h"

UENUM(BlueprintType, meta = (ToolTip = "스포너에 등록된 아이템 배열에서 다음 생성 아이템을 선택하는 방식입니다. 실제 선택과 생성은 서버에서 처리됩니다."))
enum class E_ItemSpawnType : uint8
{
	Sequential UMETA(DisplayName = "Sequential", ToolTip = "배열 순서대로 반복 생성합니다. 예: 1 → 2 → 3 → 1 → 2 → 3"),

	PingPong UMETA(DisplayName = "PingPong", ToolTip = "배열의 양 끝에서 방향을 반전하며 왕복 생성합니다. 예: 1 → 2 → 3 → 2 → 1 → 2"),

	Random UMETA(DisplayName = "Random", ToolTip = "생성할 때마다 배열에서 무작위로 선택합니다. 같은 아이템이 연속으로 선택될 수 있습니다.")
};

UCLASS()
class SHOOTINGARENA_API UItemSpawnSelectionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Chooses the row for one server-owned spawn cycle and advances the shared
	 * sequence state. CurrentSpawnIndex must start at -1 and SpawnDirection at 1.
	 */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	static bool SelectNextSpawnItem(
		const TArray<FDataTableRowHandle>& SpawnItemTable,
		E_ItemSpawnType SpawnType,
		int32 CurrentSpawnIndex,
		int32 SpawnDirection,
		int32& NextSpawnIndex,
		int32& NextSpawnDirection,
		FDataTableRowHandle& CurrentSpawnItem);

	UFUNCTION(BlueprintPure, Category = "Spawner")
	static float GetSafeSpawnDelay(float Delay)
	{
		return FMath::Max(Delay, 0.001f);
	}
};

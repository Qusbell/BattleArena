#include "UI/MapResultImageLibrary.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* const MapDataTablePath = TEXT("/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MapData.DT_MapData");

	bool IsRowForWorld(const UStruct* RowStruct, const uint8* Row, const FString& CurrentMap)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().StartsWith(TEXT("Map_Asset")))
			{
				continue;
			}
			// Map_Asset 은 FSoftWorldReference(구조체) 이므로 안쪽 소프트 오브젝트를 꺼냅니다.
			if (const FStructProperty* Wrapper = CastField<FStructProperty>(*It))
			{
				const void* WrapperValue = Wrapper->ContainerPtrToValuePtr<void>(Row);
				for (TFieldIterator<FSoftObjectProperty> Inner(Wrapper->Struct); Inner; ++Inner)
				{
					const FSoftObjectPtr* Ptr = Inner->ContainerPtrToValuePtr<FSoftObjectPtr>(WrapperValue);
					return Ptr && Ptr->ToSoftObjectPath().GetLongPackageName() == CurrentMap;
				}
			}
			else if (const FSoftObjectProperty* Asset = CastField<FSoftObjectProperty>(*It))
			{
				const FSoftObjectPtr* Ptr = Asset->ContainerPtrToValuePtr<FSoftObjectPtr>(Row);
				return Ptr && Ptr->ToSoftObjectPath().GetLongPackageName() == CurrentMap;
			}
			return false;
		}
		return false;
	}

	UTexture2D* ReadResultImage(const UStruct* RowStruct, const uint8* Row)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (!It->GetName().StartsWith(TEXT("Result_Image")))
			{
				continue;
			}
			if (const FObjectPropertyBase* Object = CastField<FObjectPropertyBase>(*It))
			{
				// 하드 레퍼런스/소프트 레퍼런스 모두 로드해서 반환합니다.
				if (const FSoftObjectProperty* Soft = CastField<FSoftObjectProperty>(*It))
				{
					return Cast<UTexture2D>(Soft->ContainerPtrToValuePtr<FSoftObjectPtr>(Row)->LoadSynchronous());
				}
				return Cast<UTexture2D>(Object->GetObjectPropertyValue_InContainer(Row));
			}
		}
		return nullptr;
	}
}

UTexture2D* UMapResultImageLibrary::GetCurrentMapResultImage(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	const UDataTable* Table = LoadObject<UDataTable>(nullptr, MapDataTablePath);
	if (!Table || !Table->GetRowStruct())
	{
		return nullptr;
	}
	const UStruct* RowStruct = Table->GetRowStruct();

	FName SelectedRow = NAME_None;
	if (const UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (const FNameProperty* MapIdProperty = CastField<FNameProperty>(GameInstance->GetClass()->FindPropertyByName(TEXT("Selected_Map_ID"))))
		{
			SelectedRow = MapIdProperty->GetPropertyValue_InContainer(GameInstance);
		}
	}

	const FString CurrentMap = UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());
	const uint8* FoundRow = nullptr;
	for (const TPair<FName, uint8*>& Pair : Table->GetRowMap())
	{
		if (IsRowForWorld(RowStruct, Pair.Value, CurrentMap) && (!FoundRow || Pair.Key == SelectedRow))
		{
			FoundRow = Pair.Value;
		}
	}
	if (!FoundRow && !SelectedRow.IsNone())
	{
		FoundRow = Table->GetRowMap().FindRef(SelectedRow);
	}
	return FoundRow ? ReadResultImage(RowStruct, FoundRow) : nullptr;
}

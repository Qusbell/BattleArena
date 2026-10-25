#include "MultiRulesLibrary.h"
#include "Engine/DataTable.h"
#include "UObject/UnrealType.h"

const FName UMultiRulesLibrary::TimeLimitRowName(TEXT("TimeLimit"));
const FName UMultiRulesLibrary::KillTargetRowName(TEXT("KillTarget"));

namespace
{
	const TCHAR* const MultiRulesTablePath = TEXT("/Game/QuakeLike_1_0/GameFlow/Table/DT/DT_MultiRules.DT_MultiRules");

	// 블루프린트 구조체의 프로퍼티 이름은 "Options_3_<GUID>" 처럼 뒤에 접미사가 붙으므로 저작 이름으로 찾습니다.
	const FProperty* FindRowProperty(const UScriptStruct* RowStruct, const TCHAR* AuthoredName)
	{
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			if (It->GetAuthoredName().Equals(AuthoredName, ESearchCase::IgnoreCase))
			{
				return *It;
			}
		}
		return nullptr;
	}

	bool ReadInt(const FProperty* Property, const void* ValuePtr, int32& OutValue)
	{
		if (const FNumericProperty* Numeric = CastField<FNumericProperty>(Property))
		{
			OutValue = static_cast<int32>(Numeric->GetSignedIntPropertyValue(ValuePtr));
			return true;
		}
		return false;
	}

	const uint8* FindRow(FName RuleRowName, const UScriptStruct*& OutStruct)
	{
		static TWeakObjectPtr<UDataTable> CachedTable;
		if (!CachedTable.IsValid())
		{
			CachedTable = LoadObject<UDataTable>(nullptr, MultiRulesTablePath);
		}
		UDataTable* Table = CachedTable.Get();
		if (!Table)
		{
			return nullptr;
		}
		OutStruct = Table->GetRowStruct();
		return Table->FindRowUnchecked(RuleRowName);
	}

	FText ReadText(FName RuleRowName, const TCHAR* FieldName)
	{
		const UScriptStruct* RowStruct = nullptr;
		const uint8* Row = FindRow(RuleRowName, RowStruct);
		if (!Row || !RowStruct)
		{
			return FText::GetEmpty();
		}
		const FProperty* Property = FindRowProperty(RowStruct, FieldName);
		if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			return TextProperty->GetPropertyValue(TextProperty->ContainerPtrToValuePtr<void>(Row));
		}
		if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
		{
			return FText::FromString(StrProperty->GetPropertyValue(StrProperty->ContainerPtrToValuePtr<void>(Row)));
		}
		if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
		{
			return FText::FromName(NameProperty->GetPropertyValue(NameProperty->ContainerPtrToValuePtr<void>(Row)));
		}
		return FText::GetEmpty();
	}
}

TArray<int32> UMultiRulesLibrary::GetRuleOptions(FName RuleRowName)
{
	TArray<int32> Result;
	const UScriptStruct* RowStruct = nullptr;
	const uint8* Row = FindRow(RuleRowName, RowStruct);
	if (!Row || !RowStruct)
	{
		return Result;
	}

	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(FindRowProperty(RowStruct, TEXT("Options")));
	if (!ArrayProperty)
	{
		return Result;
	}

	FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Row));
	for (int32 i = 0; i < Helper.Num(); ++i)
	{
		int32 Value = 0;
		if (ReadInt(ArrayProperty->Inner, Helper.GetRawPtr(i), Value))
		{
			Result.Add(Value);
		}
	}
	return Result;
}

int32 UMultiRulesLibrary::GetRuleOptionCount(FName RuleRowName)
{
	return GetRuleOptions(RuleRowName).Num();
}

int32 UMultiRulesLibrary::GetRuleDefaultIndex(FName RuleRowName)
{
	const UScriptStruct* RowStruct = nullptr;
	const uint8* Row = FindRow(RuleRowName, RowStruct);
	if (!Row || !RowStruct)
	{
		return 0;
	}

	int32 Index = 0;
	if (const FProperty* Property = FindRowProperty(RowStruct, TEXT("DefaultIndex")))
	{
		ReadInt(Property, Property->ContainerPtrToValuePtr<void>(Row), Index);
	}
	return FMath::Clamp(Index, 0, FMath::Max(0, GetRuleOptionCount(RuleRowName) - 1));
}

int32 UMultiRulesLibrary::GetRuleValue(FName RuleRowName, int32 Index)
{
	const TArray<int32> Options = GetRuleOptions(RuleRowName);
	if (Options.IsValidIndex(Index))
	{
		return Options[Index];
	}
	const int32 DefaultIndex = GetRuleDefaultIndex(RuleRowName);
	return Options.IsValidIndex(DefaultIndex) ? Options[DefaultIndex] : 0;
}

int32 UMultiRulesLibrary::GetRuleDefaultValue(FName RuleRowName)
{
	return GetRuleValue(RuleRowName, GetRuleDefaultIndex(RuleRowName));
}

FText UMultiRulesLibrary::GetRuleLabel(FName RuleRowName)
{
	return ReadText(RuleRowName, TEXT("Label"));
}

FText UMultiRulesLibrary::GetRuleUnit(FName RuleRowName)
{
	return ReadText(RuleRowName, TEXT("Unit"));
}

int32 UMultiRulesLibrary::FindRuleOptionIndex(FName RuleRowName, int32 Value)
{
	return GetRuleOptions(RuleRowName).IndexOfByKey(Value);
}

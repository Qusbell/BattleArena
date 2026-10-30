#include "JsonPropertyNameNormalizer.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "UObject/UnrealType.h"

namespace
{
	FString ComparisonKey(const FString& name)
	{
		FString key;
		for (TCHAR c : name)
		{
			if (!FChar::IsWhitespace(c))
			{
				key.AppendChar(FChar::ToLower(c));
			}
		}
		return key;
	}

	bool NormalizeValue(const TSharedPtr<FJsonValue>& value, const FProperty* property,
		bool converterNames, TSharedPtr<FJsonValue>& output, FString& error)
	{
		output = value;
		if (!value.IsValid()) return true;
		if (const FStructProperty* structure = CastField<FStructProperty>(property))
		{
			if (value->Type == EJson::Object)
			{
				TSharedPtr<FJsonObject> object;
				if (!FJsonPropertyNameNormalizer::NormalizeObject(value->AsObject(), structure->Struct,
					converterNames, object, error)) return false;
				output = MakeShared<FJsonValueObject>(object);
			}
		}
		else if (const FMapProperty* map = CastField<FMapProperty>(property))
		{
			if (value->Type == EJson::Object)
			{
				TSharedPtr<FJsonObject> object = MakeShared<FJsonObject>();
				for (const auto& entry : value->AsObject()->Values)
				{
					TSharedPtr<FJsonValue> child;
					if (!NormalizeValue(entry.Value, map->ValueProp, converterNames, child, error)) return false;
					object->SetField(entry.Key, child); // Map keys are user data, never property aliases.
				}
				output = MakeShared<FJsonValueObject>(object);
			}
		}
		else
		{
			const FArrayProperty* array = CastField<FArrayProperty>(property);
			const FSetProperty* set = CastField<FSetProperty>(property);
			const FProperty* element = array ? array->Inner : (set ? set->ElementProp : nullptr);
			if (element && value->Type == EJson::Array)
			{
				TArray<TSharedPtr<FJsonValue>> values;
				for (const auto& item : value->AsArray())
				{
					TSharedPtr<FJsonValue> child;
					if (!NormalizeValue(item, element, converterNames, child, error)) return false;
					values.Add(child);
				}
				output = MakeShared<FJsonValueArray>(values);
			}
		}
		return true;
	}
}

bool FJsonPropertyNameNormalizer::NormalizeObject(const TSharedPtr<FJsonObject>& source,
	const UStruct* type, bool converterNames, TSharedPtr<FJsonObject>& output, FString& error)
{
	if (!source.IsValid() || !type)
	{
		error = TEXT("JSON property normalization requires an object and a reflected type.");
		return false;
	}
	output = MakeShared<FJsonObject>();
	TSet<const FProperty*> matchedProperties;
	for (const auto& entry : source->Values)
	{
		const FProperty* match = nullptr;
		for (TFieldIterator<FProperty> it(type); it; ++it)
		{
			if (entry.Key == it->GetName() || entry.Key == it->GetAuthoredName())
			{
				match = *it;
				break;
			}
		}
		if (!match)
		{
			const FString key = ComparisonKey(entry.Key);
			for (TFieldIterator<FProperty> it(type); it; ++it)
			{
				if (key == ComparisonKey(it->GetName()) || key == ComparisonKey(it->GetAuthoredName()))
				{
					if (match)
					{
						error = FString::Printf(TEXT("Ambiguous JSON field '%s' in %s."), *entry.Key, *type->GetPathName());
						return false;
					}
					match = *it;
				}
			}
		}
		if (!match)
		{
			if (output->HasField(entry.Key))
			{
				error = FString::Printf(TEXT("JSON field collision at '%s'."), *entry.Key);
				return false;
			}
			output->SetField(entry.Key, entry.Value); // Let strict validation report unknown fields.
			continue;
		}
		if (matchedProperties.Contains(match))
		{
			error = FString::Printf(TEXT("Multiple JSON fields address property '%s' in %s."), *match->GetName(), *type->GetPathName());
			return false;
		}
		matchedProperties.Add(match);
		TSharedPtr<FJsonValue> value;
		if (!NormalizeValue(entry.Value, match, converterNames, value, error)) return false;
		const FString fieldName = converterNames
			? type->GetAuthoredNameForField(match) : match->GetName();
		if (output->HasField(fieldName))
		{
			error = FString::Printf(TEXT("JSON field collision at '%s'."), *fieldName);
			return false;
		}
		output->SetField(fieldName, value);
	}
	return true;
}

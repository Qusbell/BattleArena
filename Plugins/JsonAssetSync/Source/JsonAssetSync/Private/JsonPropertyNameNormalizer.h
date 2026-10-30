#pragma once

#include "CoreMinimal.h"

class FJsonObject;

/** Match authored names against cooked reflection without discarding unknown/ambiguous fields. */
class FJsonPropertyNameNormalizer
{
public:
	static bool NormalizeObject(const TSharedPtr<FJsonObject>& source, const UStruct* type,
		bool converterNames, TSharedPtr<FJsonObject>& output, FString& error);
};

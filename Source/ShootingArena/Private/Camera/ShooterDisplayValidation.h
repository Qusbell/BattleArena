#pragma once

#include "CoreMinimal.h"

namespace ShooterDisplayValidation
{
	inline float FiniteClamp(float Value, float Min, float Max, float Fallback)
	{
		return FMath::IsFinite(Value) ? FMath::Clamp(Value, Min, Max) : Fallback;
	}

	inline FLinearColor FiniteColor(const FLinearColor& Value, const FLinearColor& Fallback)
	{
		if (!FMath::IsFinite(Value.R) || !FMath::IsFinite(Value.G)
			|| !FMath::IsFinite(Value.B) || !FMath::IsFinite(Value.A)) return Fallback;
		FLinearColor Result = Value;
		Result.A = FMath::Clamp(Result.A, 0.0f, 1.0f);
		return Result;
	}
}

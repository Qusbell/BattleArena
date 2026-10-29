#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "CharacterPoolSettings.generated.h"

/** Each participant owns one pawn; the population determines the allocation count. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Character Pool"))
class SHOOTINGARENA_API UCharacterPoolSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }
	UPROPERTY(Config, EditAnywhere, Category="Lifecycle")
	bool bEnabled = true;
};

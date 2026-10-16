#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InitialWeaponLibrary.generated.h"


/** Shared default-weapon check for the firing logic. */
UCLASS()
class SHOOTINGARENA_API UInitialWeaponLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Initial Weapons", meta=(DefaultToSelf="Weapon"))
	static bool IsDefaultWeapon(const AActor* Weapon);

};

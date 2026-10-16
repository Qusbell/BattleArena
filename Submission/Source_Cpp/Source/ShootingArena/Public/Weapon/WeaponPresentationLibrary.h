#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponPresentationLibrary.generated.h"

class UActorComponent;

/** Local presentation repairs; network selection and firing continue through the existing BP paths. */
UCLASS()
class SHOOTINGARENA_API UWeaponPresentationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Replays the existing equip handler only if Standalone selection and equip state disagree. */
	UFUNCTION(BlueprintCallable, Category="Weapon|Presentation", meta=(DefaultToSelf="Inventory"))
	static void ReconcileStandaloneEquip(UActorComponent* Inventory);
};

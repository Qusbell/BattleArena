#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RespawnCountdownLibrary.generated.h"

class UUserWidget;

UCLASS()
class SHOOTINGARENA_API URespawnCountdownLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Initializes the death screen's existing time fields and timer; its Blueprint events still update the display. */
	UFUNCTION(BlueprintCallable, Category="UI|Respawn", meta=(DefaultToSelf="Widget"))
	static bool StartRespawnCountdown(UUserWidget* Widget, double RespawnDuration);
};

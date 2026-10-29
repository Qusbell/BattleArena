#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CharacterPoolSubsystem.generated.h"

class APawn;
class AController;
class APlayerController;

USTRUCT()
struct FOwnedCharacterPawn
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<APawn> Pawn = nullptr;
	UPROPERTY() TWeakObjectPtr<AController> Controller;
	UPROPERTY() FString ReconnectToken;
	FTimerHandle Expiry;
	bool bClaiming = false;
};

USTRUCT(BlueprintType)
struct FCharacterPoolStats
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Created = 0;
	UPROPERTY(BlueprintReadOnly) int32 Reused = 0;
	UPROPERTY(BlueprintReadOnly) int32 Reconnected = 0;
	UPROPERTY(BlueprintReadOnly) int32 OwnedPawns = 0;
	UPROPERTY(BlueprintReadOnly) int32 ReservedPawns = 0;
};

/** Server registry of participant-owned pawns and bounded reconnection reservations. */
UCLASS()
class SHOOTINGARENA_API UCharacterPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	static bool Supports(const UClass* PawnClass);
	APawn* Acquire(AController* Controller, TSubclassOf<APawn> PawnClass, const FTransform& Transform);
	void BindController(APawn* Pawn, AController* Controller);
	void Reserve(APlayerController* Controller, float Seconds);
	APawn* Claim(const FString& Token);
	void ForgetPawn(APawn* Pawn);
	APawn* FindOwnedPawn(AController* Controller) const;
	bool IsReserved(APawn* Pawn) const;
	UFUNCTION(BlueprintPure, Category="Character Pool") FCharacterPoolStats GetPoolStats() const;
	virtual void Deinitialize() override;
protected:
	virtual bool DoesSupportWorldType(EWorldType::Type Type) const override;
private:
	UPROPERTY(Transient) TArray<FOwnedCharacterPawn> Entries;
	int32 Created = 0, Reused = 0, Reconnected = 0;
	void DestroyOwnedPawn(APawn* Pawn);
	UFUNCTION() void OnControllerDestroyed(AActor* Actor);
	UFUNCTION() void OnClaimedControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);
};

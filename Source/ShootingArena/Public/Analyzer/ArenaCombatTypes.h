#pragma once

#include "CoreMinimal.h"
#include "ArenaCombatTypes.generated.h"

USTRUCT(BlueprintType)
struct FArenaParticipant
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 ControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) bool bPlayer = false;
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) FString PlayerId;
};

/** Immutable firing identity. IDs are session-local; TrackId is local to ControllerId. */
USTRUCT(BlueprintType)
struct FArenaShotSample
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) int32 ShotId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) double TimeSeconds = 0;
    UPROPERTY(BlueprintReadOnly) int32 ControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 TrackId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FString WeaponId;
    UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector Direction = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FArenaHitSample
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) double TimeSeconds = 0;
    UPROPERTY(BlueprintReadOnly) int32 ShotId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 TargetControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 TargetTrackId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FString TargetName;
    UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector Normal = FVector::ZeroVector;
    // Explosion overlap contacts are distinct from trace/impact contacts.
    UPROPERTY(BlueprintReadOnly) bool bDamageContact = false;
};

USTRUCT(BlueprintType)
struct FArenaDeathSample
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) double TimeSeconds = 0;
    UPROPERTY(BlueprintReadOnly) int32 VictimControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 VictimTrackId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 KillerControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) int32 KillerTrackId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FVector VictimLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector KillerLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) bool bHasKillerLocation = false;
    UPROPERTY(BlueprintReadOnly) FString Cause = TEXT("Unknown");
    UPROPERTY(BlueprintReadOnly) int32 ShotId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FString WeaponId;
    UPROPERTY(BlueprintReadOnly) int32 CreditedControllerId = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly) FString CreditSource;
};

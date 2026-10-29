#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ArenaMatchLogLibrary.generated.h"

class AActor;
class APawn;
class UActorComponent;

/**
 * Blueprint entry points for the match log events that are decided in Blueprint graphs
 * (weapon equip, item spawn / pickup / removal, buff changes). All calls are no-ops outside the
 * authoritative match world, so they can be placed on shared paths without a HasAuthority branch.
 */
UCLASS()
class SHOOTINGARENA_API UArenaMatchLogLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Call after the new weapon is actually equipped. Reason: Manual, AutoOnPickup, AutoOnEmpty. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordWeaponEquip(APawn* Pawn, FName WeaponBefore, FName WeaponAfter, FName Reason);

    /** Call when an item actor is created. Category: HeldItem, Ammo, Life, Buff. Reason: Spawner, DeathDrop. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static int32 RecordItemSpawn(AActor* Item, FName Category, FName ItemRowName, FName Reason);

    /** Call after the pickup effect is decided but before health/armor change. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordItemPickup(APawn* Pawn, AActor* Item, bool bHasAmounts, float ItemAmount, float AppliedAmount);

    /** Reason: PickedUp, Despawned (MatchEnd is written automatically). */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordItemRemove(AActor* Item, FName Reason);

    /** ChangeType: Gain, Refresh, Replace, Expire. BuffAfter None clears. ExpiresAtSeconds is in MatchElapsedSeconds. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordBuffChange(APawn* Pawn, FName BuffAfter, FName ChangeType, bool bHasExpiry, float ExpiresAtSeconds);

    // ---- One-node hooks used by the item / inventory / buff Blueprints ----

    /** BP_DropItem BeginPlay: derives category, data row and spawn reason from the item itself. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordDropItemSpawned(AActor* Item);

    /** BP_DropItem overlap, right BEFORE ApplyItemEffect: captures the picker's health/armor. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void BeginItemPickup(AActor* Item, AActor* Picker);

    /** BP_DropItem overlap, after ApplyItemEffect succeeded: writes the pickup (amounts derived). */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void CommitItemPickup(AActor* Item, AActor* Picker);

    /** BPC_Inventory Swap event: records the equip change from the old/new held item actors. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordWeaponSwap(AActor* Owner, AActor* BeforeItem, AActor* AfterItem);

    /** BPC_Buff after a buff was applied/refreshed (reads tag and end time from the component). */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordBuffApplied(UActorComponent* BuffComponent);

    /** BPC_Buff when the buff timer finished. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordBuffExpired(UActorComponent* BuffComponent);

    /** BPC_Life OwnerTakeDamage: the damage amount (Final Damage) right before health and armor are reduced. */
    UFUNCTION(BlueprintCallable, Category = "Arena Match Log")
    static void RecordDamageInput(AActor* Victim, float InputDamage);

    /** Seconds since the match start used by every event, for computing ExpiresAtSeconds. */
    UFUNCTION(BlueprintPure, Category = "Arena Match Log", meta = (WorldContext = "WorldContextObject"))
    static float GetMatchElapsedSeconds(const UObject* WorldContextObject);
};

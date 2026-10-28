#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArenaCombatLibrary.generated.h"

UCLASS()
class SHOOTINGARENA_API UArenaCombatLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void BeginWeaponShot(AActor* Weapon, UObject* WeaponData);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void BeginProjectileDamage(AActor* Projectile);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void EndProjectileDamage(AActor* Projectile);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static float ApplyAnalyzedDamage(AActor* DamagedActor, float BaseDamage, AController* EventInstigator,
        AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass, AActor* FiringWeapon = nullptr);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void ConfirmDeath(AActor* Victim, AController* InstigatedBy, AActor* DamageCauser);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void RecordTrace(AActor* Weapon, const FHitResult& Hit, FVector AimStart = FVector::ZeroVector, FVector AimEnd = FVector::ZeroVector);
    UFUNCTION(BlueprintCallable, Category="Arena Analyze")
    static void RecordTraces(AActor* Weapon, const TArray<FHitResult>& Hits, FVector AimStart = FVector::ZeroVector, FVector AimEnd = FVector::ZeroVector);
};

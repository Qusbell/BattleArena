#include "Analyzer/ArenaCombatLibrary.h"
#include "Analyzer/ArenaAnalyzeSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"

namespace
{
UArenaAnalyzeSubsystem* Recorder(const AActor* Actor)
{
    return IsValid(Actor) && Actor->HasAuthority() && Actor->GetWorld()
        ? Actor->GetWorld()->GetSubsystem<UArenaAnalyzeSubsystem>() : nullptr;
}
}
void UArenaCombatLibrary::BeginWeaponShot(AActor* Weapon, UObject* Data)
{
    if (auto* S = Recorder(Weapon))
    {
        APawn* Pawn = Cast<APawn>(Weapon->GetOwner());
        if (!Pawn) Pawn = Weapon->GetInstigator();
        FVector Location = Weapon->GetActorLocation(); FRotator Rotation = Weapon->GetActorRotation();
        if (Pawn) Pawn->GetActorEyesViewPoint(Location, Rotation);
        S->RecordShot(Weapon, Data, Location, Rotation.Vector());
    }
}
void UArenaCombatLibrary::BeginProjectileDamage(AActor* Projectile)
{ if (auto* S = Recorder(Projectile)) S->PushProjectile(Projectile); }
void UArenaCombatLibrary::EndProjectileDamage(AActor* Projectile)
{ if (Projectile && Projectile->GetWorld()) if (auto* S = Projectile->GetWorld()->GetSubsystem<UArenaAnalyzeSubsystem>()) S->PopProjectile(); }
float UArenaCombatLibrary::ApplyAnalyzedDamage(AActor* Victim, float Damage, AController* Instigator,
    AActor* Causer, TSubclassOf<UDamageType> Type, AActor* FiringWeapon)
{
    if (auto* S = Recorder(Victim)) return S->ApplyShotDamage(Victim, Damage, Instigator, Causer, Type, FiringWeapon);
    return UGameplayStatics::ApplyDamage(Victim, Damage, Instigator, Causer, Type);
}
void UArenaCombatLibrary::ConfirmDeath(AActor* Victim, AController* Instigator, AActor* Causer)
{ if (auto* S = Recorder(Victim)) S->RecordDeath(Cast<APawn>(Victim), Instigator, Causer); }
void UArenaCombatLibrary::RecordTrace(AActor* Weapon, const FHitResult& Hit, FVector Start, FVector End)
{ if (auto* S = Recorder(Weapon)) { S->UpdateShotAim(Weapon, Start, End); S->RecordHit(Weapon, Hit); } }
void UArenaCombatLibrary::RecordTraces(AActor* Weapon, const TArray<FHitResult>& Hits, FVector Start, FVector End)
{ if (auto* S = Recorder(Weapon)) { S->UpdateShotAim(Weapon, Start, End); for (const auto& Hit : Hits) S->RecordHit(Weapon, Hit); } }

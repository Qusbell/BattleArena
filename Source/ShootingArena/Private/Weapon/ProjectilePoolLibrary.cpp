#include "Weapon/ProjectilePoolLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Weapon/ProjectilePoolComponent.h"
#include "Weapon/ProjectilePoolSubsystem.h"

AActor* UProjectilePoolLibrary::SpawnPooledProjectile(const UObject* WorldContextObject,
	TSubclassOf<AActor> ProjectileClass, FTransform SpawnTransform, AActor* Weapon, UObject* WeaponData,
	APawn* Instigator, bool bCanDealDamage, int32 ProjectileId)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	UProjectilePoolSubsystem* Pool = World ? World->GetSubsystem<UProjectilePoolSubsystem>() : nullptr;
	return Pool ? Pool->Acquire(ProjectileClass, SpawnTransform, Weapon, WeaponData, Instigator, bCanDealDamage, ProjectileId) : nullptr;
}

void UProjectilePoolLibrary::ReleaseProjectile(AActor* Projectile)
{
	if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed()) return;
	if (auto* Component = Projectile->FindComponentByClass<UProjectilePoolComponent>()) Component->Release();
	else Projectile->Destroy();
}

bool UProjectilePoolLibrary::IsPooledProjectile(AActor* Projectile)
{
	return IsValid(Projectile) && Projectile->FindComponentByClass<UProjectilePoolComponent>() != nullptr;
}

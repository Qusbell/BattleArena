#include "Weapon/ProjectileImpactEffectLibrary.h"
#include "Weapon/ProjectileWorldCollisionComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "UObject/UnrealType.h"

namespace ProjectileImpactEffectHelpers
{
template<class T> T* ObjectField(const UObject* Config, FName Name)
{
	if (!IsValid(Config)) return nullptr;
	const auto* Property = FindFProperty<FObjectPropertyBase>(Config->GetClass(), Name);
	UObject* Value = Property ? Property->GetObjectPropertyValue_InContainer(Config) : nullptr;
	return IsValid(Value) ? Cast<T>(Value) : nullptr;
}
}

bool UProjectileImpactEffectLibrary::GetProjectileImpactWasCharacter(const UProjectileWorldCollisionComponent* Collision)
{
	return IsValid(Collision) && !Collision->IsBeingDestroyed() && Collision->bLastImpactWasCharacter;
}

bool UProjectileImpactEffectLibrary::CanPlayProjectileImpactEffects(const UObject* WorldContextObject, const UObject* WeaponConfig)
{
	if (!IsValid(WorldContextObject) || !IsValid(WeaponConfig) || !GEngine) return false;
	if (const auto* Component = Cast<UActorComponent>(WorldContextObject))
	{
		const AActor* Owner = Component->GetOwner();
		if (Component->IsBeingDestroyed() || !IsValid(Owner) || Owner->IsActorBeingDestroyed()) return false;
	}
	if (const auto* Actor = Cast<AActor>(WorldContextObject); Actor && Actor->IsActorBeingDestroyed()) return false;
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	return IsValid(World) && World->IsGameWorld() && !World->bIsTearingDown && World->GetNetMode() != NM_DedicatedServer;
}

void UProjectileImpactEffectLibrary::SelectProjectileImpactEffects(UObject* WeaponConfig, bool bHitCharacter,
	UParticleSystem*& ParticleEffect, UNiagaraSystem*& NiagaraEffect)
{
	ParticleEffect = nullptr;
	NiagaraEffect = nullptr;
	if (!IsValid(WeaponConfig)) return;
	const auto* Separate = FindFProperty<FBoolProperty>(WeaponConfig->GetClass(), TEXT("useSeparatePlayerProjectileDestroyEffects"));
	const bool bPlayerOverride = bHitCharacter && Separate && Separate->GetPropertyValue_InContainer(WeaponConfig);
	ParticleEffect = ProjectileImpactEffectHelpers::ObjectField<UParticleSystem>(WeaponConfig,
		bPlayerOverride ? TEXT("projectilePlayerDestroyEffect") : TEXT("projectileDestroyEffect"));
	NiagaraEffect = ProjectileImpactEffectHelpers::ObjectField<UNiagaraSystem>(WeaponConfig,
		bPlayerOverride ? TEXT("NSProjectilePlayerDestroyEffect") : TEXT("NSProjectileDestroyEffect"));
	if (!IsValid(ParticleEffect)) ParticleEffect = nullptr;
	if (!IsValid(NiagaraEffect)) NiagaraEffect = nullptr;
}

#include "Weapon/ProjectileMuzzleFlashLibrary.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "UObject/UnrealType.h"

namespace
{
bool Flag(const UObject* Object, FName Name)
{
	const FBoolProperty* Property = IsValid(Object) ? FindFProperty<FBoolProperty>(Object->GetClass(), Name) : nullptr;
	return Property && Property->GetPropertyValue_InContainer(Object);
}

UObject* ObjectField(const UObject* Object, FName Name)
{
	const FObjectPropertyBase* Property = IsValid(Object) ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
	return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
}

APawn* Shooter(AActor* Weapon)
{
	for (int32 Depth = 0; IsValid(Weapon) && Depth < 4; ++Depth)
	{
		if (APawn* Pawn = Cast<APawn>(Weapon)) return Pawn;
		AActor* Next = Weapon->GetOwner();
		Weapon = Next ? Next : Weapon->GetAttachParentActor();
	}
	return nullptr;
}
}

bool UProjectileMuzzleFlashLibrary::ShouldUseProjectileMuzzleFlash(AActor* Weapon)
{
	const APawn* Pawn = Shooter(Weapon);
	return Flag(Weapon, TEXT("bAlignProjectileMuzzleFlash"))
		&& Flag(Weapon, TEXT("bUseMuzzleProjectileAim")) && IsValid(Pawn) && !Pawn->IsLocallyControlled();
}

FVector UProjectileMuzzleFlashLibrary::ResolveFlashDirection(FVector Velocity, FVector ShotStart, FVector ShotTarget)
{
	if (!Velocity.ContainsNaN() && !Velocity.IsNearlyZero()) return Velocity.GetSafeNormal();
	if (ShotStart.ContainsNaN() || ShotTarget.ContainsNaN()) return FVector::ZeroVector;
	return (ShotTarget - ShotStart).GetSafeNormal();
}

bool UProjectileMuzzleFlashLibrary::SpawnProjectileMuzzleFlash(AActor* Weapon,
	USceneComponent* SelectedMesh, AActor* Projectile, FVector ShotStart, FVector ShotTarget)
{
	if (!ShouldUseProjectileMuzzleFlash(Weapon) || !Weapon->GetWorld()
		|| Weapon->GetNetMode() == NM_DedicatedServer || !IsValid(SelectedMesh)
		|| !SelectedMesh->DoesSocketExist(TEXT("Muzzle"))) return false;

	const UProjectileMovementComponent* Movement = IsValid(Projectile)
		? Projectile->FindComponentByClass<UProjectileMovementComponent>() : nullptr;
	const FVector Forward = ResolveFlashDirection(Movement ? Movement->Velocity : FVector::ZeroVector, ShotStart, ShotTarget);
	if (Forward.ContainsNaN() || Forward.IsNearlyZero()) return false;
	const FRotator Rotation = Forward.Rotation();
	UObject* Data = ObjectField(Weapon, TEXT("weaponData"));
	// Match the existing Cascade-first / Niagara-fallback choice in BP_WeaponBase.
	USceneComponent* Effect = nullptr;
	if (UParticleSystem* Template = Cast<UParticleSystem>(ObjectField(Data, TEXT("fireFX"))))
	{
		Effect = UGameplayStatics::SpawnEmitterAttached(Template, SelectedMesh, TEXT("Muzzle"),
			FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector,
			EAttachLocation::SnapToTarget, true, EPSCPoolMethod::None, false);
	}
	else if (UNiagaraSystem* NiagaraTemplate = Cast<UNiagaraSystem>(ObjectField(Data, TEXT("NSFireFX"))))
	{
		Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraTemplate, SelectedMesh, TEXT("Muzzle"),
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget,
			true, false, ENCPoolMethod::None, false);
	}
	if (!IsValid(Effect)) return false;
	// Set before activation so even the first emitted particles use the shot direction.
	// Recoil can move the socket afterward, but must not rotate an already-fired flash.
	Effect->SetAbsolute(false, true, false);
	Effect->SetWorldRotation(Rotation);
	Effect->Activate(true);
	return true;
}

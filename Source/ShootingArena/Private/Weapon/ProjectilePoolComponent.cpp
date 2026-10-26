#include "Weapon/ProjectilePoolComponent.h"

#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "Weapon/ProjectileConvergenceComponent.h"
#include "Weapon/ProjectilePoolSubsystem.h"
#include "Weapon/ProjectileWorldCollisionComponent.h"

namespace ProjectilePoolComponentHelpers
{
UObject* ObjectField(UObject* Object, FName Name)
{
	const auto* Property = IsValid(Object) ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
	return Property ? Property->GetObjectPropertyValue_InContainer(Object) : nullptr;
}

double Number(UObject* Object, FName Name)
{
	const auto* Property = IsValid(Object) ? FindFProperty<FNumericProperty>(Object->GetClass(), Name) : nullptr;
	return Property && Property->IsFloatingPoint()
		? Property->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<void>(Object)) : 0.0;
}
}

UProjectilePoolComponent::UProjectilePoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UProjectilePoolComponent::Capture(UProjectilePoolSubsystem* InPool, bool bInRetained)
{
	AActor* Actor = GetOwner();
	Pool = InPool;
	bRetained = bInRetained;
	ConfigProperty = FindFProperty<FStructProperty>(Actor->GetClass(), TEXT("wConfig"));
	DamageProperty = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("canDealDamage"));
	IdProperty = FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("projectileId"));
	if (ConfigProperty)
	{
		for (TFieldIterator<FObjectPropertyBase> It(ConfigProperty->Struct); It; ++It)
		{
			if (It->GetName().StartsWith(TEXT("weapon_"))) ConfigWeapon = *It;
			if (It->GetName().StartsWith(TEXT("config_"))) ConfigData = *It;
		}
	}
	Movement = Actor->FindComponentByClass<UProjectileMovementComponent>();
	Collision = Actor->FindComponentByClass<UProjectileWorldCollisionComponent>();
	if (!ConfigWeapon || !ConfigData || !DamageProperty || !IdProperty || !Movement || !Collision
		|| !Actor->GetRootComponent()) return false;
	bRotationFollowsVelocity = Movement->bRotationFollowsVelocity;
	bRotationRemainsVertical = Movement->bRotationRemainsVertical;
	const bool bRocket = Actor->GetClass()->GetFName() == TEXT("BP_RocketLauncherProjectile_C");
	DelayedMesh = Cast<USceneComponent>(ProjectilePoolComponentHelpers::ObjectField(Actor,
		bRocket ? TEXT("SkeletalMesh") : TEXT("ProjectileMesh")));
	TInlineComponentArray<UActorComponent*> Components(Actor);
	for (UActorComponent* Component : Components)
	{
		if (Component == this) continue;
		FComponentDefault& Default = Defaults.AddDefaulted_GetRef();
		Default.Component = Component;
		Default.bTick = Component->IsComponentTickEnabled();
		Default.bActive = Component->IsActive();
		if (const auto* Scene = Cast<USceneComponent>(Component))
		{
			Default.bVisible = Scene->IsVisible();
			Default.bHidden = Scene->bHiddenInGame;
		}
		if (auto* Audio = Cast<UAudioComponent>(Component)) Audio->bAutoDestroy = false;
		if (auto* Particle = Cast<UParticleSystemComponent>(Component)) Particle->bAutoDestroy = false;
		if (auto* Niagara = Cast<UNiagaraComponent>(Component)) Niagara->SetAutoDestroy(false);
	}
	Convergence = NewObject<UProjectileConvergenceComponent>(Actor);
	Actor->AddInstanceComponent(Convergence);
	Convergence->RegisterComponent();
	StopShot();
	return true;
}

void UProjectilePoolComponent::SetConfig(AActor* Weapon, UObject* Data)
{
	void* Config = ConfigProperty->ContainerPtrToValuePtr<void>(GetOwner());
	ConfigWeapon->SetObjectPropertyValue_InContainer(Config, Weapon);
	ConfigData->SetObjectPropertyValue_InContainer(Config, Data);
}

void UProjectilePoolComponent::ResetBlueprintState()
{
	AActor* Actor = GetOwner();
	for (const FName Name : {FName(TEXT("exploded")), FName(TEXT("hitSoundCount")), FName(TEXT("impactNormalEffect")),
		FName(TEXT("explosionDamage")), FName(TEXT("knockbackDamage"))})
	{
		if (const FProperty* Property = Actor->GetClass()->FindPropertyByName(Name))
			Property->CopyCompleteValue_InContainer(Actor, Actor->GetClass()->GetDefaultObject());
	}
}

bool UProjectilePoolComponent::StartShot(const FTransform& Transform, AActor* Weapon, UObject* Data,
	APawn* Instigator, bool bCanDealDamage, int32 InShotId)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProjectilePool_Activate);
	AActor* Actor = GetOwner();
	const double Speed = ProjectilePoolComponentHelpers::Number(Data, TEXT("projectileSpeed"));
	const double Range = ProjectilePoolComponentHelpers::Number(Data, TEXT("range"));
	const double Offset = ProjectilePoolComponentHelpers::Number(Data, TEXT("projectileVisualForwardOffset"));
	if (State != EProjectilePoolState::Idle || !IsValid(Weapon) || Weapon->IsActorBeingDestroyed()
		|| !IsValid(Data) || !Weapon->IsA(ConfigWeapon->PropertyClass) || !Data->IsA(ConfigData->PropertyClass)
		|| !FMath::IsFinite(Speed) || Speed <= UE_SMALL_NUMBER || !FMath::IsFinite(Range) || Range <= 0.0
		|| !FMath::IsFinite(Offset) || Transform.ContainsNaN()) return false;
	++Generation;
	ResetBlueprintState();
	SetConfig(Weapon, Data);
	ShotWeapon = Weapon;
	ShotWeaponKey = FObjectKey(Weapon);
	ShotId = InShotId;
	// Local nonreplicated Actors can themselves have authority. The weapon is the source of truth.
	bDamaging = bCanDealDamage && Weapon->HasAuthority() && GetWorld()->GetNetMode() != NM_Client;
	DamageProperty->SetPropertyValue_InContainer(Actor, bDamaging);
	IdProperty->SetPropertyValue_InContainer(Actor, ShotId);
	Actor->SetOwner(Weapon);
	Actor->SetInstigator(Instigator);
	Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
	Movement->SetUpdatedComponent(Actor->GetRootComponent());
	Movement->ClearPendingForce(true);
	Movement->ResetInterpolation();
	Movement->ProjectileGravityScale = 0.f;
	Movement->bSimulationEnabled = true;
	Movement->bRotationFollowsVelocity = bRotationFollowsVelocity;
	Movement->bRotationRemainsVertical = bRotationRemainsVertical;
	Movement->PreviousHitTime = 1.f;
	Movement->PreviousHitNormal = FVector::UpVector;
	Movement->SetVelocityInLocalSpace(FVector(Speed, 0, 0));
	Movement->UpdateComponentVelocity();
	if (auto* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
	{
		Primitive->ClearMoveIgnoreActors();
		if (IsValid(Instigator)) Primitive->IgnoreActorWhenMoving(Instigator, true);
	}
	if (auto* Character = Cast<ACharacter>(Instigator)) Character->GetCapsuleComponent()->IgnoreActorWhenMoving(Actor, true);
	State = EProjectilePoolState::Active;
	Actor->SetActorHiddenInGame(false);
	for (const FComponentDefault& Default : Defaults)
	{
		UActorComponent* Component = Default.Component.Get();
		if (!IsValid(Component) || Component == Movement || Component == Collision) continue;
		if (auto* Scene = Cast<USceneComponent>(Component))
		{
			Scene->SetVisibility(Default.bVisible, false);
			Scene->SetHiddenInGame(Default.bHidden, false);
		}
		// Dedicated servers keep the same gameplay components, without starting presentation.
		if (auto* Audio = Cast<UAudioComponent>(Component))
		{
			if (GetWorld()->GetNetMode() != NM_DedicatedServer) Audio->Play(0.f);
		}
		else if (Default.bActive && GetWorld()->GetNetMode() != NM_DedicatedServer) Component->Activate(true);
		Component->SetComponentTickEnabled(Default.bTick && GetWorld()->GetNetMode() != NM_DedicatedServer);
	}
	if (DelayedMesh) DelayedMesh->SetVisibility(false, true);
	Actor->SetActorEnableCollision(true);
	Movement->Activate(true);
	Movement->SetComponentTickEnabled(true);
	Collision->ResetForPooledShot();
	const uint64 ShotGeneration = Generation;
	GetWorld()->GetTimerManager().SetTimer(ExpiryTimer, FTimerDelegate::CreateWeakLambda(this, [this, ShotGeneration]
	{
		if (IsShotActive() && Generation == ShotGeneration) Release();
	}), Range / Speed, false);
	FTimerDelegate Show = FTimerDelegate::CreateWeakLambda(this, [this, ShotGeneration]
	{
		if (IsShotActive() && Generation == ShotGeneration && IsValid(DelayedMesh)) DelayedMesh->SetVisibility(true, true);
	});
	if (Offset > 0.0) GetWorld()->GetTimerManager().SetTimer(VisibilityTimer, Show, Offset / Speed, false);
	else VisibilityTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(Show);
	return true;
}

void UProjectilePoolComponent::StopShot()
{
	AActor* Actor = GetOwner();
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(ExpiryTimer);
		World->GetTimerManager().ClearTimer(VisibilityTimer);
		World->GetLatentActionManager().RemoveActionsForObject(Actor);
	}
	Actor->SetLifeSpan(0.f);
	Actor->SetActorEnableCollision(false);
	Actor->SetActorTickEnabled(false);
	Actor->SetActorHiddenInGame(true);
	if (auto* Character = Cast<ACharacter>(Actor->GetInstigator()); IsValid(Character))
		Character->GetCapsuleComponent()->IgnoreActorWhenMoving(Actor, false);
	if (auto* Primitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent())) Primitive->ClearMoveIgnoreActors();
	if (Collision) Collision->EndPooledShot();
	if (Convergence) Convergence->ResetForPool();
	if (Movement)
	{
		Movement->StopMovementImmediately();
		Movement->ClearPendingForce(true);
		Movement->Deactivate();
		Movement->SetComponentTickEnabled(false);
	}
	for (const FComponentDefault& Default : Defaults)
	{
		UActorComponent* Component = Default.Component.Get();
		if (!IsValid(Component)) continue;
		if (auto* Audio = Cast<UAudioComponent>(Component)) Audio->Stop();
		if (auto* Niagara = Cast<UNiagaraComponent>(Component)) Niagara->DeactivateImmediate();
		else if (auto* Particle = Cast<UParticleSystemComponent>(Component))
		{
			Particle->DeactivateSystem();
			Particle->KillParticlesForced();
		}
		Component->Deactivate();
		Component->SetComponentTickEnabled(false);
	}
	if (DamageProperty) DamageProperty->SetPropertyValue_InContainer(Actor, false);
	if (ConfigWeapon && ConfigData) SetConfig(nullptr, nullptr);
	Actor->SetOwner(nullptr);
	Actor->SetInstigator(nullptr);
}

bool UProjectilePoolComponent::Release()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProjectilePool_Return);
	if (!IsShotActive()) return false;
	State = EProjectilePoolState::Returning;
	// Remove the shot index before clearing its identity. Free-list insertion is deferred by the subsystem.
	if (Pool.IsValid()) Pool->Return(this);
	StopShot();
	ShotWeapon.Reset();
	bDamaging = false;
	return true;
}

void UProjectilePoolComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Pool.IsValid()) Pool->Forget(this);
	State = EProjectilePoolState::Destroyed;
	StopShot();
	Super::EndPlay(EndPlayReason);
}

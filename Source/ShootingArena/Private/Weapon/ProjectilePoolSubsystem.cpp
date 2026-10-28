#include "Weapon/ProjectilePoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Character.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Weapon/ProjectilePoolComponent.h"

namespace ProjectilePoolSubsystemHelpers
{
static TAutoConsoleVariable<int32> Enabled(TEXT("sa.ProjectilePool.Enabled"), -1,
	TEXT("-1: Project Settings. 0: destroy shots on return. 1: reuse shots."));
static TAutoConsoleVariable<int32> PlasmaPrewarm(TEXT("sa.ProjectilePool.PlasmaPrewarm"), -1, TEXT("-1: Project Settings. Otherwise overrides initial Plasma actors per world."));
static TAutoConsoleVariable<int32> RocketPrewarm(TEXT("sa.ProjectilePool.RocketPrewarm"), -1, TEXT("-1: Project Settings. Otherwise overrides initial Rocket actors per world."));
static TAutoConsoleVariable<int32> PlasmaCapacity(TEXT("sa.ProjectilePool.PlasmaCapacity"), -1, TEXT("-1: Project Settings. Otherwise overrides maximum retained Plasma actors."));
static TAutoConsoleVariable<int32> RocketCapacity(TEXT("sa.ProjectilePool.RocketCapacity"), -1, TEXT("-1: Project Settings. Otherwise overrides maximum retained Rocket actors."));
static TAutoConsoleVariable<int32> PrewarmBudget(TEXT("sa.ProjectilePool.PrewarmPerTick"), -1, TEXT("-1: Project Settings. Otherwise overrides actors pre-created per world tick. 0 pauses prewarming."));

bool IsRocket(const UClass* Class) { return Class->GetFName() == TEXT("BP_RocketLauncherProjectile_C"); }
int32 Capacity(const UClass* Class, const FProjectilePoolConfiguration& Configuration)
{
	return IsRocket(Class) ? Configuration.RocketCapacity : Configuration.PlasmaCapacity;
}
}

void UProjectilePoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldConfiguration = GetDefault<UProjectilePoolSettings>()->GetConfiguration();
}

FProjectilePoolConfiguration UProjectilePoolSubsystem::GetConfiguration() const
{
	using namespace ProjectilePoolSubsystemHelpers;
	FProjectilePoolConfiguration Result = WorldConfiguration;
	const int32 EnabledOverride = Enabled.GetValueOnGameThread();
	if (EnabledOverride >= 0) Result.bEnabled = EnabledOverride != 0;
	auto Override = [](const TAutoConsoleVariable<int32>& Variable, int32& Value)
	{
		const int32 Console = Variable.GetValueOnGameThread();
		if (Console >= 0) Value = Console;
	};
	Override(PlasmaCapacity, Result.PlasmaCapacity); Override(RocketCapacity, Result.RocketCapacity);
	Override(PlasmaPrewarm, Result.PlasmaPrewarm); Override(RocketPrewarm, Result.RocketPrewarm);
	Override(PrewarmBudget, Result.PrewarmPerTick);
	Result.PlasmaPrewarm = FMath::Clamp(Result.PlasmaPrewarm, 0, Result.PlasmaCapacity);
	Result.RocketPrewarm = FMath::Clamp(Result.RocketPrewarm, 0, Result.RocketCapacity);
	return Result;
}

bool UProjectilePoolSubsystem::SupportsClass(const UClass* Class)
{
	if (!Class || !Class->IsChildOf<AActor>() || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists)) return false;
	const FName Package = Class->GetOutermost()->GetFName();
	return (Class->GetFName() == TEXT("BP_PlasmaProjectile_C")
		&& Package == TEXT("/Game/QuakeLike_1_0/HeldItem/Projectile/BP_PlasmaProjectile"))
		|| (Class->GetFName() == TEXT("BP_RocketLauncherProjectile_C")
		&& Package == TEXT("/Game/QuakeLike_1_0/HeldItem/Projectile/BP_RocketLauncherProjectile"));
}

void UProjectilePoolSubsystem::DetachInstigatorForLife(APawn* Instigator)
{
	if (!IsValid(Instigator)) return;
	for (AActor* Projectile : Owned)
	{
		if (!IsValid(Projectile) || Projectile->GetInstigator() != Instigator) continue;
		if (auto* Body = Cast<UPrimitiveComponent>(Projectile->GetRootComponent())) Body->IgnoreActorWhenMoving(Instigator, false);
		if (auto* Character = Cast<ACharacter>(Instigator)) Character->GetCapsuleComponent()->IgnoreActorWhenMoving(Projectile, false);
		Projectile->SetInstigator(nullptr);
	}
}

bool UProjectilePoolSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

AActor* UProjectilePoolSubsystem::Create(UClass* Class, const FTransform& Transform, bool bRetained)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProjectilePool_Create);
	// Persistent-level ownership prevents a streamed firing actor from taking the pool with it.
	FActorSpawnParameters Parameters;
	Parameters.OverrideLevel = GetWorld()->PersistentLevel;
	Parameters.bDeferConstruction = true;
	Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = GetWorld()->SpawnActor<AActor>(Class, Transform, Parameters);
	if (!Actor) return nullptr;
	Actor->SetActorEnableCollision(false);
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorTickEnabled(false);
	auto* Component = NewObject<UProjectilePoolComponent>(Actor);
	Actor->AddInstanceComponent(Component);
	Component->RegisterComponent();
	// IsPooledProjectile is already true when the BP's first BeginPlay runs.
	Actor->FinishSpawning(Transform);
	if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || !Component->Capture(this, bRetained))
	{
		if (IsValid(Actor)) Actor->Destroy();
		return nullptr;
	}
	Owned.Add(Actor);
	FProjectilePoolBucket& Bucket = Pools.FindOrAdd(Class);
	++Bucket.Stats.Created;
	if (bRetained) ++Bucket.Retained;
	else ++Bucket.Stats.OverflowCreated;
	return Actor;
}

AActor* UProjectilePoolSubsystem::Acquire(TSubclassOf<AActor> Class, const FTransform& Transform,
	AActor* Weapon, UObject* WeaponData, APawn* Instigator, bool bCanDealDamage, int32 ShotId)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProjectilePool_Acquire);
	if (bShuttingDown || !SupportsClass(Class) || !IsValid(Weapon) || !IsValid(WeaponData)
		|| Weapon->GetWorld() != GetWorld() || (Instigator && Instigator->GetWorld() != GetWorld())
		|| Transform.ContainsNaN() || GetWorld()->bIsTearingDown) return nullptr;
	FProjectilePoolBucket& Bucket = Pools.FindOrAdd(Class.Get());
	AActor* Actor = nullptr;
	const FProjectilePoolConfiguration Configuration = GetConfiguration();
	if (Configuration.bEnabled)
	{
		while (!Bucket.Free.IsEmpty() && !Actor)
		{
			AActor* Candidate = Bucket.Free.Pop(EAllowShrinking::No);
			if (IsValid(Candidate) && !Candidate->IsActorBeingDestroyed()) Actor = Candidate;
		}
	}
	if (Actor)
	{
		--Bucket.Stats.Idle;
		++Bucket.Stats.Reused;
	}
	else
	{
		const bool bRetained = Configuration.bEnabled
			&& Bucket.Retained < ProjectilePoolSubsystemHelpers::Capacity(Class, Configuration);
		Actor = Create(Class, Transform, bRetained);
	}
	if (!Actor) return nullptr;
	auto* Component = Actor->FindComponentByClass<UProjectilePoolComponent>();
	if (!Component || !Component->StartShot(Transform, Weapon, WeaponData, Instigator, bCanDealDamage, ShotId))
	{
		Actor->Destroy();
		return nullptr;
	}
	FProjectilePoolBucket& CurrentBucket = Pools.FindChecked(Class.Get());
	++CurrentBucket.Stats.Active;
	CurrentBucket.Stats.PeakActive = FMath::Max(CurrentBucket.Stats.PeakActive, CurrentBucket.Stats.Active);
	Shots.FindOrAdd({FObjectKey(Weapon), ShotId}).Add(Component);
	for (auto It = TrackedWeapons.CreateIterator(); It; ++It) if (!It->IsValid()) It.RemoveCurrent();
	TrackedWeapons.Add(Weapon);
	return Actor;
}

void UProjectilePoolSubsystem::Untrack(UProjectilePoolComponent* Component)
{
	const FProjectilePoolShotKey Key{Component->ShotWeaponKey, Component->ShotId};
	if (auto* Copies = Shots.Find(Key))
	{
		Copies->RemoveAllSwap([Component](const TWeakObjectPtr<UProjectilePoolComponent>& Copy)
			{ return !Copy.IsValid() || Copy.Get() == Component; }, EAllowShrinking::No);
		if (Copies->IsEmpty()) Shots.Remove(Key);
	}
}

void UProjectilePoolSubsystem::Return(UProjectilePoolComponent* Component)
{
	if (bShuttingDown) return;
	Untrack(Component);
	FProjectilePoolBucket& Bucket = Pools.FindChecked(Component->GetOwner()->GetClass());
	--Bucket.Stats.Active;
	++Bucket.Stats.Returning;
	++Bucket.Stats.Returned;
	PendingReturns.Add(Component);
}

void UProjectilePoolSubsystem::Forget(UProjectilePoolComponent* Component)
{
	if (bShuttingDown || Owned.RemoveSingleSwap(Component->GetOwner(), EAllowShrinking::No) == 0) return;
	Untrack(Component);
	FProjectilePoolBucket& Bucket = Pools.FindChecked(Component->GetOwner()->GetClass());
	Bucket.Free.RemoveSingleSwap(Component->GetOwner(), EAllowShrinking::No);
	if (Component->bRetained) --Bucket.Retained;
	if (Component->State == EProjectilePoolState::Active) --Bucket.Stats.Active;
	else if (Component->State == EProjectilePoolState::Returning) --Bucket.Stats.Returning;
	else if (Component->State == EProjectilePoolState::Idle) Bucket.Stats.Idle = Bucket.Free.Num();
}

bool UProjectilePoolSubsystem::TracksWeapon(const AActor* Weapon) const
{
	return TrackedWeapons.Contains(TWeakObjectPtr<AActor>(const_cast<AActor*>(Weapon)));
}

int32 UProjectilePoolSubsystem::RetireVisuals(AActor* Weapon, int32 ShotId)
{
	const auto* Matches = Shots.Find({FObjectKey(Weapon), ShotId});
	if (!Matches) return 0;
	// Release mutates the index. Snapshot only this shot's copies, never iterate all UObjects.
	const auto Copies = *Matches;
	int32 Count = 0;
	for (const auto& Copy : Copies)
		if (Copy.IsValid() && !Copy->bDamaging && Copy->Release()) ++Count;
	return Count;
}

FProjectilePoolStats UProjectilePoolSubsystem::GetPoolStats(TSubclassOf<AActor> Class) const
{
	const auto* Bucket = Pools.Find(Class.Get());
	return Bucket ? Bucket->Stats : FProjectilePoolStats();
}

void UProjectilePoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	const FProjectilePoolConfiguration Configuration = GetConfiguration();
	if (!Configuration.bEnabled) return;
	for (const TCHAR* Path : {TEXT("/Game/QuakeLike_1_0/HeldItem/Projectile/BP_PlasmaProjectile.BP_PlasmaProjectile_C"),
		TEXT("/Game/QuakeLike_1_0/HeldItem/Projectile/BP_RocketLauncherProjectile.BP_RocketLauncherProjectile_C")})
	{
		if (UClass* Class = LoadClass<AActor>(nullptr, Path); SupportsClass(Class))
		{
			const int32 Count = ProjectilePoolSubsystemHelpers::IsRocket(Class)
				? Configuration.RocketPrewarm : Configuration.PlasmaPrewarm;
			Pools.FindOrAdd(Class).PrewarmRemaining = Count;
		}
	}
}

bool UProjectilePoolSubsystem::IsTickable() const
{
	if (IsTemplate() || bShuttingDown || !GetWorld() || GetWorld()->bIsTearingDown) return false;
	if (!PendingReturns.IsEmpty()) return true;
	for (const auto& Pair : Pools) if (Pair.Value.PrewarmRemaining > 0) return true;
	return false;
}

TStatId UProjectilePoolSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UProjectilePoolSubsystem, STATGROUP_Tickables);
}

void UProjectilePoolSubsystem::Tick(float DeltaTime)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ProjectilePool_Maintenance);
	const FProjectilePoolConfiguration Configuration = GetConfiguration();
	// Take returns off the queue first: new callbacks can enqueue more during destruction.
	TArray<TWeakObjectPtr<UProjectilePoolComponent>> Returns;
	Swap(Returns, PendingReturns);
	for (const auto& Weak : Returns)
	{
		UProjectilePoolComponent* Component = Weak.Get();
		if (!IsValid(Component) || Component->State != EProjectilePoolState::Returning) continue;
		AActor* Actor = Component->GetOwner();
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed()) continue;
		FProjectilePoolBucket& Bucket = Pools.FindChecked(Actor->GetClass());
		if (!Component->bRetained || !Configuration.bEnabled
			|| Bucket.Retained > ProjectilePoolSubsystemHelpers::Capacity(Actor->GetClass(), Configuration))
		{
			Actor->Destroy();
			continue;
		}
		--Bucket.Stats.Returning;
		++Bucket.Stats.Idle;
		Component->State = EProjectilePoolState::Idle;
		Bucket.Free.Add(Actor);
	}
	int32 Budget = Configuration.PrewarmPerTick;
	for (auto& Pair : Pools)
	{
		FProjectilePoolBucket& Bucket = Pair.Value;
		if (!Configuration.bEnabled) Bucket.PrewarmRemaining = 0;
		while (Budget > 0 && Bucket.PrewarmRemaining > 0)
		{
			--Bucket.PrewarmRemaining;
			--Budget;
			if (Bucket.Retained >= ProjectilePoolSubsystemHelpers::Capacity(Pair.Key, Configuration)) { Bucket.PrewarmRemaining = 0; break; }
			if (AActor* Actor = Create(Pair.Key, FTransform::Identity, true))
			{
				Bucket.Free.Add(Actor);
				++Bucket.Stats.Idle;
			}
		}
	}
}

void UProjectilePoolSubsystem::Deinitialize()
{
	bShuttingDown = true;
	Shots.Empty();
	TrackedWeapons.Empty();
	PendingReturns.Empty();
	for (AActor* Actor : Owned)
		if (IsValid(Actor))
		{
			if (auto* Component = Actor->FindComponentByClass<UProjectilePoolComponent>())
			{
				Component->State = EProjectilePoolState::Destroyed;
				Component->StopShot();
			}
		}
	Pools.Empty();
	Owned.Empty();
	Super::Deinitialize();
}

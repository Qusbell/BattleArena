#include "Weapon/ProjectileWorldCollisionComponent.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/UnrealType.h"

namespace
{
// Unity builds can include other .cpp files in this translation unit. Keep
// these helpers separate from their anonymous-namespace helpers as well.
namespace ProjectileCollisionHelpers
{
bool LiveComponent(const UActorComponent* Component)
{
	return IsValid(Component) && !Component->IsBeingDestroyed();
}

bool ValidHit(const FHitResult& Hit)
{
	return FMath::IsFinite(Hit.Time) && Hit.Time >= 0.f && Hit.Time <= 1.f
		&& !Hit.Location.ContainsNaN() && !Hit.ImpactPoint.ContainsNaN()
		&& !Hit.Normal.ContainsNaN() && !Hit.ImpactNormal.ContainsNaN();
}

FCollisionQueryParams QueryFor(const AActor* Projectile)
{
	FCollisionQueryParams Query(SCENE_QUERY_STAT(ProjectileSplitCollision), false);
	Query.AddIgnoredActor(Projectile);
	Query.AddIgnoredActor(Projectile->GetOwner());
	Query.AddIgnoredActor(Projectile->GetInstigator());
	return Query;
}

bool CanDamage(const AActor* Projectile)
{
	const FBoolProperty* Flag = FindFProperty<FBoolProperty>(Projectile->GetClass(), TEXT("canDealDamage"));
	return Projectile->HasAuthority() && (!Flag || Flag->GetPropertyValue_InContainer(Projectile));
}
}
}

UProjectileWorldCollisionComponent::UProjectileWorldCollisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// Movement (including the convergence driver's two flight legs) finishes first.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	bAutoActivate = true;
	// CDO construction must not create/update a physics BodySetup subobject.
	InitSphereRadius(8.f);
	SetUsingAbsoluteScale(true);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetGenerateOverlapEvents(false);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	SetHiddenInGame(true);
}

void UProjectileWorldCollisionComponent::BeginPlay()
{
	Super::BeginPlay();
	bResolved = false;
	bResolving = false;
	BindPlayerCollision();
}

bool UProjectileWorldCollisionComponent::HasLiveContext() const
{
	const AActor* Projectile = GetOwner();
	const USphereComponent* Player = PlayerCollision.Get();
	const UWorld* World = GetWorld();
	return !IsBeingDestroyed() && IsRegistered() && IsActive()
		&& IsValid(Projectile) && !Projectile->IsActorBeingDestroyed()
		&& IsValid(World) && !World->bIsTearingDown && ProjectileCollisionHelpers::LiveComponent(Player) && Player->IsRegistered()
		&& Projectile->GetRootComponent() == Player && Player->GetOwner() == Projectile
		&& Player->GetWorld() == World && GetAttachParent() == Player
		&& !GetComponentTransform().ContainsNaN() && !Player->GetComponentTransform().ContainsNaN();
}

bool UProjectileWorldCollisionComponent::BindPlayerCollision()
{
	Unbind();
	if (bResolved || IsBeingDestroyed() || !IsRegistered() || !IsActive() || !IsComponentTickEnabled()) return false;
	AActor* Projectile = GetOwner();
	USphereComponent* Player = IsValid(Projectile) && !Projectile->IsActorBeingDestroyed()
		? Cast<USphereComponent>(Projectile->GetRootComponent()) : nullptr;
	if (!ProjectileCollisionHelpers::LiveComponent(Player) || Player == this) return false;
	PlayerCollision = Player;
	if (!HasLiveContext()) { Unbind(); return false; }
	// Damage uses explicit continuous queries, not actor/child overlap callbacks.
	Player->SetGenerateOverlapEvents(false);
	// Changing overlap state can invoke callbacks that unregister/destroy objects.
	if (!HasLiveContext()) { Unbind(); return false; }
	LastPosition = Player->GetComponentLocation();
	bHasLastPosition = true;
	Player->TransformUpdated.AddUObject(this, &ThisClass::RecordTravel);
	return true;
}

void UProjectileWorldCollisionComponent::OnRegister()
{
	Super::OnRegister();
	if (HasBegunPlay() && !bResolved) BindPlayerCollision();
}

void UProjectileWorldCollisionComponent::Activate(bool bReset)
{
	const bool bWasActive = IsActive();
	Super::Activate(bReset);
	if (HasBegunPlay() && !bResolved && (!bWasActive || bReset || !HasLiveContext())) BindPlayerCollision();
}

void UProjectileWorldCollisionComponent::Deactivate()
{
	Unbind();
	Super::Deactivate();
}

void UProjectileWorldCollisionComponent::SetComponentTickEnabled(bool bEnabled)
{
	const bool bChanged = bEnabled != IsComponentTickEnabled();
	Super::SetComponentTickEnabled(bEnabled);
	if (!bChanged) return;
	Travel.Reset();
	bHasLastPosition = false;
	if (bEnabled && HasBegunPlay() && !bResolved) BindPlayerCollision();
}

void UProjectileWorldCollisionComponent::RecordTravel(USceneComponent* Updated, EUpdateTransformFlags Flags, ETeleportType Teleport)
{
	if (bResolved || bResolving || !IsValid(Updated)) return;
	if (!IsComponentTickEnabled() || !HasLiveContext() || Updated != PlayerCollision.Get())
	{
		Travel.Reset(); bHasLastPosition = false;
		return;
	}
	const FVector Position = Updated->GetComponentLocation();
	if (Position.ContainsNaN()) { Travel.Reset(); bHasLastPosition = false; return; }
	if (Teleport != ETeleportType::None) Travel.Reset();
	else if (bHasLastPosition && !Position.Equals(LastPosition, UE_SMALL_NUMBER)) Travel.Add({LastPosition, Position});
	LastPosition = Position;
	bHasLastPosition = true;
}

bool UProjectileWorldCollisionComponent::FindImpact(const FVector& Start, const FVector& End, FHitResult& OutHit) const
{
	OutHit = FHitResult();
	if (!HasLiveContext()) return false;
	const AActor* Projectile = GetOwner();
	const USphereComponent* Player = PlayerCollision.Get();
	UWorld* World = GetWorld();
	const float WallRadius = GetScaledSphereRadius();
	const float PlayerRadius = IsValid(Player) ? Player->GetScaledSphereRadius() : 0.f;
	if (!IsValid(Projectile) || Projectile->IsActorBeingDestroyed() || !IsValid(Player) || !IsValid(World)
		|| World->bIsTearingDown || Start.ContainsNaN() || End.ContainsNaN()
		|| !FMath::IsFinite(WallRadius) || WallRadius <= UE_SMALL_NUMBER
		|| !FMath::IsFinite(PlayerRadius) || PlayerRadius <= UE_SMALL_NUMBER) return false;
	FCollisionQueryParams Query = ProjectileCollisionHelpers::QueryFor(Projectile);
	Query.AddIgnoredActors(Player->GetMoveIgnoreActors());
	FCollisionResponseParams Responses(GetCollisionResponseToChannels());
	Responses.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	Responses.CollisionResponse.SetResponse(GetCollisionObjectType(), ECR_Ignore);
	bool bFound = World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, GetCollisionObjectType(),
		FCollisionShape::MakeSphere(WallRadius), Query, Responses);
	if (bFound && !ProjectileCollisionHelpers::ValidHit(OutHit)) { OutHit = FHitResult(); return false; }
	if (!ProjectileCollisionHelpers::CanDamage(Projectile)) return bFound;
	TArray<FHitResult> Candidates;
	World->SweepMultiByObjectType(Candidates, Start, End, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(PlayerRadius), Query);
	for (const FHitResult& Candidate : Candidates)
	{
		if (!IsValid(Candidate.GetActor()) || !Candidate.GetActor()->IsA<ACharacter>()
			|| Candidate.GetActor()->IsActorBeingDestroyed() || !ProjectileCollisionHelpers::LiveComponent(Candidate.GetComponent()) || !ProjectileCollisionHelpers::ValidHit(Candidate)
			|| Candidate.GetComponent()->GetCollisionResponseToChannel(Player->GetCollisionObjectType()) != ECR_Block
			|| (bFound && Candidate.Time >= OutHit.Time - UE_KINDA_SMALL_NUMBER)) continue;
		// A large player radius can reach through an adjacent thin wall even when
		// the flight centre never crosses it. Reject these candidates separately.
		FVector Target = Candidate.GetActor()->GetActorLocation();
		FVector ClosestPoint = Target;
		const float Distance = Candidate.GetComponent()->GetClosestPointOnCollision(Candidate.Location, ClosestPoint);
		if (FMath::IsFinite(Distance) && Distance >= 0.f && !ClosestPoint.ContainsNaN()) Target = ClosestPoint;
		if (Target.ContainsNaN() || World->LineTraceTestByChannel(Candidate.Location, Target,
			GetCollisionObjectType(), Query, Responses)) continue;
		OutHit = Candidate;
		bFound = true;
	}
	return bFound;
}

void UProjectileWorldCollisionComponent::ResolveImpact(const FHitResult& Hit)
{
	AActor* Projectile = GetOwner();
	USphereComponent* Player = PlayerCollision.Get();
	if (bResolved || bResolving || !HasLiveContext() || !ProjectileCollisionHelpers::ValidHit(Hit)) return;
	bResolved = true;
	bResolving = true;
	Travel.Reset();
	SetComponentTickEnabled(false);
	Player->SetWorldLocation(Hit.Location, false, nullptr, ETeleportType::TeleportPhysics);
	if (!HasLiveContext()) { bResolving = false; return; }
	if (auto* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>(); ProjectileCollisionHelpers::LiveComponent(Movement)) Movement->StopSimulating(Hit);
	// OnProjectileStop can remove the actor, the collider, or the root component.
	if (HasLiveContext())
	{
		AActor* Other = Hit.GetActor();
		UPrimitiveComponent* OtherComponent = Hit.GetComponent();
		OnResolvedImpact.Broadcast(Player, IsValid(Other) && !Other->IsActorBeingDestroyed() ? Other : nullptr,
			ProjectileCollisionHelpers::LiveComponent(OtherComponent) ? OtherComponent : nullptr, FVector::ZeroVector, Hit);
	}
	bResolving = false;
}

bool UProjectileWorldCollisionComponent::ResolveTravel()
{
	// Move the array out before calling BP: an impact may destroy this actor.
	TArray<FTravelSegment> Segments = MoveTemp(Travel);
	for (const FTravelSegment& Segment : Segments)
	{
		FHitResult Hit;
		if (FindImpact(Segment.Start, Segment.End, Hit))
		{
			ResolveImpact(Hit);
			return bResolved;
		}
	}
	return false;
}

void UProjectileWorldCollisionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bResolved || bResolving) return;
	if (!IsActive() || !IsComponentTickEnabled()) { Unbind(); return; }
	if (!HasLiveContext() && (!HasBegunPlay() || !BindPlayerCollision())) return;
	if (!bHasLastPosition) { LastPosition = PlayerCollision->GetComponentLocation(); bHasLastPosition = true; }
	if (Travel.IsEmpty()) Travel.Add({LastPosition, LastPosition});
	ResolveTravel();
}

void UProjectileWorldCollisionComponent::Unbind()
{
	if (USphereComponent* Player = PlayerCollision.Get(); ProjectileCollisionHelpers::LiveComponent(Player))
	{
		Player->TransformUpdated.RemoveAll(this);
	}
	PlayerCollision.Reset();
	Travel.Reset();
	bHasLastPosition = false;
}

void UProjectileWorldCollisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bResolved = true;
	Unbind();
	Super::EndPlay(EndPlayReason);
}

void UProjectileWorldCollisionComponent::OnUnregister()
{
	Unbind();
	Super::OnUnregister();
}

#include "Items/DeathDropCarrier.h"

#include "Components/SphereComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogDeathDropCarrier, Log, All);

namespace
{
	bool SetBool(UObject* Object, const TCHAR* Name, bool Value)
	{
		if (!IsValid(Object)) return false;
		FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), Name);
		if (!Property) return false;
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool IsPickedUp(const AActor* Actor)
	{
		if (!IsValid(Actor)) return false;
		const FBoolProperty* Property = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("IsPickedUp"));
		return Property && Property->GetPropertyValue_InContainer(Actor);
	}

	void CenterPickupSphere(AActor* Item)
	{
		if (!IsValid(Item) || Item->IsActorBeingDestroyed()) return;
		TArray<USphereComponent*> Spheres;
		Item->GetComponents(Spheres);
		if (!Spheres.IsEmpty() && IsValid(Spheres[0]) && Spheres[0] != Item->GetRootComponent())
		{
			// Sight and navigation use GetActorLocation(). Keep the actor at the
			// carrier center and align its child pickup sphere instead of lowering it.
			Spheres[0]->SetRelativeLocation(FVector::ZeroVector);
		}
	}
}

ADeathDropCarrier::ADeathDropCarrier()
{
	bReplicates = true;
	SetReplicateMovement(true);
	SetNetUpdateFrequency(30.0f);
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	PhysicsBody = CreateDefaultSubobject<USphereComponent>(TEXT("DeathDropPhysics"));
	SetRootComponent(PhysicsBody);
	PhysicsBody->InitSphereRadius(18.0f);
	PhysicsBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsBody->SetCollisionObjectType(ECC_PhysicsBody);
	PhysicsBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	PhysicsBody->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	PhysicsBody->SetGenerateOverlapEvents(false);
	PhysicsBody->SetNotifyRigidBodyCollision(true);
	PhysicsBody->SetEnableGravity(true);
	PhysicsBody->SetSimulatePhysics(true);
	PhysicsBody->BodyInstance.bUseCCD = true;
	PhysicsBody->SetLinearDamping(0.0f); // The authored deceleration affects XY only.
	PhysicsBody->SetAngularDamping(10.0f);
}

void ADeathDropCarrier::Initialize(const FDeathDropSettings& Settings, UClass* PickupClass,
	const FDataTableRowHandle& Item, double BuffEndTime, bool bIsBuff)
{
	DropSettings = Settings;
	PickupActorClass = PickupClass;
	PickupItem = Item;
	BuffEndServerTime = BuffEndTime;
	bBuffPickup = bIsBuff;
}

void ADeathDropCarrier::BeginPlay()
{
	Super::BeginPlay();
	if (!IsValid(PhysicsBody))
	{
		UE_LOG(LogDeathDropCarrier, Warning, TEXT("Death drop %s has no valid physics body."), *GetName());
		SetActorTickEnabled(false);
		if (HasAuthority()) Destroy();
		return;
	}
	// Keep the pickup upright when the physical sphere hits slopes or walls.
	// The pickup Blueprint can still animate its mesh around the vertical axis.
	PhysicsBody->BodyInstance.bLockXRotation = true;
	PhysicsBody->BodyInstance.bLockYRotation = true;
	PhysicsBody->SetConstraintMode(EDOFMode::SixDOF);
	PhysicsBody->OnComponentHit.AddDynamic(this, &ADeathDropCarrier::OnBodyHit);
	if (HasAuthority()) SpawnPickup();
}

bool ADeathDropCarrier::ConfigurePickup(AActor* Item, bool bUpdateMesh)
{
	if (!IsValid(Item) || Item->IsActorBeingDestroyed() || !IsValid(PickupItem.DataTable)
		|| PickupItem.RowName.IsNone()) return false;
	FStructProperty* Handle = FindFProperty<FStructProperty>(Item->GetClass(), TEXT("DataTable Handle"));
	if (!Handle || Handle->Struct != FDataTableRowHandle::StaticStruct()
		|| !SetBool(Item, TEXT("IsSnapToGround"), false)) return false;
	*Handle->ContainerPtrToValuePtr<FDataTableRowHandle>(Item) = PickupItem;
	if (bBuffPickup)
	{
		FDoubleProperty* EndTime = FindFProperty<FDoubleProperty>(Item->GetClass(), TEXT("EndServerTime"));
		if (!EndTime || !SetBool(Item, TEXT("IsDroppedBuff"), true)) return false;
		EndTime->SetPropertyValue_InContainer(Item, BuffEndServerTime);
	}
	if (bUpdateMesh)
	{
		CenterPickupSphere(Item);
		UFunction* Setup = Item->FindFunction(TEXT("Setup Mesh"));
		// ProcessEvent with a null parameter buffer is only safe for a parameterless function.
		if (!Setup || Setup->ParmsSize != 0) return false;
		Item->ProcessEvent(Setup, nullptr);
	}
	return IsValid(Item) && !Item->IsActorBeingDestroyed();
}

void ADeathDropCarrier::SpawnPickup()
{
	if (!PickupActorClass) { Destroy(); return; }
	const FTransform Transform(FRotator::ZeroRotator, GetActorLocation());
	AActor* Item = GetWorld()->SpawnActorDeferred<AActor>(PickupActorClass, Transform,
		this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!IsValid(Item) || !ConfigurePickup(Item, false))
	{
		UE_LOG(LogDeathDropCarrier, Error, TEXT("Cannot configure death pickup %s, row %s"),
			*GetNameSafe(PickupActorClass), *PickupItem.RowName.ToString());
		if (IsValid(Item)) Item->Destroy();
		Destroy();
		return;
	}
	PickupActor = Item;
	// Keep the existing overlap and visual components; physics is exclusively on the carrier.
	Item->FinishSpawning(Transform);
	if (!IsValid(Item)) { Destroy(); return; }
	Item->AttachToComponent(PhysicsBody, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	CenterPickupSphere(Item);
	bPickupConfigured = true;
	ForceNetUpdate();
}

void ADeathDropCarrier::Launch(const FVector& Impulse)
{
	if (!HasAuthority() || !IsValid(PickupActor) || bLaunched) return;
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || IsActorBeingDestroyed()) return;
	if (!IsValid(PhysicsBody) || Impulse.ContainsNaN())
	{
		UE_LOG(LogDeathDropCarrier, Warning, TEXT("Skipping invalid death drop launch for %s."), *GetName());
		Destroy();
		return;
	}
	PhysicsBody->SetMassOverrideInKg(NAME_None, 1.0f, true);
	PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsBody->AddImpulse(Impulse); // One impulse, one kilogram; force settings have a consistent scale.
	InitialHorizontalVelocity = FVector(Impulse.X, Impulse.Y, 0.0f);
	LaunchTime = World->GetTimeSeconds();
	bLaunched = true;
	// Invalid imported values must never reach TimerManager or produce NaN transforms.
	const float Lifetime = FMath::IsFinite(DropSettings.DespawnTime)
		? FMath::Max(DropSettings.DespawnTime, 0.01f) : 30.0f;
	const double Now = GetServerTimeSeconds();
	DespawnShrinkStartServerTime = -1.0;
	DespawnServerTime = 0.0;
	if (DropSettings.DespawnShrinkEnabled && FMath::IsFinite(DropSettings.DespawnShrinkDuration)
		&& DropSettings.DespawnShrinkDuration > 0.0f && Now >= 0.0)
	{
		DespawnServerTime = Now + Lifetime;
		DespawnShrinkStartServerTime = DespawnServerTime - FMath::Min(DropSettings.DespawnShrinkDuration, Lifetime);
	}
	SetLifeSpan(Lifetime);
	ForceNetUpdate();
}

double ADeathDropCarrier::GetServerTimeSeconds() const
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown) return -1.0;
	const AGameStateBase* State = World->GetGameState();
	// A client's local world clock is not interchangeable with the replicated server clock.
	if (!IsValid(State) && !HasAuthority()) return -1.0;
	const double Now = IsValid(State) ? State->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	return FMath::IsFinite(Now) ? Now : -1.0;
}

void ADeathDropCarrier::UpdateDespawnVisuals()
{
	UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || IsActorBeingDestroyed()
		|| !IsValid(PickupActor) || PickupActor->IsActorBeingDestroyed()) return;
	if (GetNetMode() == NM_DedicatedServer) return;
	if (IsPickedUp(PickupActor)) { RestoreDespawnScale(); return; }
	if (!FMath::IsFinite(DespawnShrinkStartServerTime) || !FMath::IsFinite(DespawnServerTime)
		|| DespawnShrinkStartServerTime < 0.0) return;
	const double Duration = DespawnServerTime - DespawnShrinkStartServerTime;
	if (!FMath::IsFinite(Duration) || Duration <= UE_SMALL_NUMBER) return;
	const double Now = GetServerTimeSeconds();
	if (Now < 0.0 || Now < DespawnShrinkStartServerTime) return;

	if (DespawnVisualOwner.Get() != PickupActor.Get())
	{
		RestoreDespawnScale();
		DespawnScalePivots.Reset();
		DespawnVisualOwner = PickupActor.Get();
		bDespawnVisualsInitialized = false;
	}
	if (!bDespawnVisualsInitialized)
	{
		// Attachment/registration failure is a graceful fallback to instant despawn,
		// not a reason to create and destroy components every frame.
		bDespawnVisualsInitialized = true;
		AActor* Item = PickupActor.Get();
		TArray<UMeshComponent*> Meshes;
		Item->GetComponents(Meshes);
		for (UMeshComponent* Mesh : Meshes)
		{
			if (!IsValid(Item) || Item->IsActorBeingDestroyed() || IsActorBeingDestroyed()) return;
			if (!IsValid(Mesh) || !Mesh->IsRegistered() || Mesh->GetOwner() != Item
				|| Mesh->Mobility != EComponentMobility::Movable || Mesh->IsSimulatingPhysics()
				|| Mesh->GetIsReplicated()) continue;
			USceneComponent* Parent = Mesh->GetAttachParent();
			if (!IsValid(Parent) || !Parent->IsRegistered() || Mesh == Item->GetRootComponent()) continue;
			// Nested meshes inherit their ancestor's scale; do not shrink them twice.
			bool bHasMeshAncestor = false;
			for (USceneComponent* Ancestor = Parent; IsValid(Ancestor); Ancestor = Ancestor->GetAttachParent())
			{
				if (Ancestor->IsA<UMeshComponent>()) { bHasMeshAncestor = true; break; }
			}
			if (bHasMeshAncestor) continue;

			// BP_DropItem.Spin writes the mesh transform every frame. Scale an
			// independent visual parent so its animation and pickup collision stay intact.
			USceneComponent* Pivot = NewObject<USceneComponent>(Item, NAME_None, RF_Transient);
			if (!IsValid(Pivot)) continue;
			Pivot->SetMobility(Mesh->Mobility);
			Pivot->SetupAttachment(Parent);
			Item->AddInstanceComponent(Pivot);
			Pivot->RegisterComponent();
			if (!IsValid(Item) || Item->IsActorBeingDestroyed() || IsActorBeingDestroyed()
				|| !IsValid(Mesh) || !IsValid(Parent) || !IsValid(Pivot))
			{
				if (IsValid(Pivot)) Pivot->DestroyComponent();
				return;
			}
			if (!Pivot->IsRegistered() || !Mesh->AttachToComponent(Pivot, FAttachmentTransformRules::KeepRelativeTransform))
			{
				Pivot->DestroyComponent();
				continue;
			}
			if (IsValid(Pivot)) DespawnScalePivots.Add(Pivot);
		}
		if (DespawnScalePivots.IsEmpty())
			UE_LOG(LogDeathDropCarrier, Warning, TEXT("No supported visual mesh for death drop shrink on %s; using instant despawn."),
				*GetNameSafe(Item));
	}

	// Avoid singular zero-scale transforms while the destruction packet is in flight.
	const float Scale = float(FMath::Clamp(
		(DespawnServerTime - Now) / Duration, 0.001, 1.0));
	for (const TWeakObjectPtr<USceneComponent>& Pivot : DespawnScalePivots)
		if (Pivot.IsValid() && Pivot->IsRegistered()) Pivot->SetRelativeScale3D(FVector(Scale));
}

void ADeathDropCarrier::RestoreDespawnScale()
{
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown) return;
	for (const TWeakObjectPtr<USceneComponent>& Pivot : DespawnScalePivots)
		if (Pivot.IsValid() && Pivot->IsRegistered()) Pivot->SetRelativeScale3D(FVector::OneVector);
}

void ADeathDropCarrier::OnRep_Pickup()
{
	// A replicated pickup may arrive after the carrier (or its Blueprint BeginPlay).
	bPickupConfigured = false;
}

void ADeathDropCarrier::OnRep_Settled()
{
	if (!bSettled || !IsValid(PhysicsBody) || SettledLocation.ContainsNaN()) return;
	PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PhysicsBody->SetSimulatePhysics(false);
	const FRotator UprightRotation(0.0f, GetActorRotation().Yaw, 0.0f);
	SetActorLocationAndRotation(SettledLocation, UprightRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void ADeathDropCarrier::OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || !bLaunched || bSettled || !IsValid(PhysicsBody)
		|| Hit.ImpactPoint.ContainsNaN() || Hit.ImpactNormal.ContainsNaN()) return;
	if (Hit.ImpactNormal.Z >= 0.6f)
	{
		// Reconstruct the sphere's resting center from the contact point instead of
		// freezing a penetrated physics position. Keep the pickup origin above ground.
		const float HeightOffset = FMath::IsFinite(DropSettings.SettlementHeightOffset)
			? FMath::Max(0.0f, DropSettings.SettlementHeightOffset) : 0.0f;
		SettledLocation = Hit.ImpactPoint + Hit.ImpactNormal * PhysicsBody->GetScaledSphereRadius()
			+ FVector::UpVector * HeightOffset;
		if (SettledLocation.ContainsNaN()) return;
		bSettled = true;
		OnRep_Settled();
		ForceNetUpdate();
	}
	else
	{
		// Do not restore the launch velocity into a wall on the next deceleration update.
		bHorizontalBlocked = true;
	}
}

void ADeathDropCarrier::FinishPickup()
{
	// BP_BuffItem attaches itself to the recipient and lives until its buff expires.
	// Destroying that actor here would also break its existing buff lifecycle.
	bWasPickedUp = true;
	Destroy();
}

void ADeathDropCarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const UWorld* World = GetWorld();
	if (!IsValid(World) || World->bIsTearingDown || IsActorBeingDestroyed()) return;
	if (!HasAuthority())
	{
		if (!bPickupConfigured && IsValid(PickupActor))
			bPickupConfigured = ConfigurePickup(PickupActor, true);
		if (bPickupConfigured) UpdateDespawnVisuals();
		return;
	}
	if (!IsValid(PickupActor)) { Destroy(); return; }
	if (IsPickedUp(PickupActor)) { FinishPickup(); return; }
	UpdateDespawnVisuals();
	if (!bLaunched || bSettled || World->bIsTearingDown || IsActorBeingDestroyed() || !IsValid(PhysicsBody)) return;
	const float DecelTime = FMath::IsFinite(DropSettings.HorizontalDecelTime)
		? FMath::Max(DropSettings.HorizontalDecelTime, 0.01f) : 1.2f;
	const float Exponent = FMath::IsFinite(DropSettings.HorizontalDecelExponent)
		? FMath::Max(DropSettings.HorizontalDecelExponent, 1.01f) : 2.0f;
	const float Progress = FMath::Clamp(
		float(World->GetTimeSeconds() - LaunchTime) / DecelTime,
		0.0f, 1.0f);
	const float Ratio = bHorizontalBlocked ? 0.0f :
		1.0f - FMath::Pow(Progress, Exponent);
	const FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
	PhysicsBody->SetPhysicsLinearVelocity(FVector(
		InitialHorizontalVelocity.X * Ratio, InitialHorizontalVelocity.Y * Ratio, Velocity.Z));
}

void ADeathDropCarrier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Restore a collected buff, but do not flash an expired pickup back to full
	// size while its destruction is still being replicated to the client.
	if (bWasPickedUp || IsPickedUp(PickupActor) || DespawnServerTime <= 0.0
		|| GetServerTimeSeconds() < DespawnServerTime)
		RestoreDespawnScale();
	if (HasAuthority() && !bWasPickedUp && IsValid(PickupActor) && !IsPickedUp(PickupActor))
		PickupActor->Destroy();
	Super::EndPlay(EndPlayReason);
}

void ADeathDropCarrier::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADeathDropCarrier, PickupActor);
	DOREPLIFETIME(ADeathDropCarrier, PickupItem);
	DOREPLIFETIME(ADeathDropCarrier, BuffEndServerTime);
	DOREPLIFETIME(ADeathDropCarrier, bBuffPickup);
	DOREPLIFETIME(ADeathDropCarrier, bSettled);
	DOREPLIFETIME(ADeathDropCarrier, SettledLocation);
	DOREPLIFETIME(ADeathDropCarrier, DespawnShrinkStartServerTime);
	DOREPLIFETIME(ADeathDropCarrier, DespawnServerTime);
}

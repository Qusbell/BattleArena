#include "Items/DeathDropCarrier.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogDeathDropCarrier, Log, All);

namespace
{
	bool SetBool(UObject* Object, const TCHAR* Name, bool Value)
	{
		FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), Name);
		if (!Property) return false;
		Property->SetPropertyValue_InContainer(Object, Value);
		return true;
	}

	bool IsPickedUp(const AActor* Actor)
	{
		const FBoolProperty* Property = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("IsPickedUp"));
		return Property && Property->GetPropertyValue_InContainer(Actor);
	}

	void CenterPickupSphere(AActor* Item)
	{
		TArray<USphereComponent*> Spheres;
		Item->GetComponents(Spheres);
		if (!Spheres.IsEmpty() && Spheres[0] != Item->GetRootComponent())
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
	PhysicsBody->OnComponentHit.AddDynamic(this, &ADeathDropCarrier::OnBodyHit);
	if (HasAuthority()) SpawnPickup();
}

bool ADeathDropCarrier::ConfigurePickup(AActor* Item, bool bUpdateMesh)
{
	if (!IsValid(Item) || !PickupItem.DataTable || PickupItem.RowName.IsNone()) return false;
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
		if (!Setup) return false;
		Item->ProcessEvent(Setup, nullptr);
	}
	return true;
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
	PhysicsBody->SetMassOverrideInKg(NAME_None, 1.0f, true);
	PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsBody->AddImpulse(Impulse); // One impulse, one kilogram; force settings have a consistent scale.
	InitialHorizontalVelocity = FVector(Impulse.X, Impulse.Y, 0.0f);
	LaunchTime = GetWorld()->GetTimeSeconds();
	bLaunched = true;
	SetLifeSpan(FMath::Max(DropSettings.DespawnTime, 0.01f));
}

void ADeathDropCarrier::OnRep_Pickup()
{
	// A replicated pickup may arrive after the carrier (or its Blueprint BeginPlay).
	bPickupConfigured = false;
}

void ADeathDropCarrier::OnRep_Settled()
{
	if (!bSettled) return;
	PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
	PhysicsBody->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	PhysicsBody->SetSimulatePhysics(false);
}

void ADeathDropCarrier::OnBodyHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || !bLaunched || bSettled) return;
	if (Hit.ImpactNormal.Z >= 0.6f)
	{
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
	if (!HasAuthority())
	{
		if (!bPickupConfigured && IsValid(PickupActor))
			bPickupConfigured = ConfigurePickup(PickupActor, true);
		return;
	}
	if (!IsValid(PickupActor)) { Destroy(); return; }
	if (IsPickedUp(PickupActor)) { FinishPickup(); return; }
	if (!bLaunched || bSettled) return;
	const float Progress = FMath::Clamp(
		float(GetWorld()->GetTimeSeconds() - LaunchTime) / FMath::Max(DropSettings.HorizontalDecelTime, 0.01f),
		0.0f, 1.0f);
	const float Ratio = bHorizontalBlocked ? 0.0f :
		1.0f - FMath::Pow(Progress, FMath::Max(DropSettings.HorizontalDecelExponent, 1.01f));
	const FVector Velocity = PhysicsBody->GetPhysicsLinearVelocity();
	PhysicsBody->SetPhysicsLinearVelocity(FVector(
		InitialHorizontalVelocity.X * Ratio, InitialHorizontalVelocity.Y * Ratio, Velocity.Z));
}

void ADeathDropCarrier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
}

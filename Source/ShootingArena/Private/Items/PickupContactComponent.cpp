#include "Items/PickupContactComponent.h"

#include "Character/CharacterPoolComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"

UPickupContactComponent::UPickupContactComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UPickupContactComponent::BeginPlay()
{
	Super::BeginPlay();
	AActor* Item = GetOwner();
	if (!IsValid(Item) || !Item->HasAuthority()) { SetComponentTickEnabled(false); return; }
	// Reinstanced Blueprint classes can temporarily retain an older inherited component.
	// Only the first component may intercept the shared overlap delegate.
	if (Item->FindComponentByClass<UPickupContactComponent>() != this) { SetComponentTickEnabled(false); return; }
	TArray<USphereComponent*> Spheres;
	Item->GetComponents(Spheres);
	for (USphereComponent* Candidate : Spheres)
		if (Candidate && Candidate->GetFName() == TEXT("SphereCollision")) { Sphere = Candidate; break; }
	if (!Sphere) { UE_LOG(LogTemp, Error, TEXT("Pickup contact: missing SphereCollision on %s"), *GetNameSafe(Item)); SetComponentTickEnabled(false); return; }
	const float Scale = Sphere->GetShapeScale();
	const float ConfigRadius = GetDefault<UPickupContactSettings>()->Radius;
	if (FMath::IsFinite(ConfigRadius) && ConfigRadius > 0.f && Scale > UE_SMALL_NUMBER)
		Sphere->SetSphereRadius(ConfigRadius / Scale, false);
	if (const auto Existing = Sphere->OnComponentBeginOverlap.GetShared())
		OriginalOverlap = MakeShared<FComponentBeginOverlapSignature_MCSignature>(*Existing);
	Sphere->OnComponentBeginOverlap.Clear();
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &UPickupContactComponent::OnContact);
	PreviousLocation = Sphere->GetComponentLocation();
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// Collision can be enabled after registration while a pawn already overlaps.
	TArray<UPrimitiveComponent*> Overlaps;
	Sphere->GetOverlappingComponents(Overlaps);
	for (UPrimitiveComponent* Other : Overlaps) if (Other) Enqueue(Other->GetOwner(), Other);
}

bool UPickupContactComponent::IsPickedUp() const
{
	const AActor* Item = GetOwner();
	const FBoolProperty* Property = IsValid(Item) ? FindFProperty<FBoolProperty>(Item->GetClass(), TEXT("IsPickedUp")) : nullptr;
	return Property && Property->GetPropertyValue_InContainer(Item);
}

void UPickupContactComponent::Enqueue(AActor* Other, UPrimitiveComponent* OtherComponent)
{
	if (!IsValid(Other) || !Other->IsA<APawn>() || UCharacterPoolComponent::IsInactive(Other) || IsPickedUp()) return;
	if (Pending.ContainsByPredicate([Other](const FContact& Contact) { return Contact.Actor.Get() == Other; })) return;
	Pending.Add({Other, OtherComponent, GetWorld()->GetTimeSeconds()});
	if (bReady && !bProcessing) ProcessContacts();
}

void UPickupContactComponent::OnContact(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent* OtherComponent,
	int32, bool, const FHitResult&)
{
	Enqueue(Other, OtherComponent);
}

void UPickupContactComponent::ProcessContacts()
{
	if (bProcessing || !IsValid(Sphere) || !IsValid(GetOwner())) return;
	TGuardValue<bool> Guard(bProcessing, true);
	while (!Pending.IsEmpty() && IsValid(GetOwner()) && !GetOwner()->IsActorBeingDestroyed() && !IsPickedUp())
	{
		const FContact Contact = Pending[0];
		Pending.RemoveAt(0);
		AActor* Other = Contact.Actor.Get();
		if (!IsValid(Other) || UCharacterPoolComponent::IsInactive(Other)) continue;
		// Expiry only cancels a past contact; a pawn still overlapping remains eligible.
		if (GetWorld()->GetTimeSeconds() - Contact.Time > 0.1 && !Sphere->IsOverlappingActor(Other)) continue;
		FHitResult Hit;
		if (OriginalOverlap) OriginalOverlap->Broadcast(Sphere, Other, Contact.Component.Get(), 0, false, Hit);
	}
	if (IsPickedUp()) Pending.Reset();
}

void UPickupContactComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
	Super::TickComponent(DeltaTime, TickType, TickFunction);
	if (!IsValid(Sphere) || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed() || IsPickedUp()) return;
	// Tick occurs after Blueprint BeginPlay and the deferred-spawn caller returns.
	// Registration can finish after BeginPlay, so inspect overlaps once more now.
	if (!bReady)
	{
		bReady = true;
		TArray<UPrimitiveComponent*> Overlaps;
		Sphere->GetOverlappingComponents(Overlaps);
		for (UPrimitiveComponent* Other : Overlaps)
		{
			if (!IsValid(Sphere) || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed() || IsPickedUp()) break;
			if (Other) Enqueue(Other->GetOwner(), Other);
		}
	}
	ProcessContacts();
	if (!IsValid(Sphere) || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed() || IsPickedUp()) return;
	const FVector Current = Sphere->GetComponentLocation();
	if (!Current.Equals(PreviousLocation, 0.01f))
	{
		FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(PickupSweep), false, GetOwner());
		TArray<FHitResult> Hits;
		GetWorld()->SweepMultiByObjectType(Hits, PreviousLocation, Current, FQuat::Identity, Objects,
			FCollisionShape::MakeSphere(Sphere->GetScaledSphereRadius()), Query);
		for (const FHitResult& Hit : Hits)
		{
			if (!IsValid(Sphere) || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed() || IsPickedUp()) break;
			UPrimitiveComponent* Component = Hit.GetComponent();
			if (Component && Sphere->GetCollisionResponseToChannel(Component->GetCollisionObjectType()) == ECR_Overlap
				&& Component->GetCollisionResponseToChannel(Sphere->GetCollisionObjectType()) != ECR_Ignore)
				Enqueue(Hit.GetActor(), Component);
		}
	}
	PreviousLocation = Current;
}

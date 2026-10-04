#include "Weapon/ProjectileFirstPersonRenderingComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "UObject/UnrealType.h"
#include "Weapon/ProjectileConvergenceComponent.h"

namespace
{
UObject* Field(const UObject* Object, FName Name)
{
	const FObjectPropertyBase* P = IsValid(Object) ? FindFProperty<FObjectPropertyBase>(Object->GetClass(), Name) : nullptr;
	return P ? P->GetObjectPropertyValue_InContainer(Object) : nullptr;
}
bool UsesFirstPersonRendering(AActor* Weapon)
{
	const UProjectileConvergenceSettings* Settings = IsValid(Weapon) ? Weapon->FindComponentByClass<UProjectileConvergenceSettings>() : nullptr;
	const FBoolProperty* Mode = IsValid(Weapon) ? FindFProperty<FBoolProperty>(Weapon->GetClass(), TEXT("bUseMuzzleProjectileAim")) : nullptr;
	return Settings && Settings->bUseFirstPersonRendering && Mode && Mode->GetPropertyValue_InContainer(Weapon);
}
bool Eligible(AActor* Weapon)
{
	return UsesFirstPersonRendering(Weapon) && !Weapon->IsHidden();
}
}

UProjectileFirstPersonRenderingComponent::UProjectileFirstPersonRenderingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UProjectileFirstPersonRenderingComponent::Track(UPrimitiveComponent* Primitive)
{
	if (!IsValid(Primitive)) return;
	if (!PreviousTypes.Contains(Primitive)) PreviousTypes.Add(Primitive, Primitive->FirstPersonPrimitiveType);
	if (Primitive->FirstPersonPrimitiveType != EFirstPersonPrimitiveType::FirstPerson)
		Primitive->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
}

void UProjectileFirstPersonRenderingComponent::RestoreView()
{
	for (const auto& Pair : PreviousTypes) if (UPrimitiveComponent* Primitive = Pair.Key.Get()) Primitive->SetFirstPersonPrimitiveType(Pair.Value);
	PreviousTypes.Reset();
	if (UCameraComponent* SavedCamera = Camera.Get())
	{
		SavedCamera->SetFirstPersonScale(PreviousScale);
		SavedCamera->SetEnableFirstPersonScale(bPreviousEnableScale);
	}
	Camera.Reset(); ActiveWeapon.Reset();
}

void UProjectileFirstPersonRenderingComponent::Restore()
{
	RestoreView();
	for (const auto& Pair : PreparedGunTypes)
		if (UPrimitiveComponent* Gun = Pair.Key.Get()) Gun->SetFirstPersonPrimitiveType(Pair.Value);
	PreparedGunTypes.Reset(); PreparedMeshes.Reset(); NextTextureRefreshTime = 0.f;
}

void UProjectileFirstPersonRenderingComponent::PrepareWeapons(USkeletalMeshComponent* Arms)
{
	TArray<USceneComponent*> Children; Arms->GetChildrenComponents(false, Children);
	TSet<UPrimitiveComponent*> RetainedGuns;
	TSet<USkeletalMeshComponent*> RetainedMeshes;
	const float Now = GetWorld()->GetTimeSeconds();
	const bool bRefreshTextures = Now >= NextTextureRefreshTime;
	for (USceneComponent* Child : Children)
	{
		AActor* Weapon = IsValid(Child) ? Child->GetOwner() : nullptr;
		USkeletalMeshComponent* Gun = Cast<USkeletalMeshComponent>(Child);
		if (!IsValid(Weapon) || !Gun || Gun != Field(Weapon, TEXT("FP_Mesh"))) continue;
		if (UsesFirstPersonRendering(Weapon))
		{
			RetainedGuns.Add(Gun);
			if (!PreparedGunTypes.Contains(Gun)) PreparedGunTypes.Add(Gun, Gun->FirstPersonPrimitiveType);
			Gun->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		}
		RetainedMeshes.Add(Gun);
		USkeletalMesh* Mesh = Gun->GetSkeletalMeshAsset();
		const bool bMeshChanged = !PreparedMeshes.Contains(Gun) || PreparedMeshes[Gun].Get() != Mesh;
		if (bMeshChanged)
		{
			PreparedMeshes.Add(Gun, Mesh);
			Gun->PrecachePSOs();
		}
		// Common preparation for every owned FP gun, including hitscan weapons.
		// The projectile-only rendering-mode switch above does not gate these requests.
		// Skeletal mesh LOD buffers also stream; warming only textures leaves geometry cold.
		if (IsValid(Mesh) && (bMeshChanged || bRefreshTextures)) Mesh->SetForceMipLevelsToBeResident(5.f);
		// Hidden inventory weapons are prepared without displaying or equipping them.
		// Do not prioritize character textures: that would pause world streaming for 30 frames.
		if (bPrestreamOwnedWeaponTextures && (bMeshChanged || bRefreshTextures)) Gun->PrestreamTextures(5.f, false);
	}
	if (bRefreshTextures) NextTextureRefreshTime = Now + 3.f;
	for (auto It = PreparedGunTypes.CreateIterator(); It; ++It)
	{
		UPrimitiveComponent* Gun = It.Key().Get();
		if (!Gun || !RetainedGuns.Contains(Gun))
		{
			if (Gun) Gun->SetFirstPersonPrimitiveType(It.Value());
			It.RemoveCurrent();
		}
	}
	for (auto It = PreparedMeshes.CreateIterator(); It; ++It)
		if (!It.Key().IsValid() || !RetainedMeshes.Contains(It.Key().Get())) It.RemoveCurrent();
}

void UProjectileFirstPersonRenderingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	APawn* Pawn = Cast<APawn>(GetOwner());
	USkeletalMeshComponent* Arms = Cast<USkeletalMeshComponent>(Field(Pawn, TEXT("FP_Mesh")));
	UCameraComponent* View = Cast<UCameraComponent>(Field(Pawn, TEXT("FP_Camera")));
	AActor* Weapon = nullptr;
	UPrimitiveComponent* Gun = nullptr;
	if (!bEnabled || !Pawn || !Pawn->IsLocallyControlled() || !IsValid(Arms) || !IsValid(View) || !View->IsActive())
	{
		Restore();
		return;
	}
	PrepareWeapons(Arms);
	{
		TArray<USceneComponent*> Children; Arms->GetChildrenComponents(false, Children);
		for (USceneComponent* Child : Children)
		{
			AActor* Candidate = IsValid(Child) ? Child->GetOwner() : nullptr;
			if (Eligible(Candidate) && !Child->bHiddenInGame && Child == Field(Candidate, TEXT("FP_Mesh")))
			{ Weapon = Candidate; Gun = Cast<UPrimitiveComponent>(Child); break; }
		}
	}
	// Keep the arm/camera state across plasma <-> rocket switches. Recreating the
	// same render state at every swap discards renderer history unnecessarily.
	if (!Weapon || View != Camera.Get()) RestoreView();
	if (!Weapon || !Gun) return;
	if (!Camera.IsValid())
	{
		Camera = View; ActiveWeapon = Weapon;
		PreviousScale = View->FirstPersonScale; bPreviousEnableScale = View->bEnableFirstPersonScale;
	}
	ActiveWeapon = Weapon;
	View->SetEnableFirstPersonScale(true);
	View->SetFirstPersonScale(FMath::IsFinite(FirstPersonScale) ? FMath::Clamp(FirstPersonScale, 0.001f, 1.f) : 0.25f);
	Track(Arms);
	TArray<USceneComponent*> Effects; Gun->GetChildrenComponents(true, Effects);
	for (USceneComponent* Effect : Effects) Track(Cast<UPrimitiveComponent>(Effect));
	// Prune expired one-shot effects so sustained firing does not grow the map indefinitely.
	for (auto It = PreviousTypes.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
}

void UProjectileFirstPersonRenderingComponent::ApplyFirstPersonMuzzleEffect(AActor* Weapon, UPrimitiveComponent* Effect)
{
	if (!Eligible(Weapon) || !IsValid(Effect)) return;
	UPrimitiveComponent* Gun = Cast<UPrimitiveComponent>(Field(Weapon, TEXT("FP_Mesh")));
	if (!Gun || Effect->GetAttachParent() != Gun) return;
	APawn* Pawn = Cast<APawn>(Gun->GetAttachParent() ? Gun->GetAttachParent()->GetOwner() : nullptr);
	if (Pawn && Pawn->IsLocallyControlled())
		if (auto* Rendering = Pawn->FindComponentByClass<UProjectileFirstPersonRenderingComponent>())
			if (Rendering->bEnabled) Rendering->Track(Effect);
}

void UProjectileFirstPersonRenderingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Restore();
	Super::EndPlay(EndPlayReason);
}

#include "Weapon/WeaponProjectileAimLibrary.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapon/ProjectileConvergenceComponent.h"
#include "UObject/UnrealType.h"

namespace
{
float ProjectileRadius(const USphereComponent* Collision)
{
	return Collision ? static_cast<float>(Collision->GetUnscaledSphereRadius() * Collision->GetRelativeScale3D().GetAbsMax()) : 0.f;
}
FCollisionQueryParams ProjectileQuery(AActor* Weapon, AActor* Shooter)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectileMuzzleOverlap), false);
	Params.AddIgnoredActor(Weapon); Params.AddIgnoredActor(Shooter);
	if (Weapon) { Params.AddIgnoredActor(Weapon->GetOwner()); Params.AddIgnoredActor(Weapon->GetInstigator()); }
	return Params;
}
void BlockingEnvironmentOverlaps(UWorld* World, const USphereComponent* Collision, FVector Start,
	float Radius, AActor* Weapon, AActor* Shooter, TArray<FOverlapResult>& Out)
{
	World->OverlapMultiByChannel(Out, Start, FQuat::Identity, Collision->GetCollisionObjectType(),
		FCollisionShape::MakeSphere(Radius), ProjectileQuery(Weapon, Shooter), FCollisionResponseParams(Collision->GetCollisionResponseToChannels()));
	Out.RemoveAll([Collision](const FOverlapResult& Hit)
	{
		return !Hit.bBlockingHit || !IsValid(Hit.GetComponent()) || Hit.GetComponent()->GetCollisionObjectType() == ECC_Pawn
			|| Hit.GetComponent()->GetCollisionObjectType() == Collision->GetCollisionObjectType()
			|| (Hit.GetActor() && Hit.GetActor()->IsA<APawn>());
	});
}
bool CameraCorrectionPathClear(UWorld* World, const USphereComponent* Collision, FVector CameraStart,
	FVector Candidate, AActor* Weapon, AActor* Shooter)
{
	// Only the short eye-to-correction segment. Do not relocate through solid geometry.
	return !World->LineTraceTestByChannel(CameraStart, Candidate, Collision->GetCollisionObjectType(),
		ProjectileQuery(Weapon, Shooter), FCollisionResponseParams(Collision->GetCollisionResponseToChannels()));
}
}

FVector UWeaponProjectileAimLibrary::ResolveProjectileViewStart(AActor* Shooter,
	USceneComponent* SelectedMesh, FVector AuthoritativeStart, bool bCameraFallback)
{
	// A server-resolved camera fallback must never be replaced with a blocked FP/TP muzzle.
	if (bCameraFallback) return AuthoritativeStart;
	const APawn* Pawn = Cast<APawn>(Shooter);
	// SelectMesh already selects FP for the local shooter and TP for remote shooters.
	// Every non-authority view needs its rendered muzzle, including remote observers.
	if (IsValid(Pawn) && !Pawn->HasAuthority()
		&& IsValid(SelectedMesh) && SelectedMesh->DoesSocketExist(TEXT("Muzzle")))
	{
		const FVector Muzzle = SelectedMesh->GetSocketLocation(TEXT("Muzzle"));
		AActor* Weapon = SelectedMesh->GetOwner();
		UProjectileConvergenceSettings* Settings = IsValid(Weapon) ? Weapon->FindComponentByClass<UProjectileConvergenceSettings>() : nullptr;
		const FObjectPropertyBase* CameraProperty = FindFProperty<FObjectPropertyBase>(Pawn->GetClass(), TEXT("FP_Camera"));
		const UCameraComponent* Camera = CameraProperty ? Cast<UCameraComponent>(CameraProperty->GetObjectPropertyValue_InContainer(Pawn)) : nullptr;
		if (Camera && Settings && Settings->bUseCameraWhenMuzzleBlocked)
		{
			bool bLocalFallback;
			return ResolveSafeProjectileStart(Weapon, Shooter, Settings, Camera->GetComponentLocation(), Muzzle, bLocalFallback);
		}
		return Muzzle;
	}
	return AuthoritativeStart;
}

bool UWeaponProjectileAimLibrary::TryGetProjectileBoreTransform(USceneComponent* Mesh,
	FName MuzzleSocket, FName AimDirectionSocket, FTransform& OutTransform)
{
	OutTransform = FTransform::Identity;
	if (!IsValid(Mesh) || !Mesh->DoesSocketExist(MuzzleSocket)) return false;
	const FName DirectionSocket = AimDirectionSocket.IsNone() ? MuzzleSocket : AimDirectionSocket;
	// A missing explicitly configured direction must not silently reuse an edited FX rotation.
	if (!Mesh->DoesSocketExist(DirectionSocket)) return false;
	OutTransform = Mesh->GetSocketTransform(MuzzleSocket);
	OutTransform.SetRotation(Mesh->GetSocketTransform(DirectionSocket).GetRotation());
	return !OutTransform.ContainsNaN();
}

const USphereComponent* UWeaponProjectileAimLibrary::GetProjectileCollisionTemplate(AActor* Weapon)
{
	if (!IsValid(Weapon)) return nullptr;
	const FObjectPropertyBase* DataProperty = FindFProperty<FObjectPropertyBase>(Weapon->GetClass(), TEXT("weaponData"));
	UObject* Data = DataProperty ? DataProperty->GetObjectPropertyValue_InContainer(Weapon) : nullptr;
	const FClassProperty* ClassProperty = IsValid(Data) ? FindFProperty<FClassProperty>(Data->GetClass(), TEXT("projectileClass")) : nullptr;
	UClass* Class = ClassProperty ? Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Data)) : nullptr;
	return Class && Class->IsChildOf<AActor>() ? AActor::GetActorClassDefaultComponent<USphereComponent>(Class) : nullptr;
}

bool UWeaponProjectileAimLibrary::IsProjectileStartBlocked(UWorld* World, const USphereComponent* Collision,
	FVector Start, AActor* Weapon, AActor* Shooter)
{
	if (!World || !IsValid(Collision) || Start.ContainsNaN()) return false;
	const float Radius = ProjectileRadius(Collision);
	if (!FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER) return false;
	TArray<FOverlapResult> Overlaps;
	BlockingEnvironmentOverlaps(World, Collision, Start, Radius, Weapon, Shooter, Overlaps);
	return !Overlaps.IsEmpty();
}

bool UWeaponProjectileAimLibrary::IsProjectileMuzzleObstructed(UWorld* World, const USphereComponent* Collision,
	FVector CameraStart, FVector MuzzleStart, AActor* Weapon, AActor* Shooter)
{
	if (!World || !IsValid(Collision) || CameraStart.ContainsNaN() || MuzzleStart.ContainsNaN()) return false;
	if (IsProjectileStartBlocked(World, Collision, MuzzleStart, Weapon, Shooter)) return true;
	// The muzzle can be clear on the far side of a thin wall. Check only the line
	// from the eye to the muzzle, using projectile responses and ignoring normal pawn/projectile contacts.
	FCollisionResponseParams Responses(Collision->GetCollisionResponseToChannels());
	Responses.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);
	Responses.CollisionResponse.SetResponse(Collision->GetCollisionObjectType(), ECR_Ignore);
	return World->LineTraceTestByChannel(CameraStart, MuzzleStart, Collision->GetCollisionObjectType(),
		ProjectileQuery(Weapon, Shooter), Responses);
}

bool UWeaponProjectileAimLibrary::TryResolveCameraProjectileStart(UWorld* World, const USphereComponent* Collision,
	FVector CameraStart, AActor* Weapon, AActor* Shooter, FVector& SafeStart)
{
	SafeStart = CameraStart;
	const float Radius = ProjectileRadius(Collision);
	if (!World || !IsValid(Collision) || CameraStart.ContainsNaN() || !FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER) return false;
	TArray<FOverlapResult> PointOverlaps;
	BlockingEnvironmentOverlaps(World, Collision, CameraStart, 0.1f, Weapon, Shooter, PointOverlaps);
	// A camera embedded in a wall must not teleport a shot out through the other side.
	if (!PointOverlaps.IsEmpty()) return false;
	FVector Candidate = CameraStart;
	constexpr float Clearance = 0.5f;
	for (int32 Attempt = 0; Attempt < 4; ++Attempt)
	{
		TArray<FOverlapResult> Overlaps;
		BlockingEnvironmentOverlaps(World, Collision, Candidate, Radius, Weapon, Shooter, Overlaps);
		if (Overlaps.IsEmpty())
		{
			if (!CameraCorrectionPathClear(World, Collision, CameraStart, Candidate, Weapon, Shooter)) return false;
			SafeStart = Candidate; return true;
		}
		bool bMoved = false;
		for (const FOverlapResult& Hit : Overlaps)
		{
			FMTDResult Penetration;
			if (Hit.GetComponent()->ComputePenetration(Penetration, FCollisionShape::MakeSphere(Radius), Candidate, FQuat::Identity)
				&& FMath::IsFinite(Penetration.Distance) && Penetration.Distance > 0.f
				&& !Penetration.Direction.ContainsNaN() && !Penetration.Direction.IsNearlyZero())
			{
				Candidate += Penetration.Direction.GetSafeNormal() * (Penetration.Distance + Clearance);
				if (FVector::DistSquared(CameraStart, Candidate) > FMath::Square(Radius + Clearance)) return false;
				bMoved = true;
			}
		}
		if (!bMoved) return false;
	}
	if (!IsProjectileStartBlocked(World, Collision, Candidate, Weapon, Shooter)
		&& CameraCorrectionPathClear(World, Collision, CameraStart, Candidate, Weapon, Shooter))
	{ SafeStart = Candidate; return true; }
	return false;
}

FVector UWeaponProjectileAimLibrary::ResolveSafeProjectileStart(AActor* Weapon, AActor* Shooter,
	UProjectileConvergenceSettings* Settings, FVector CameraStart, FVector MuzzleStart, bool& bCameraFallback)
{
	bCameraFallback = false;
	if (!IsValid(Weapon) || !IsValid(Settings) || !Settings->bUseCameraWhenMuzzleBlocked
		|| CameraStart.ContainsNaN() || MuzzleStart.ContainsNaN()) return MuzzleStart;
	const FBoolProperty* Mode = FindFProperty<FBoolProperty>(Weapon->GetClass(), TEXT("bUseMuzzleProjectileAim"));
	if (!Mode || !Mode->GetPropertyValue_InContainer(Weapon)) return MuzzleStart;
	const USphereComponent* Collision = GetProjectileCollisionTemplate(Weapon);
	if (!IsProjectileMuzzleObstructed(Weapon->GetWorld(), Collision, CameraStart, MuzzleStart, Weapon, Shooter)) return MuzzleStart;
	bCameraFallback = true;
	// Use the actual eye origin, with no arbitrary forward offset that could cross the wall.
	// The existing eye has a lateral offset. If the projectile itself cannot fit there,
	// try the pawn centre at the same eye height, rather than moving forward through a wall.
	if (IsValid(Shooter) && IsProjectileStartBlocked(Weapon->GetWorld(), Collision, CameraStart, Weapon, Shooter))
	{
		const FVector CentredEye(Shooter->GetActorLocation().X, Shooter->GetActorLocation().Y, CameraStart.Z);
		if (!IsProjectileStartBlocked(Weapon->GetWorld(), Collision, CentredEye, Weapon, Shooter)
			&& CameraCorrectionPathClear(Weapon->GetWorld(), Collision, CameraStart, CentredEye, Weapon, Shooter)) return CentredEye;
		FVector CorrectedEye;
		if (TryResolveCameraProjectileStart(Weapon->GetWorld(), Collision, CameraStart, Weapon, Shooter, CorrectedEye)) return CorrectedEye;
	}
	return CameraStart;
}

void UWeaponProjectileAimLibrary::ResolveMuzzleProjectileAim(const UObject* WorldContextObject,
	AActor* Weapon, AActor* Shooter, USceneComponent* MuzzleMesh, FVector CameraStart,
	FVector CameraEnd, TEnumAsByte<ETraceTypeQuery> TraceChannel,
	FVector& ProjectileStart, FVector& AimTarget)
{
	ProjectileStart = CameraStart;
	AimTarget = CameraEnd;
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject,
		EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World || CameraStart.ContainsNaN() || CameraEnd.ContainsNaN())
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(MuzzleProjectileAim), false);
	Params.AddIgnoredActor(Weapon);
	Params.AddIgnoredActor(Shooter);
	if (Weapon)
	{
		Params.AddIgnoredActor(Weapon->GetOwner());
		Params.AddIgnoredActor(Weapon->GetInstigator());
	}
	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, CameraStart, CameraEnd,
		UEngineTypes::ConvertToCollisionChannel(TraceChannel), Params))
	{
		AimTarget = Hit.ImpactPoint;
	}

	const FName MuzzleSocket(TEXT("Muzzle"));
	if (IsValid(MuzzleMesh) && MuzzleMesh->DoesSocketExist(MuzzleSocket))
	{
		ProjectileStart = MuzzleMesh->GetSocketLocation(MuzzleSocket);
	}
	// Avoid a zero-length look-at when the camera hit lies exactly at the muzzle.
	if (ProjectileStart.Equals(AimTarget, KINDA_SMALL_NUMBER))
	{
		AimTarget = ProjectileStart + (CameraEnd - CameraStart).GetSafeNormal();
	}
}

bool UWeaponProjectileAimLibrary::FindMuzzleRayJoin(FVector CameraStart, FVector CameraForward,
	FVector MuzzleStart, FVector MuzzleForward, double MaxDistance, double Tolerance, FVector& JoinPoint)
{
	if (CameraStart.ContainsNaN() || CameraForward.ContainsNaN() || MuzzleStart.ContainsNaN()
		|| MuzzleForward.ContainsNaN() || !FMath::IsFinite(MaxDistance) || !FMath::IsFinite(Tolerance)) return false;
	const FVector A = CameraForward.GetSafeNormal(), B = MuzzleForward.GetSafeNormal();
	if (A.IsNearlyZero() || B.IsNearlyZero()) return false;
	const double Dot = FVector::DotProduct(A, B), Denom = 1.0 - Dot * Dot;
	if (Denom <= 1.e-6) return false; // Parallel or nearly parallel rays have no stable join point.
	const FVector W = CameraStart - MuzzleStart;
	const double D = FVector::DotProduct(A, W), E = FVector::DotProduct(B, W);
	const double CameraDistance = (Dot * E - D) / Denom;
	const double MuzzleDistance = (E - Dot * D) / Denom;
	if (CameraDistance <= 0.0 || MuzzleDistance <= 0.0 || CameraDistance > MaxDistance
		|| MuzzleDistance > MaxDistance) return false;
	const FVector CameraPoint = CameraStart + A * CameraDistance;
	const FVector MuzzlePoint = MuzzleStart + B * MuzzleDistance;
	if (FVector::Distance(CameraPoint, MuzzlePoint) > FMath::Max(0.0, Tolerance)) return false;
	JoinPoint = CameraPoint;
	return true;
}

void UWeaponProjectileAimLibrary::ResolveProjectileConvergence(UProjectileConvergenceSettings* Settings,
	USceneComponent* MuzzleMesh, FVector CameraStart, FVector CameraEnd, FVector ProjectileStart,
	FVector AimTarget, FVector& FlightTarget, FVector& FlightForward, bool& bConverge, bool bCameraFallback)
{
	FlightTarget = AimTarget;
	FlightForward = (CameraEnd - CameraStart).GetSafeNormal();
	bConverge = false;
	// A fallback shot goes directly from the safe eye origin to the camera's blocking hit.
	// Never bend it back toward the blocked barrel's ray intersection.
	if (bCameraFallback) return;
	if (!IsValid(Settings) || !Settings->bEnabled || CameraStart.ContainsNaN() || CameraEnd.ContainsNaN()
		|| ProjectileStart.ContainsNaN() || AimTarget.ContainsNaN() || FlightForward.IsNearlyZero()
		|| !FMath::IsFinite(Settings->Distance)) return;
	const double HitDistance = FVector::DotProduct(AimTarget - CameraStart, FlightForward);
	const double MuzzleDistance = FVector::DotProduct(ProjectileStart - CameraStart, FlightForward);
	// A target in front of the muzzle but before the requested join must still be hit normally.
	if (HitDistance <= MuzzleDistance + 1.0) return;
	FVector Join = CameraStart + FlightForward * FMath::Min<double>(FMath::Max(1.f, Settings->Distance), HitDistance);
	FTransform BoreTransform;
	if (Settings->Mode == EProjectileConvergenceMode::MuzzleRayIntersection
		&& TryGetProjectileBoreTransform(MuzzleMesh, TEXT("Muzzle"), Settings->AimDirectionSocket, BoreTransform))
	{
		const FVector MuzzleForward = BoreTransform
			.TransformVectorNoScale(Settings->MuzzleDirectionOffset.Vector()).GetSafeNormal();
		FVector Intersection;
		if (FindMuzzleRayJoin(CameraStart, FlightForward, ProjectileStart, MuzzleForward,
			FMath::Min<double>(Settings->MaxIntersectionDistance, HitDistance), Settings->IntersectionTolerance, Intersection)
			&& FVector::DotProduct(Intersection - ProjectileStart, FlightForward)
				>= FMath::Max(0.f, Settings->MinimumIntersectionForwardDistance))
			Join = Intersection;
	}
	if (FVector::DotProduct(Join - ProjectileStart, FlightForward) <= 0.0) return;
	FlightTarget = Join;
	bConverge = true;
}

bool UWeaponProjectileAimLibrary::ConfigureProjectileConvergence(AActor* Projectile,
	FVector JoinPoint, FVector FlightForward, bool bConverge)
{
	if (!bConverge || !IsValid(Projectile)) return false;
	if (Projectile->FindComponentByClass<UProjectileConvergenceComponent>()) return false;
	UProjectileMovementComponent* Movement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
	UProjectileConvergenceComponent* Driver = NewObject<UProjectileConvergenceComponent>(Projectile);
	if (!Driver->Initialize(Movement, JoinPoint, FlightForward)) return false;
	Projectile->AddInstanceComponent(Driver);
	Driver->RegisterComponent();
	return true;
}

#include "Weapon/WeaponProjectileAimLibrary.h"

#include "Components/SceneComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapon/ProjectileConvergenceComponent.h"

FVector UWeaponProjectileAimLibrary::ResolveProjectileViewStart(AActor* Shooter,
	USceneComponent* SelectedMesh, FVector AuthoritativeStart)
{
	const APawn* Pawn = Cast<APawn>(Shooter);
	// SelectMesh already selects FP for the local shooter and TP for remote shooters.
	// Every non-authority view needs its rendered muzzle, including remote observers.
	if (IsValid(Pawn) && !Pawn->HasAuthority()
		&& IsValid(SelectedMesh) && SelectedMesh->DoesSocketExist(TEXT("Muzzle")))
	{
		return SelectedMesh->GetSocketLocation(TEXT("Muzzle"));
	}
	return AuthoritativeStart;
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
	FVector AimTarget, FVector& FlightTarget, FVector& FlightForward, bool& bConverge)
{
	FlightTarget = AimTarget;
	FlightForward = (CameraEnd - CameraStart).GetSafeNormal();
	bConverge = false;
	if (!IsValid(Settings) || !Settings->bEnabled || CameraStart.ContainsNaN() || CameraEnd.ContainsNaN()
		|| ProjectileStart.ContainsNaN() || AimTarget.ContainsNaN() || FlightForward.IsNearlyZero()
		|| !FMath::IsFinite(Settings->Distance)) return;
	const double HitDistance = FVector::DotProduct(AimTarget - CameraStart, FlightForward);
	const double MuzzleDistance = FVector::DotProduct(ProjectileStart - CameraStart, FlightForward);
	// A target in front of the muzzle but before the requested join must still be hit normally.
	if (HitDistance <= MuzzleDistance + 1.0) return;
	FVector Join = CameraStart + FlightForward * FMath::Min<double>(FMath::Max(1.f, Settings->Distance), HitDistance);
	if (Settings->Mode == EProjectileConvergenceMode::MuzzleRayIntersection && IsValid(MuzzleMesh)
		&& MuzzleMesh->DoesSocketExist(TEXT("Muzzle")))
	{
		const FVector MuzzleForward = MuzzleMesh->GetSocketTransform(TEXT("Muzzle"))
			.TransformVectorNoScale(Settings->MuzzleDirectionOffset.Vector()).GetSafeNormal();
		FVector Intersection;
		if (FindMuzzleRayJoin(CameraStart, FlightForward, ProjectileStart, MuzzleForward,
			FMath::Min<double>(Settings->MaxIntersectionDistance, HitDistance), Settings->IntersectionTolerance, Intersection))
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

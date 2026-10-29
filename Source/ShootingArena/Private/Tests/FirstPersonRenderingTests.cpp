#if WITH_DEV_AUTOMATION_TESTS

#include "Weapon/FirstPersonRenderingMath.h"
#include "Weapon/ProjectileFirstPersonRenderingComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonCapsuleFitTest, "ShootingArena.Weapon.FirstPersonCapsuleFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstPersonCapsuleFitTest::RunTest(const FString& Parameters)
{
	const FVector Centre(70, -100, 150), Up = FVector::UpVector;
	const double Radius = 42, HalfHeight = 96, Margin = 2;
	// Check rendered points against the physical capsule independently of the fit
	// formula: side walls, floor/ceiling, and a camera near the capsule's upper cap.
	for (const FVector EyeOffset : {FVector::ZeroVector, FVector(3, 10, 80)})
		for (const double Extent : {30.0, 150.0, 350.0})
			for (const double FOV : {1.0, 1.5})
			{
				const FVector Eye = Centre + EyeOffset;
				const float Scale = FirstPersonRendering::FitScaleToCapsule(0.25f, Eye, Extent, Centre, Up, Radius, HalfHeight, Margin, FOV, 0.001f);
				TestTrue(TEXT("Fit never expands the configured maximum"), Scale > 0 && Scale <= 0.25f);
				for (const FVector Direction : {FVector(1,0,0), FVector(-1,0,0), FVector(0,1,0), FVector(0,-1,0),
					FVector(0,0,1), FVector(0,0,-1), FVector(1,1,1).GetSafeNormal(), FVector(-1,-1,-1).GetSafeNormal()})
				{
					const FVector Rendered = Eye + Direction * Extent * FOV * Scale;
					const double Distance = FMath::PointDistToSegment(Rendered,
						Centre - Up * (HalfHeight - Radius), Centre + Up * (HalfHeight - Radius));
					TestTrue(TEXT("Rendered bound remains inside the capsule's safety margin"), Distance <= Radius - Margin + 0.001);
				}
			}
	TestEqual(TEXT("A camera outside the capsule retains the configured fallback"),
		FirstPersonRendering::FitScaleToCapsule(0.25f, Centre + FVector(100,0,0), 200, Centre, Up, Radius, HalfHeight, Margin), 0.25f);
	TestEqual(TEXT("Near-edge fit preserves the minimum visible scale"),
		FirstPersonRendering::FitScaleToCapsule(0.25f, Centre + FVector(0, 0, 93), 250, Centre, Up, Radius, HalfHeight, Margin), 0.1f);
	TestEqual(TEXT("The floor never exceeds the requested scale"),
		FirstPersonRendering::FitScaleToCapsule(0.05f, Centre + FVector(0, 0, 93), 250, Centre, Up, Radius, HalfHeight, Margin), 0.05f);
	TestEqual(TEXT("Empty bounds retain the configured scale"),
		FirstPersonRendering::FitScaleToCapsule(0.25f, Centre, 0, Centre, Up, Radius, HalfHeight, Margin), 0.25f);
	TestEqual(TEXT("Invalid configuration remains finite"),
		FirstPersonRendering::FitScaleToCapsule(std::numeric_limits<float>::quiet_NaN(), Centre, 30, Centre, Up, Radius, HalfHeight, Margin), 0.25f);
	TestEqual(TEXT("An overflowing bound retains a finite configured scale"),
		FirstPersonRendering::FitScaleToCapsule(0.25f, Centre, 1e308, Centre, Up, Radius, HalfHeight, Margin, 2.0), 0.25f);
	TestEqual(TEXT("An overflowing axis is rejected before normalization"),
		FirstPersonRendering::FitScaleToCapsule(0.25f, Centre, 200, Centre, FVector(1e308, 0, 0), Radius, HalfHeight, Margin), 0.25f);
	TestEqual(TEXT("A malformed capsule retains the fallback"),
		FirstPersonRendering::FitScaleToCapsule(0.25f, Centre, 200, Centre, Up, Radius, -1, Margin), 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonViewInputTest, "ShootingArena.Weapon.FirstPersonViewInputSafety",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstPersonViewInputTest::RunTest(const FString& Parameters)
{
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const float Inf = std::numeric_limits<float>::infinity();
	for (const float Invalid : {NaN, Inf, -Inf, -1.f, 0.f, 180.f, 360.f})
	{
		const float FOV = FirstPersonRendering::SafeFirstPersonFOV(Invalid, 90.f);
		const float ProjectionFactor = FMath::Tan(FMath::DegreesToRadians(90.f * 0.5f)) / FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f));
		TestTrue(TEXT("Invalid FP FOV cannot produce an invalid projection factor"), FMath::IsFinite(ProjectionFactor) && ProjectionFactor > 0.f);
		const float Near = FirstPersonRendering::SafeNearClip(Invalid, Invalid, Invalid);
		TestTrue(TEXT("Invalid clip configuration remains positive and finite"), FMath::IsFinite(Near) && Near >= 0.01f && Near <= 5.f);
	}
	TestEqual(TEXT("A narrower valid weapon FOV remains unchanged"), FirstPersonRendering::SafeFirstPersonFOV(65.f, 90.f), 65.f);
	TestEqual(TEXT("Invalid FOV falls back to the main view"), FirstPersonRendering::SafeFirstPersonFOV(NaN, 80.f), 80.f);
	TestEqual(TEXT("A smaller existing near plane remains unchanged"), FirstPersonRendering::SafeNearClip(0.5f, 0.1f, 5.f), 0.1f);
	TestEqual(TEXT("Invalid original camera scale has a safe restore value"), FirstPersonRendering::SafeScale(NaN, 1.f), 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFirstPersonRenderingDeactivateTest, "ShootingArena.Weapon.FirstPersonRenderingDeactivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFirstPersonRenderingDeactivateTest::RunTest(const FString& Parameters)
{
	// No world or gameplay actors: a component may tick without having been
	// activated. Deactivate must still stop that tick, or the next update reapplies it.
	TStrongObjectPtr<UProjectileFirstPersonRenderingComponent> Component(NewObject<UProjectileFirstPersonRenderingComponent>());
	Component->SetComponentTickEnabled(true);
	TestTrue(TEXT("An inactive component can have an enabled tick"), !Component->IsActive() && Component->IsComponentTickEnabled());
	Component->Deactivate();
	TestFalse(TEXT("Deactivate stops rendering updates even if already inactive"), Component->IsComponentTickEnabled());
	Component->Activate(true);
	TestTrue(TEXT("Explicit reactivation still works"), Component->IsComponentTickEnabled());
	Component->Deactivate();
	TestFalse(TEXT("Repeated deactivation leaves updates disabled"), Component->IsComponentTickEnabled());
	return true;
}

#endif

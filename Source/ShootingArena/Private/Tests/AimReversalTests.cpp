#if WITH_DEV_AUTOMATION_TESTS
#include "../AI/AimReversalState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimReversalTest, "ShootingArena.AI.AimReversal.DirectionAndTiming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAimReversalTest::RunTest(const FString&)
{
    FAimReversalState State;
    TestFalse(TEXT("First sample has no penalty"), State.Update(FVector(100,0,0), 1., .3, .2));
    TestFalse(TEXT("Same direction and vertical movement do not reverse"), State.Update(FVector(100,0,-500), 1.1, .3, .2));
    TestFalse(TEXT("Short stop retains direction"), State.Update(FVector::ZeroVector, 1.2, .3, .2));
    TestTrue(TEXT("Reverse after short stop"), State.Update(FVector(-100,0,0), 1.3, .3, .2));
    TestTrue(TEXT("Penalty persists while stopped"), State.Update(FVector::ZeroVector, 1.4, .3, .2));
    TestFalse(TEXT("Penalty expires"), State.Update(FVector::ZeroVector, 1.51, .3, .2));
    TestFalse(TEXT("Long stop expires previous direction"), State.Update(FVector(100,0,0), 2., .3, .2));
    TestTrue(TEXT("One unit per second is valid"), State.Update(FVector(-1,0,0), 2.1, .3, .2));
    TestTrue(TEXT("Sub-threshold speed does not erase penalty"), State.Update(FVector(.5,0,0), 2.2, .3, .2));
    return !HasAnyErrors();
}
#endif

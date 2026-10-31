#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "../../MyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeDamageLookTest,
    "ShootingArena.AI.DamageLook.QueueAndCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeDamageLookTest::RunTest(const FString&)
{
    const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
        .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(true).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("World"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
    auto* Controller = World->SpawnActor<AMyAIController>();
    auto* Pawn = World->SpawnActor<APawn>();
    Controller->Possess(Pawn);
    auto* Data = NewObject<UBlackboardData>();
    FBlackboardEntry Enemy; Enemy.EntryName = TEXT("Enemy"); Enemy.KeyType = NewObject<UBlackboardKeyType_Object>(Data);
    FBlackboardEntry Visible; Visible.EntryName = TEXT("Is Enemy Visible"); Visible.KeyType = NewObject<UBlackboardKeyType_Bool>(Data);
    Data->Keys.Add(Enemy); Data->Keys.Add(Visible);
    UBlackboardComponent* Blackboard = nullptr;
    if (!TestTrue(TEXT("Blackboard initialized"), Controller->UseBlackboard(Data, Blackboard))) return false;

    Controller->SetControlRotation(FRotator::ZeroRotator);
    Controller->RequestDamageLook(FVector::RightVector);
    Controller->RequestDamageLook(-FVector::ForwardVector);
    Controller->RequestDamageLook(-FVector::RightVector);
    Controller->UpdateControlRotation(1.f, false);
    TestTrue(TEXT("First hit completes before queued hits"), FMath::IsNearlyEqual(Controller->GetControlRotation().Yaw, 90.f, 0.1f));
    Controller->UpdateControlRotation(1.f, false);
    TestTrue(TEXT("Only the most recent queued direction is used"), FMath::IsNearlyEqual(Controller->GetControlRotation().Yaw, -90.f, 0.1f));

    Controller->RequestDamageLook(FVector::ForwardVector);
    Controller->RequestDamageLook(FVector::RightVector);
    Blackboard->SetValueAsObject(TEXT("Enemy"), World->SpawnActor<AActor>());
    Blackboard->SetValueAsBool(TEXT("Is Enemy Visible"), true);
    Controller->UpdateControlRotation(.1f, false);
    Blackboard->SetValueAsBool(TEXT("Is Enemy Visible"), false);
    Controller->SetControlRotation(FRotator::ZeroRotator);
    Controller->UpdateControlRotation(.1f, false);
    TestTrue(TEXT("Combat cancels active and queued looks"), FMath::IsNearlyZero(Controller->GetControlRotation().Yaw, .1f));
    return !HasAnyErrors();
}

#endif

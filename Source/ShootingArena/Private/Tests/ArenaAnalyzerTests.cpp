#if WITH_DEV_AUTOMATION_TESTS
#include "Analyzer/ArenaAnalyzeLibrary.h"
#include "Analyzer/ArenaCombatLibrary.h"
#include "Analyzer/ArenaAnalyzeRenderLibrary.h"
#include "Analyzer/ArenaLoggingSettings.h"
#include "Analyzer/ArenaMatchLogSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaAnalyzerIdentityTest, "ShootingArena.Analyzer.Identity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArenaAnalyzerIdentityTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Context.GetTestWorld();
    World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
    World->SetGameMode(FURL());
    World->InitializeActorsForPlay(FURL());
    auto* S = World->GetSubsystem<UArenaAnalyzeSubsystem>();
    if (!bArenaLoggingEnabled)
    {
        TestNull(TEXT("Analyzer is disabled"), S);
        TestNull(TEXT("Match log is disabled"), World->GetSubsystem<UArenaMatchLogSubsystem>());
        AddInfo(TEXT("Recording identity checks require bArenaLoggingEnabled to be true."));
        return true;
    }
    if (!TestNotNull(TEXT("Recorder exists"), S)) return false;
    auto* PC = World->SpawnActor<APlayerController>(); auto* AI = World->SpawnActor<AAIController>();
    auto* Shooter = World->SpawnActor<APawn>(); auto* Target = World->SpawnActor<APawn>();
    PC->Possess(Shooter); AI->Possess(Target);
    Target->SetCanBeDamaged(true); Shooter->SetCanBeDamaged(true);
    S->RegisterController(PC); S->RegisterController(PC); S->RegisterController(AI);
    auto* Weapon = World->SpawnActor<AActor>(); Weapon->SetOwner(Shooter);
    auto* Data = World->SpawnActor<AActor>();
    auto* Projectile = World->SpawnActor<AActor>();
    const int32 First = S->RecordShot(Weapon, Data, FVector(1,2,3), FVector::ForwardVector);
    S->BindProjectile(Projectile, Weapon);
    const int32 Miss = S->RecordShot(Weapon, nullptr, FVector::ZeroVector, FVector::RightVector);
    TestTrue(TEXT("Session shot IDs increase including misses"), Miss > First);
    PC->UnPossess();
    auto* Respawn = World->SpawnActor<APawn>(); Respawn->SetCanBeDamaged(true); PC->Possess(Respawn); Weapon->SetOwner(Respawn);
    S->RecordShot(Weapon, nullptr, FVector::ZeroVector, FVector::UpVector);
    S->PushProjectile(Projectile);
    S->ApplyShotDamage(Target, 10, PC, Weapon, nullptr);
    S->ApplyShotDamage(Target, 15, PC, Weapon, nullptr);
    S->PopProjectile();
    auto Snapshot = S->Snapshot();
    TestEqual(TEXT("No duplicate controller registration"), Snapshot.Participants.Num(), 2);
    TestTrue(TEXT("Player distinguished"), Snapshot.Participants[0].bPlayer && !Snapshot.Participants[1].bPlayer);
    TestEqual(TEXT("One shot can cause multiple damage events"), Snapshot.DamageSamples.Num(), 2);
    for (const auto& D : Snapshot.DamageSamples)
    {
        TestEqual(TEXT("Projectile keeps original shot after refire/respawn"), D.ShotId, First);
        TestEqual(TEXT("Projectile keeps original weapon data"), D.WeaponId, Data->GetPathName());
        TestEqual(TEXT("Projectile keeps original firing life"), D.InstigatorTrackId, Snapshot.ShotSamples[0].TrackId);
    }
    TestTrue(TEXT("Respawn starts new track"), Snapshot.ShotSamples.Last().TrackId != Snapshot.ShotSamples[0].TrackId);
    S->ApplyShotDamage(Respawn, 2, PC, Shooter, nullptr, Weapon);
    TestEqual(TEXT("Pawn damage causer resolves explicit firing weapon"), S->Snapshot().DamageSamples.Last().ShotId, Snapshot.ShotSamples.Last().ShotId);
    S->UpdateShotAim(Weapon, FVector(2,3,4), FVector(102,3,4));
    TestTrue(TEXT("Actual central aim recorded"), S->Snapshot().ShotSamples.Last().Direction.Equals(FVector::ForwardVector));
    S->RecordDeath(Target, nullptr, nullptr); S->RecordDeath(Target, PC, Weapon);
    Snapshot = S->Snapshot();
    TestEqual(TEXT("Confirmed death is idempotent"), Snapshot.DeathSamples.Num(), 1);
    TestEqual(TEXT("Unknown death cannot inherit latest weapon shot"), Snapshot.DeathSamples[0].ShotId, INDEX_NONE);
    S->RecordCredit(Target, nullptr, TEXT("RecentAttacker"));
    const int32 PreviousLife = Snapshot.DeathSamples[0].VictimTrackId;
    AI->UnPossess(); AI->Possess(Target);
    S->RecordDeath(Target, nullptr, nullptr);
    const auto ReusedLife = S->Snapshot();
    TestEqual(TEXT("Repossessed pooled pawn can record another confirmed death"), ReusedLife.DeathSamples.Num(), 2);
    TestTrue(TEXT("Pooled pawn receives a fresh life track"), ReusedLife.DeathSamples.Last().VictimTrackId > PreviousLife);
    TestTrue(TEXT("Pooled pawn does not inherit prior-life kill credit"), ReusedLife.DeathSamples.Last().CreditSource.IsEmpty());
    S->RecordCredit(Target, nullptr, TEXT("None"));
    TestEqual(TEXT("Credit updates cannot rewrite the previous pooled life"), S->Snapshot().DeathSamples[0].CreditSource, FString(TEXT("RecentAttacker")));
    S->ForgetProjectile(Projectile); S->PushProjectile(Projectile);
    S->ApplyShotDamage(Respawn, 1, PC, Weapon, nullptr); S->PopProjectile();
    const auto Reused = S->Snapshot();
    if (TestTrue(TEXT("Reused projectile damage exists"), !Reused.DamageSamples.IsEmpty()))
        TestEqual(TEXT("Reused unbound projectile has no stale shot"), Reused.DamageSamples.Last().ShotId, INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaAnalyzerJsonTest, "ShootingArena.Analyzer.JsonAndFilters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArenaAnalyzerJsonTest::RunTest(const FString& Parameters)
{
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Automation/Analyzer");
    IFileManager::Get().MakeDirectory(*Dir, true);
    const FString Legacy = Dir / TEXT("legacy.json");
    FFileHelper::SaveStringToFile(TEXT("{\"mapName\":\"Old\",\"movementSamples\":[{\"timeSeconds\":1,\"controllerId\":0,\"trackId\":0}],\"damageSamples\":[{\"damage\":10}]}"), *Legacy);
    FArenaAnalyzeSession S; S.ShotSamples.AddDefaulted();
    TestTrue(TEXT("Legacy JSON loads"), UArenaAnalyzeLibrary::LoadAnalyzeFile(Legacy, S));
    TestEqual(TEXT("Missing version means v1"), S.SchemaVersion, 1);
    TestEqual(TEXT("Missing arrays clear stale data"), S.ShotSamples.Num(), 0);
    TestEqual(TEXT("Legacy shot is unassociated"), S.DamageSamples[0].ShotId, INDEX_NONE);
    S.SchemaVersion = 2; S.SessionId = TEXT("fixture");
    FArenaShotSample Shot; Shot.ShotId = 42; Shot.WeaponId = TEXT("WeaponA"); Shot.ControllerId = 3; Shot.TimeSeconds = 1; S.ShotSamples.Add(Shot);
    FArenaParticipant P; P.ControllerId = 3; P.bPlayer = true; S.Participants.Add(P);
    FString Text; FJsonObjectConverter::UStructToJsonObjectString(S, Text);
    FFileHelper::SaveStringToFile(Text, *(Dir / TEXT("ArenaAnalyze_fixture_000000.json")));
    S.ChunkIndex = 1; S.ShotSamples.Reset(); S.MovementSamples.Reset(); S.DamageSamples.Reset();
    FArenaHitSample Hit; Hit.ShotId = 42; Hit.TimeSeconds = 3; S.HitSamples.Add(Hit);
    FJsonObjectConverter::UStructToJsonObjectString(S, Text);
    const FString Second = Dir / TEXT("ArenaAnalyze_fixture_000001.json"); FFileHelper::SaveStringToFile(Text, *Second);
    TestTrue(TEXT("All chunks load from any selected chunk"), UArenaAnalyzeLibrary::LoadAnalyzeFile(Second, S));
    TestEqual(TEXT("Shot retained across chunk boundary"), S.ShotSamples.Num(), 1);
    const auto Filtered = UArenaAnalyzeLibrary::FilterSession(S, TEXT("WeaponA"), 1, -1, -1, 2, 4);
    TestEqual(TEXT("Shot before time range excluded from displayed shots"), Filtered.ShotSamples.Num(), 0);
    TestEqual(TEXT("Later hit still joins original shot"), Filtered.HitSamples.Num(), 1);
    TestEqual(TEXT("Wrong weapon excluded"), UArenaAnalyzeLibrary::FilterSession(S, TEXT("WeaponB")).HitSamples.Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArenaAnalyzerISMTest, "ShootingArena.Analyzer.InstancedRendering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArenaAnalyzerISMTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    AActor* Actor = Context.GetTestWorld()->SpawnActor<AActor>();
    auto* Root = NewObject<USceneComponent>(Actor); Actor->AddInstanceComponent(Root); Actor->SetRootComponent(Root); Root->RegisterComponent();
    Actor->SetActorLocation(FVector(1000,2000,3000));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto Find = [&](FName Name) -> UInstancedStaticMeshComponent*
    {
        TInlineComponentArray<UInstancedStaticMeshComponent*> Cs(Actor);
        for (auto* C : Cs) if (C->GetFName() == Name) return C;
        return nullptr;
    };
    for (const TCHAR* Name : {TEXT("MovementISM"), TEXT("MovementLinkISM"), TEXT("DamagedISM"), TEXT("InstigatorISM"), TEXT("DamageLinkISM")})
    {
        auto* C = NewObject<UInstancedStaticMeshComponent>(Actor, FName(Name));
        C->SetStaticMesh(FString(Name).Contains(TEXT("Link")) ? Cube : Sphere);
        C->SetupAttachment(Root); Actor->AddInstanceComponent(C); C->RegisterComponent();
    }
    FArenaAnalyzeSession S; S.SchemaVersion = 2;
    FArenaParticipant Player; Player.ControllerId = 0; Player.bPlayer = true; S.Participants.Add(Player);
    for (int32 I = 0; I < 4; ++I)
    {
        FArenaMovementSample M; M.ControllerId = I == 3 ? 1 : 0; M.TrackId = I == 2 ? 1 : 0;
        M.TimeSeconds = I; M.Location = FVector(110 + I * 100,0,0); S.MovementSamples.Add(M);
    }
    FArenaShotSample Shot; Shot.ControllerId = 0; Shot.ShotId = 5; Shot.Location = FVector(10,0,0); Shot.Direction = FVector::ForwardVector; S.ShotSamples.Add(Shot);
    FArenaHitSample Hit; Hit.ShotId = 5; Hit.Location = FVector(80,0,0); S.HitSamples.Add(Hit);
    FArenaDamageSample Damage; Damage.DamagedLocation = FVector(90,0,0); Damage.InstigatorLocation = FVector(70,0,0); Damage.bHasInstigatorLocation = true; S.DamageSamples.Add(Damage);
    FArenaDeathSample Death; Death.VictimLocation = FVector(90,0,0); S.DeathSamples.Add(Death);
    TestEqual(TEXT("Expected point/link instances, with no cross-respawn segment or unknown killer line"), UArenaAnalyzeRenderLibrary::RenderSession(Actor,S), 13);
    TestEqual(TEXT("Repeat draw replaces instances"), UArenaAnalyzeRenderLibrary::RenderSession(Actor,S), 13);
    auto* PlayerPoints = Find(TEXT("MovementISM"));
    if (!TestNotNull(TEXT("Shared movement layer exists"), PlayerPoints)) return false;
    TestTrue(TEXT("Player point template reused"), PlayerPoints->GetStaticMesh() == Sphere);
    TestEqual(TEXT("Player and AI share the movement component"), Find(TEXT("MovementISM"))->GetInstanceCount(), 4);
    FTransform T; Find(TEXT("MovementLinkISM"))->GetInstanceTransform(0,T,true);
    TestTrue(TEXT("Link midpoint is world-space despite actor offset"), T.GetLocation().Equals(FVector(160,0,0)));
    TestTrue(TEXT("Link uses original length/100 and .02 thickness"), T.GetScale3D().Equals(FVector(1,.02,.02),0.0001));
    TestEqual(TEXT("Event switch replaces other categories"), UArenaAnalyzeRenderLibrary::RenderSession(Actor,S,1), 2);
    TestEqual(TEXT("Movement removed after switching to shots"), PlayerPoints->GetInstanceCount(), 0);
    UArenaAnalyzeRenderLibrary::ClearVisualizer(Actor);
    TInlineComponentArray<UInstancedStaticMeshComponent*> Cs(Actor);
    for (auto* C : Cs) TestEqual(TEXT("Clear covers original and added layers"), C->GetInstanceCount(), 0);
    S.MovementSamples.Reset(); S.ShotSamples.Reset(); S.HitSamples.Reset(); S.DamageSamples.Reset(); S.DeathSamples.Reset();
    for (int32 I = 0; I < 10000; ++I)
    {
        FArenaMovementSample M; M.ControllerId = 0; M.TrackId = 0; M.TimeSeconds = I; M.Location = FVector(I*10,0,0); S.MovementSamples.Add(M);
    }
    const double Start = FPlatformTime::Seconds();
    TestEqual(TEXT("Large movement batch has one point per sample and one link per adjacent pair"), UArenaAnalyzeRenderLibrary::RenderSession(Actor,S,4), 19999);
    AddInfo(FString::Printf(TEXT("10,000 movement samples -> 19,999 ISM instances in %.2f ms (CPU fixture, not viewport FPS)."), (FPlatformTime::Seconds()-Start)*1000));
    return true;
}
#endif

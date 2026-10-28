#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Character/CharacterPoolComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Logging/MessageLog.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterPoolResetTest,
	"ShootingArena.CharacterPool.CutCapPhysicsAndFirstPersonTransforms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterPoolResetTest::RunTest(const FString& Parameters)
{
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated regression world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ACharacter* Pawn = World->SpawnActor<ACharacter>();
	auto* Cap = NewObject<UStaticMeshComponent>(Pawn, TEXT("Head_IN1"));
	Pawn->AddInstanceComponent(Cap); Cap->SetupAttachment(Pawn->GetRootComponent());
	Cap->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision); Cap->SetRelativeLocation(FVector(0,0,140)); Cap->RegisterComponent();
	auto* Arms = NewObject<USkeletalMeshComponent>(Pawn, TEXT("FP_Mesh"));
	Pawn->AddInstanceComponent(Arms); Arms->SetupAttachment(Pawn->GetRootComponent()); Arms->SetRelativeLocation(FVector(0,0,-215)); Arms->RegisterComponent();
	Arms->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/ParagonSteel/Characters/Heroes/Steel/Meshes/Arm_Steel.Arm_Steel")));
	// Exercise montage lifetime using the real arm skeleton/equip asset without
	// invoking the gameplay ABP, which requires a BP_ShooterBase owner and its HUD.
	Arms->SetAnimInstanceClass(UAnimInstance::StaticClass());
	UAnimInstance* ArmAnimation = Arms->GetAnimInstance();
	UAnimMontage* EquipMontage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/QuakeLike_1_0/Animation/Steel/Anim/Steel_PistolAnim_Weapon_Up_Montage.Steel_PistolAnim_Weapon_Up_Montage"));
	TestNotNull(TEXT("FP animation instance on the default arm skeleton"), ArmAnimation);
	TestNotNull(TEXT("Actual default-weapon equip montage"), EquipMontage);
	auto* Camera = NewObject<UCameraComponent>(Pawn, TEXT("FP_Camera"));
	Pawn->AddInstanceComponent(Camera); Camera->SetupAttachment(Pawn->GetRootComponent()); Camera->SetRelativeLocation(FVector(0,0,60)); Camera->RegisterComponent();
	auto* Life = NewObject<UCharacterPoolComponent>(Pawn);
	Pawn->AddInstanceComponent(Life); Life->RegisterComponent(); Life->Life.bManaged = true; Life->CapturePresentation();
	const FTransform UserArms(FRotator(0,12,0), FVector(36,0,-215));
	const FTransform UserCamera(FRotator(5,0,0), FVector(3,0,64));
	Arms->SetRelativeTransform(UserArms); Camera->SetRelativeTransform(UserCamera); Camera->SetFieldOfView(110.f);
	const int32 WarningsBefore = FMessageLog(TEXT("PIE")).NumMessages(EMessageSeverity::Warning);
	for (int32 Cycle = 0; Cycle < 3; ++Cycle)
	{
		// On a client, new weapon equip can arrive before the pawn's life OnRep.
		// The subsequent presentation reset must not cancel this valid new montage.
		if (ArmAnimation && EquipMontage)
			TestTrue(TEXT("Regression starts a real equip montage"), ArmAnimation->Montage_Play(EquipMontage) > 0.f);
		// Death makes the static cut cap simulate and detaches it from its skeletal parent.
		Cap->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Cap->SetSimulatePhysics(true);
		TestTrue(TEXT("Cut-cap regression actually starts physics"), Cap->IsSimulatingPhysics());
		Life->Life.bDead = true;
		TestTrue(TEXT("Same pawn prepares another life"), Life->PrepareRespawn(FTransform(FVector(500.f * Cycle, 0, 200))));
		TestFalse(TEXT("Static cut-cap simulation stops before restoring no collision"), Cap->IsSimulatingPhysics());
		TestEqual(TEXT("Alive cut-cap collision restores"), Cap->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		TestEqual(TEXT("Detached cut cap reattaches to the cached parent"), Cap->GetAttachParent(), Pawn->GetRootComponent());
		TestTrue(TEXT("Cut-cap relative pose restores"), Cap->GetRelativeLocation().Equals(FVector(0,0,140)));
		TestTrue(TEXT("Configured FP arm transform survives reuse"), Arms->GetRelativeTransform().Equals(UserArms));
		TestTrue(TEXT("Configured FP camera transform survives reuse"), Camera->GetRelativeTransform().Equals(UserCamera));
		TestEqual(TEXT("Configured FOV survives presentation restore"), Camera->FieldOfView, 110.f);
		TestEqual(TEXT("FP AnimInstance survives reuse"), Arms->GetAnimInstance(), ArmAnimation);
		if (ArmAnimation && EquipMontage)
			TestTrue(TEXT("A new equip montage survives late life replication"), ArmAnimation->Montage_IsActive(EquipMontage));
		Pawn->SetActorEnableCollision(true);
	}
	TestEqual(TEXT("Repeated reset emits no physics/collision warnings"),
		FMessageLog(TEXT("PIE")).NumMessages(EMessageSeverity::Warning), WarningsBefore);
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterPoolSpawnScaleTest,
	"ShootingArena.CharacterPool.BlueprintSpawnScaleSurvivesReuse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterPoolSpawnScaleTest::RunTest(const FString& Parameters)
{
	UClass* PawnClass = LoadClass<ACharacter>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Character/BP_PlayerShooter.BP_PlayerShooter_C"));
	if (!TestNotNull(TEXT("Actual PlayerShooter class"), PawnClass)) return false;
	const FVector DefaultScale = PawnClass->GetDefaultObject<AActor>()->GetRootComponent()->GetRelativeScale3D();
	TestTrue(TEXT("Regression exercises the real non-unit BP root scale"), DefaultScale.Equals(FVector(1.5)));
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated spawn scale world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	// Deferred creation gives the real engine/BP native-root scale contract without
	// running Construction Script, BeginPlay, HUD, equipment, or a PIE match.
	ACharacter* Pawn = World->SpawnActorDeferred<ACharacter>(PawnClass, FTransform::Identity, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (TestNotNull(TEXT("Deferred BP pawn"), Pawn))
	{
		Pawn->GetMesh()->SetAnimInstanceClass(nullptr);
		TestTrue(TEXT("Fresh engine spawn includes BP root scale"), Pawn->GetActorScale3D().Equals(DefaultScale));
		auto* Pool = NewObject<UCharacterPoolComponent>(Pawn);
		Pawn->AddInstanceComponent(Pool); Pool->RegisterComponent(); Pool->Life.bManaged = true; Pool->CapturePresentation();
		for (const double RequestedScale : {1., 1., 0.75, 2., 1.})
		{
			const FTransform Spawn(FRotator(0,45,0), FVector(1000,0,500), FVector(RequestedScale));
			ACharacter* Fresh = World->SpawnActorDeferred<ACharacter>(PawnClass, Spawn, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			Pool->Life.bDead = true;
			TestTrue(TEXT("Reused pawn starts another life"), Pool->PrepareRespawn(Spawn));
			if (TestNotNull(TEXT("Fresh reference spawn"), Fresh))
			{
				Fresh->GetMesh()->SetAnimInstanceClass(nullptr);
				TestTrue(TEXT("Reused scale matches a fresh engine spawn, without cumulative multiplication"),
					Pawn->GetActorScale3D().Equals(Fresh->GetActorScale3D()));
				TestEqual(TEXT("Reused capsule radius matches fresh spawn"),
					Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius(), Fresh->GetCapsuleComponent()->GetScaledCapsuleRadius());
				TestEqual(TEXT("Reused capsule height matches fresh spawn"),
					Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), Fresh->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
				Fresh->Destroy();
			}
			TestTrue(TEXT("Replicated placement carries final actor scale"),
				Pool->Life.SpawnTransform.GetScale3D().Equals(Pawn->GetActorScale3D()));
		}
	}
	GEngine->DestroyWorldContext(World); World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif

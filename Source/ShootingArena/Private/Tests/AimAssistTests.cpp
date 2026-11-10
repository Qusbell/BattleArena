#if WITH_DEV_AUTOMATION_TESTS
#include "../Camera/AimAssistMath.h"
#include "Camera/AimAssistComponent.h"
#include "Camera/AimAssistSettings.h"
#include "MyPlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Camera/CameraTypes.h"
#include "Components/BoxComponent.h"
#include "Camera/ShooterDisplayLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistMathTest, "ShootingArena.AimAssist.ConvergenceAndEligibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAimAssistMathTest::RunTest(const FString&)
{
	for (const int32 FPS : {30, 60, 144})
	{
		FRotator Rotation = FRotator::ZeroRotator;
		for (int32 Frame = 0; Frame < FPS; ++Frame)
			Rotation = AimAssistMath::Interpolate(Rotation, FRotator(0, 90, 0), 1.f / FPS, .3f);
		const double Expected = 90.0 * (1.0 - FMath::Pow(0.1, 1.0 / .3));
		TestTrue(FString::Printf(TEXT("Frame-rate-independent at %d FPS"), FPS), FMath::IsNearlyEqual(Rotation.Yaw, Expected, .001));
	}
	TestTrue(TEXT("90 percent in 0.3 seconds"), FMath::IsNearlyEqual(
		AimAssistMath::Interpolate(FRotator::ZeroRotator, FRotator(0, 90, 0), .3f, .3f).Yaw, 81., .001));
	TestTrue(TEXT("Shortest path across yaw seam"), FMath::IsNearlyEqual(
		AimAssistMath::Interpolate(FRotator(0,179,0), FRotator(0,-179,0), .3f, .3f).Yaw, -179.2, .001));
	TestTrue(TEXT("Paused frame does not rotate"), AimAssistMath::Interpolate(FRotator(2,3,0), FRotator(20,30,0), 0,.3f).Equals(FRotator(2,3,0)));
	TestTrue(TEXT("Invalid tuning falls back safely"), !AimAssistMath::Interpolate(FRotator::ZeroRotator, FRotator(20,30,0), .1f, std::numeric_limits<float>::quiet_NaN()).ContainsNaN());
	const FIntRect Rect(100,50,900,650);
	TestTrue(TEXT("Constrained view includes chest"), AimAssistMath::ScreenCandidate(FVector2D(500,350),Rect));
	TestFalse(TEXT("Letterbox excluded"), AimAssistMath::ScreenCandidate(FVector2D(500,20),Rect));
	TestFalse(TEXT("Right edge excluded"), AimAssistMath::ScreenCandidate(FVector2D(900,350),Rect));
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAimAssistIntegrationTest, "ShootingArena.AimAssist.BlueprintBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAimAssistIntegrationTest::RunTest(const FString&)
{
	UClass* PlayerClass = LoadClass<APawn>(nullptr, TEXT("/Game/QuakeLike_1_0/Character/BP_PlayerShooter.BP_PlayerShooter_C"));
	UClass* PCClass = LoadClass<AMyPlayerController>(nullptr, TEXT("/Game/QuakeLike_1_0/Controller/BP_QuakePlayerController.BP_QuakePlayerController_C"));
	if (!TestNotNull(TEXT("Actual player class"), PlayerClass) || !TestNotNull(TEXT("Actual controller class"), PCClass)) return false;
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); };
	auto* PC = World->SpawnActor<AMyPlayerController>(PCClass);
	auto* Pawn = World->SpawnActor<APawn>(PlayerClass, FVector(200,300,400), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("Player spawned"), Pawn) || !TestNotNull(TEXT("Controller spawned"), PC)) return false;
	PC->Possess(Pawn);
	// Allow getters after possession; this isolated controller has no local UI player.
	TGuardValue<bool> ScriptGuard(GAllowActorScriptExecutionInEditor, true);
	if (!TestNotNull(TEXT("Inherited native aim assist"), PC->AimAssist.Get())) return false;
	TestFalse(TEXT("Default disabled"), PC->AimAssist->bEnabled);
	PC->AimAssist->bEnabled = true;
	TestFalse(TEXT("No local viewport cannot capture look"), PC->AimAssist->CanAssist());
	for (const TCHAR* Name : {TEXT("GetHeart"), TEXT("GetAim")})
	{
		UFunction* Function = Pawn->FindFunction(Name);
		if (!TestNotNull(Name, Function)) continue;
		FStructOnScope Params(Function);
		Pawn->ProcessEvent(Function, Params.GetStructMemory());
		if (FString(Name) == TEXT("GetHeart"))
		{
			const FStructProperty* Result = FindFProperty<FStructProperty>(Function,TEXT("heartLocation"));
			if (TestNotNull(TEXT("Chest output"), Result))
				TestTrue(TEXT("Chest is attached to actual pawn"), FVector::Dist(*Result->ContainerPtrToValuePtr<FVector>(Params.GetStructMemory()),Pawn->GetActorLocation()) < 200.);
		}
		else
		{
			PC->SetControlRotation(FRotator(15,25,0));
			Pawn->ProcessEvent(Function, Params.GetStructMemory());
			const FStructProperty* Result = FindFProperty<FStructProperty>(Function,TEXT("aimRotation"));
			if (TestNotNull(TEXT("Existing aim output"), Result))
				TestTrue(TEXT("Existing weapon aim follows control rotation"), Result->ContainerPtrToValuePtr<FRotator>(Params.GetStructMemory())->Equals(PC->GetControlRotation(),.01));
		}
	}

	UClass* StateClass = LoadClass<APlayerState>(nullptr, TEXT("/Game/QuakeLike_1_0/GameMode/BP_QuakePlayerState.BP_QuakePlayerState_C"));
	if (!TestNotNull(TEXT("Actual player state class"),StateClass)) return false;
	auto SetState = [&](APawn* Target)
	{
		APlayerState* State = Target->GetPlayerState();
		if (!State || !State->IsA(StateClass)) { State = World->SpawnActor<APlayerState>(StateClass); Target->SetPlayerState(State); }
	};
	auto SetHealth = [&](APawn* Target, double Health)
	{
		TInlineComponentArray<UActorComponent*> Components(Target);
		for (auto* Component : Components)
		{
			const FStructProperty* Life = FindFProperty<FStructProperty>(Component->GetClass(),TEXT("LifeInfo"));
			if (!Life) continue;
			void* Info = Life->ContainerPtrToValuePtr<void>(Component);
			for (TFieldIterator<FProperty> It(Life->Struct); It; ++It)
				if (It->GetName().StartsWith(TEXT("NowHealth_")))
					if (const auto* Number = CastField<FNumericProperty>(*It))
					{
						void* Value = Number->ContainerPtrToValuePtr<void>(Info);
						if (Number->IsFloatingPoint()) Number->SetFloatingPointPropertyValue(Value,Health);
						else Number->SetIntPropertyValue(Value,static_cast<int64>(Health));
						return;
					}
		}
		AddError(TEXT("Life health bridge missing"));
	};
	FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APawn* NearCenter = World->SpawnActor<APawn>(PlayerClass,FVector(2000,300,400),FRotator::ZeroRotator,Spawn);
	APawn* Side = World->SpawnActor<APawn>(PlayerClass,FVector(2000,1000,400),FRotator::ZeroRotator,Spawn);
	if (!TestNotNull(TEXT("Centered target"),NearCenter) || !TestNotNull(TEXT("Side target"),Side)) return false;
	SetState(Pawn); SetState(NearCenter); SetState(Side);
	SetHealth(Pawn,100); SetHealth(NearCenter,100); SetHealth(Side,100);
	TestTrue(TEXT("Real Life bridge alive"), UShooterDisplayLibrary::IsShooterAlive(NearCenter));
	const FVector Origin(200,300,460);
	FMinimalViewInfo View; View.Location=Origin; View.Rotation=FRotator::ZeroRotator; View.FOV=90; View.AspectRatio=16.f/9.f;
	const FMatrix Swizzle(FPlane(0,0,1,0),FPlane(1,0,0,0),FPlane(0,1,0,0),FPlane(0,0,0,1));
	const FMatrix Matrix=FTranslationMatrix(-Origin)*Swizzle*View.CalculateProjectionMatrix();
	const FIntRect Rect(0,0,1280,720);
	FVector SelectedHeart;
	auto Select = [&]() { return PC->AimAssist->SelectTargetInView(Pawn,Origin,Rect,Matrix,SelectedHeart); };
	TestEqual(TEXT("Closest chest to crosshair selected"),Select(),NearCenter);
	NearCenter->SetActorLocation(FVector(2000,1500,400));
	TestEqual(TEXT("Selection changes immediately with screen distance"),Select(),Side);
	NearCenter->SetActorLocation(FVector(2000,300,400));
	SetHealth(NearCenter,0);
	TestEqual(TEXT("Actual dead shooter excluded"),Select(),Side);
	Side->SetPlayerState(nullptr);
	TestEqual(TEXT("AI without PlayerState remains an enemy"),Select(),Side);
	SetHealth(NearCenter,100);
	NearCenter->GetPlayerState()->SetIsSpectator(true);
	TestEqual(TEXT("Spectator excluded"),Select(),Side);
	NearCenter->GetPlayerState()->SetIsSpectator(false);
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(20,60,150)); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
	Box->RegisterComponent(); Wall->SetActorLocation(FVector(1000,300,460));
	TestEqual(TEXT("Occluded chest excluded immediately"),Select(),Side);
	Wall->Destroy();
	TestEqual(TEXT("Visible chest reacquired immediately"),Select(),NearCenter);
	NearCenter->SetActorLocation(FVector(-2000,300,400));
	Side->SetActorLocation(FVector(2000,10000,400));
	TestNull(TEXT("Behind-camera and offscreen targets excluded"),Select());
	NearCenter->SetActorLocation(FVector(10200,300,400));
	TestEqual(TEXT("No artificial 50 meter limit"),Select(),NearCenter);
	NearCenter->SetPlayerState(nullptr);
	TestEqual(TEXT("No Team or PlayerState dependency"),Select(),NearCenter);
	return !HasAnyErrors();
}
#endif

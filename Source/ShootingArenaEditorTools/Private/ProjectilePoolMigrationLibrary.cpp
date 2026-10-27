#include "ProjectilePoolMigrationLibrary.h"

#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_Event.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_Self.h"
#include "K2Node_SpawnActorFromClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Weapon/ProjectilePoolLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "Weapon/ProjectilePoolComponent.h"
#include "Weapon/ProjectilePoolSubsystem.h"
#include "Weapon/ProjectileConvergenceComponent.h"
#include "Weapon/ProjectileWorldCollisionComponent.h"
#include "Weapon/WeaponProjectileAimLibrary.h"

namespace ProjectilePoolMigrationHelpers
{
UEdGraph* EventGraph(UBlueprint* BP)
{
	for (UEdGraph* Graph : BP->UbergraphPages) if (Graph->GetFName() == TEXT("EventGraph")) return Graph;
	return nullptr;
}

UK2Node_CallFunction* Call(UEdGraph* Graph, FName Function, int32 X, int32 Y)
{
	FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
	auto* Node = Creator.CreateNode(false);
	Node->SetFromFunction(UProjectilePoolLibrary::StaticClass()->FindFunctionByName(Function));
	Node->NodePosX = X; Node->NodePosY = Y;
	Creator.Finalize();
	return Node;
}

bool Move(UEdGraphPin* From, UEdGraphPin* To)
{
	if (!From || !To) return false;
	const auto Links = From->LinkedTo;
	To->DefaultValue = From->DefaultValue;
	To->DefaultObject = From->DefaultObject;
	To->DefaultTextValue = From->DefaultTextValue;
	From->BreakAllPinLinks();
	for (UEdGraphPin* Link : Links)
		if (!GetDefault<UEdGraphSchema_K2>()->TryCreateConnection(To, Link)) return false;
	return true;
}
}

bool UProjectilePoolMigrationLibrary::ConnectProjectile(UBlueprint* BP)
{
	using namespace ProjectilePoolMigrationHelpers;
	if (!BP || (BP->GetName() != TEXT("BP_PlasmaProjectile") && BP->GetName() != TEXT("BP_RocketLauncherProjectile"))) return false;
	UEdGraph* Graph = EventGraph(BP);
	if (!Graph) return false;
	UK2Node_Event* Begin = nullptr;
	TArray<UK2Node_CallFunction*> DestroyNodes;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (auto* CallNode = Cast<UK2Node_CallFunction>(Node))
		{
			if (CallNode->GetFunctionName() == GET_FUNCTION_NAME_CHECKED(UProjectilePoolLibrary, IsPooledProjectile)) return true;
			if (CallNode->GetFunctionName() == TEXT("K2_DestroyActor"))
			{
				if (!CallNode->FindPinChecked(TEXT("self"))->LinkedTo.IsEmpty()) return false;
				DestroyNodes.Add(CallNode);
			}
		}
		if (auto* Event = Cast<UK2Node_Event>(Node); Event && Event->EventReference.GetMemberName() == TEXT("ReceiveBeginPlay")) Begin = Event;
	}
	if (!Begin || DestroyNodes.IsEmpty()) return false;
	BP->Modify(); Graph->Modify();
	const auto* Schema = GetDefault<UEdGraphSchema_K2>();
	FGraphNodeCreator<UK2Node_IfThenElse> BranchCreator(*Graph);
	auto* Branch = BranchCreator.CreateNode(false);
	Branch->NodePosX = Begin->NodePosX + 240; Branch->NodePosY = Begin->NodePosY - 160;
	Branch->NodeComment = TEXT("Pool factory initializes each shot explicitly; ordinary legacy spawns keep their original BeginPlay initialization.");
	BranchCreator.Finalize();
	auto* Predicate = Call(Graph, GET_FUNCTION_NAME_CHECKED(UProjectilePoolLibrary, IsPooledProjectile), Begin->NodePosX - 64, Begin->NodePosY - 300);
	FGraphNodeCreator<UK2Node_Self> SelfCreator(*Graph);
	auto* Self = SelfCreator.CreateNode(false);
	Self->NodePosX = Begin->NodePosX - 256; Self->NodePosY = Begin->NodePosY - 220;
	SelfCreator.Finalize();
	if (!Move(Begin->FindPin(TEXT("then")), Branch->GetElsePin())
		|| !Schema->TryCreateConnection(Begin->FindPin(TEXT("then")), Branch->GetExecPin())
		|| !Schema->TryCreateConnection(Self->FindPin(TEXT("self")), Predicate->FindPin(TEXT("Projectile")))
		|| !Schema->TryCreateConnection(Predicate->GetReturnValuePin(), Branch->GetConditionPin())) return false;
	for (auto* Destroy : DestroyNodes)
	{
		auto* Release = Call(Graph, GET_FUNCTION_NAME_CHECKED(UProjectilePoolLibrary, ReleaseProjectile), Destroy->NodePosX, Destroy->NodePosY);
		Release->NodeComment = TEXT("Managed Plasma/Rocket returns to its world pool; unmanaged legacy instances still Destroy.");
		if (!Move(Destroy->GetExecPin(), Release->GetExecPin()) || !Move(Destroy->GetThenPin(), Release->GetThenPin())) return false;
		FBlueprintEditorUtils::RemoveNode(BP, Destroy, true);
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

bool UProjectilePoolMigrationLibrary::ConnectWeapon(UBlueprint* BP)
{
	using namespace ProjectilePoolMigrationHelpers;
	if (!BP || (BP->GetName() != TEXT("BP_Weapon_PlasmaGun") && BP->GetName() != TEXT("BP_Weapon_RocketLauncher"))) return false;
	UEdGraph* Graph = EventGraph(BP);
	if (!Graph) return false;
	UK2Node_SpawnActorFromClass* Spawn = nullptr;
	UEdGraphNode* TrackingAdd = nullptr;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (auto* CallNode = Cast<UK2Node_CallFunction>(Node);
			CallNode && CallNode->GetFunctionName() == GET_FUNCTION_NAME_CHECKED(UProjectilePoolLibrary, SpawnPooledProjectile)) return true;
		if (auto* Found = Cast<UK2Node_SpawnActorFromClass>(Node)) { if (Spawn) return false; Spawn = Found; }
		if (Node->GetFName() == TEXT("K2Node_CallArrayFunction_1")) TrackingAdd = Node;
	}
	if (!Spawn || !TrackingAdd) return false;
	BP->Modify(); Graph->Modify();
	auto* Factory = Call(Graph, GET_FUNCTION_NAME_CHECKED(UProjectilePoolLibrary, SpawnPooledProjectile), Spawn->NodePosX, Spawn->NodePosY);
	Factory->NodeComment = TEXT("World-local pool; same authoritative shot data and original reliable multicast. All existing convergence/damage/effect code remains downstream.");
	const TPair<FName, FName> Pins[] = {
		{TEXT("execute"), TEXT("execute")}, {TEXT("then"), TEXT("then")},
		{TEXT("Class"), TEXT("ProjectileClass")}, {TEXT("SpawnTransform"), TEXT("SpawnTransform")},
		{TEXT("Owner"), TEXT("Weapon")}, {TEXT("Instigator"), TEXT("Instigator")},
		{TEXT("canDealDamage"), TEXT("bCanDealDamage")}, {TEXT("projectileId"), TEXT("ProjectileId")}
	};
	for (const auto& Pair : Pins) if (!Move(Spawn->FindPin(Pair.Key), Factory->FindPin(Pair.Value))) return false;
	for (UEdGraphPin* Pin : Spawn->Pins)
		if (Pin->PinName.ToString().StartsWith(TEXT("wConfig_config_")))
			if (!Move(Pin, Factory->FindPin(TEXT("WeaponData")))) return false;
	FGraphNodeCreator<UK2Node_DynamicCast> CastCreator(*Graph);
	auto* CastNode = CastCreator.CreateNode(false);
	CastNode->TargetType = Cast<UClass>(Spawn->GetResultPin()->PinType.PinSubCategoryObject.Get());
	CastNode->SetPurity(true);
	CastNode->NodePosX = Spawn->NodePosX + 240; CastNode->NodePosY = Spawn->NodePosY + 320;
	CastCreator.Finalize();
	const auto* Schema = GetDefault<UEdGraphSchema_K2>();
	if (!Schema->TryCreateConnection(Factory->GetReturnValuePin(), CastNode->GetCastSourcePin())
		|| !Move(Spawn->GetResultPin(), CastNode->GetCastResultPin())) return false;
	// The pool's active-shot map replaces the terminal append-only BP tracking array.
	FBlueprintEditorUtils::RemoveNode(BP, TrackingAdd, true);
	FBlueprintEditorUtils::RemoveNode(BP, Spawn, true);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	return true;
}

FString UProjectilePoolMigrationLibrary::VerifyPool()
{
	TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Failures;
	int32 Checks = 0;
	auto Check = [&](bool bCondition, const TCHAR* Message)
	{
		++Checks;
		if (!bCondition) Failures.Add(MakeShared<FJsonValueString>(Message));
		return bCondition;
	};
	IConsoleVariable* Enabled = IConsoleManager::Get().FindConsoleVariable(TEXT("sa.ProjectilePool.Enabled"));
	const int32 PreviousEnabled = Enabled->GetInt();
	Enabled->Set(1, ECVF_SetByCode);
	const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false).RequiresHitProxies(false)
		.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
	if (World)
	{
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->GetWorldSettings()->NotifyBeginPlay();
		auto* Pool = World->GetSubsystem<UProjectilePoolSubsystem>();
		Check(Pool != nullptr && World->HasBegunPlay(), TEXT("Game world and subsystem initialized"));
		ACharacter* Shooter1 = World->SpawnActor<ACharacter>();
		ACharacter* Shooter2 = World->SpawnActor<ACharacter>();
		Shooter1->SetActorLocation(FVector(-5000, -5000, 0));
		Shooter2->SetActorLocation(FVector(-5000, -6000, 0));
		const FTransform Shot(FRotator::ZeroRotator, FVector(0, 0, 300));
		for (const FString Kind : {FString(TEXT("Plasma")), FString(TEXT("Rocket"))})
		{
			const bool bRocket = Kind == TEXT("Rocket");
			const FString ProjectileName = bRocket ? TEXT("BP_RocketLauncherProjectile") : TEXT("BP_PlasmaProjectile");
			const FString WeaponName = bRocket ? TEXT("RocketLauncher") : TEXT("PlasmaGun");
			UClass* Class = LoadClass<AActor>(nullptr, *(TEXT("/Game/QuakeLike_1_0/HeldItem/Projectile/") + ProjectileName + TEXT(".") + ProjectileName + TEXT("_C")));
			UClass* WeaponClass = LoadClass<AActor>(nullptr, *(TEXT("/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_") + WeaponName + TEXT(".BP_Weapon_") + WeaponName + TEXT("_C")));
			UObject* Data = LoadObject<UObject>(nullptr, *(TEXT("/Game/QuakeLike_1_0/Data/Item/DA_") + WeaponName + TEXT(".DA_") + WeaponName));
			if (!Check(Class && WeaponClass && Data, TEXT("Saved projectile, weapon and data loaded"))) continue;
			// A deferred real weapon supplies authority and typed config without running unrelated inventory/HUD setup.
			AActor* Weapon = World->SpawnActorDeferred<AActor>(WeaponClass, Shot, Shooter1, Shooter1, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (auto* Property = FindFProperty<FObjectPropertyBase>(WeaponClass, TEXT("weaponData"))) Property->SetObjectPropertyValue_InContainer(Weapon, Data);
			AActor* First = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, true, 1);
			if (!Check(First != nullptr, TEXT("First pooled shot acquired"))) continue;
			auto* Lifecycle = First->FindComponentByClass<UProjectilePoolComponent>();
			auto* Movement = First->FindComponentByClass<UProjectileMovementComponent>();
			const int32 ComponentCount = First->GetComponents().Num();
			Check(First->HasActorBegunPlay(), TEXT("Real projectile BeginPlay executed"));
			Check(!First->GetIsReplicated() && Lifecycle->bDamaging, TEXT("Nonreplicated server shot retains damage authority"));
			Check(First->GetLifeSpan() == 0.f && World->GetLatentActionManager().GetNumActionsForObject(First) == 0,
				TEXT("Pool shot has no destructive actor lifespan or legacy Delay"));
			Check(UWeaponProjectileAimLibrary::ConfigureProjectileConvergence(First, FVector(100, 0, 300), FVector::ForwardVector, true), TEXT("Convergence initialized"));
			Movement->TickComponent(0.001f, LEVELTICK_All, &Movement->PrimaryComponentTick);
			Check(First->GetActorLocation().X > 0, TEXT("Movement advances after BeginPlay"));
			Check(Shooter1->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(First), TEXT("Reciprocal shooter ignore applied"));
			const uint64 FirstGeneration = Lifecycle->GetGeneration();
			Check(Lifecycle->Release() && !Lifecycle->Release(), TEXT("Return is idempotent"));
			Check(!Shooter1->GetCapsuleComponent()->GetMoveIgnoreActors().Contains(First), TEXT("Reciprocal ignore removed on return"));
			Check(First->GetOwner() == nullptr && First->GetInstigator() == nullptr && First->IsHidden() && !First->GetActorEnableCollision(), TEXT("Returned actor releases context and collision"));
			for (UActorComponent* Component : First->GetComponents()) Check(!Component->IsComponentTickEnabled(), TEXT("Returned component tick disabled"));
			AActor* DuringCallback = Pool->Acquire(Class, Shot, Weapon, Data, Shooter2, false, 2);
			Check(DuringCallback && DuringCallback != First, TEXT("Returning actor cannot be reacquired in the current callback"));
			if (DuringCallback) UProjectilePoolLibrary::ReleaseProjectile(DuringCallback);
			Pool->Tick(0.f);
			for (int32 Index = 0; Index < 120; ++Index)
			{
				AActor* Actor = Pool->Acquire(Class, Shot, Weapon, Data, (Index % 2) ? Shooter1 : Shooter2, false, 10 + Index);
				if (!Check(Actor != nullptr, TEXT("Repeated acquisition succeeds"))) break;
				auto* Component = Actor->FindComponentByClass<UProjectilePoolComponent>();
				auto* MoveComponent = Actor->FindComponentByClass<UProjectileMovementComponent>();
				Check(Actor->GetComponents().Num() == ComponentCount, TEXT("No per-shot components accumulate"));
				Check(MoveComponent->UpdatedComponent == Actor->GetRootComponent() && !MoveComponent->Velocity.IsNearlyZero(), TEXT("Movement root and velocity restored"));
				Check(Component->GetGeneration() > FirstGeneration, TEXT("New lease generation issued"));
				if (auto* Exploded = FindFProperty<FBoolProperty>(Class, TEXT("exploded")))
				{
					Check(!Exploded->GetPropertyValue_InContainer(Actor), TEXT("Explosion latch reset"));
					Exploded->SetPropertyValue_InContainer(Actor, true);
				}
				const bool bConverge = (Index % 2) != 0;
				const bool bConfigured = UWeaponProjectileAimLibrary::ConfigureProjectileConvergence(Actor, FVector(100,0,300), FVector::ForwardVector, bConverge);
				Check(bConfigured == bConverge, TEXT("Convergence and straight shots alternate safely"));
				Check(MoveComponent->IsComponentTickEnabled() != bConverge, TEXT("Exactly one movement driver enabled"));
				Component->Release();
				Pool->Tick(0.f);
			}
			Check(Pool->GetPoolStats(Class).Created == 2, TEXT("120 shots reuse the two original actors"));
			AActor* Visual = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, false, 900);
			AActor* Authority = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, true, 900);
			Check(UWeaponProjectileAimLibrary::DestroyProjectileVisualsById(Weapon, 900) == 1, TEXT("Server retirement returns only matching visual copy"));
			Check(UWeaponProjectileAimLibrary::DestroyProjectileVisualsById(Weapon, 900) == 0, TEXT("Duplicate retirement ignored"));
			if (Authority) Check(Authority->FindComponentByClass<UProjectilePoolComponent>()->IsShotActive(), TEXT("Authority shot survives cosmetic retirement"));
			UProjectilePoolLibrary::ReleaseProjectile(Authority);
			Pool->Tick(0.f);
			// Real BP collision callback must return the actor; this is not an editor-only construction fixture.
			AActor* Wall = World->SpawnActor<AActor>();
			auto* Box = NewObject<UBoxComponent>(Wall);
			Wall->SetRootComponent(Box); Box->SetBoxExtent(FVector(10, 200, 200));
			Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Box->SetCollisionResponseToAllChannels(ECR_Block); Box->RegisterComponent();
			Wall->SetActorLocation(FVector(100, 0, 300));
			AActor* HitActor = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, false, 901);
			if (HitActor)
			{
				auto* MoveComponent = HitActor->FindComponentByClass<UProjectileMovementComponent>();
				MoveComponent->TickComponent(0.04f, LEVELTICK_All, &MoveComponent->PrimaryComponentTick);
				auto* WorldCollision = HitActor->FindComponentByClass<UProjectileWorldCollisionComponent>();
				WorldCollision->TickComponent(0.04f, LEVELTICK_All, &WorldCollision->PrimaryComponentTick);
				Check(!HitActor->IsActorBeingDestroyed() && HitActor->FindComponentByClass<UProjectilePoolComponent>()->State == EProjectilePoolState::Returning, TEXT("Saved BP impact graph returns rather than destroys"));
			}
			Wall->Destroy(); Pool->Tick(0.f);
			AActor* Expiring = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, false, 902);
			if (Expiring)
			{
				++GFrameCounter;
				World->GetTimerManager().Tick(0.01f);
				++GFrameCounter;
				World->GetTimerManager().Tick(bRocket ? 21.f : 0.6f);
				Check(Expiring->FindComponentByClass<UProjectilePoolComponent>()->State == EProjectilePoolState::Returning,
					TEXT("Range/speed expiry timer returns the shot"));
				Pool->Tick(0.f);
			}
			AActor* Exceptional = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, false, 903);
			if (Exceptional) Exceptional->Destroy();
			Check(Pool->GetPoolStats(Class).Active == 0 && Pool->GetPoolStats(Class).Returning == 0, TEXT("Exceptional Destroy removes active index and counts"));
			Enabled->Set(0, ECVF_SetByCode);
			AActor* Overflow = Pool->Acquire(Class, Shot, Weapon, Data, Shooter1, false, 904);
			Check(Overflow && !Overflow->FindComponentByClass<UProjectilePoolComponent>()->bRetained, TEXT("Pool-disabled path uses a temporary actor"));
			UProjectilePoolLibrary::ReleaseProjectile(Overflow); Pool->Tick(0.f);
			Check(Overflow && Overflow->IsActorBeingDestroyed(), TEXT("Temporary actor is destroyed after its callback finishes"));
			Enabled->Set(1, ECVF_SetByCode);
			const FProjectilePoolStats Stats = Pool->GetPoolStats(Class);
			TSharedRef<FJsonObject> ClassReport = MakeShared<FJsonObject>();
			ClassReport->SetNumberField(TEXT("created"), Stats.Created);
			ClassReport->SetNumberField(TEXT("reused"), Stats.Reused);
			ClassReport->SetNumberField(TEXT("idle"), Stats.Idle);
			ClassReport->SetNumberField(TEXT("active"), Stats.Active);
			Report->SetObjectField(Kind, ClassReport);
			Weapon->Destroy();
		}
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}
	else Check(false, TEXT("Test world created"));
	Enabled->Set(PreviousEnabled, ECVF_SetByCode);
	Report->SetBoolField(TEXT("success"), Failures.IsEmpty());
	Report->SetNumberField(TEXT("checks"), Checks);
	Report->SetArrayField(TEXT("failures"), Failures);
	FString Json;
	FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Json));
	return Json;
}

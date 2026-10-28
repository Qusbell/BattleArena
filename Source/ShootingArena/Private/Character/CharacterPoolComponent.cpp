#include "Character/CharacterPoolComponent.h"

#include "Character/CharacterPoolSubsystem.h"
#include "CharacterPoolReflection.h"
#include "AIController.h"
#include "AI/Area/AreaManagerSubsystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Buff/BuffPresentationComponent.h"
#include "Combat/RecentAttackerComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Items/DeathDropSubsystem.h"
#include "../../JumpPadFlightComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "../../SpawnSelectionSystem/SpawnSelectionComponent.h"
#include "../../Teleport/PortalTraversalGuard.h"
#include "../../Teleport/SeamlessPortalTraversalSubsystem.h"
#include "TimerManager.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"
#include "Weapon/ProjectileFirstPersonRenderingComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Weapon/ProjectilePoolSubsystem.h"

UCharacterPoolComponent::UCharacterPoolComponent()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterPoolComponent::BeginPlay()
{
	Super::BeginPlay();
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn) return;
	if (Pawn->HasAuthority())
	{
		auto* Controller = Cast<AController>(Pawn->GetOwner());
		auto* Pool = GetWorld()->GetSubsystem<UCharacterPoolSubsystem>();
		Life.bManaged = Pool && Controller && Pool->FindOwnedPawn(Controller) == Pawn;
		AssignedController = Controller;
		Life.SpawnTransform = Pawn->GetActorTransform();
	}
	Pawn->ReceiveControllerChangedDelegate.AddUniqueDynamic(this, &ThisClass::OnControllerChanged);
	bInitialActorTick = Pawn->IsActorTickEnabled();
	if (Pawn->HasAuthority() && Life.bManaged)
		if (auto* Health = CharacterPoolReflection::Component(Pawn, TEXT("BPC_Life_C")))
			if (auto* Death = FindFProperty<FMulticastDelegateProperty>(Health->GetClass(), TEXT("OnDeath")))
			{
				FScriptDelegate Delegate; Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, OnPawnDeath));
				Death->AddDelegate(Delegate, Health, Death->ContainerPtrToValuePtr<void>(Health));
			}
	if (const auto* Character = Cast<ACharacter>(Pawn))
	{
		InitialGravity = Character->GetCharacterMovement()->GravityScale;
		InitialWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
	}
	CaptureTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this] { CapturePresentation(); }));
}

void UCharacterPoolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCharacterPoolComponent, Life);
}

bool UCharacterPoolComponent::IsInactive(const AActor* Actor)
{
	const auto* C = IsValid(Actor) ? Actor->FindComponentByClass<UCharacterPoolComponent>() : nullptr;
	return C && C->Life.bManaged && (C->Life.bDead || C->bResetting);
}

void UCharacterPoolComponent::CapturePresentation()
{
	using namespace CharacterPoolReflection;
	if (auto* Controller = AssignedController.Get(); IsValid(Controller))
	{
		if (auto* Helper = Component(Controller, TEXT("BPC_RespawnHelper_C")))
		{
			const double Delay = Number(Helper, TEXT("Respawn Time"));
			if (Delay > 0.) RespawnDelaySeconds = Delay;
		}
		if (auto* Movement = Component(Controller, TEXT("BPC_VirtualInputAIMovement_C")))
		{
			if (!bControllerBaselineCaptured) { bAIMovementTick = Movement->IsComponentTickEnabled(); bControllerBaselineCaptured = true; }
			else if (!Life.bDead) Movement->SetComponentTickEnabled(bAIMovementTick);
		}
	}
	if (const auto* Pawn = Cast<APawn>(GetOwner()); Pawn && Life.bManaged && !Life.bDead && bHasBeenPossessed && Pawn->GetController()) GetOwner()->SetActorEnableCollision(true);
	if (!Presentation.IsEmpty() || !IsValid(GetOwner()) || Life.bDead) return;
	if (auto* Drone = Cast<AActor>(Object(GetOwner(), TEXT("Target Drone")))) bDroneTick = Drone->IsActorTickEnabled();
	for (UActorComponent* C : GetOwner()->GetComponents())
	{
		if (!IsValid(C) || C == this) continue;
		FPresentation& P = Presentation.AddDefaulted_GetRef();
		P.Component = C; P.bTick = C->IsComponentTickEnabled(); P.bActive = C->IsActive();
		if (auto* Scene = Cast<USceneComponent>(C))
		{
			P.Transform = Scene->GetRelativeTransform(); P.Parent = Scene->GetAttachParent();
			P.Socket = Scene->GetAttachSocketName(); P.bVisible = Scene->IsVisible(); P.bHidden = Scene->bHiddenInGame;
		}
		if (auto* Primitive = Cast<UPrimitiveComponent>(C))
		{
			P.CollisionProfile = Primitive->GetCollisionProfileName(); P.CollisionEnabled = static_cast<uint8>(Primitive->GetCollisionEnabled());
			P.Responses = Primitive->GetCollisionResponseToChannels(); P.ObjectType = Primitive->GetCollisionObjectType(); P.bOverlap = Primitive->GetGenerateOverlapEvents();
		}
	}
}

void UCharacterPoolComponent::OnPawnDeath(AController* InstigatedBy, AActor* DamageCauser)
{
	BeginDeath(AssignedController.Get(), RespawnDelaySeconds);
}

bool UCharacterPoolComponent::BeginDeath(AController* Controller, float Delay)
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->HasAuthority() || !Life.bManaged || GetWorld()->bIsTearingDown || bResetting) return false;
	if (Life.bDead) return true;
	CapturePresentation();
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_BeginDeath);
	Life.bDead = true;
	if (auto* Inventory = CharacterPoolReflection::Component(Pawn, TEXT("BPC_Inventory_C")))
		if (UFunction* GetItem = Inventory->FindFunction(TEXT("GetSelectedItem")))
		{
			FStructOnScope Params(GetItem); Inventory->ProcessEvent(GetItem, Params.GetStructMemory());
			if (const auto* Result = FindFProperty<FObjectPropertyBase>(GetItem, TEXT("selectedItem")))
			{
				UObject* Item = Result->GetObjectPropertyValue_InContainer(Params.GetStructMemory());
				CharacterPoolReflection::Call(Item, TEXT("StopUsing"));
				CharacterPoolReflection::Call(Item, TEXT("StopAbility"));
			}
		}
	if (IsValid(Controller)) AssignedController = Controller;
	RespawnDeadline = GetWorld()->GetTimeSeconds() + FMath::Max(0.01f, FMath::IsFinite(Delay) ? Delay : 5.f);
	if (auto* AI = Cast<AAIController>(Controller))
	{
		AI->StopMovement();
		if (AI->GetBrainComponent()) AI->GetBrainComponent()->StopLogic(TEXT("Pawn died"));
		CharacterPoolReflection::Call(AI, TEXT("Death"));
		ResetAIForLife(AI, Pawn);
	}
	if (auto* Drone = Cast<AActor>(CharacterPoolReflection::Object(Pawn, TEXT("Target Drone")))) Drone->SetActorTickEnabled(false);
	Pawn->ForceNetUpdate();
	const int32 Generation = Life.Generation;
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateWeakLambda(this, [this, Generation]
	{
		if (Life.Generation == Generation && Life.bDead) TryRespawn();
	}), FMath::Max(0.01, RespawnDeadline - GetWorld()->GetTimeSeconds()), false);
	return true;
}

void UCharacterPoolComponent::TryRespawn()
{
	APawn* Pawn = Cast<APawn>(GetOwner());
	AController* Controller = AssignedController.Get();
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown || !Pawn || !Pawn->HasAuthority() || !Life.bDead || bResetting || !IsValid(Controller)) return;
	if (const auto* Pool = World->GetSubsystem<UCharacterPoolSubsystem>(); Pool && Pool->IsReserved(Pawn)) return;
	if (const auto* Ended = World->GetGameState())
		if (const auto* P = FindFProperty<FBoolProperty>(Ended->GetClass(), TEXT("bIsMatchEnded")); P && P->GetPropertyValue_InContainer(Ended)) return;
	if (Controller->GetPawn() && Controller->GetPawn() != Pawn) return;
	const FVector DeathLocation = Pawn->GetActorLocation();
	AGameModeBase* Mode = World->GetAuthGameMode();
	auto* Selection = IsValid(Mode) ? Mode->FindComponentByClass<USpawnSelectionComponent>() : nullptr;
	if (!Selection) return;
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_Respawn);
	DetachControllerLife(Controller, Pawn);
	ResetAIForLife(Controller, Pawn);
	Controller->UnPossess();
	APawn* Result = nullptr; FTransform Transform; int32 Point = INDEX_NONE;
	if (!Selection->RespawnPawn(Controller, Pawn->GetClass(), DeathLocation, Result, Transform, Point))
	{
		// A failed selection/possession must not orphan the participant's only pawn.
		Life.bDead = true;
		World->GetTimerManager().SetTimer(RespawnTimer, this, &ThisClass::TryRespawn, 1.f, false);
	}
}

void UCharacterPoolComponent::DetachControllerLife(AController* Controller, APawn* Pawn)
{
	using namespace CharacterPoolReflection;
	if (!Controller || !Pawn) return;
	RemoveBindings(Component(Pawn, TEXT("BPC_Life_C")), TEXT("OnDeath"), Controller);
	RemoveBindings(Component(Pawn, TEXT("BPC_Inventory_C")), TEXT("OnSwapItem"), Controller);
	Pawn->OnTakeAnyDamage.RemoveAll(Controller);
	Controller->ResetIgnoreMoveInput(); Controller->ResetIgnoreLookInput();
}

void UCharacterPoolComponent::ResetAIForLife(AController* Controller, APawn* Pawn)
{
	using namespace CharacterPoolReflection;
	auto* AI = Cast<AAIController>(Controller);
	if (!AI) return;
	DetachControllerLife(AI, Pawn);
	AI->StopMovement(); AI->ClearFocus(EAIFocusPriority::Gameplay);
	if (AI->GetBrainComponent()) AI->GetBrainComponent()->StopLogic(TEXT("Pawn lifecycle changed"));
	Call(AI, TEXT("Clear Target Lost Timer"));
	Call(AI, TEXT("TargetEnemyDeath"));
	// Keep controller-lifetime caches/characteristics and their periodic score update timer.
	if (auto* TargetTimer = FindFProperty<FStructProperty>(AI->GetClass(), TEXT("LTG Setting Timer")))
		AI->GetWorld()->GetTimerManager().ClearTimer(*TargetTimer->ContainerPtrToValuePtr<FTimerHandle>(AI));
	if (auto* Perception = AI->GetAIPerceptionComponent()) Perception->ForgetAll();
	if (auto* Blackboard = AI->GetBlackboardComponent())
		for (const FName Key : {TEXT("Enemy"), TEXT("Enemy Last Location"), TEXT("Is Enemy Visible"), TEXT("STG"), TEXT("LTG"), TEXT("TourPoint")}) Blackboard->ClearValue(Key);
	Reset(AI, {TEXT("STG"), TEXT("LTG"), TEXT("MyCharacter"), TEXT("CurrentTarget"), TEXT("Chase Points"), TEXT("Cached Aggression Score")});
	if (auto* Movement = Component(AI, TEXT("BPC_VirtualInputAIMovement_C")))
	{
		AI->GetWorld()->GetTimerManager().ClearAllTimersForObject(Movement);
		Movement->SetComponentTickEnabled(false);
		Reset(Movement, {TEXT("Way Route Queue"), TEXT("Path Link Route"), TEXT("Path Points"), TEXT("Path Length"), TEXT("Path Points Index"),
			TEXT("Has Goal"), TEXT("Final Goal"), TEXT("Final Goal Location"), TEXT("Possessed Character"), TEXT("Input Direction"),
			TEXT("Area Route Result"), TEXT("Start Point"), TEXT("Is Stuck"), TEXT("Input Direction When Stuck")});
	}
	if (auto* Areas = AI->GetWorld()->GetSubsystem<UAreaManagerSubsystem>()) Areas->RemoveObserverRiskMemory(AI);
}

void UCharacterPoolComponent::ResetGameplay()
{
	using namespace CharacterPoolReflection;
	AActor* Actor = GetOwner(); UWorld* World = GetWorld();
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_ResetGameplay);
	// A shot from the old life must not mistake this reused pawn for its still-living instigator.
	if (auto* Projectiles = World->GetSubsystem<UProjectilePoolSubsystem>()) Projectiles->DetachInstigatorForLife(Cast<APawn>(Actor));
	// Sight caches survive an Actor reuse. Explicitly forget this life so observers
	// can sense the same Actor again at its new spawn position.
	for (TActorIterator<AAIController> It(World); It; ++It)
	{
		AAIController* Observer = *It;
		if (auto* BB = Observer->GetBlackboardComponent(); BB && BB->GetValueAsObject(TEXT("Enemy")) == Actor)
			Call(Observer, TEXT("TargetEnemyDeath"));
		if (auto* Perception = Observer->GetAIPerceptionComponent())
		{
			Perception->ForgetActor(Actor);
			Perception->RequestStimuliListenerUpdate();
		}
		if (auto* Areas = World->GetSubsystem<UAreaManagerSubsystem>()) Areas->ForgetRecognizedUnit(Actor, Observer);
	}
	World->GetTimerManager().ClearAllTimersForObject(Actor);
	World->GetLatentActionManager().RemoveActionsForObject(Actor);
	for (UActorComponent* C : Actor->GetComponents()) if (C != this)
	{
		World->GetTimerManager().ClearAllTimersForObject(C);
		World->GetLatentActionManager().RemoveActionsForObject(C);
	}
	if (auto* Flight = Actor->FindComponentByClass<UJumpPadFlightComponent>()) Flight->EndFlight();
	if (auto* Skill = Component(Actor, TEXT("BPC_TeleportSkill_C")))
	{
		Call(Skill, TEXT("StopTravelLoopSound"));
		AActor* Projectile = Cast<AActor>(Object(Skill, TEXT("activeSkillProjectile")));
		Reset(Skill, {TEXT("activeSkillProjectile"), TEXT("skillCooldownEndTime"), TEXT("travelLoopAudio")});
		if (IsValid(Projectile)) Projectile->Destroy();
		// The old BeginPlay starts a full cooldown, rather than making the skill instantly ready.
		SetNumber(Skill, TEXT("skillCooldownEndTime"), World->GetTimeSeconds() + Number(Object(Skill, TEXT("skillData")), TEXT("skillCooldown")));
		Call(Skill, TEXT("OnRep_skillCooldownEndTime"));
	}
	if (auto* Buff = Component(Actor, TEXT("BPC_Buff_C")))
	{
		Call(Buff, TEXT("OnBuffTimerFinished"));
		Reset(Buff, {TEXT("ActiveBuffTags"), TEXT("CurrentBuffItem"), TEXT("BuffMap"), TEXT("TagsToRemove"), TEXT("currentBuffRowName"), TEXT("currentModifierType")});
	}
	if (auto* Visuals = Actor->FindComponentByClass<UBuffPresentationComponent>()) Visuals->ClearBuffPresentation();
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_ResetInventory);
		if (auto* Inventory = Component(Actor, TEXT("BPC_Inventory_C")))
		{
			Call(Inventory, TEXT("Clear"));
			Reset(Inventory, {TEXT("All_Items"), TEXT("ItemCache"), TEXT("CachedItemCount"), TEXT("IndexInfo")});
		}
		if (auto* Loadout = Actor->FindComponentByClass<UInitialWeaponLoadoutComponent>()) Loadout->ResetForNextLife();
	}
	if (auto* Credit = Actor->FindComponentByClass<URecentAttackerComponent>()) Credit->ResetForNextLife();
	if (auto* Drops = World->GetSubsystem<UDeathDropSubsystem>()) Drops->ResetForNextLife(Actor);
	Reset(Actor, {TEXT("movingTime"), TEXT("AimTargetLocation"), TEXT("bIsMovementInputBlocked"), TEXT("zoomAimSpeedMultiplier")});
	if (auto* Zoom = Component(Actor, TEXT("BPC_Zoom_C"))) Call(Zoom, TEXT("StopZoom"));
	if (auto* Drone = Cast<AActor>(Object(Actor, TEXT("Target Drone"))))
	{
		Reset(Drone, {TEXT("LookTargetActor"), TEXT("AimOffset"), TEXT("NextAimTarget"), TEXT("CurrentRotation")});
		Drone->SetActorTickEnabled(bDroneTick);
	}
	if (auto* Health = Component(Actor, TEXT("BPC_Life_C")))
	{
		Reset(Health, {TEXT("LifeInfo"), TEXT("DamageWeaponData"), TEXT("OverLifeTimerHandle")});
		Call(Health, TEXT("Init Info"));
		Call(Health, TEXT("OnRep_LifeInfo"));
	}
	ResetAnimationForLife();
	if (auto* Loadout = Actor->FindComponentByClass<UInitialWeaponLoadoutComponent>()) Loadout->GrantLoadout();
}

void UCharacterPoolComponent::ResetAnimationForLife()
{
	// Reinitialize the existing instances/graphs; keep mesh assets, AnimClasses and UObjects.
	TInlineComponentArray<USkeletalMeshComponent*> Meshes(GetOwner());
	for (USkeletalMeshComponent* Mesh : Meshes)
		if (Mesh->GetFName() != TEXT("FP_Mesh") && Mesh->IsVisible() && !Mesh->bHiddenInGame)
			if (UAnimInstance* Anim = Mesh->GetAnimInstance()) Anim->InitializeAnimation();
}

void UCharacterPoolComponent::StopDeathPhysics()
{
	// Static cut caps detach when they start simulating too. Stop all bodies before
	// disabling Actor collision; otherwise the engine reports incompatible simulate options.
	TInlineComponentArray<UPrimitiveComponent*> Bodies(GetOwner());
	for (UPrimitiveComponent* Body : Bodies)
	{
		if (auto* Mesh = Cast<USkeletalMeshComponent>(Body))
		{
			Mesh->SetSimulatePhysics(false);
			Mesh->SetAllBodiesSimulatePhysics(false);
		}
		else if (Body->IsSimulatingPhysics()) Body->SetSimulatePhysics(false);
	}
}

void UCharacterPoolComponent::RestorePresentation(bool bEnableCollision)
{
	AActor* Actor = GetOwner();
	StopDeathPhysics();
	if (auto* Rendering = Actor->FindComponentByClass<UProjectileFirstPersonRenderingComponent>()) Rendering->ResetForPooledLife();
	Actor->SetActorEnableCollision(false);
	for (const FPresentation& P : Presentation)
	{
		UActorComponent* C = P.Component.Get(); if (!IsValid(C)) continue;
		if (auto* Mesh = Cast<USkeletalMeshComponent>(C))
		{
			Mesh->SetSimulatePhysics(false); Mesh->SetAllBodiesSimulatePhysics(false);
			// New equip replication may already have played an FP montage. Do not cancel it.
			if (Mesh->GetFName() != TEXT("FP_Mesh"))
				if (auto* Anim = Mesh->GetAnimInstance()) Anim->StopAllMontages(0.f);
		}
		if (auto* Scene = Cast<USceneComponent>(C))
		{
			if (P.Parent.IsValid() && Scene->GetAttachParent() != P.Parent.Get()) Scene->AttachToComponent(P.Parent.Get(), FAttachmentTransformRules::KeepRelativeTransform, P.Socket);
			// Local FOV/settings can move these after the initial server-side snapshot.
			if (Scene->GetFName() != TEXT("FP_Mesh") && Scene->GetFName() != TEXT("FP_Camera"))
				Scene->SetRelativeTransform(P.Transform, false, nullptr, ETeleportType::TeleportPhysics);
			Scene->SetVisibility(P.bVisible); Scene->SetHiddenInGame(P.bHidden);
		}
		if (auto* Primitive = Cast<UPrimitiveComponent>(C))
		{
			if (!P.CollisionProfile.IsNone() && P.CollisionProfile != TEXT("Custom")) Primitive->SetCollisionProfileName(P.CollisionProfile, false);
			Primitive->SetCollisionObjectType(P.ObjectType); Primitive->SetCollisionResponseToChannels(P.Responses);
			Primitive->SetGenerateOverlapEvents(P.bOverlap);
			Primitive->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(P.CollisionEnabled));
		}
		if (auto* Audio = Cast<UAudioComponent>(C)) Audio->Stop();
		if (auto* FX = Cast<UNiagaraComponent>(C)) { FX->DeactivateImmediate(); if (P.bActive) FX->Activate(true); }
		else if (C->IsActive() != P.bActive) C->SetActive(P.bActive);
		C->SetComponentTickEnabled(P.bTick);
	}
	if (auto* Character = Cast<ACharacter>(Actor))
	{
		Character->UnCrouch(); Character->StopJumping(); Character->ConsumeMovementInputVector();
		auto* Movement = Character->GetCharacterMovement();
		Movement->SetUpdatedComponent(Character->GetCapsuleComponent());
		Movement->StopMovementImmediately(); Movement->ClearAccumulatedForces();
		Movement->GravityScale = InitialGravity; Movement->MaxWalkSpeed = InitialWalkSpeed;
		Movement->SetMovementMode(MOVE_Walking);
	}
	CharacterPoolReflection::Reset(Actor, {TEXT("bIsMovementInputBlocked"), TEXT("zoomAimSpeedMultiplier")});
	if (auto* Zoom = CharacterPoolReflection::Component(Actor, TEXT("BPC_Zoom_C")))
	{
		CharacterPoolReflection::Call(Zoom, TEXT("StopZoom"));
		if (auto* Camera = Cast<UCameraComponent>(CharacterPoolReflection::Object(Zoom, TEXT("cameraRef"))))
		{
			const double FOV = CharacterPoolReflection::Number(Zoom, TEXT("defaultFov"));
			if (FOV > 1. && FOV < 179.) Camera->SetFieldOfView(FOV);
		}
	}
	CharacterPoolReflection::Call(Actor, TEXT("ResetPooledPlayerPresentation"));
	Actor->SetActorTickEnabled(bInitialActorTick);
	Actor->SetActorHiddenInGame(false); Actor->SetActorEnableCollision(bEnableCollision);
}

bool UCharacterPoolComponent::PrepareRespawn(const FTransform& Transform)
{
	if (!GetOwner()->HasAuthority() || !Life.bManaged || !Life.bDead || bResetting) return false;
	TRACE_CPUPROFILER_EVENT_SCOPE(CharacterPool_ResetLife);
	TGuardValue<bool> Resetting(bResetting, true);
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	StopDeathPhysics();
	GetOwner()->SetActorEnableCollision(false);
	if (auto* Traveller = GetWorld()->GetSubsystem<USeamlessPortalTraversalSubsystem>()) Traveller->ForgetCharacter(Cast<ACharacter>(GetOwner()));
	PortalTraversalGuard::Unlock(GetOwner());
	RestorePresentation(false);
	FTransform Placement = Transform;
	// SpawnActorDeferred multiplies the requested scale by the BP root default.
	// SetActorTransform replaces it, so explicitly preserve the same spawn contract.
	const USceneComponent* TemplateRoot = GetOwner()->GetClass()->GetDefaultObject<AActor>()->GetRootComponent();
	// CDO world transforms can be uninitialized; spawning reads the root's relative transform.
	Placement.SetScale3D(Transform.GetScale3D() * (TemplateRoot ? TemplateRoot->GetRelativeScale3D() : FVector::OneVector));
	if (const auto* Character = Cast<ACharacter>(GetOwner()))
	{
		// Try to resolve spawn penetration without enabling overlaps on the old corpse.
		const auto* Capsule = Character->GetCapsuleComponent();
		const float SpawnShapeScale = Placement.GetScale3D().GetAbsMin();
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(
			Capsule->GetUnscaledCapsuleRadius() * SpawnShapeScale, Capsule->GetUnscaledCapsuleHalfHeight() * SpawnShapeScale);
		FCollisionQueryParams Query(SCENE_QUERY_STAT(CharacterPoolSpawn), false, GetOwner());
		const FCollisionResponseParams Responses(Capsule->GetCollisionResponseToChannels());
		const FVector Requested = Transform.GetLocation(); FVector Candidate = Requested;
		for (int32 Attempt = 0; Attempt < 6; ++Attempt)
		{
			FHitResult Hit;
			if (!GetWorld()->SweepSingleByChannel(Hit, Candidate, Candidate, Transform.GetRotation(), Capsule->GetCollisionObjectType(), Shape, Query, Responses))
			{ Placement.SetLocation(Candidate); break; }
			if (!Hit.bStartPenetrating || Hit.Normal.IsNearlyZero()) break;
			Candidate += Hit.Normal * (Hit.PenetrationDepth + 0.5f);
		}
	}
	GetOwner()->SetActorTransform(Placement, false, nullptr, ETeleportType::TeleportPhysics);
	ResetGameplay();
	Life.SpawnTransform = Placement;
	++Life.Generation; Life.bDead = false; PresentedGeneration = Life.Generation;
	GetOwner()->SetCanBeDamaged(true); GetOwner()->ForceNetUpdate();
	return true;
}

void UCharacterPoolComponent::OnRep_Life()
{
	if (!bReceivedLife)
	{
		bReceivedLife = true;
		PresentedGeneration = Life.Generation;
		return;
	}
	if (!Life.bManaged || Life.Generation == PresentedGeneration) return;
	// Late join receives current presentation, not the history of prior lives.
	if (!Life.bDead)
	{
		TGuardValue<bool> Resetting(bResetting, true);
		GetWorld()->GetTimerManager().ClearAllTimersForObject(GetOwner());
		GetWorld()->GetLatentActionManager().RemoveActionsForObject(GetOwner());
		for (UActorComponent* C : GetOwner()->GetComponents()) if (C != this)
		{
			GetWorld()->GetTimerManager().ClearAllTimersForObject(C);
			GetWorld()->GetLatentActionManager().RemoveActionsForObject(C);
		}
		if (auto* Skill = CharacterPoolReflection::Component(GetOwner(), TEXT("BPC_TeleportSkill_C"))) CharacterPoolReflection::Call(Skill, TEXT("StopTravelLoopSound"));
		if (auto* Flight = GetOwner()->FindComponentByClass<UJumpPadFlightComponent>()) Flight->EndFlight();
		if (auto* Projectiles = GetWorld()->GetSubsystem<UProjectilePoolSubsystem>()) Projectiles->DetachInstigatorForLife(Cast<APawn>(GetOwner()));
		RestorePresentation(false);
		GetOwner()->SetActorTransform(Life.SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
		ResetAnimationForLife();
		GetOwner()->SetActorEnableCollision(true);
	}
	PresentedGeneration = Life.Generation;
}

void UCharacterPoolComponent::OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	if (!Life.bManaged || !Pawn->HasAuthority() || !IsValid(NewController)) return;
	AssignedController = NewController;
	if (auto* Pool = GetWorld()->GetSubsystem<UCharacterPoolSubsystem>()) Pool->BindController(Pawn, NewController);
	bHasBeenPossessed = true;
	if (bReconnectClaim)
		ReconnectTimer = GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this] { CompleteReconnect(); }));
}

void UCharacterPoolComponent::MarkReconnectClaim()
{
	bReconnectClaim = true;
}

void UCharacterPoolComponent::CompleteReconnect()
{
	bReconnectClaim = false;
	if (!Life.bDead) return;
	AController* Controller = AssignedController.Get();
	if (!IsValid(Controller)) return;
	const double Remaining = RespawnDeadline - GetWorld()->GetTimeSeconds();
	if (Remaining <= 0.) { TryRespawn(); return; }
	// Rebuild only controller/UI bindings. HP, inventory, buffs and cooldown deadlines stay on the reserved pawn.
	using namespace CharacterPoolReflection;
	if (UFunction* Death = Controller->FindFunction(TEXT("OnPawnDeath")))
	{
		FStructOnScope Params(Death);
		if (UActorComponent* Health = Component(GetOwner(), TEXT("BPC_Life_C")))
			if (const auto* Info = FindFProperty<FStructProperty>(Health->GetClass(), TEXT("LifeInfo")))
				for (TFieldIterator<FObjectPropertyBase> It(Info->Struct); It; ++It)
					for (const FName Parameter : {FName(TEXT("InstigatedBy")), FName(TEXT("DamageCauser"))})
						if (It->GetName().StartsWith(Parameter.ToString() + TEXT("_")))
							if (auto* Dest = FindFProperty<FObjectPropertyBase>(Death, Parameter))
								Dest->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), It->GetObjectPropertyValue_InContainer(Info->ContainerPtrToValuePtr<void>(Health)));
		Controller->ProcessEvent(Death, Params.GetStructMemory());
	}
	SetNumber(Controller, TEXT("LastRespawnDuration"), Remaining);
	if (UFunction* Show = Controller->FindFunction(TEXT("ClientShowDeathScreen")))
	{
		FStructOnScope Params(Show);
		if (auto* Duration = FindFProperty<FNumericProperty>(Show, TEXT("RespawnDuration"))) Duration->SetFloatingPointPropertyValue(Duration->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), Remaining);
		if (const auto* Source = FindFProperty<FStructProperty>(Controller->GetClass(), TEXT("LastDeathInfo ")))
			if (const auto* Dest = FindFProperty<FStructProperty>(Show, TEXT("DamageInfo")); Dest && Dest->Struct == Source->Struct)
				Dest->CopyCompleteValue(Dest->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), Source->ContainerPtrToValuePtr<void>(Controller));
		if (const auto* Source = FindFProperty<FStrProperty>(Controller->GetClass(), TEXT("KillerNameStr")))
			if (const auto* Dest = FindFProperty<FStrProperty>(Show, TEXT("KillerName")))
				Dest->SetPropertyValue_InContainer(Params.GetStructMemory(), Source->GetPropertyValue_InContainer(Controller));
		Controller->ProcessEvent(Show, Params.GetStructMemory());
	}
	const int32 Generation = Life.Generation;
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateWeakLambda(this, [this, Generation]
	{
		if (Life.Generation == Generation && Life.bDead) TryRespawn();
	}), Remaining, false);
}

void UCharacterPoolComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
		if (auto* Pool = GetWorld()->GetSubsystem<UCharacterPoolSubsystem>()) Pool->ForgetPawn(Cast<APawn>(GetOwner()));
	}
	if (auto* Health = CharacterPoolReflection::Component(GetOwner(), TEXT("BPC_Life_C")))
		if (auto* Death = FindFProperty<FMulticastDelegateProperty>(Health->GetClass(), TEXT("OnDeath")))
		{
			FScriptDelegate Delegate; Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, OnPawnDeath));
			Death->RemoveDelegate(Delegate, Health, Death->ContainerPtrToValuePtr<void>(Health));
		}
	if (auto* Pawn = Cast<APawn>(GetOwner())) Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::OnControllerChanged);
	Presentation.Reset();
	Super::EndPlay(Reason);
}

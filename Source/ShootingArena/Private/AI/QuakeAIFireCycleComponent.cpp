#include "AI/QuakeAIFireCycleComponent.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "Character/CharacterPoolComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "QuakeAIFireCycleState.h"
#include "TimerManager.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

namespace QuakeFireCycle
{
	constexpr float UpdateInterval = 0.02f;
	const FName EnemyKey(TEXT("Enemy"));
	const FName KillSwitchKey(TEXT("Debug_KillSwitch"));
}

UQuakeAIFireCycleComponent::UQuakeAIFireCycleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void UQuakeAIFireCycleComponent::BeginPlay()
{
	Super::BeginPlay();
	// Possession/blackboard initialization may precede component activation.
	ObserveBlackboard();
}

void UQuakeAIFireCycleComponent::InitializeForPawn(APawn* Pawn)
{
	ResetForLife();
	LifePawn = Pawn;
	bLifeEnded = !IsValid(Pawn);
	if (!IsValid(Pawn) || !Pawn->HasAuthority()) return;
	UsingItemFunction = Pawn->FindFunction(TEXT("UsingItem"));
	for (UActorComponent* Component : Pawn->GetComponents())
	{
		if (Component->GetClass()->GetName() != TEXT("BPC_Life_C")) continue;
		if (auto* Death = FindFProperty<FMulticastDelegateProperty>(Component->GetClass(), TEXT("OnDeath")))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, OnPawnDeath));
			Death->AddDelegate(Delegate, Component, Death->ContainerPtrToValuePtr<void>(Component));
			LifeComponent = Component;
		}
		break;
	}
}

void UQuakeAIFireCycleComponent::ObserveBlackboard()
{
	auto* Controller = Cast<AAIController>(GetOwner());
	UBlackboardComponent* Blackboard = Controller ? Controller->GetBlackboardComponent() : nullptr;
	if (ObservedBlackboard.Get() != Blackboard)
	{
		if (auto* Previous = ObservedBlackboard.Get()) Previous->UnregisterObserversFrom(this);
		ObservedBlackboard = Blackboard;
		if (Blackboard)
		{
			for (FName Name : {QuakeFireCycle::EnemyKey, QuakeFireCycle::KillSwitchKey})
			{
				const FBlackboard::FKey Key = Blackboard->GetKeyID(Name);
				if (Key != FBlackboard::InvalidKey)
					Blackboard->RegisterObserver(Key, this,
						FOnBlackboardChangeNotification::CreateUObject(this, &ThisClass::OnBlackboardChanged));
			}
		}
	}
	RefreshCombat();
}

EBlackboardNotificationResult UQuakeAIFireCycleComponent::OnBlackboardChanged(
	const UBlackboardComponent&, FBlackboard::FKey)
{
	RefreshCombat();
	return EBlackboardNotificationResult::ContinueObserving;
}

void UQuakeAIFireCycleComponent::RefreshCombat()
{
	const auto* Blackboard = ObservedBlackboard.Get();
	const auto* Pawn = LifePawn.Get();
	if (!IsActive() || bLifeEnded || !IsValid(Pawn) || !Pawn->HasAuthority() ||
		UCharacterPoolComponent::IsInactive(Pawn) || !Blackboard ||
		Blackboard->GetValueAsBool(QuakeFireCycle::KillSwitchKey) ||
		!IsValid(Blackboard->GetValueAsObject(QuakeFireCycle::EnemyKey)))
	{
		ResetCombat();
		return;
	}
	// Before the first service activation BP has not supplied the fixed settings yet.
	if (!bHasCharacteristic) return;
	if (bCombatActive) return; // Target/weapon/BT branch changes preserve the phase and deadline.
	bCombatActive = true;
	BeginNextPhase(GetWorld()->GetTimeSeconds());
	FTimerManagerTimerParameters TimerParameters;
	TimerParameters.bLoop = true;
	TimerParameters.bMaxOncePerFrame = true;
	GetWorld()->GetTimerManager().SetTimer(UpdateTimer, this, &ThisClass::UpdateCycle,
		QuakeFireCycle::UpdateInterval, TimerParameters);
}

void UQuakeAIFireCycleComponent::SetFireGateForController(AAIController* Controller, UObject* Service, bool bAllowed)
{
	if (IsValid(Controller))
		if (auto* Cycle = Controller->FindComponentByClass<UQuakeAIFireCycleComponent>())
			Cycle->SetFireGate(Service, bAllowed);
}

void UQuakeAIFireCycleComponent::ActivateFireGateForController(
	AAIController* Controller, UObject* Service, double AimSkill, double FireThrottle)
{
	if (!IsValid(Controller) || !IsValid(Service)) return;
	if (auto* Cycle = Controller->FindComponentByClass<UQuakeAIFireCycleComponent>())
	{
		Cycle->CachedAimSkill = AimSkill;
		Cycle->CachedFireThrottle = FireThrottle;
		Cycle->bHasCharacteristic = true;
		Cycle->SetFireGate(Service, true);
	}
}

void UQuakeAIFireCycleComponent::SetFireGate(UObject* Service, bool bAllowed)
{
	if (!IsValid(Service)) return;
	if (bAllowed)
	{
		ObserveBlackboard();
		if (bCombatActive) ActiveService = Service;
	}
	else if (ActiveService.Get() == Service)
	{
		ActiveService.Reset();
		SendWeaponInput(false);
	}
}

void UQuakeAIFireCycleComponent::UpdateCycle()
{
	const auto* Controller = Cast<AAIController>(GetOwner());
	const UBrainComponent* Brain = Controller ? Controller->GetBrainComponent() : nullptr;
	if (!Brain || !Brain->IsRunning() || Brain->IsPaused() || Controller->GetPawn() != LifePawn.Get())
	{
		ResetCombat();
		return;
	}
	RefreshCombat();
	if (!bCombatActive) return;
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now > PhaseEndTime) BeginNextPhase(Now);
	if (ActiveService.IsValid()) SendWeaponInput(bFirePhase);
}

void UQuakeAIFireCycleComponent::BeginNextPhase(double Now)
{
	if (!bHasCharacteristic || CachedAimSkill <= 0. || !FMath::IsFinite(CachedFireThrottle))
	{
		bFirePhase = false;
		PhaseEndTime = Now;
		return;
	}
	FQuakeAIFireCycleState Next;
	Next.BeginNext(Now, CachedFireThrottle, FMath::FRand());
	bFirePhase = Next.bFiring;
	PhaseEndTime = Next.EndTime;
}

void UQuakeAIFireCycleComponent::SendWeaponInput(bool bUse)
{
	APawn* Pawn = LifePawn.Get();
	if (!IsValid(Pawn) || !UsingItemFunction) return;
	FStructOnScope Params(UsingItemFunction);
	if (const auto* Use = FindFProperty<FBoolProperty>(UsingItemFunction, TEXT("use")))
	{
		Use->SetPropertyValue_InContainer(Params.GetStructMemory(), bUse);
		Pawn->ProcessEvent(UsingItemFunction, Params.GetStructMemory());
	}
}

void UQuakeAIFireCycleComponent::ResetCombat()
{
	const bool bHadCombat = bCombatActive || ActiveService.IsValid();
	bCombatActive = false;
	bFirePhase = false;
	PhaseEndTime = 0.;
	ActiveService.Reset();
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(UpdateTimer);
	if (bHadCombat) SendWeaponInput(false);
}

void UQuakeAIFireCycleComponent::OnPawnDeath(AController*, AActor*)
{
	bLifeEnded = true;
	ResetCombat();
}

void UQuakeAIFireCycleComponent::UnbindLife()
{
	if (auto* Life = LifeComponent.Get())
		if (auto* Death = FindFProperty<FMulticastDelegateProperty>(Life->GetClass(), TEXT("OnDeath")))
		{
			FScriptDelegate Delegate;
			Delegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(ThisClass, OnPawnDeath));
			Death->RemoveDelegate(Delegate, Life, Death->ContainerPtrToValuePtr<void>(Life));
		}
	LifeComponent.Reset();
}

void UQuakeAIFireCycleComponent::ResetForLife()
{
	bLifeEnded = true;
	ResetCombat();
	UnbindLife();
	if (auto* Blackboard = ObservedBlackboard.Get()) Blackboard->UnregisterObserversFrom(this);
	ObservedBlackboard.Reset();
	LifePawn.Reset();
	UsingItemFunction = nullptr;
	bHasCharacteristic = false;
	CachedAimSkill = 0.;
	CachedFireThrottle = 0.;
}

void UQuakeAIFireCycleComponent::Deactivate()
{
	ResetCombat();
	Super::Deactivate();
}

void UQuakeAIFireCycleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ResetForLife();
	Super::EndPlay(EndPlayReason);
}

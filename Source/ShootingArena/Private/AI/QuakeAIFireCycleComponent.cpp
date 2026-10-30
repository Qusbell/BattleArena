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
	CharacteristicFunction = GetOwner()->FindFunction(TEXT("Get Characteristic"));
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
	double AimSkill = 0., Throttle = 0.;
	if (!ReadCharacteristic(AimSkill, Throttle) || AimSkill <= 0. || !FMath::IsFinite(Throttle))
	{
		bFirePhase = false;
		PhaseEndTime = Now;
		return;
	}
	FQuakeAIFireCycleState Next;
	Next.BeginNext(Now, Throttle, FMath::FRand());
	bFirePhase = Next.bFiring;
	PhaseEndTime = Next.EndTime;
}

bool UQuakeAIFireCycleComponent::ReadCharacteristic(double& AimSkill, double& Throttle)
{
	if (!CharacteristicFunction) return false;
	FStructOnScope Params(CharacteristicFunction);
	GetOwner()->ProcessEvent(CharacteristicFunction, Params.GetStructMemory());
	bool bAimFound = false, bThrottleFound = false;
	for (TFieldIterator<FStructProperty> It(CharacteristicFunction); It; ++It)
	{
		const FStructProperty* Struct = *It;
		if (!Struct->HasAllPropertyFlags(CPF_Parm | CPF_OutParm)) continue;
		const void* Data = Struct->ContainerPtrToValuePtr<void>(Params.GetStructMemory());
		for (TFieldIterator<FNumericProperty> Field(Struct->Struct); Field; ++Field)
		{
			if (!Field->IsFloatingPoint()) continue;
			const double Value = Field->GetFloatingPointPropertyValue(Field->ContainerPtrToValuePtr<void>(Data));
			if (Field->GetName().StartsWith(TEXT("Aim_Skill_"))) { AimSkill = Value; bAimFound = true; }
			if (Field->GetName().StartsWith(TEXT("Fire_Throttle_"))) { Throttle = Value; bThrottleFound = true; }
		}
	}
	// The original service reads the struct directly; it does not branch on the Valid output.
	return bAimFound && bThrottleFound;
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
	CharacteristicFunction = nullptr;
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

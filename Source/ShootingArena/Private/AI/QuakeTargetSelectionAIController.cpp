#include "AI/QuakeTargetSelectionAIController.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Sight.h"
#include "UObject/UnrealType.h"

namespace QuakeTargetSelection
{
	struct FCandidate
	{
		ACharacter* Character = nullptr;
		float DistanceSquared = TNumericLimits<float>::Max();
		float DamageDirectionDot = -1.0f;
	};

}

AQuakeTargetSelectionAIController::AQuakeTargetSelectionAIController()
{
}

void AQuakeTargetSelectionAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CharacterTargets.Reset();
	RecentDamageDirection = FVector::ZeroVector;
	RecentDamageExpireTime = 0.0f;

	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddUniqueDynamic(
			this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	if (IsValid(InPawn))
	{
		InPawn->OnTakeAnyDamage.AddUniqueDynamic(this, &ThisClass::HandlePossessedPawnDamaged);
	}
}

void AQuakeTargetSelectionAIController::OnUnPossess()
{
	if (APawn* PossessedPawn = GetPawn())
	{
		PossessedPawn->OnTakeAnyDamage.RemoveDynamic(
			this, &ThisClass::HandlePossessedPawnDamaged);
	}

	CharacterTargets.Reset();
	Super::OnUnPossess();
}

void AQuakeTargetSelectionAIController::GetCharacterTargets_Implementation(
	TArray<ACharacter*>& Characters) const
{
	Characters.Reset();
	Characters.Reserve(CharacterTargets.Num());
	for (ACharacter* TargetCharacter : CharacterTargets)
	{
		if (IsValid(TargetCharacter))
		{
			Characters.Add(TargetCharacter);
		}
	}
}

void AQuakeTargetSelectionAIController::HandleTargetPerceptionUpdated(
	AActor* Actor, const FAIStimulus Stimulus)
{
	if (Stimulus.Type != UAISense::GetSenseID<UAISense_Sight>())
	{
		return;
	}

	ACharacter* TargetCharacter = Cast<ACharacter>(Actor);
	if (!IsValid(TargetCharacter) || TargetCharacter == GetPawn())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		CharacterTargets.AddUnique(TargetCharacter);
	}
	else
	{
		CharacterTargets.Remove(TargetCharacter);
	}
}

void AQuakeTargetSelectionAIController::HandlePossessedPawnDamaged(
	AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	ACharacter* Attacker = ResolveAttacker(InstigatedBy, DamageCauser);
	if (!IsValid(Attacker) || Attacker == GetPawn() || IsCharacterDead(Attacker))
	{
		return;
	}

	RecentDamageDirection =
		(Attacker->GetActorLocation() - DamagedActor->GetActorLocation()).GetSafeNormal();
	RecentDamageExpireTime = GetWorld()->GetTimeSeconds() + RecentDamageWindow;
}

ACharacter* AQuakeTargetSelectionAIController::ResolveAttacker(
	AController* InstigatedBy, AActor* DamageCauser) const
{
	if (IsValid(InstigatedBy))
	{
		if (ACharacter* InstigatorCharacter = Cast<ACharacter>(InstigatedBy->GetPawn()))
		{
			return InstigatorCharacter;
		}
	}

	if (!IsValid(DamageCauser))
	{
		return nullptr;
	}

	if (ACharacter* InstigatorCharacter = Cast<ACharacter>(DamageCauser->GetInstigator()))
	{
		return InstigatorCharacter;
	}

	AActor* DamageOwner = DamageCauser->GetOwner();
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(DamageOwner))
	{
		return OwnerCharacter;
	}
	if (AController* OwnerController = Cast<AController>(DamageOwner))
	{
		return Cast<ACharacter>(OwnerController->GetPawn());
	}

	return nullptr;
}

bool AQuakeTargetSelectionAIController::IsCharacterDead(const ACharacter* Character)
{
	if (!IsValid(Character))
	{
		return true;
	}

	auto QueryDeathState = [](UObject* Object, bool& bOutDead)
	{
		if (!IsValid(Object))
		{
			return false;
		}

		UFunction* Function = Object->FindFunction(TEXT("Is Death"));
		if (!Function)
		{
			Function = Object->FindFunction(TEXT("IsDeath"));
		}
		if (!Function)
		{
			return false;
		}

		TArray<uint8> Params;
		Params.SetNumZeroed(Function->ParmsSize);
		Object->ProcessEvent(Function, Params.GetData());
		for (TFieldIterator<FBoolProperty> It(Function); It; ++It)
		{
			const FBoolProperty* Property = *It;
			if (Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
			{
				bOutDead = Property->GetPropertyValue_InContainer(Params.GetData());
				return true;
			}
		}
		return false;
	};

	bool bDead = false;
	if (QueryDeathState(const_cast<ACharacter*>(Character), bDead))
	{
		return bDead;
	}

	TInlineComponentArray<UActorComponent*> Components(Character);
	for (UActorComponent* Component : Components)
	{
		if (QueryDeathState(Component, bDead))
		{
			return bDead;
		}
	}

	return false;
}

bool AQuakeTargetSelectionAIController::TryGetAggression(
	float& OutScore, float& OutThreshold)
{
	OutScore = 0.0f;
	OutThreshold = 1.0f;

	UFunction* ScoreFunction = FindFunction(TEXT("Try Get Aggression Score"));
	if (!ScoreFunction)
	{
		ScoreFunction = FindFunction(TEXT("TryGetAggressionScore"));
	}
	if (!ScoreFunction)
	{
		return false;
	}

	{
		TArray<uint8> Params;
		Params.SetNumZeroed(ScoreFunction->ParmsSize);
		ProcessEvent(ScoreFunction, Params.GetData());
		bool bValid = false;
		for (TFieldIterator<FProperty> It(ScoreFunction); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
			{
				continue;
			}
			if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
			{
				bValid = BoolProperty->GetPropertyValue_InContainer(Params.GetData());
			}
			else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
			{
				OutScore = static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(Params.GetData()));
			}
			else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
			{
				OutScore = FloatProperty->GetPropertyValue_InContainer(Params.GetData());
			}
		}
		if (!bValid)
		{
			return false;
		}
	}

	UFunction* CharacteristicFunction = FindFunction(TEXT("Get Characteristic"));
	if (!CharacteristicFunction)
	{
		CharacteristicFunction = FindFunction(TEXT("GetCharacteristic"));
	}
	if (!CharacteristicFunction)
	{
		return false;
	}

	TArray<uint8> Params;
	Params.SetNumZeroed(CharacteristicFunction->ParmsSize);
	ProcessEvent(CharacteristicFunction, Params.GetData());
	for (TFieldIterator<FStructProperty> It(CharacteristicFunction); It; ++It)
	{
		FStructProperty* StructProperty = *It;
		if (!StructProperty->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			continue;
		}

		void* StructAddress = StructProperty->ContainerPtrToValuePtr<void>(Params.GetData());
		for (TFieldIterator<FProperty> FieldIt(StructProperty->Struct); FieldIt; ++FieldIt)
		{
			FProperty* Field = *FieldIt;
			if (!Field->GetName().Contains(TEXT("Aggression_Threshold")))
			{
				continue;
			}
			if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Field))
			{
				OutThreshold = static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(StructAddress));
				return true;
			}
			if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Field))
			{
				OutThreshold = FloatProperty->GetPropertyValue_InContainer(StructAddress);
				return true;
			}
		}
	}

	return false;
}

void AQuakeTargetSelectionAIController::SelectBestTarget()
{
	APawn* SelfPawn = GetPawn();
	UClass* AcceptedTargetClass = GetAcceptedTargetClass();
	if (!IsValid(SelfPawn) || !AcceptedTargetClass)
	{
		return;
	}

	CharacterTargets.RemoveAll(
		[](const TObjectPtr<ACharacter>& TargetCharacter)
		{
			return !IsValid(TargetCharacter) || IsCharacterDead(TargetCharacter);
		});

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	AActor* CurrentTarget = BlackboardComponent
		? Cast<AActor>(BlackboardComponent->GetValueAsObject(TEXT("Enemy")))
		: nullptr;
	const bool bCurrentVisible = BlackboardComponent &&
		BlackboardComponent->GetValueAsBool(TEXT("Is Enemy Visible"));
	const FVector SelfLocation = SelfPawn->GetActorLocation();
	const float CurrentDistanceSquared = IsValid(CurrentTarget)
		? FVector::DistSquared(SelfLocation, CurrentTarget->GetActorLocation())
		: TNumericLimits<float>::Max();
	const bool bRecentDamage = GetWorld()->GetTimeSeconds() <= RecentDamageExpireTime &&
		!RecentDamageDirection.IsNearlyZero();

	TArray<QuakeTargetSelection::FCandidate> Candidates;
	for (ACharacter* TargetCharacter : CharacterTargets)
	{
		if (!IsValid(TargetCharacter) || !TargetCharacter->IsA(AcceptedTargetClass) ||
			TargetCharacter == SelfPawn || TargetCharacter == CurrentTarget)
		{
			continue;
		}

		const FVector ToCandidate = TargetCharacter->GetActorLocation() - SelfLocation;
		const float DistanceSquared = ToCandidate.SizeSquared();
		if (IsValid(CurrentTarget) && bCurrentVisible && DistanceSquared >= CurrentDistanceSquared)
		{
			continue;
		}

		QuakeTargetSelection::FCandidate& Candidate = Candidates.AddDefaulted_GetRef();
		Candidate.Character = TargetCharacter;
		Candidate.DistanceSquared = DistanceSquared;
		Candidate.DamageDirectionDot = bRecentDamage
			? FVector::DotProduct(RecentDamageDirection, ToCandidate.GetSafeNormal())
			: -1.0f;
	}

	Candidates.StableSort(
		[bRecentDamage](const QuakeTargetSelection::FCandidate& Left,
			const QuakeTargetSelection::FCandidate& Right)
		{
			if (bRecentDamage && !FMath::IsNearlyEqual(
				Left.DamageDirectionDot, Right.DamageDirectionDot))
			{
				return Left.DamageDirectionDot > Right.DamageDirectionDot;
			}
			return Left.DistanceSquared < Right.DistanceSquared;
		});

	float AggressionScore = 0.0f;
	float AggressionThreshold = 1.0f;
	const bool bHasAggression = TryGetAggression(AggressionScore, AggressionThreshold);
	for (const QuakeTargetSelection::FCandidate& Candidate : Candidates)
	{
		const FVector CandidateToSelf =
			(SelfLocation - Candidate.Character->GetActorLocation()).GetSafeNormal();
		const bool bLookingAtSelf = FVector::DotProduct(
			Candidate.Character->GetActorForwardVector(), CandidateToSelf) >= LookDotThreshold;
		if (bLookingAtSelf || (bHasAggression && AggressionScore >= AggressionThreshold))
		{
			ApplySelectedTarget(Candidate.Character);
			return;
		}
	}
}

UClass* AQuakeTargetSelectionAIController::GetAcceptedTargetClass() const
{
	UFunction* SetTargetFunction = FindFunction(TEXT("Set Current Target"));
	if (!SetTargetFunction)
	{
		SetTargetFunction = FindFunction(TEXT("SetCurrentTarget"));
	}
	if (!SetTargetFunction)
	{
		return nullptr;
	}

	for (TFieldIterator<FStructProperty> It(SetTargetFunction); It; ++It)
	{
		FStructProperty* StructProperty = *It;
		if (!StructProperty->HasAnyPropertyFlags(CPF_Parm) ||
			StructProperty->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			continue;
		}

		for (TFieldIterator<FObjectPropertyBase> FieldIt(StructProperty->Struct); FieldIt; ++FieldIt)
		{
			FObjectPropertyBase* TargetProperty = *FieldIt;
			if (TargetProperty->GetName().Contains(TEXT("Target")) &&
				TargetProperty->PropertyClass->IsChildOf(ACharacter::StaticClass()))
			{
				return TargetProperty->PropertyClass;
			}
		}
	}

	return nullptr;
}

void AQuakeTargetSelectionAIController::ApplySelectedTarget(ACharacter* Target)
{
	UClass* AcceptedTargetClass = GetAcceptedTargetClass();
	if (!IsValid(Target) || !AcceptedTargetClass || !Target->IsA(AcceptedTargetClass))
	{
		return;
	}

	UFunction* SetTargetFunction = FindFunction(TEXT("Set Current Target"));
	if (!SetTargetFunction)
	{
		SetTargetFunction = FindFunction(TEXT("SetCurrentTarget"));
	}
	if (!SetTargetFunction)
	{
		return;
	}

	TArray<uint8> Params;
	Params.SetNumZeroed(SetTargetFunction->ParmsSize);
	for (TFieldIterator<FStructProperty> It(SetTargetFunction); It; ++It)
	{
		FStructProperty* StructProperty = *It;
		if (!StructProperty->HasAnyPropertyFlags(CPF_Parm) ||
			StructProperty->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
		{
			continue;
		}

		void* StructAddress = StructProperty->ContainerPtrToValuePtr<void>(Params.GetData());
		for (TFieldIterator<FProperty> FieldIt(StructProperty->Struct); FieldIt; ++FieldIt)
		{
			FProperty* Field = *FieldIt;
			if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Field))
			{
				ObjectProperty->SetObjectPropertyValue_InContainer(StructAddress, Target);
			}
			else if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Field))
			{
				BoolProperty->SetPropertyValue_InContainer(StructAddress, true);
			}
			else if (Field->GetName().Contains(TEXT("VisibleSince")))
			{
				if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Field))
				{
					DoubleProperty->SetPropertyValue_InContainer(StructAddress, GetWorld()->GetTimeSeconds());
				}
				else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Field))
				{
					FloatProperty->SetPropertyValue_InContainer(StructAddress, GetWorld()->GetTimeSeconds());
				}
			}
		}
		break;
	}

	ProcessEvent(SetTargetFunction, Params.GetData());
	if (UFunction* ClearTimerFunction = FindFunction(TEXT("Clear Target Lost Timer")))
	{
		ProcessEvent(ClearTimerFunction, nullptr);
	}
	else if (UFunction* ClearTimerNativeName = FindFunction(TEXT("ClearTargetLostTimer")))
	{
		ProcessEvent(ClearTimerNativeName, nullptr);
	}
}

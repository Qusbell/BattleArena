#include "Camera/ShooterDisplayLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Components/ActorComponent.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

bool UShooterDisplayLibrary::IsShooterAlive(APawn* Shooter)
{
	if (!IsValid(Shooter) || Shooter->IsActorBeingDestroyed()) return false;
	TInlineComponentArray<UActorComponent*> Components(Shooter);
	for (UActorComponent* Component : Components)
	{
		bool bLifeComponent = false;
		for (UClass* Class = Component->GetClass(); Class; Class = Class->GetSuperClass())
			bLifeComponent |= Class->GetName() == TEXT("BPC_Life_C");
		if (!bLifeComponent) continue;
		const FStructProperty* LifeInfo = FindFProperty<FStructProperty>(Component->GetClass(), TEXT("LifeInfo"));
		if (!LifeInfo) continue;
		const void* Value = LifeInfo->ContainerPtrToValuePtr<void>(Component);
		for (TFieldIterator<FProperty> It(LifeInfo->Struct); It; ++It)
			if (It->GetName().StartsWith(TEXT("NowHealth_")))
				if (const FNumericProperty* Health = CastField<FNumericProperty>(*It))
					return Health->GetFloatingPointPropertyValue(Health->ContainerPtrToValuePtr<void>(Value)) > 0.0;
	}
	return true;
}

FText UShooterDisplayLibrary::GetShooterDisplayName(APawn* Shooter)
{
	APlayerState* State = IsValid(Shooter) ? Shooter->GetPlayerState() : nullptr;
	if (!IsValid(State)) return FText::GetEmpty();
	// BP_QuakePlayerState resolves campaign/AI names through DT_Character in GetDisplayName.
	if (UFunction* Function = State->FindFunction(TEXT("GetDisplayName")))
	{
		FStructOnScope Parameters(Function);
		State->ProcessEvent(Function, Parameters.GetStructMemory());
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			if (!It->HasAllPropertyFlags(CPF_Parm | CPF_OutParm)) continue;
			if (const FStrProperty* String = CastField<FStrProperty>(*It))
			{
				const FString Name = String->GetPropertyValue_InContainer(Parameters.GetStructMemory()).TrimStartAndEnd();
				if (!Name.IsEmpty()) return FText::FromString(Name);
			}
			if (const FTextProperty* Text = CastField<FTextProperty>(*It))
			{
				const FText Name = Text->GetPropertyValue_InContainer(Parameters.GetStructMemory());
				if (!Name.IsEmpty()) return Name;
			}
		}
	}
	return FText::FromString(State->GetPlayerName());
}

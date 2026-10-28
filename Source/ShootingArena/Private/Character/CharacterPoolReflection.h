#pragma once

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

// BP-owned gameplay state remains in its existing components. Only named reset contracts are touched.
namespace CharacterPoolReflection
{
inline UObject* Object(const UObject* Owner, const TCHAR* Name)
{
	const auto* P = IsValid(Owner) ? FindFProperty<FObjectPropertyBase>(Owner->GetClass(), Name) : nullptr;
	return P ? P->GetObjectPropertyValue_InContainer(Owner) : nullptr;
}
inline double Number(const UObject* Owner, const TCHAR* Name)
{
	const auto* P = IsValid(Owner) ? FindFProperty<FNumericProperty>(Owner->GetClass(), Name) : nullptr;
	if (!P) return 0.;
	const void* Value = P->ContainerPtrToValuePtr<void>(Owner);
	return P->IsFloatingPoint() ? P->GetFloatingPointPropertyValue(Value) : P->GetSignedIntPropertyValue(Value);
}
inline void SetNumber(UObject* Owner, const TCHAR* Name, double Value)
{
	if (auto* P = IsValid(Owner) ? FindFProperty<FNumericProperty>(Owner->GetClass(), Name) : nullptr)
	{
		void* Data = P->ContainerPtrToValuePtr<void>(Owner);
		if (P->IsFloatingPoint()) P->SetFloatingPointPropertyValue(Data, Value);
		else P->SetIntPropertyValue(Data, static_cast<int64>(Value));
	}
}
inline void Reset(UObject* Owner, std::initializer_list<const TCHAR*> Names)
{
	if (!IsValid(Owner)) return;
	for (const TCHAR* Name : Names)
		if (const auto* P = Owner->GetClass()->FindPropertyByName(Name))
			P->CopyCompleteValue_InContainer(Owner, Owner->GetClass()->GetDefaultObject());
}
inline bool Call(UObject* Owner, const TCHAR* Name)
{
	UFunction* Function = IsValid(Owner) ? Owner->FindFunction(Name) : nullptr;
	if (!Function) return false;
	FStructOnScope Params(Function);
	Owner->ProcessEvent(Function, Params.GetStructMemory());
	return true;
}
inline UActorComponent* Component(AActor* Owner, const TCHAR* ClassName)
{
	if (!IsValid(Owner)) return nullptr;
	for (UActorComponent* C : Owner->GetComponents())
		if (IsValid(C) && C->GetClass()->GetName() == ClassName) return C;
	return nullptr;
}
inline void RemoveBindings(UObject* Object, FName Name, UObject* Listener)
{
	if (!IsValid(Object) || !Listener) return;
	if (auto* P = FindFProperty<FMulticastDelegateProperty>(Object->GetClass(), Name))
		if (const FMulticastScriptDelegate* Delegate = P->GetMulticastDelegate(P->ContainerPtrToValuePtr<void>(Object)))
		{
			FMulticastScriptDelegate Copy = *Delegate;
			Copy.RemoveAll(Listener);
			P->SetMulticastDelegate(P->ContainerPtrToValuePtr<void>(Object), Copy);
		}
}
}

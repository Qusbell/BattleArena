#include "Weapon/InitialWeaponLibrary.h"
#include "Weapon/InitialWeaponLoadoutComponent.h"

bool UInitialWeaponLibrary::IsDefaultWeapon(const AActor* Weapon)
{
	if (!IsValid(Weapon))
	{
		return false;
	}

	const AActor* Owner = Weapon->GetOwner();
	const UInitialWeaponLoadoutComponent* Loadout = IsValid(Owner)
		? Owner->FindComponentByClass<UInitialWeaponLoadoutComponent>()
		: nullptr;
	return Loadout && Loadout->IsDefaultWeapon(Weapon);
}

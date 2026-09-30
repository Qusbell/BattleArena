#include "Weapon/InitialWeaponLoadoutDataAsset.h"

UInitialWeaponLoadoutDataAsset::UInitialWeaponLoadoutDataAsset()
{
	FInitialWeaponEntry& Pistol = DefaultWeapons.AddDefaulted_GetRef();
	Pistol.WeaponClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_Pistol.BP_Weapon_Pistol_C")));
	Pistol.WeaponData = TSoftObjectPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/Data/Item/DA_Pistol.DA_Pistol")));

	FInitialWeaponEntry& MachineGun = GrantedWeapons.AddDefaulted_GetRef();
	MachineGun.WeaponClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/HeldItem/Gun/BP_Weapon_MachineGun.BP_Weapon_MachineGun_C")));
	MachineGun.WeaponData = TSoftObjectPtr<UPrimaryDataAsset>(FSoftObjectPath(TEXT(
		"/Game/QuakeLike_1_0/Data/Item/DA_MachineGun.DA_MachineGun")));
}

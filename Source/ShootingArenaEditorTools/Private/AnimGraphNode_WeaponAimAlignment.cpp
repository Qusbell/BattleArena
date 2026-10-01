#include "AnimGraphNode_WeaponAimAlignment.h"

FText UAnimGraphNode_WeaponAimAlignment::GetControllerDescription() const
{
	return NSLOCTEXT("WeaponAimAlignment", "Description", "Weapon Muzzle Aim Alignment");
}

FText UAnimGraphNode_WeaponAimAlignment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_WeaponAimAlignment::GetTooltipText() const
{
	return NSLOCTEXT("WeaponAimAlignment", "Tooltip", "Aligns the equipped TP weapon's muzzle to its resolved projectile flight target, solving both arms from the incoming pose. Requires WeaponAimAlignmentComponent on the weapon.");
}

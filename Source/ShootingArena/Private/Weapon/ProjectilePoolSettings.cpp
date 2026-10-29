#include "Weapon/ProjectilePoolSettings.h"

FProjectilePoolConfiguration UProjectilePoolSettings::GetConfiguration() const
{
	FProjectilePoolConfiguration Result;
	Result.bEnabled = bEnabled;
	Result.PlasmaCapacity = FMath::Max(0, PlasmaCapacity);
	Result.RocketCapacity = FMath::Max(0, RocketCapacity);
	Result.PlasmaPrewarm = FMath::Clamp(PlasmaPrewarm, 0, Result.PlasmaCapacity);
	Result.RocketPrewarm = FMath::Clamp(RocketPrewarm, 0, Result.RocketCapacity);
	Result.PrewarmPerTick = FMath::Max(1, PrewarmPerTick);
	return Result;
}

#if WITH_EDITOR
void UProjectilePoolSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FProjectilePoolConfiguration Valid = GetConfiguration();
	PlasmaPrewarm = Valid.PlasmaPrewarm; PlasmaCapacity = Valid.PlasmaCapacity;
	RocketPrewarm = Valid.RocketPrewarm; RocketCapacity = Valid.RocketCapacity;
	PrewarmPerTick = Valid.PrewarmPerTick;
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

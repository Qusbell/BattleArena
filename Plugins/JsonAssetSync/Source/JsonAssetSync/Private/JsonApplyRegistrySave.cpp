#include "JsonApplyRegistry.h"

#include "JsonAssetSyncLog.h"
#include "UObject/ObjectSaveContext.h"

namespace
{
	template <typename T>
	void RestoreTarget(TObjectPtr<T>& target, const FString& path)
	{
		if (IsValid(target) || path.IsEmpty())
		{
			return;
		}
		target = LoadObject<T>(nullptr, *path);
		if (!IsValid(target))
		{
			UE_LOG(LogJsonAssetSync, Error,
				TEXT("JSON target could not be loaded: %s. Check server cook output and Blueprint parent classes."), *path);
		}
	}
}

void UJsonApplyRegistry::RestoreCookedTargets()
{
	if (GIsEditor || IsRunningCommandlet())
	{
		return;
	}
	for (FJsonDataTableBinding& binding : dataTableBindings)
	{
		RestoreTarget(binding.targetDataTable, binding.cookedTargetPath);
	}
	for (FJsonCurveTableBinding& binding : curveTableBindings)
	{
		RestoreTarget(binding.targetCurveTable, binding.cookedTargetPath);
	}
	for (FJsonFloatCurveBinding& binding : floatCurveBindings)
	{
		RestoreTarget(binding.targetFloatCurve, binding.cookedTargetPath);
	}
	for (FJsonDataAssetBinding& binding : dataAssetBindings)
	{
		RestoreTarget(binding.targetDataAsset, binding.cookedTargetPath);
	}
}

void UJsonApplyRegistry::PreSave(FObjectPreSaveContext saveContext)
{
	// Capture paths before the cooker serializes hard references. An explicitly
	// cleared target must also clear its saved path, rather than resurrect a target.
	for (FJsonDataTableBinding& binding : dataTableBindings)
	{
		binding.cookedTargetPath = IsValid(binding.targetDataTable) ? binding.targetDataTable->GetPathName() : FString();
	}
	for (FJsonCurveTableBinding& binding : curveTableBindings)
	{
		binding.cookedTargetPath = IsValid(binding.targetCurveTable) ? binding.targetCurveTable->GetPathName() : FString();
	}
	for (FJsonFloatCurveBinding& binding : floatCurveBindings)
	{
		binding.cookedTargetPath = IsValid(binding.targetFloatCurve) ? binding.targetFloatCurve->GetPathName() : FString();
	}
	for (FJsonDataAssetBinding& binding : dataAssetBindings)
	{
		binding.cookedTargetPath = IsValid(binding.targetDataAsset) ? binding.targetDataAsset->GetPathName() : FString();
	}
	Super::PreSave(saveContext);
}

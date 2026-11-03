#pragma once

#include "Engine/AssetUserData.h"
#include "BeamEffectPlaybackData.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UNiagaraComponent;

/** Keeps the beam's dynamic materials alive between plays of this Niagara component. */
UCLASS(Transient)
class UBeamEffectPlaybackData final : public UAssetUserData
{
	GENERATED_BODY()

public:
	UMaterialInstanceDynamic* GetOrCreateMaterial(FName Parameter, UMaterialInterface* Parent);
	void EnableFirstPerson(UNiagaraComponent* Component);

private:
	UFUNCTION()
	void OnSystemFinished(UNiagaraComponent* Component);

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> Materials;
};

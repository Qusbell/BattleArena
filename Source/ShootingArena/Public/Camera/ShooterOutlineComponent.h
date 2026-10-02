#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/ShooterOutlineSettings.h"
#include "ShooterOutlineComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class FSceneViewExtensionBase;
class UMeshComponent;
class FSceneView;

USTRUCT()
struct FShooterOutlineProxy
{
	GENERATED_BODY()
	UPROPERTY(Transient)
	TWeakObjectPtr<UMeshComponent> Source;
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> Shooter;
	UPROPERTY(Transient)
	TObjectPtr<UMeshComponent> Mesh;
};

/** Local PlayerController presentation, automatically inherited by BP_QuakePlayerController. */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UShooterOutlineComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UShooterOutlineComponent();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shooter Outline")
	TSoftObjectPtr<UShooterOutlineSettings> Settings;
	UFUNCTION(BlueprintCallable, Category = "Shooter Outline")
	void SetOutlineStyle(const FShooterOutlineStyle& NewStyle);
	UFUNCTION(BlueprintCallable, Category = "Shooter Outline")
	void SetOutlineEnabled(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = "Shooter Outline")
	void ApplyOutlineSettings(UShooterOutlineSettings* NewSettings);
	UFUNCTION(BlueprintCallable, Category = "Shooter Outline")
	void ResetOutlineStyle();
	UFUNCTION(BlueprintPure, Category = "Shooter Outline")
	bool IsOutlineEnabled() const { return Style.bEnabled; }
	UFUNCTION(BlueprintPure, Category = "Shooter Outline")
	FShooterOutlineStyle GetOutlineStyle() const { return Style; }
	UMaterialInstanceDynamic* GetActiveMaterial() const { return bPresentationActive ? Material.Get() : nullptr; }
	/** Keep per-camera hidden/show-only lists aligned with the original rendered meshes. */
	void ApplyViewVisibility(FSceneView& View) const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FCharacterDisplayRegressionTest;
#endif
	UPROPERTY(Transient)
	FShooterOutlineStyle Style;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;
	UPROPERTY(Transient)
	TSubclassOf<APawn> ShooterClass;
	UPROPERTY(Transient)
	TArray<FShooterOutlineProxy> Proxies;
	TSharedPtr<FSceneViewExtensionBase, ESPMode::ThreadSafe> ViewExtension;
	bool bPresentationActive = false;
	float RefreshInterval = 0.2f;
	float RefreshElapsed = 0.0f;
	int32 StencilValue = 2;
	void RefreshShooters();
	void UpdateProxies();
	void ClearProxies();
	void UpdateMaterial();
	void PrepareMaterial(UMaterialInterface* SourceMaterial);
	bool IsMaterialReady() const;
};

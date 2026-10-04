#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/ShooterNicknameSettings.h"
#include "ShooterNicknameComponent.generated.h"

class UShooterNicknameWidget;
struct FCollisionResponseParams;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShooterAimTargetChanged, APawn*, Target);

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class SHOOTINGARENA_API UShooterNicknameComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UShooterNicknameComponent();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shooter Nickname")
	TSoftObjectPtr<UShooterNicknameSettings> Settings;
	UPROPERTY(BlueprintAssignable, Category = "Shooter Nickname")
	FShooterAimTargetChanged OnAimTargetChanged;
	UFUNCTION(BlueprintCallable, Category = "Shooter Nickname")
	void ApplyNicknameSettings(UShooterNicknameSettings* NewSettings);
	UFUNCTION(BlueprintCallable, Category = "Shooter Nickname")
	void SetNicknameStyle(const FShooterNicknameStyle& NewStyle);
	UFUNCTION(BlueprintCallable, Category = "Shooter Nickname")
	void SetNicknameEnabled(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = "Shooter Nickname")
	void SetNicknameLingerTime(float Seconds);
	UFUNCTION(BlueprintCallable, Category = "Shooter Nickname")
	void SetCrosshairUV(FVector2D UV);
	UFUNCTION(BlueprintCallable, Category = "Shooter Nickname")
	void ResetNicknameStyle();
	UFUNCTION(BlueprintPure, Category = "Shooter Nickname")
	FShooterNicknameStyle GetNicknameStyle() const { return Style; }
	UFUNCTION(BlueprintPure, Category = "Shooter Nickname")
	APawn* GetAimTarget() const { return AimTarget.Get(); }
	UFUNCTION(BlueprintPure, Category = "Shooter Nickname")
	APawn* GetDisplayedTarget() const { return DisplayedTarget.Get(); }
	UFUNCTION(BlueprintPure, Category = "Shooter Nickname")
	FText GetDisplayedNickname() const { return DisplayedName; }
	UFUNCTION(BlueprintPure, Category = "Shooter Nickname")
	bool IsNicknameEnabled() const { return Style.bEnabled; }
	UFUNCTION(BlueprintPure, Category = "Shooter Nickname")
	FLinearColor GetDisplayedNicknameColor() const;
	FVector2D GetCrosshairViewportPosition() const { return CrosshairViewportPosition; }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
	/** Future name providers can override this without changing targeting or the overlay. */
	UFUNCTION(BlueprintNativeEvent, Category = "Shooter Nickname")
	FText ResolveNickname(APawn* Shooter) const;
private:
#if WITH_DEV_AUTOMATION_TESTS
	friend class FCharacterDisplayRegressionTest;
	friend class FCharacterNicknameWeaponTraceTest;
#endif
	UPROPERTY(Transient)
	FShooterNicknameStyle Style;
	UPROPERTY(Transient)
	TSubclassOf<APawn> ShooterClass;
	UPROPERTY(Transient)
	TObjectPtr<UShooterNicknameWidget> Widget;
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> AimTarget;
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> DisplayedTarget;
	UPROPERTY(Transient)
	FText DisplayedName;
	FVector2D CrosshairUV = FVector2D(0.5, 0.5);
	FVector2D CrosshairViewportPosition = FVector2D::ZeroVector;
	ECollisionChannel TraceChannel = ECC_GameTraceChannel2;
	bool bUseEquippedWeaponTraceChannel = true;
	float MaxDistance = 20000.0f;
	double LostAimTime = -1.0;
	bool bEndingPlay = false;
	bool ResolveAimRay(FVector& Origin, FVector& Direction);
	APawn* FindAimTarget(const FVector& Origin, const FVector& Direction);
	ECollisionChannel ResolveDamageTraceChannel(float& Distance, FCollisionResponseParams& Responses) const;
	ECollisionChannel ResolveWeaponDamageTraceChannel(UObject* Data, float& Distance, FCollisionResponseParams& Responses) const;
	void UpdateTarget(APawn* Target);
	void ClearDisplay();
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WeaponAimAlignmentMigrationLibrary.generated.h"

class UAnimBlueprint;
class UBlueprint;
class UWeaponAimAlignmentDataAsset;

UCLASS()
class UWeaponAimAlignmentMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Refuses the original ABP; inserts a correction after the duplicate's existing complete pose. */
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static bool AddAlignmentToDuplicate(UAnimBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static bool AddWeaponAlignmentComponent(UBlueprint* WeaponBlueprint, UWeaponAimAlignmentDataAsset* Settings);
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static bool SetThirdPersonAnimation(UObject* WeaponData, UAnimBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static FString InspectAlignment(UAnimBlueprint* Blueprint, UBlueprint* WeaponBlueprint, UObject* WeaponData);
	/** Adds an AnimInstance self reference only to a duplicated TP ABP's EventGraph. */
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static FString AddAnimationSelfNode(UAnimBlueprint* Blueprint, int32 X, int32 Y);
	/** Adds a downward-only free left arm layer to the plasma duplicate. */
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static bool AddPlasmaFreeHandLayer(UAnimBlueprint* Blueprint);
	/** Reuses the plasma cache to preserve its neutral weapon stance on downward aim. */
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static bool RetargetPlasmaFreeHandStance(UAnimBlueprint* Blueprint);
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static FString InspectPitchPose(UAnimBlueprint* Blueprint);
	/** Offline coordinate verification only. Does not create a world or run gameplay. */
	UFUNCTION(BlueprintCallable, Category="Editor|Weapon Aim Alignment")
	static FString VerifyPitchPoseMath();
};

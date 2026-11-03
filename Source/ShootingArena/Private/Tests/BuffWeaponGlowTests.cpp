#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Buff/BuffPresentationComponent.h"
#include "Buff/BuffWeaponGlow.h"
#include "UI/BuffScreenEffect.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffWeaponGlowTest, "ShootingArena.Buff.WeaponGlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuffWeaponGlowTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	World->InitializeActorsForPlay(FURL());
	APawn* Pawn = World->SpawnActor<APawn>();
	USceneComponent* PawnRoot = NewObject<USceneComponent>(Pawn);
	Pawn->SetRootComponent(PawnRoot);
	PawnRoot->RegisterComponent();
	USkeletalMeshComponent* Body = NewObject<USkeletalMeshComponent>(Pawn, TEXT("CharacterMesh0"));
	USkeletalMeshComponent* Arms = NewObject<USkeletalMeshComponent>(Pawn, TEXT("FP_Mesh"));
	USkeletalMeshComponent* DeathMesh = NewObject<USkeletalMeshComponent>(Pawn, TEXT("DeathMesh"));
	for (USkeletalMeshComponent* Mesh : {Body, Arms, DeathMesh})
	{
		Mesh->SetupAttachment(PawnRoot);
		Mesh->RegisterComponent();
	}
	DeathMesh->SetVisibility(false);
	UBuffPresentationComponent* Buff = NewObject<UBuffPresentationComponent>(Pawn);
	Pawn->AddInstanceComponent(Buff);
	Buff->RegisterComponent();

	UClass* HeldItemClass = LoadClass<AActor>(nullptr, TEXT("/Game/QuakeLike_1_0/HeldItem/BP_HeldItem.BP_HeldItem_C"));
	if (!TestNotNull(TEXT("BP_HeldItem exists"), HeldItemClass)) return false;
	AActor* Weapon = World->SpawnActor<AActor>(HeldItemClass);
	const FObjectPropertyBase* FPProperty = FindFProperty<FObjectPropertyBase>(HeldItemClass, TEXT("FP_Mesh"));
	const FObjectPropertyBase* TPProperty = FindFProperty<FObjectPropertyBase>(HeldItemClass, TEXT("TP_Mesh"));
	if (!TestNotNull(TEXT("BP_HeldItem keeps FP_Mesh"), FPProperty) || !TestNotNull(TEXT("BP_HeldItem keeps TP_Mesh"), TPProperty)) return false;
	auto* FP = Cast<USkeletalMeshComponent>(FPProperty->GetObjectPropertyValue_InContainer(Weapon));
	auto* TP = Cast<USkeletalMeshComponent>(TPProperty->GetObjectPropertyValue_InContainer(Weapon));
	if (!TestNotNull(TEXT("FP mesh"), FP) || !TestNotNull(TEXT("TP mesh"), TP)) return false;
	Weapon->AttachToComponent(PawnRoot, FAttachmentTransformRules::KeepRelativeTransform);

	UMaterialInterface* Overlay = UMaterial::GetDefaultMaterial(MD_Surface);
	UBuffWeaponGlowSettings* Glow = NewObject<UBuffWeaponGlowSettings>(GetTransientPackage());
	Glow->Profiles.SetNum(2);
	FBuffGlowProfile& DefProfile = Glow->Profiles[0];
	DefProfile.BuffTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Def"));
	DefProfile.Gun.OverlayMaterial = Overlay;
	DefProfile.Gun.TintStrength = 0.4f;
	DefProfile.Gun.WaveStrength = 0.8f;
	DefProfile.Body.OverlayMaterial = Overlay;
	DefProfile.Body.TintStrength = 0.08f;
	DefProfile.Body.WaveStrength = 0.0f;
	FBuffGlowProfile& AtkProfile = Glow->Profiles[1];
	AtkProfile.BuffTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"));
	AtkProfile.Gun.OverlayMaterial = Overlay;
	AtkProfile.Gun.TintStrength = 0.9f;
	AtkProfile.Gun.bUseScreenEffectColors = false;
	AtkProfile.Gun.Color1 = FLinearColor::Red;
	AtkProfile.Body.bEnabled = false;
	UBuffScreenEffectSettings* Screen = NewObject<UBuffScreenEffectSettings>(GetTransientPackage());
	Screen->Color1 = FLinearColor(0.1f, 0.2f, 0.9f);
	Buff->WeaponGlowSettings = Glow;
	Buff->ScreenEffectSettings = Screen;

	Buff->UpdateWeaponGlow();
	TestNull(TEXT("No buff, no glow"), FP->GetOverlayMaterial());

	Buff->Presentation.Tag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Def"));
	Buff->Presentation.EndServerTime = World->GetTimeSeconds() + 30.0;
	Buff->UpdateWeaponGlow();
	UMaterialInstanceDynamic* MID = Buff->WeaponGlowMaterial;
	if (!TestNotNull(TEXT("Gun glow material is created from the buff's profile"), MID)) return false;
	TestTrue(TEXT("Defence buff lights the first-person gun"), FP->GetOverlayMaterial() == MID);
	TestTrue(TEXT("Others see the third-person gun glow"), TP->GetOverlayMaterial() == MID);
	TestTrue(TEXT("Colour follows DA_BuffScreenEffect by default"), MID->K2_GetVectorParameterValue(TEXT("Color1")).Equals(Screen->Color1));
	UMaterialInstanceDynamic* BodyMID = Buff->BodyGlowMaterial;
	if (!TestNotNull(TEXT("Body has its own glow material"), BodyMID)) return false;
	TestTrue(TEXT("Others see the third-person body glow"), Body->GetOverlayMaterial() == BodyMID);
	TestEqual(TEXT("Body uses its own tint"), BodyMID->K2_GetScalarParameterValue(TEXT("TintStrength")), DefProfile.Body.TintStrength);
	TestEqual(TEXT("Gun uses its own tint"), MID->K2_GetScalarParameterValue(TEXT("TintStrength")), DefProfile.Gun.TintStrength);
	TestEqual(TEXT("Designer brightness cap reaches the material"), MID->K2_GetScalarParameterValue(TEXT("MaxBrightness")), DefProfile.Gun.MaxBrightness);
	TestEqual(TEXT("Gun wave follows the gun look"), MID->K2_GetScalarParameterValue(TEXT("ShimmerStrength")), DefProfile.Gun.WaveStrength);
	TestEqual(TEXT("Wave colour shift reaches the material"), MID->K2_GetScalarParameterValue(TEXT("WaveColorShift")), DefProfile.Gun.WaveColorShift);
	TestEqual(TEXT("Body wave follows the body look"), BodyMID->K2_GetScalarParameterValue(TEXT("ShimmerStrength")), 0.0f);
	TestNull(TEXT("First-person arms stay plain"), Arms->GetOverlayMaterial());
	TestNull(TEXT("Hidden death meshes stay plain"), DeathMesh->GetOverlayMaterial());
	DefProfile.Body.bEnabled = false;
	Buff->UpdateWeaponGlow();
	TestTrue(TEXT("Body glow can be turned off on its own"), Body->GetOverlayMaterial() == nullptr && FP->GetOverlayMaterial() == MID);
	DefProfile.Body.bEnabled = true;

	Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Buff->UpdateWeaponGlow();
	TestNull(TEXT("A dropped or swapped-out gun stops glowing"), FP->GetOverlayMaterial());
	Weapon->AttachToComponent(PawnRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Buff->UpdateWeaponGlow();
	TestTrue(TEXT("A newly held gun glows at once"), FP->GetOverlayMaterial() == MID);
	TestEqual(TEXT("A gun without Muzzle/GripPoint sockets keeps the DA wave direction"), FP->GetCustomPrimitiveData().Data.Num(), 0);

	Buff->Presentation.EndServerTime = World->GetTimeSeconds();
	Buff->UpdateWeaponGlow();
	TestTrue(TEXT("Expiry removes the glow"), FP->GetOverlayMaterial() == nullptr && TP->GetOverlayMaterial() == nullptr
		&& Body->GetOverlayMaterial() == nullptr);

	Buff->Presentation.Tag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"));
	Buff->Presentation.EndServerTime = World->GetTimeSeconds() + 30.0;
	Buff->UpdateWeaponGlow();
	UMaterialInstanceDynamic* AtkMID = Buff->WeaponGlowMaterial;
	if (!TestNotNull(TEXT("Attack buff builds its own gun glow"), AtkMID)) return false;
	TestTrue(TEXT("Another buff swaps to its own material instance"), AtkMID != MID && FP->GetOverlayMaterial() == AtkMID);
	TestEqual(TEXT("Attack buff uses its own tint"), AtkMID->K2_GetScalarParameterValue(TEXT("TintStrength")), AtkProfile.Gun.TintStrength);
	TestTrue(TEXT("A look can use its own colour"), AtkMID->K2_GetVectorParameterValue(TEXT("Color1")).Equals(FLinearColor::Red));
	TestNull(TEXT("Attack profile without body glow leaves the body plain"), Body->GetOverlayMaterial());
	Glow->bEnabled = false;
	Buff->UpdateWeaponGlow();
	TestNull(TEXT("Designer can disable the glow"), FP->GetOverlayMaterial());
	return !HasAnyErrors();
}

#endif

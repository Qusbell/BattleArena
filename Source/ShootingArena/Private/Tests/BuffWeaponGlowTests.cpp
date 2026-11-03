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

	UBuffWeaponGlowSettings* Glow = NewObject<UBuffWeaponGlowSettings>(GetTransientPackage());
	Glow->OverlayMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	Glow->TintStrength = 0.4f;
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
	if (!TestNotNull(TEXT("Glow material is created from DA_Buff"), MID)) return false;
	TestTrue(TEXT("Defence buff lights the first-person gun"), FP->GetOverlayMaterial() == MID);
	TestTrue(TEXT("Others see the third-person gun glow"), TP->GetOverlayMaterial() == MID);
	TestTrue(TEXT("Colour follows DA_BuffScreenEffect"), MID->K2_GetVectorParameterValue(TEXT("Color1")).Equals(Screen->Color1));
	UMaterialInstanceDynamic* BodyMID = Buff->BodyGlowMaterial;
	if (!TestNotNull(TEXT("Body has its own glow material"), BodyMID)) return false;
	TestTrue(TEXT("Others see the third-person body glow"), Body->GetOverlayMaterial() == BodyMID);
	TestEqual(TEXT("Body tint is fainter than the gun's"), BodyMID->K2_GetScalarParameterValue(TEXT("TintStrength")), Glow->BodyTintStrength);
	TestEqual(TEXT("Gun keeps its full tint"), MID->K2_GetScalarParameterValue(TEXT("TintStrength")), Glow->TintStrength);
	TestEqual(TEXT("Designer brightness cap reaches the material"), MID->K2_GetScalarParameterValue(TEXT("MaxBrightness")), Glow->MaxBrightness);
	TestEqual(TEXT("Body glow has no sweeping wave"), BodyMID->K2_GetScalarParameterValue(TEXT("ShimmerStrength")), 0.0f);
	TestNull(TEXT("First-person arms stay plain"), Arms->GetOverlayMaterial());
	TestNull(TEXT("Hidden death meshes stay plain"), DeathMesh->GetOverlayMaterial());
	Glow->bGlowCharacter = false;
	Buff->UpdateWeaponGlow();
	TestTrue(TEXT("Body glow can be turned off on its own"), Body->GetOverlayMaterial() == nullptr && FP->GetOverlayMaterial() == MID);
	Glow->bGlowCharacter = true;

	Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Buff->UpdateWeaponGlow();
	TestNull(TEXT("A dropped or swapped-out gun stops glowing"), FP->GetOverlayMaterial());
	Weapon->AttachToComponent(PawnRoot, FAttachmentTransformRules::KeepRelativeTransform);
	Buff->UpdateWeaponGlow();
	TestTrue(TEXT("A newly held gun glows at once"), FP->GetOverlayMaterial() == MID);

	Buff->Presentation.EndServerTime = World->GetTimeSeconds();
	Buff->UpdateWeaponGlow();
	TestTrue(TEXT("Expiry removes the glow"), FP->GetOverlayMaterial() == nullptr && TP->GetOverlayMaterial() == nullptr
		&& Body->GetOverlayMaterial() == nullptr);

	Buff->Presentation.Tag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"));
	Buff->Presentation.EndServerTime = World->GetTimeSeconds() + 30.0;
	Glow->bEnabled = false;
	Buff->UpdateWeaponGlow();
	TestNull(TEXT("Designer can disable the glow"), FP->GetOverlayMaterial());
	return !HasAnyErrors();
}

#endif

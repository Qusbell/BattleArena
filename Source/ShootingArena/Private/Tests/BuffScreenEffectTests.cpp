#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Buff/BuffPresentationComponent.h"
#include "UI/BuffScreenEffect.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformMisc.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffScreenEffectTest, "ShootingArena.UI.BuffScreenEffect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBuffScreenEffectTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	UGameInstance* GI = World->GetGameInstance();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
	ON_SCOPE_EXIT { GI->RemoveLocalPlayer(Player); };
	APlayerController* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(Player);
	World->InitializeActorsForPlay(FURL());
	APawn* Pawn = World->SpawnActor<APawn>();
	PC->Possess(Pawn);
	UBuffPresentationComponent* Buff = NewObject<UBuffPresentationComponent>(Pawn);
	Pawn->AddInstanceComponent(Buff);
	Buff->RegisterComponent();
	UBuffScreenEffectSettings* Asset = LoadObject<UBuffScreenEffectSettings>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffScreenEffect.DA_BuffScreenEffect"));
	if (!TestNotNull(TEXT("Designer DA is saved and loadable"), Asset)) return false;
	UBuffScreenEffectSettings* Settings = DuplicateObject<UBuffScreenEffectSettings>(Asset, GetTransientPackage());
	if (!TestNotNull(TEXT("DA holds the existing CompShooter effect"), Settings->EffectClass.Get())) return false;
	// Exercise editable values without relying on the designer's current tuning or changing the asset.
	Settings->bEnabled = true;
	Settings->Opacity = 0.2f;
	Settings->FadeInSeconds = 0.3f;
	Settings->FadeOutSeconds = 0.4f;
	Settings->PulsePeriod = 4.0f;
	Settings->Color1 = FLinearColor(0.05f, 0.3f, 0.9f);
	Settings->ElectricIntensity = 0.3f;
	Settings->ElectricFrequency = 7.0f;
	Settings->ElectricCoverage = 0.6f;
	Settings->BurstIntensity = 1.0f;
	Settings->BurstSeconds = 0.5f;
	Buff->Presentation.Tag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Atk"));
	Buff->Presentation.EndServerTime = World->GetTimeSeconds() + 30.0;
	UTextBlock* TimeText = NewObject<UTextBlock>(Pawn);
	UImage* Icon = NewObject<UImage>(Pawn);
	UCanvasPanel* Canvas = NewObject<UCanvasPanel>(Pawn);
	UVerticalBox* Column = NewObject<UVerticalBox>(Pawn);
	UCanvasPanelSlot* Position = Canvas->AddChildToCanvas(Column);
	UVerticalBoxSlot* TimeSlot = Column->AddChildToVerticalBox(TimeText);
	Column->AddChildToVerticalBox(Icon);
	Buff->ScreenEffectSettings = Settings;
	Settings->StatusPosition = FVector2D(25.0, -90.0);
	Settings->StatusGap = 14.0f;
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Pawn, TimeText, Icon);
	TestEqual(TEXT("Designer position reaches the HUD group"), Position->GetPosition(), Settings->StatusPosition);
	TestEqual(TEXT("Designer spacing is below the time and above the icon"), TimeSlot->GetPadding().Bottom, Settings->StatusGap);
	Settings->StatusPosition = FVector2D(-40.0, -120.0);
	Settings->StatusGap = -5.0f;
	Settings->bEnabled = false;
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Pawn, TimeText, Icon);
	TestEqual(TEXT("Layout edits apply even when screen VFX is disabled"), Position->GetPosition(), Settings->StatusPosition);
	TestEqual(TEXT("Negative spacing is clamped to zero"), TimeSlot->GetPadding().Bottom, 0.0f);
	Settings->bEnabled = true;
	TestEqual(TEXT("HUD shows seconds without the buff name"), TimeText->GetText().ToString(), FString(TEXT("30s")));
	TestTrue(TEXT("Active buff reveals both the icon and the time"), Icon->GetVisibility() == ESlateVisibility::HitTestInvisible
		&& TimeText->GetVisibility() == ESlateVisibility::HitTestInvisible);
	if (!TestTrue(TEXT("Fixture owns a local pawn"), Pawn->IsLocallyControlled())
		|| !TestTrue(TEXT("Fixture buff has an active server deadline"), Buff->GetPresentationRemainingTime() > 0.0)) return false;
	UBuffScreenEffectWidget* Widget = CreateWidget<UBuffScreenEffectWidget>(PC);
	if (!TestTrue(TEXT("Active owning player can initialize the effect"), Widget->InitializeEffect(Buff, Settings))) return false;
	const TSharedRef<SWidget> Slate = Widget->TakeWidget();
	UImage* Mask = Cast<UImage>(Widget->Effect->GetWidgetFromName(TEXT("DamageMask1")));
	if (!TestNotNull(TEXT("Existing CompShooter mask is reused"), Mask)) return false;
	TestTrue(TEXT("Designer color reaches the actual mask"), Mask->GetColorAndOpacity().Equals(Settings->Color1));
	FNumericProperty* Pulse = FindFProperty<FNumericProperty>(Widget->Effect->GetClass(), TEXT("FadeInDuration"));
	if (!TestNotNull(TEXT("CompShooter pulse timing"), Pulse)) return false;
	TestEqual(TEXT("Designer period reaches the real effect"),
		Pulse->GetFloatingPointPropertyValue(Pulse->ContainerPtrToValuePtr<void>(Widget->Effect)), static_cast<double>(Settings->PulsePeriod * 0.5f));
	if (!TestNotNull(TEXT("Electric layer is built from the DA material"), Widget->ElectricMID.Get())) return false;
	TestTrue(TEXT("Electric halo receives the same primary purple as the mask"),
		Widget->ElectricMID->K2_GetVectorParameterValue(TEXT("Color1")).Equals(Settings->Color1));
	TestTrue(TEXT("Electric inner glow receives the secondary colour"),
		Widget->ElectricMID->K2_GetVectorParameterValue(TEXT("Color2")).Equals(Settings->Color2));
	TestTrue(TEXT("Designer frequency reaches the electric material"),
		FMath::IsNearlyEqual(Widget->ElectricMID->K2_GetScalarParameterValue(TEXT("FlickerFPS")), Settings->ElectricFrequency));
	TestTrue(TEXT("Designer coverage reaches the electric material"),
		FMath::IsNearlyEqual(Widget->ElectricMID->K2_GetScalarParameterValue(TEXT("CoverageScale")), Settings->ElectricCoverage));
	UMaterialInstanceDynamic* OriginalElectricMID = Widget->ElectricMID;
	auto Burst = [Widget] { return Widget->ElectricMID->K2_GetScalarParameterValue(TEXT("Burst")); };
	auto Intensity = [Widget] { return Widget->ElectricMID->K2_GetScalarParameterValue(TEXT("Intensity")); };
	TestEqual(TEXT("New effect starts transparent"), Widget->GetRenderOpacity(), 0.0f);
	Widget->NativeTick(FGeometry(), Settings->FadeInSeconds * 0.5f);
	TestTrue(TEXT("Fade in is gradual"), FMath::IsNearlyEqual(Widget->GetRenderOpacity(), 0.5f));
	TestTrue(TEXT("Pickup starts with a strong flash"), FMath::IsNearlyEqual(Burst(), 0.7f, 0.01f));
	Widget->NativeTick(FGeometry(), Settings->FadeInSeconds);
	TestTrue(TEXT("Fade in reaches full"), FMath::IsNearlyEqual(Widget->GetRenderOpacity(), 1.0f));
	TestTrue(TEXT("Designer strength dims only the gradient"), FMath::IsNearlyEqual(Widget->Effect->GetRenderOpacity(), Settings->Opacity));
	Buff->Presentation.EndServerTime += 30.0;
	Widget->NativeTick(FGeometry(), 0.1f);
	TestFalse(TEXT("Refreshing a buff does not remove its effect"), Widget->HasFinished());
	TestTrue(TEXT("Refreshing a buff flashes again"), FMath::IsNearlyEqual(Burst(), 0.8f, 0.01f));
	Widget->NativeTick(FGeometry(), Settings->BurstSeconds);
	TestTrue(TEXT("Colour and burst updates reuse the electric MID"), Widget->ElectricMID == OriginalElectricMID);
	TestTrue(TEXT("Flash settles to the sustained crackle"), Burst() == 0.0f && FMath::IsNearlyEqual(Intensity(), Settings->ElectricIntensity));
	Buff->Presentation.EndServerTime = World->GetTimeSeconds();
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Pawn, TimeText, Icon);
	TestTrue(TEXT("Expiry hides the icon and the time together"), Icon->GetVisibility() == ESlateVisibility::Collapsed
		&& TimeText->GetVisibility() == ESlateVisibility::Collapsed);
	Widget->NativeTick(FGeometry(), Settings->FadeOutSeconds * 0.5f);
	TestTrue(TEXT("Expiry starts a gradual fade out"), Widget->GetRenderOpacity() > 0.0f && Widget->GetRenderOpacity() < 1.0f);
	Widget->NativeTick(FGeometry(), Settings->FadeOutSeconds);
	TestTrue(TEXT("Expiry finishes with zero residual tint"), Widget->HasFinished() && Widget->GetRenderOpacity() == 0.0f);

	Buff->Presentation.EndServerTime = World->GetTimeSeconds() + 10.0;
	UBuffPresentationComponent::UpdateBuffRemainingTimeText(Pawn, TimeText, Icon);
	TestTrue(TEXT("Another buff reveals the icon again"), Icon->GetVisibility() == ESlateVisibility::HitTestInvisible);
	UBuffScreenEffectWidget* Next = CreateWidget<UBuffScreenEffectWidget>(PC);
	TestTrue(TEXT("A later buff creates a fresh effect"), Next->InitializeEffect(Buff, Settings));
	Next->TakeWidget();
	Next->NativeTick(FGeometry(), Settings->FadeInSeconds);
	Buff->OnOwnerDeath(nullptr, nullptr);
	Next->NativeTick(FGeometry(), Settings->FadeOutSeconds);
	TestTrue(TEXT("Death also removes the tint"), Next->HasFinished() && Next->GetRenderOpacity() == 0.0f);

	Buff->Presentation.Tag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Def"));
	Buff->Presentation.EndServerTime = World->GetTimeSeconds() + 10.0;
	Settings->bEnabled = false;
	UBuffScreenEffectWidget* Disabled = CreateWidget<UBuffScreenEffectWidget>(PC);
	TestFalse(TEXT("Designer can disable the effect"), Disabled->InitializeEffect(Buff, Settings));
	Settings->bEnabled = true;
	PC->UnPossess();
	TestFalse(TEXT("Unpossessed/remote pawn cannot tint this player's screen"), Disabled->InitializeEffect(Buff, Settings));
	TestTrue(TEXT("Screen rendering never changes the buff deadline"), Buff->GetPresentationRemainingTime() > 0.0);
	return !HasAnyErrors();
}

#endif

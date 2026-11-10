#include "UI/BuffScreenEffect.h"
#include "UI/UIRecordingSettings.h"

#include "Buff/BuffPresentationComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

bool UBuffScreenEffectWidget::InitializeEffect(UBuffPresentationComponent* Buff, const UBuffScreenEffectSettings* InSettings)
{
	const APawn* Pawn = Buff ? Cast<APawn>(Buff->GetOwner()) : nullptr;
	if (!Pawn || !Pawn->IsLocallyControlled() || Pawn->GetController() != GetOwningPlayer()
		|| !InSettings || !InSettings->bEnabled || !InSettings->EffectClass || Buff->GetPresentationRemainingTime() <= 0.0) return false;
	Settings = InSettings;
	Source = Buff;
	Effect = CreateWidget<UUserWidget>(GetOwningPlayer(), Settings->EffectClass);
	if (!Effect) return false;
	UFunction* Create = Effect->FindFunction(TEXT("CreateDamageFX"));
	if (!Create) return false;
	FStructOnScope Args(Create);
	FStructProperty* Color1 = FindFProperty<FStructProperty>(Create, TEXT("DamageColor1"));
	FStructProperty* Color2 = FindFProperty<FStructProperty>(Create, TEXT("DamageColor2"));
	FBoolProperty* Loop = FindFProperty<FBoolProperty>(Create, TEXT("PulseLoop"));
	FStructProperty* Range = FindFProperty<FStructProperty>(Create, TEXT("PulseOpacityMinMax"));
	FNumericProperty* In = FindFProperty<FNumericProperty>(Create, TEXT("FadeInDuration"));
	FNumericProperty* Out = FindFProperty<FNumericProperty>(Create, TEXT("FadeOutDuration"));
	if (!Color1 || !Color2 || !Loop || !Range || !In || !Out) return false;
	if (Color1->Struct != TBaseStructure<FLinearColor>::Get() || Color2->Struct != TBaseStructure<FLinearColor>::Get()
		|| Range->Struct != TBaseStructure<FVector2D>::Get() || !In->IsFloatingPoint() || !Out->IsFloatingPoint()) return false;
	*Color1->ContainerPtrToValuePtr<FLinearColor>(Args.GetStructMemory()) = Settings->Color1;
	*Color2->ContainerPtrToValuePtr<FLinearColor>(Args.GetStructMemory()) = Settings->Color2;
	Loop->SetPropertyValue_InContainer(Args.GetStructMemory(), true);
	*Range->ContainerPtrToValuePtr<FVector2D>(Args.GetStructMemory()) = FVector2D(FMath::Clamp(Settings->PulseMinimum, 0.0f, 1.0f), 1.0);
	const double HalfPeriod = FMath::Max(0.1f, Settings->PulsePeriod) * 0.5;
	In->SetFloatingPointPropertyValue(In->ContainerPtrToValuePtr<void>(Args.GetStructMemory()), HalfPeriod);
	Out->SetFloatingPointPropertyValue(Out->ContainerPtrToValuePtr<void>(Args.GetStructMemory()), HalfPeriod);
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this);
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	URecordingUIMask* RecordingMask = WidgetTree->ConstructWidget<URecordingUIMask>();
	RecordingMask->Section = ERecordingUISection::Buffs;
	RecordingMask->SetContent(Root);
	WidgetTree->RootWidget = RecordingMask;
	auto AddFill = [Root](UWidget* Child)
	{
		UOverlaySlot* Slot = Root->AddChildToOverlay(Child);
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetVerticalAlignment(VAlign_Fill);
	};
	AddFill(Effect);
	if (Settings->ElectricMaterial)
	{
		ElectricMID = UMaterialInstanceDynamic::Create(Settings->ElectricMaterial, this);
		ElectricMID->SetVectorParameterValue(TEXT("Color1"), Settings->Color1);
		ElectricMID->SetVectorParameterValue(TEXT("Color2"), Settings->Color2);
		UImage* Electric = WidgetTree->ConstructWidget<UImage>();
		Electric->SetBrushFromMaterial(ElectricMID);
		AddFill(Electric);
	}
	// Opacity only dims the gradient so the electric layer is not crushed to 15% as well.
	Effect->SetRenderOpacity(FMath::Clamp(Settings->Opacity, 0.0f, 1.0f));
	BurstTime = Settings->BurstSeconds;
	LastRemaining = Buff->GetPresentationRemainingTime();
	Effect->ProcessEvent(Create, Args.GetStructMemory());
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.0f);
	return true;
}

void UBuffScreenEffectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const UBuffPresentationComponent* Buff = Source.Get();
	const APawn* Pawn = Buff ? Cast<APawn>(Buff->GetOwner()) : nullptr;
	const bool bActive = Settings && Settings->bEnabled && Pawn && Pawn->IsLocallyControlled()
		&& Pawn->GetController() == GetOwningPlayer() && Buff->GetPresentationRemainingTime() > 0.0;
	const float Duration = Settings ? (bActive ? Settings->FadeInSeconds : Settings->FadeOutSeconds) : 0.0f;
	Fade = Duration > 0.0f ? FMath::FInterpConstantTo(Fade, bActive ? 1.0f : 0.0f, InDeltaTime, 1.0f / Duration) : (bActive ? 1.0f : 0.0f);
	// Each opacity write invalidates the widget, so only write when the fade actually moved.
	if (GetRenderOpacity() != Fade) SetRenderOpacity(Fade);
	if (bActive)
	{
		// A refreshed buff raises the remaining time; flash again like a fresh pickup.
		const double Remaining = Buff->GetPresentationRemainingTime();
		if (Remaining > LastRemaining + 0.1) BurstTime = Settings->BurstSeconds;
		LastRemaining = Remaining;
	}
	BurstTime = FMath::Max(0.0f, BurstTime - InDeltaTime);
	if (ElectricMID && Settings)
	{
		const float Burst = Settings->BurstSeconds > 0.0f ? BurstTime / Settings->BurstSeconds : 0.0f;
		// Burst stays at 0 for the whole buff after the flash; skip the per-frame parameter pushes then.
		if (Burst != AppliedBurst)
		{
			AppliedBurst = Burst;
			ElectricMID->SetScalarParameterValue(TEXT("Burst"), Burst);
			ElectricMID->SetScalarParameterValue(TEXT("Intensity"), FMath::Lerp(Settings->ElectricIntensity, Settings->BurstIntensity, Burst));
		}
	}
	if (!bActive && Fade <= 0.0f)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

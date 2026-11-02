#include "UI/BuffScreenEffect.h"

#include "Buff/BuffPresentationComponent.h"
#include "Blueprint/WidgetTree.h"
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
	WidgetTree->RootWidget = Effect;
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
	SetRenderOpacity(Fade * (Settings ? FMath::Clamp(Settings->Opacity, 0.0f, 1.0f) : 0.0f));
	if (!bActive && Fade <= 0.0f)
	{
		bFinished = true;
		RemoveFromParent();
	}
}

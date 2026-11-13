#include "UI/VitalsHUDFeedbackSettings.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/RetainerBox.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	void ApplyLayer(UUserWidget& HUDWidget, FName LayerName, float Opacity, float Blur)
	{
		URetainerBox* Layer = Cast<URetainerBox>(HUDWidget.GetWidgetFromName(LayerName));
		if (!Layer)
		{
			return;
		}

		Layer->SetRenderOpacity(Opacity);
		Layer->SetRetainRendering(Opacity > 0.0f && Blur > 0.0f);
		if (UMaterialInstanceDynamic* Material = Layer->GetEffectMaterial())
		{
			Material->SetScalarParameterValue(TEXT("BlurPixels"), Blur * 4.0f);
		}
	}
}

void UVitalsHUDFeedbackLibrary::ApplyNumberFeedback(UUserWidget* HUDWidget,
	const UVitalsHUDFeedbackSettings* Settings, bool bNonDamage)
{
	if (!HUDWidget || !Settings)
	{
		return;
	}

	const FVitalsHUDFeedbackStyle& Style = bNonDamage ? Settings->NonDamage : Settings->Damage;
	const float Opacity = FMath::Clamp(Style.NumberOpacity, 0.0f, 1.0f);
	const float Blur = FMath::Clamp(Style.NumberBlur, 0.0f, 1.0f);
	ApplyLayer(*HUDWidget, TEXT("NumberFeedbackLayer"), Opacity, Blur);
	ApplyLayer(*HUDWidget, TEXT("MaxNumberFeedbackLayer"),
		FMath::Clamp(Settings->NonDamage.NumberOpacity, 0.0f, 1.0f),
		FMath::Clamp(Settings->NonDamage.NumberBlur, 0.0f, 1.0f));
}

void UVitalsHUDFeedbackLibrary::ApplyColorFeedback(UUserWidget* EffectWidget,
	const UVitalsHUDFeedbackSettings* Settings, UMaterialInterface* BlurMaterial)
{
	const UPanelWidget* Parent = EffectWidget ? EffectWidget->GetParent() : nullptr;
	if (!Parent || !Settings)
	{
		return;
	}

	const FName ParentName = Parent->GetFName();
	const bool bDamage = ParentName == TEXT("OV_DamageAnim");
	if (!bDamage && ParentName != TEXT("OV_HealthAnim") && ParentName != TEXT("OV_ArmorAnim"))
	{
		return;
	}

	const FVitalsHUDFeedbackStyle& Style = bDamage ? Settings->Damage : Settings->NonDamage;
	const float Opacity = FMath::Clamp(Style.EffectOpacity, 0.0f, 1.0f);
	const float Blur = FMath::Clamp(Style.EffectBlur, 0.0f, 1.0f);
	EffectWidget->SetRenderOpacity(Opacity);
	if (UImage* Image = Cast<UImage>(EffectWidget->GetWidgetFromName(TEXT("Image_0"))))
	{
		UObject* Resource = Image->GetBrush().GetResourceObject();
		UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Resource);
		if (!Material && BlurMaterial && Blur > 0.0f)
		{
			if (UTexture* Texture = Cast<UTexture>(Resource))
			{
				Material = UMaterialInstanceDynamic::Create(BlurMaterial, Image);
				Material->SetTextureParameterValue(TEXT("Texture"), Texture);
				FSlateBrush Brush = Image->GetBrush();
				Brush.SetResourceObject(Material);
				Image->SetBrush(Brush);
			}
		}
		if (Material)
		{
			Material->SetScalarParameterValue(TEXT("BlurPixels"), Blur * 4.0f);
		}
	}
}

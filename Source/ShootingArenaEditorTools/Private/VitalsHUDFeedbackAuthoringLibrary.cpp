#include "VitalsHUDFeedbackAuthoringLibrary.h"

#include "UI/VitalsHUDFeedbackSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Texture2D.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR SettingsPath[] = TEXT("/Game/QuakeLike_1_0/Data/UI/DA_VitalsHUDFeedback");
	constexpr TCHAR NumberMaterialPath[] = TEXT("/Game/QuakeLike_1_0/UI/PlayerUI/M_UI_VitalsNumberBlur");
	constexpr TCHAR ColorMaterialPath[] = TEXT("/Game/QuakeLike_1_0/UI/PlayerUI/M_UI_VitalsColorBlur");
	constexpr TCHAR HealthBarPath[] = TEXT("/Game/CompShooterHUD/UI/Content/HealthBar/Content/WB_HealthBar");
	constexpr TCHAR ArmorBarPath[] = TEXT("/Game/CompShooterHUD/UI/Content/ArmorBar/Content/WB_ArmorBar");
	constexpr TCHAR ColorEffectPath[] = TEXT("/Game/CompShooterHUD/UI/Content/HealthBar/Animations/WB_HealthAnim");
	constexpr TCHAR DamageEffectPath[] = TEXT("/Game/CompShooterHUD/UI/Content/HealthBar/Animations/WB_DamageAnim");

	template <typename T>
	T* LoadAsset(const FString& PackageName)
	{
		return LoadObject<T>(nullptr, *(PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName)));
	}

	template <typename T>
	T* CreateAsset(const TCHAR* Path)
	{
		T* Asset = NewObject<T>(CreatePackage(Path), *FPackageName::GetLongPackageAssetName(Path),
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		return Asset;
	}

	template <typename T>
	T* AddExpression(UMaterial& Material, int32 X, int32 Y)
	{
		T* Expression = NewObject<T>(&Material);
		Expression->Material = &Material;
		Expression->MaterialExpressionEditorX = X;
		Expression->MaterialExpressionEditorY = Y;
		Material.GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	UMaterial* CreateBlurMaterial(const TCHAR* Path, UTexture2D& Texture, bool bRetainedLayer)
	{
		if (UMaterial* Existing = LoadAsset<UMaterial>(Path))
		{
			return Existing;
		}

		UMaterial* Material = CreateAsset<UMaterial>(Path);
		Material->MaterialDomain = MD_UI;
		Material->BlendMode = bRetainedLayer ? BLEND_AlphaComposite : BLEND_Translucent;
		auto* Source = AddExpression<UMaterialExpressionTextureObjectParameter>(*Material, -800, 0);
		Source->ParameterName = TEXT("Texture");
		Source->Texture = &Texture;
		auto* UV = AddExpression<UMaterialExpressionTextureCoordinate>(*Material, -800, 160);
		auto* Strength = AddExpression<UMaterialExpressionScalarParameter>(*Material, -800, 320);
		Strength->ParameterName = TEXT("BlurPixels");
		auto* Tint = AddExpression<UMaterialExpressionVertexColor>(*Material, -800, 480);
		auto* Filter = AddExpression<UMaterialExpressionCustom>(*Material, -440, 0);
		Filter->Description = TEXT("Nine-tap blur of the HUD feedback layer");
		Filter->OutputType = CMOT_Float4;
		Filter->Inputs.Reset();
		for (const TPair<FName, UMaterialExpression*>& Pair : TArray<TPair<FName, UMaterialExpression*>>{
			{TEXT("Tex"), Source}, {TEXT("UV"), UV}, {TEXT("Radius"), Strength}, {TEXT("Tint"), Tint}})
		{
			FCustomInput& Input = Filter->Inputs.AddDefaulted_GetRef();
			Input.InputName = Pair.Key;
			Input.Input.Expression = Pair.Value;
		}
		// Slate's retained layer has premultiplied RGB. The standalone image uses straight alpha.
		const FString Premultiply = bRetainedLayer ? TEXT("") : TEXT("Sample.rgb *= Sample.a;\n");
		const FString Output = bRetainedLayer
			? TEXT("return float4(Color.rgb * Tint, Color.a);\n")
			: TEXT("return float4(Color.rgb * Tint / max(Color.a, 0.00001), Color.a);\n");
		Filter->Code = TEXT(
			"float2 DX = ddx(UV) * Radius;\n"
			"float2 DY = ddy(UV) * Radius;\n"
			"float4 Color = 0;\n"
			"[unroll] for (int Y = -1; Y <= 1; ++Y) {\n"
			"  [unroll] for (int X = -1; X <= 1; ++X) {\n"
			"    float2 P = UV + X * DX + Y * DY;\n"
			"    float4 Sample = Texture2DSample(Tex, TexSampler, saturate(P));\n"
			"    Sample *= all(P >= 0) && all(P <= 1) ? 1 : 0;\n") + Premultiply + TEXT(
			"    Color += Sample * (X == 0 ? 2 : 1) * (Y == 0 ? 2 : 1) / 16.0;\n"
			"  }\n"
			"}\n") + Output;
		auto* Alpha = AddExpression<UMaterialExpressionComponentMask>(*Material, -100, 180);
		Alpha->Input.Expression = Filter;
		Alpha->R = Alpha->G = Alpha->B = false;
		Alpha->A = true;
		auto* Opacity = AddExpression<UMaterialExpressionMultiply>(*Material, 100, 180);
		Opacity->A.Connect(0, Alpha);
		Opacity->B.Connect(4, Tint);
		Material->GetEditorOnlyData()->EmissiveColor.Expression = Filter;
		Material->GetEditorOnlyData()->Opacity.Expression = Opacity;
		Material->PostEditChange();
		return Material;
	}

	bool WrapNumber(UWidgetTree& Tree, FName BorderName, FName LayerName, UMaterial& Material)
	{
		if (Tree.FindWidget(LayerName))
		{
			return true;
		}
		UWidget* Border = Tree.FindWidget(BorderName);
		UOverlay* Parent = Border ? Cast<UOverlay>(Border->GetParent()) : nullptr;
		if (!Parent)
		{
			return false;
		}
		// Give the enlarged glyph room to draw inside the blur render target. The two
		// margins cancel in the desired size and keep the original text position.
		const FMargin Margin(48.0f);
		auto* Outer = Tree.ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("%sBounds"), *LayerName.ToString()));
		auto* Inner = Tree.ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("%sPadding"), *LayerName.ToString()));
		auto* Layer = Tree.ConstructWidget<URetainerBox>(URetainerBox::StaticClass(), LayerName);
		Layer->SetVisibility(ESlateVisibility::HitTestInvisible);
		Layer->SetEffectMaterial(&Material);
		Layer->SetRetainRendering(false);
		Border->Modify();
		Parent->Modify();
		Border->Slot->Modify();
		Parent->ReplaceChild(Border, Outer);
		Border->Slot = nullptr;
		CastChecked<USizeBoxSlot>(Outer->SetContent(Layer))->SetPadding(FMargin(-Margin.Left));
		Layer->SetContent(Inner);
		CastChecked<USizeBoxSlot>(Inner->SetContent(Border))->SetPadding(Margin);
		return true;
	}

	bool InsertFeedbackCall(UWidgetBlueprint& Blueprint, FName EventNodeName, FName FunctionName,
		UDataAsset& Settings, UMaterial* ColorMaterial)
	{
		UEdGraph* Graph = Blueprint.UbergraphPages.IsEmpty() ? nullptr : Blueprint.UbergraphPages[0];
		if (!Graph)
		{
			return false;
		}
		UEdGraphNode* Event = nullptr;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (const auto* Call = Cast<UK2Node_CallFunction>(Node); Call && Call->GetFunctionName() == FunctionName)
			{
				return true;
			}
			if (Node && Node->GetFName() == EventNodeName)
			{
				Event = Node;
			}
		}
		UEdGraphPin* Exec = Event ? Event->FindPin(UEdGraphSchema_K2::PN_Then) : nullptr;
		if (!Exec || Exec->LinkedTo.Num() != 1)
		{
			return false;
		}
		UEdGraphPin* NonDamage = Event->FindPin(TEXT("bIsAddingHealth"));
		if (!NonDamage)
		{
			NonDamage = Event->FindPin(TEXT("bIsAddingArmor"));
		}
		UEdGraphPin* Next = Exec->LinkedTo[0];
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		Graph->Modify();
		FGraphNodeCreator<UK2Node_CallFunction> Creator(*Graph);
		auto* Call = Creator.CreateNode();
		UClass* LibraryClass = FindObject<UClass>(nullptr, TEXT("/Script/ShootingArena.VitalsHUDFeedbackLibrary"));
		UFunction* Function = LibraryClass ? LibraryClass->FindFunctionByName(FunctionName) : nullptr;
		if (!Function)
		{
			return false;
		}
		Call->SetFromFunction(Function);
		Call->NodePosX = Event->NodePosX + 220;
		Call->NodePosY = Event->NodePosY - 220;
		Creator.Finalize();
		Schema->TrySetDefaultObject(*Call->FindPinChecked(TEXT("Settings")), &Settings);
		if (ColorMaterial)
		{
			Schema->TrySetDefaultObject(*Call->FindPinChecked(TEXT("BlurMaterial")), ColorMaterial);
		}
		else if (!NonDamage || !Schema->TryCreateConnection(NonDamage, Call->FindPinChecked(TEXT("bNonDamage"))))
		{
			return false;
		}
		Exec->BreakLinkTo(Next);
		return Schema->TryCreateConnection(Exec, Call->GetExecPin()) && Schema->TryCreateConnection(Call->GetThenPin(), Next);
	}

	bool SaveAsset(UObject& Asset)
	{
		UPackage* Package = Asset.GetOutermost();
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, &Asset, *Filename, Args);
	}
}

FString UVitalsHUDFeedbackAuthoringLibrary::InstallVitalsHUDFeedback()
{
	auto* Health = LoadAsset<UWidgetBlueprint>(HealthBarPath);
	auto* Armor = LoadAsset<UWidgetBlueprint>(ArmorBarPath);
	auto* ColorEffect = LoadAsset<UWidgetBlueprint>(ColorEffectPath);
	auto* DamageEffect = LoadAsset<UWidgetBlueprint>(DamageEffectPath);
	auto* Texture = LoadAsset<UTexture2D>(TEXT("/Game/CompShooterHUD/Textures/Highlights/T_Highlight2_1920x500"));
	if (!Health || !Armor || !ColorEffect || !DamageEffect || !Texture)
	{
		return TEXT("Could not load the HP, armor or color feedback assets.");
	}
	auto* Settings = LoadAsset<UDataAsset>(SettingsPath);
	if (!Settings)
	{
		UClass* SettingsClass = FindObject<UClass>(nullptr, TEXT("/Script/ShootingArena.VitalsHUDFeedbackSettings"));
		if (!SettingsClass)
		{
			return TEXT("Could not load the vitals settings class.");
		}
		Settings = NewObject<UDataAsset>(CreatePackage(SettingsPath), SettingsClass,
			*FPackageName::GetLongPackageAssetName(SettingsPath), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Settings);
	}
	UMaterial* NumberMaterial = CreateBlurMaterial(NumberMaterialPath, *Texture, true);
	UMaterial* ColorMaterial = CreateBlurMaterial(ColorMaterialPath, *Texture, false);
	for (UWidgetBlueprint* Bar : {Health, Armor})
	{
		Bar->Modify();
		Bar->WidgetTree->Modify();
		if (!WrapNumber(*Bar->WidgetTree, TEXT("Border_0"), TEXT("NumberFeedbackLayer"), *NumberMaterial)
			|| !WrapNumber(*Bar->WidgetTree, TEXT("Border_7"), TEXT("MaxNumberFeedbackLayer"), *NumberMaterial)
			|| !InsertFeedbackCall(*Bar, TEXT("K2Node_CustomEvent_0"),
				GET_FUNCTION_NAME_CHECKED(UVitalsHUDFeedbackLibrary, ApplyNumberFeedback), *Settings, nullptr))
		{
			return FString::Printf(TEXT("Could not connect the number feedback on %s."), *Bar->GetName());
		}
	}
	for (UWidgetBlueprint* Effect : {ColorEffect, DamageEffect})
	{
		Effect->Modify();
		if (!InsertFeedbackCall(*Effect, TEXT("K2Node_CustomEvent_11"),
			GET_FUNCTION_NAME_CHECKED(UVitalsHUDFeedbackLibrary, ApplyColorFeedback), *Settings, ColorMaterial))
		{
			return FString::Printf(TEXT("Could not connect the color feedback on %s."), *Effect->GetName());
		}
	}
	for (UWidgetBlueprint* Blueprint : {Health, Armor, ColorEffect, DamageEffect})
	{
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);
		if (Results.NumErrors > 0 || Blueprint->Status == BS_Error)
		{
			return FString::Printf(TEXT("Blueprint compilation failed: %s"), *Blueprint->GetName());
		}
	}
	for (UObject* Asset : TArray<UObject*>{Settings, NumberMaterial, ColorMaterial, Health, Armor, ColorEffect, DamageEffect})
	{
		if (!SaveAsset(*Asset))
		{
			return FString::Printf(TEXT("Could not save %s."), *Asset->GetName());
		}
	}
	return TEXT("Installed and compiled the HP, armor and color feedback assets.");
}

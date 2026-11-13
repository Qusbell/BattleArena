#include "BuffDurationRingAuthoringLibrary.h"

#include "Buff/BuffPresentationComponent.h"
#include "UI/BuffDurationRing.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Texture2D.h"
#include "IAssetTools.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompiler.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	const TCHAR* Folder = TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI");

	template<typename T> T* NewAsset(const FString& Path)
	{
		if (T* Existing = LoadObject<T>(nullptr, *(Path + TEXT(".") + FPackageName::GetLongPackageAssetName(Path)))) return Existing;
		UPackage* Package = CreatePackage(*Path);
		T* Asset = NewObject<T>(Package, *FPackageName::GetLongPackageAssetName(Path), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		return Asset;
	}

	bool Save(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		return UPackage::SavePackage(Package, Asset, *Filename, Args);
	}

	template<typename T> T* Expression(UMaterial* Material, int32 X, int32 Y)
	{
		T* Node = NewObject<T>(Material);
		Node->Material = Material;
		Node->MaterialExpressionEditorX = X;
		Node->MaterialExpressionEditorY = Y;
		Material->GetExpressionCollection().AddExpression(Node);
		return Node;
	}

	UMaterial* IconMaterial(UTexture2D* Texture)
	{
		const FString Path = FString(Folder) + TEXT("/Materials/M_UI_BuffIconBlur");
		UMaterial* Material = NewAsset<UMaterial>(Path);
		Material->GetExpressionCollection().Empty();
		Material->MaterialDomain = MD_UI;
		Material->BlendMode = BLEND_Translucent;
		auto* Tex = Expression<UMaterialExpressionTextureObjectParameter>(Material, -800, 0);
		Tex->ParameterName = TEXT("IconTexture");
		Tex->Texture = Texture;
		auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material, -800, 180);
		auto* Blur = Expression<UMaterialExpressionScalarParameter>(Material, -800, 300);
		Blur->ParameterName = TEXT("BlurPixels"); Blur->DefaultValue = 2.0f;
		auto* Opacity = Expression<UMaterialExpressionScalarParameter>(Material, -800, 440);
		Opacity->ParameterName = TEXT("IconOpacity"); Opacity->DefaultValue = 0.22f;
		auto* Step = Expression<UMaterialExpressionVectorParameter>(Material, -800, 560);
		Step->ParameterName = TEXT("IconUVStep"); Step->DefaultValue = FLinearColor(1.0f / 180.0f, 1.0f / 180.0f, 0.0f, 0.0f);
		auto* Tint = Expression<UMaterialExpressionVectorParameter>(Material, -800, 700);
		Tint->ParameterName = TEXT("IconTint"); Tint->DefaultValue = FLinearColor::White;
		auto* Filter = Expression<UMaterialExpressionCustom>(Material, -400, 0);
		Filter->Description = TEXT("Icon-only Gaussian blur; premultiplied alpha avoids dark fringes");
		Filter->OutputType = CMOT_Float4;
		Filter->Inputs.Empty();
		for (const auto& Pair : TArray<TPair<FName, UMaterialExpression*>>{
			{TEXT("Tex"), Tex}, {TEXT("UV"), UV}, {TEXT("BlurPixels"), Blur}, {TEXT("IconOpacity"), Opacity},
			{TEXT("UVStep"), Step}, {TEXT("Tint"), Tint}})
		{
			FCustomInput Input;
			Input.InputName = Pair.Key;
			Input.Input.Expression = Pair.Value;
			Filter->Inputs.Add(Input);
		}
		Filter->Code = TEXT(
			"float4 C = Texture2DSample(Tex, TexSampler, UV);\n"
			"if (BlurPixels > 0.001) {\n"
			" float2 D = min(BlurPixels, 12.0) * UVStep.xy;\n"
			" float4 Sum = float4(0,0,0,0);\n"
			" [unroll] for (int Y=-1; Y<=1; ++Y) { [unroll] for (int X=-1; X<=1; ++X) {\n"
			"  float Weight = (X==0 ? 2.0 : 1.0) * (Y==0 ? 2.0 : 1.0);\n"
			"  float4 S = Texture2DSample(Tex, TexSampler, saturate(UV + float2(X,Y)*D));\n"
			"  Sum += float4(S.rgb*S.a, S.a) * Weight;\n"
			" }}\n"
			" C = float4(Sum.rgb / max(Sum.a, 0.00001), Sum.a / 16.0);\n"
			"}\n"
			"C.rgb *= Tint.rgb; C.a *= saturate(IconOpacity); return C;\n");
		auto* Alpha = Expression<UMaterialExpressionComponentMask>(Material, -100, 180);
		Alpha->Input.Expression = Filter;
		Alpha->R = Alpha->G = Alpha->B = false; Alpha->A = true;
		Material->GetEditorOnlyData()->EmissiveColor.Expression = Filter;
		Material->GetEditorOnlyData()->Opacity.Expression = Alpha;
		Material->PostEditChange();
		return Material;
	}

	bool Compile(UBlueprint* BP)
	{
		FCompilerResultsLog Results;
		FKismetEditorUtilities::CompileBlueprint(BP, EBlueprintCompileOptions::None, &Results);
		return Results.NumErrors == 0 && BP->Status != BS_Error;
	}

	void Centre(UOverlay* Overlay, UWidget* Widget)
	{
		Widget->RemoveFromParent();
		UOverlaySlot* Slot = Overlay->AddChildToOverlay(Widget);
		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
	}

	UK2Node_VariableGet* GetVariable(UBlueprint* BP, UEdGraph* Graph, FName Name, int32 Y)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
			if (auto* Get = Cast<UK2Node_VariableGet>(Node); Get && Get->VariableReference.GetMemberName() == Name) return Get;
		FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);
		auto* Get = Creator.CreateNode();
		Get->VariableReference.SetSelfMember(Name);
		Get->NodePosX = 160; Get->NodePosY = Y;
		Creator.Finalize();
		return Get;
	}
}

FString UBuffDurationRingAuthoringLibrary::InstallBuffIconWhiteShine()
{
	auto* Settings = LoadObject<UBuffDurationRingSettings>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing"));
	auto* HUD = LoadObject<UWidgetBlueprint>(nullptr,
		*(FString(Folder) + TEXT("/WBP_PlayerHUD_sdm.WBP_PlayerHUD_sdm")));
	auto* Tree = HUD ? HUD->WidgetTree.Get() : nullptr;
	auto* Overlay = Tree ? Cast<UOverlay>(Tree->FindWidget(TEXT("BuffStatusOverlay"))) : nullptr;
	auto* Icon = Tree ? Cast<UImage>(Tree->FindWidget(TEXT("BuffStatusIcon"))) : nullptr;
	auto* Ring = Tree ? Tree->FindWidget(TEXT("BuffDurationRing")) : nullptr;
	auto* Time = Tree ? Tree->FindWidget(TEXT("BuffRemainingTimeText")) : nullptr;
	auto* IconSlot = Icon ? Cast<UOverlaySlot>(Icon->Slot) : nullptr;
	if (!Settings || !Settings->IconTexture || !Overlay || !IconSlot || !Ring || !Time ||
		Icon->GetParent() != Overlay || Ring->GetParent() != Overlay || Time->GetParent() != Overlay)
	{
		return TEXT("ERROR: expected HUD overlay, icon texture and settings");
	}

	auto* Material = NewAsset<UMaterial>(FString(Folder) + TEXT("/Materials/M_UI_BuffIconWhiteShine"));
	Material->GetExpressionCollection().Empty();
	Material->MaterialDomain = MD_UI;
	Material->BlendMode = BLEND_Additive;
	auto* Tex = Expression<UMaterialExpressionTextureObjectParameter>(Material, -700, 0);
	Tex->ParameterName = TEXT("IconTexture");
	Tex->Texture = Settings->IconTexture;
	auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material, -700, 180);
	auto* Phase = Expression<UMaterialExpressionScalarParameter>(Material, -700, 360);
	Phase->ParameterName = TEXT("SweepProgress");
	auto* Sweep = Expression<UMaterialExpressionCustom>(Material, -350, 0);
	Sweep->Description = TEXT("White diagonal sweep clipped to the icon alpha");
	Sweep->OutputType = CMOT_Float4;
	for (const auto& Pair : TArray<TPair<FName, UMaterialExpression*>>{
		{TEXT("Tex"), Tex}, {TEXT("UV"), UV}, {TEXT("Phase"), Phase}})
	{
		FCustomInput& Input = Sweep->Inputs.AddDefaulted_GetRef();
		Input.InputName = Pair.Key;
		Input.Input.Expression = Pair.Value;
	}
	Sweep->Code = TEXT("float Center = lerp(-0.22, 1.57, saturate(Phase));\n"
		"float Band = 1.0 - smoothstep(0.02, 0.18, abs(UV.x + 0.35 * UV.y - Center));\n"
		"return float4(1, 1, 1, 0.28 * Band * Texture2DSample(Tex, TexSampler, UV).a);");
	auto* Alpha = Expression<UMaterialExpressionComponentMask>(Material, -80, 170);
	Alpha->Input.Expression = Sweep;
	Alpha->R = Alpha->G = Alpha->B = false;
	Alpha->A = true;
	Material->GetEditorOnlyData()->EmissiveColor.Expression = Sweep;
	Material->GetEditorOnlyData()->Opacity.Expression = Alpha;
	Material->PostEditChange();

	auto* Shine = Cast<UImage>(Tree->FindWidget(TEXT("BuffIconWhiteShine")));
	if (!Shine)
	{
		Shine = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BuffIconWhiteShine"));
		Overlay->AddChildToOverlay(Shine);
	}
	auto* ShineSlot = Cast<UOverlaySlot>(Shine->Slot);
	if (!ShineSlot || Shine->GetParent() != Overlay) return TEXT("ERROR: unexpected shine slot");
	if (!HUD->WidgetVariableNameToGuidMap.Contains(Shine->GetFName()))
	{
		HUD->OnVariableAdded(Shine->GetFName());
	}
	ShineSlot->SetHorizontalAlignment(IconSlot->GetHorizontalAlignment());
	ShineSlot->SetVerticalAlignment(IconSlot->GetVerticalAlignment());
	ShineSlot->SetPadding(IconSlot->GetPadding());
	Shine->SetBrush(Icon->GetBrush());
	Shine->SetVisibility(ESlateVisibility::Collapsed);
	Overlay->ShiftChild(0, Icon);
	Overlay->ShiftChild(1, Shine);
	Overlay->ShiftChild(2, Ring);
	Overlay->ShiftChild(3, Time);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(HUD);
	if (!Compile(HUD)) return TEXT("ERROR: HUD compile failed");
	Settings->IconWhiteShineMaterial = Material;
	Settings->bEnableIconWhiteShine = true;
	if (!Save(Material) || !Save(HUD) || !Save(Settings)) return TEXT("ERROR: asset save failed");
	return TEXT("OK: buff icon shine material, HUD layers and data asset saved");
}

FString UBuffDurationRingAuthoringLibrary::InstallBuffRingWhiteShine()
{
	auto* Settings = LoadObject<UBuffDurationRingSettings>(nullptr,
		TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing"));
	auto* Material = LoadObject<UMaterial>(nullptr,
		*(FString(Folder) + TEXT("/Materials/M_UI_BuffDurationRing_HUD.M_UI_BuffDurationRing_HUD")));
	if (!Settings || !Material) return TEXT("ERROR: missing buff ring settings or material");
	auto& Color = Material->GetEditorOnlyData()->EmissiveColor;
	if (!Color.Expression) return TEXT("ERROR: missing ring color expression");
	const FString Description = TEXT("Buff ring white sweep (color only)");
	auto* Existing = Cast<UMaterialExpressionCustom>(Color.Expression);
	if (!Existing || Existing->Description != Description)
	{
		auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material, -400, 700);
		auto* Phase = Expression<UMaterialExpressionScalarParameter>(Material, -400, 820);
		Phase->ParameterName = TEXT("WhiteShineProgress");
		Phase->DefaultValue = -1.0f;
		auto* Sweep = Expression<UMaterialExpressionCustom>(Material, 200, 600);
		Sweep->Description = Description;
		Sweep->OutputType = CMOT_Float3;
		Sweep->Inputs.SetNum(3);
		Sweep->Inputs[0].InputName = TEXT("Base");
		Sweep->Inputs[0].Input = static_cast<const FExpressionInput&>(Color);
		Sweep->Inputs[1].InputName = TEXT("UV");
		Sweep->Inputs[1].Input.Expression = UV;
		Sweep->Inputs[2].InputName = TEXT("Phase");
		Sweep->Inputs[2].Input.Expression = Phase;
		Sweep->Code = TEXT("if (Phase < 0.0) return Base;\n"
			"float Center = lerp(-0.22, 1.57, saturate(Phase));\n"
			"float Band = 1.0 - smoothstep(0.02, 0.18, abs(UV.x + 0.35 * UV.y - Center));\n"
			"return lerp(Base, float3(1, 1, 1), 0.28 * Band);");
		Color.Connect(0, Sweep);
		Material->PostEditChange();
		if (!Save(Material)) return TEXT("ERROR: ring material save failed");
	}
	Settings->bEnableRingWhiteShine = true;
	return Save(Settings) ? TEXT("OK: ring white shine saved") : TEXT("ERROR: DA save failed");
}

FString UBuffDurationRingAuthoringLibrary::InstallBuffVisualOptions()
{
	auto* DA = LoadObject<UBuffDurationRingSettings>(nullptr, TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing"));
	auto* HUD = LoadObject<UWidgetBlueprint>(nullptr, *(FString(Folder) + TEXT("/WBP_PlayerHUD_sdm.WBP_PlayerHUD_sdm")));
	auto* Ring = DA && DA->RingMaterial ? DA->RingMaterial->GetMaterial() : nullptr;
	auto* Shine = DA ? Cast<UMaterial>(DA->IconWhiteShineMaterial) : nullptr;
	auto* Tree = HUD ? HUD->WidgetTree.Get() : nullptr;
	auto* Icon = Tree ? Cast<UImage>(Tree->FindWidget(TEXT("BuffStatusIcon"))) : nullptr;
	auto* Overlay = Icon ? Cast<UOverlay>(Icon->GetParent()) : nullptr;
	auto* Slot = Icon ? Cast<UOverlaySlot>(Icon->Slot) : nullptr;
	if (!Ring || !Shine || !Overlay || !Slot) return TEXT("ERROR: missing buff visuals");

	auto Scalar = [](UMaterial* M, FName Name, float Value)
	{
		auto* P = Expression<UMaterialExpressionScalarParameter>(M, -750, 0);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		return P;
	};
	auto Vector = [](UMaterial* M, FName Name, FLinearColor Value)
	{
		auto* P = Expression<UMaterialExpressionVectorParameter>(M, -750, 0);
		P->ParameterName = Name;
		P->DefaultValue = Value;
		return P;
	};
	auto Custom = [](UMaterial* M, const TCHAR* Description, ECustomMaterialOutputType Type,
		const TCHAR* Code, std::initializer_list<TPair<FName, UMaterialExpression*>> Inputs)
	{
		auto* Node = Expression<UMaterialExpressionCustom>(M, 250, 0);
		Node->Description = Description;
		Node->OutputType = Type;
		Node->Code = Code;
		Node->Inputs.Reset();
		for (const auto& Pair : Inputs)
		{
			auto& Input = Node->Inputs.AddDefaulted_GetRef();
			Input.InputName = Pair.Key;
			Input.Input.Expression = Pair.Value;
		}
		return Node;
	};
	auto Rewire = [](const TArray<UMaterialExpression*>& Nodes, UMaterialExpression* From, UMaterialExpression* To)
	{
		for (auto* Node : Nodes)
		{
			for (int32 Index = 0; FExpressionInput* Input = Node->GetInput(Index); ++Index)
			{
				if (Input->Expression == From)
				{
					Input->Expression = To;
					Input->OutputIndex = 0;
				}
			}
		}
	};
	auto& RingColor = Ring->GetEditorOnlyData()->EmissiveColor;
	auto& RingOpacity = Ring->GetEditorOnlyData()->Opacity;
	const TCHAR* RingDescription = TEXT("Buff ring arc halo composite");
	auto* Installed = Cast<UMaterialExpressionCustom>(RingColor.Expression);
	if (!Installed || Installed->Description != RingDescription)
	{
		TArray<UMaterialExpression*> Nodes;
		UMaterialExpressionTextureCoordinate* OldUV = nullptr;
		UMaterialExpressionVectorParameter* Fill = nullptr;
		UMaterialExpressionScalarParameter* Percent = nullptr;
		for (UMaterialExpression* Node : Ring->GetExpressionCollection().Expressions)
		{
			if (!Node) continue;
			Nodes.Add(Node);
			if (auto* UV = Cast<UMaterialExpressionTextureCoordinate>(Node)) OldUV = UV;
			if (auto* P = Cast<UMaterialExpressionVectorParameter>(Node); P && P->ParameterName == TEXT("FillColor")) Fill = P;
			if (auto* P = Cast<UMaterialExpressionScalarParameter>(Node); P && P->ParameterName == TEXT("Percent")) Percent = P;
		}
		if (!OldUV || !Fill || !Percent || !RingColor.Expression || !RingOpacity.Expression)
			return TEXT("ERROR: unexpected duration ring material graph");
		auto* UV = Custom(Ring, TEXT("Ring coordinates within padded draw area"), CMOT_Float2,
			TEXT("return (UV - 0.5) * Scale + 0.5;"),
			{{TEXT("UV"), OldUV}, {TEXT("Scale"), Scalar(Ring, TEXT("HaloCanvasScale"), 1.0f)}});
		Rewire(Nodes, OldUV, UV);
		auto* Gradient = Custom(Ring, TEXT("Seamless angular fill gradient"), CMOT_Float4,
			TEXT("float2 P=UV-0.5; float A=atan2(P.y,P.x)/6.28318530718; float T=0.5-0.5*cos((A-Angle)*6.28318530718); return float4(lerp(Base.rgb,lerp(Start.rgb,End.rgb,T),Enabled),FillAlpha);"),
			{{TEXT("UV"), UV}, {TEXT("Base"), Fill}, {TEXT("Angle"), Scalar(Ring, TEXT("GradientAngle"), 0.0f)},
			 {TEXT("Enabled"), Scalar(Ring, TEXT("GradientEnabled"), 0.0f)},
			 {TEXT("Start"), Vector(Ring, TEXT("GradientStart"), DA->RingGradientStart)},
			 {TEXT("End"), Vector(Ring, TEXT("GradientEnd"), DA->RingGradientEnd)}});
		auto& FillAlpha = Gradient->Inputs.AddDefaulted_GetRef();
		FillAlpha.InputName = TEXT("FillAlpha");
		FillAlpha.Input.Connect(4, Fill);
		Rewire(Nodes, Fill, Gradient);
		auto* Halo = Custom(Ring, RingDescription, CMOT_Float4,
			TEXT("float2 P=UV-0.5; float R=length(P); float A=frac(atan2(P.y,P.x)/6.28318530718+1.0);"
				 "float D=R-0.5; float H=(D>=0 && Percent>0 && A<=Percent && Range>0) ? Strength*pow(saturate(1-D/max(Range,0.00001)),2) : 0;"
				 "float Opacity=Alpha+H*(1-Alpha); float3 Glow=lerp(Fill.rgb,Tint.rgb,UseTint);"
				 "return float4((Base.rgb*Alpha+Glow*H*(1-Alpha))/max(Opacity,0.00001),Opacity);"),
			{{TEXT("Base"), RingColor.Expression}, {TEXT("Alpha"), RingOpacity.Expression}, {TEXT("UV"), UV},
			 {TEXT("Percent"), Percent}, {TEXT("Fill"), Gradient}, {TEXT("Range"), Scalar(Ring, TEXT("HaloRange"), 0.0f)},
			 {TEXT("Strength"), Scalar(Ring, TEXT("HaloStrength"), 0.0f)}, {TEXT("UseTint"), Scalar(Ring, TEXT("HaloCustomColor"), 0.0f)},
			 {TEXT("Tint"), Vector(Ring, TEXT("HaloColor"), DA->RingHaloColor)}});
		Halo->Inputs[0].Input = static_cast<const FExpressionInput&>(RingColor);
		Halo->Inputs[1].Input = static_cast<const FExpressionInput&>(RingOpacity);
		auto* Alpha = Expression<UMaterialExpressionComponentMask>(Ring, 550, 180);
		Alpha->Input.Expression = Halo;
		Alpha->R = Alpha->G = Alpha->B = false;
		Alpha->A = true;
		RingColor.Connect(0, Halo);
		RingOpacity.Connect(0, Alpha);
	}
	else
	{
		// Vector parameters expose component pins, while the replacement custom node has one RGBA output.
		UMaterialExpressionCustom* GradientNode = nullptr;
		UMaterialExpressionCustom* WhiteSweep = nullptr;
		UMaterialExpressionVectorParameter* Fill = nullptr;
		for (UMaterialExpression* Node : Ring->GetExpressionCollection().Expressions)
		{
			if (!Node) continue;
			if (auto* P = Cast<UMaterialExpressionVectorParameter>(Node); P && P->ParameterName == TEXT("FillColor")) Fill = P;
			if (auto* CustomNode = Cast<UMaterialExpressionCustom>(Node))
			{
				if (CustomNode->Description == TEXT("Seamless angular fill gradient")) GradientNode = CustomNode;
				if (CustomNode->Description == TEXT("Buff ring white sweep (color only)")) WhiteSweep = CustomNode;
			}
			for (int32 Index = 0; FExpressionInput* Input = Node->GetInput(Index); ++Index)
			{
				const auto* Gradient = Cast<UMaterialExpressionCustom>(Input->Expression);
				if (Gradient && Gradient->Description == TEXT("Seamless angular fill gradient")) Input->OutputIndex = 0;
			}
		}
		if (!GradientNode || !WhiteSweep || !Fill) return TEXT("ERROR: missing ring shader inputs");
		GradientNode->Inputs.RemoveAll([](const FCustomInput& Input) { return Input.InputName.IsNone(); });
		if (GradientNode->Inputs.Num() == 6)
		{
			auto& FillAlpha = GradientNode->Inputs.AddDefaulted_GetRef();
			FillAlpha.InputName = TEXT("FillAlpha");
			FillAlpha.Input.Connect(4, Fill);
			GradientNode->Code.ReplaceInline(TEXT("Base.a"), TEXT("FillAlpha"));
		}
		Installed->Inputs.RemoveAll([](const FCustomInput& Input) { return Input.InputName.IsNone(); });
		for (auto& Input : Installed->Inputs)
		{
			if (Input.InputName == TEXT("Base")) Input.Input.Connect(0, WhiteSweep);
		}
		Installed->Code.ReplaceInline(TEXT("Base*Alpha"), TEXT("Base.rgb*Alpha"));
	}

	auto* ShineNode = Cast<UMaterialExpressionCustom>(Shine->GetEditorOnlyData()->EmissiveColor.Expression);
	if (!ShineNode) return TEXT("ERROR: unexpected icon shine graph");
	Shine->BlendMode = BLEND_Additive;
	ShineNode->Inputs.RemoveAll([](const FCustomInput& Input) { return Input.InputName.IsNone(); });
	if (ShineNode->Inputs.Num() == 3)
	{
		for (const auto& Pair : TArray<TPair<FName, UMaterialExpression*>>{
			{TEXT("Extent"), Vector(Shine, TEXT("ShineExtent"), FLinearColor(180,180,180,180))},
			{TEXT("IconHeight"), Scalar(Shine, TEXT("ShineIconHeight"), 180.0f)},
			{TEXT("IconEnabled"), Scalar(Shine, TEXT("IconMaskEnabled"), 1.0f)},
			{TEXT("Diameter"), Scalar(Shine, TEXT("AreaDiameter"), 0.0f)}})
		{
			auto& Input = ShineNode->Inputs.AddDefaulted_GetRef();
			Input.InputName = Pair.Key;
			Input.Input.Expression = Pair.Value;
		}
		ShineNode->Code = TEXT("float2 P=(UV-0.5)*Extent.xy; float2 I=P/max(float2(Extent.z,IconHeight),1.0)+0.5;"
			"float Icon=(all(I>=0)&&all(I<=1)) ? Texture2DSample(Tex,TexSampler,saturate(I)).a*IconEnabled : 0;"
			"float Area=Diameter>0 ? saturate(Diameter*0.5-length(P)+0.5) : 0;"
			"float X=P.x+0.35*P.y; float W=max(Extent.z,1.0); float Travel=max(Extent.x+0.35*Extent.y,W);"
			"float Center=lerp(-0.5*Travel-0.22*W,0.5*Travel+0.22*W,saturate(Phase));"
			"float Band=1-smoothstep(0.02*W,0.18*W,abs(X-Center));"
			"return float4(1,1,1,0.28*Band*max(Icon,Area));");
	}

	auto* FrameMaterial = NewAsset<UMaterial>(FString(Folder) + TEXT("/Materials/M_UI_BuffIconFrame"));
	if (!FrameMaterial->GetEditorOnlyData()->EmissiveColor.Expression)
	{
		FrameMaterial->MaterialDomain = MD_UI;
		FrameMaterial->BlendMode = BLEND_Translucent;
		auto* Frame = Custom(FrameMaterial, TEXT("Icon frame"), CMOT_Float4,
			TEXT("float R=length(UV-0.5); float AA=max(fwidth(R),0.00001); float D=min(0.5-R,R-(0.5-Thickness)); return float4(Color.rgb,Color.a*saturate(D/AA+0.5));"),
			{{TEXT("UV"), Expression<UMaterialExpressionTextureCoordinate>(FrameMaterial, -800, 0)},
			 {TEXT("Color"), Vector(FrameMaterial, TEXT("FrameColor"), DA->IconFrameColor)},
			 {TEXT("Thickness"), Scalar(FrameMaterial, TEXT("FrameThickness"), 0.01f)}});
		auto* Alpha = Expression<UMaterialExpressionComponentMask>(FrameMaterial, 550, 180);
		Alpha->Input.Expression = Frame;
		Alpha->R = Alpha->G = Alpha->B = false;
		Alpha->A = true;
		FrameMaterial->GetEditorOnlyData()->EmissiveColor.Connect(0, Frame);
		FrameMaterial->GetEditorOnlyData()->Opacity.Connect(0, Alpha);
	}
	auto* FrameNode = CastChecked<UMaterialExpressionCustom>(FrameMaterial->GetEditorOnlyData()->EmissiveColor.Expression);
	FrameNode->Inputs.RemoveAll([](const FCustomInput& Input) { return Input.InputName.IsNone(); });
	if (FrameNode->Inputs.Num() == 3)
	{
		auto* FrameColor = FrameNode->Inputs[1].Input.Expression;
		auto& FrameAlpha = FrameNode->Inputs.AddDefaulted_GetRef();
		FrameAlpha.InputName = TEXT("FrameAlpha");
		FrameAlpha.Input.Connect(4, FrameColor);
		FrameNode->Code.ReplaceInline(TEXT("Color.a"), TEXT("FrameAlpha"));
	}
	auto* FrameImage = Cast<UImage>(Tree->FindWidget(TEXT("BuffIconFrame")));
	if (!FrameImage)
	{
		FrameImage = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BuffIconFrame"));
		HUD->OnVariableAdded(FrameImage->GetFName());
		auto* FrameSlot = Overlay->AddChildToOverlay(FrameImage);
		FrameSlot->SetHorizontalAlignment(Slot->GetHorizontalAlignment());
		FrameSlot->SetVerticalAlignment(Slot->GetVerticalAlignment());
		FrameSlot->SetPadding(Slot->GetPadding());
		FrameImage->SetVisibility(ESlateVisibility::Collapsed);
		Overlay->ShiftChild(Overlay->GetChildIndex(Icon) + 1, FrameImage);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(HUD);
		if (!Compile(HUD)) return TEXT("ERROR: HUD compile failed");
	}
	for (auto* Material : {Ring, Shine, FrameMaterial})
	{
		Material->PostEditChange();
		if (!Save(Material)) return TEXT("ERROR: material save failed");
	}
	DA->IconFrameMaterial = FrameMaterial;
	DA->bRingGradientEnabled = DA->bShowIconFrame = DA->bRingHaloEnabled = DA->bEnableAreaWhiteShine = true;
	return Save(HUD) && Save(DA) ? TEXT("OK: buff visual options saved") : TEXT("ERROR: HUD or DA save failed");
}

FString UBuffDurationRingAuthoringLibrary::InstallConnectedBuffRing()
{
	auto* DA = LoadObject<UBuffDurationRingSettings>(nullptr, TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing"));
	auto* M = DA && DA->RingMaterial ? DA->RingMaterial->GetMaterial() : nullptr;
	if (!M) return TEXT("ERROR: missing duration ring material");
	TMap<FName, UMaterialExpression*> Parameters;
	TMap<FString, UMaterialExpressionCustom*> CustomNodes;
	for (UMaterialExpression* Node : M->GetExpressionCollection().Expressions)
	{
		if (auto* P = Cast<UMaterialExpressionScalarParameter>(Node)) Parameters.Add(P->ParameterName, P);
		if (auto* P = Cast<UMaterialExpressionVectorParameter>(Node)) Parameters.Add(P->ParameterName, P);
		if (auto* C = Cast<UMaterialExpressionCustom>(Node)) CustomNodes.Add(C->Description, C);
	}
	auto* UV = CustomNodes.FindRef(TEXT("Ring coordinates within padded draw area"));
	auto* Gradient = CustomNodes.FindRef(TEXT("Seamless angular fill gradient"));
	auto* Halo = CustomNodes.FindRef(TEXT("Buff ring arc halo composite"));
	auto* Sweep = CustomNodes.FindRef(TEXT("Buff ring white sweep (color only)"));
	auto* Size = Parameters.FindRef(TEXT("Size"));
	auto* Percent = Parameters.FindRef(TEXT("Percent"));
	auto* Track = Parameters.FindRef(TEXT("EmptyColor"));
	auto* TipStrength = Parameters.FindRef(TEXT("EdgeGlowStrength"));
	auto* TipWidth = Parameters.FindRef(TEXT("EdgeGlowWidth"));
	if (!UV || !Gradient || !Halo || !Sweep || !Size || !Percent || !Track || !TipStrength || !TipWidth)
		return TEXT("ERROR: unexpected duration ring graph");
	bool bHasSweepPercent = false;
	for (auto& Input : Sweep->Inputs)
	{
		if (Input.InputName == TEXT("Percent"))
		{
			Input.Input.Connect(0, Percent);
			bHasSweepPercent = true;
		}
	}
	if (!bHasSweepPercent)
	{
		auto& Input = Sweep->Inputs.AddDefaulted_GetRef();
		Input.InputName = TEXT("Percent");
		Input.Input.Connect(0, Percent);
	}
	Sweep->Code = TEXT("if (Phase < 0.0 || Percent <= 0.0) return Base;\n"
		"float2 P=UV-0.5; float A=frac(atan2(P.y,P.x)/6.28318530718+1.0);\n"
		"float Center=saturate(Percent)*(1-saturate(Phase));\n"
		"float Delta=abs(A-Center); Delta=min(Delta,1-Delta);\n"
		"float Band=(1-smoothstep(0.01,0.065,Delta))*step(A,saturate(Percent));\n"
		"float Fade=smoothstep(0,0.12,Phase)*(1-smoothstep(0.8,1,Phase));\n"
		"return lerp(Base,float3(1,1,1),0.28*Band*Fade);");

	const TCHAR* Label = TEXT("Continuous outlined duration ring");
	auto* Core = CustomNodes.FindRef(Label);
	if (!Core) Core = Expression<UMaterialExpressionCustom>(M, 100, 520);
	Core->Description = Label;
	Core->OutputType = CMOT_Float4;
	Core->Inputs.Reset();
	auto* Segmented = Parameters.FindRef(TEXT("SegmentedRing"));
	if (!Segmented)
	{
		auto* Toggle = Expression<UMaterialExpressionScalarParameter>(M, -600, 1200);
		Toggle->ParameterName = TEXT("SegmentedRing");
		Toggle->DefaultValue = 0.0f;
		Segmented = Toggle;
	}
	auto* Steps = Parameters.FindRef(TEXT("Steps"));
	auto* Gap = Parameters.FindRef(TEXT("Gap"));
	if (!Steps || !Gap) return TEXT("ERROR: missing segment pattern parameters");
	auto ScalarOption = [&](FName Name, float DefaultValue) -> UMaterialExpression*
	{
		if (auto* Existing = Parameters.FindRef(Name)) return Existing;
		auto* Node = Expression<UMaterialExpressionScalarParameter>(M, -650, 1300);
		Node->ParameterName = Name;
		Node->DefaultValue = DefaultValue;
		return Node;
	};
	auto* CenterColor = Parameters.FindRef(TEXT("OutlineCenterColor"));
	if (!CenterColor)
	{
		auto* Node = Expression<UMaterialExpressionVectorParameter>(M, -650, 1450);
		Node->ParameterName = TEXT("OutlineCenterColor");
		Node->DefaultValue = DA->SegmentOutlineCenterColor;
		CenterColor = Node;
	}
	auto* TransparentTrack = Parameters.FindRef(TEXT("TransparentEmptyTrack"));
	if (!TransparentTrack)
	{
		auto* Toggle = Expression<UMaterialExpressionScalarParameter>(M, -600, 1150);
		Toggle->ParameterName = TEXT("TransparentEmptyTrack");
		Toggle->DefaultValue = 0.0f;
		TransparentTrack = Toggle;
	}
	auto* Outline = Parameters.FindRef(TEXT("WhiteOutlineWidth"));
	if (!Outline)
	{
		auto* Width = Expression<UMaterialExpressionScalarParameter>(M, -600, 900);
		Width->ParameterName = TEXT("WhiteOutlineWidth");
		Width->DefaultValue = 0.75f;
		Outline = Width;
	}
	auto* OutlineColor = Parameters.FindRef(TEXT("OutlineColor"));
	if (!OutlineColor)
	{
		auto* Color = Expression<UMaterialExpressionVectorParameter>(M, -600, 1050);
		Color->ParameterName = TEXT("OutlineColor");
		Color->DefaultValue = FLinearColor(0.73f, 0.80f, 0.88f);
		OutlineColor = Color;
	}
	for (const auto& Pair : TArray<TPair<FName, UMaterialExpression*>>{
		{TEXT("UV"), UV}, {TEXT("Fill"), Gradient}, {TEXT("Size"), Size}, {TEXT("Percent"), Percent},
		{TEXT("Track"), Track}, {TEXT("TipStrength"), TipStrength}, {TEXT("TipWidth"), TipWidth},
		{TEXT("Outline"), Outline}, {TEXT("OutlineColor"), OutlineColor},
		{TEXT("TransparentTrack"), TransparentTrack}, {TEXT("Segmented"), Segmented},
		{TEXT("Steps"), Steps}, {TEXT("Gap"), Gap},
		{TEXT("OutlineOpacity"), ScalarOption(TEXT("OutlineOpacity"), 1.0f)},
		{TEXT("OutlineGradient"), ScalarOption(TEXT("SegmentOutlineGradient"), 0.0f)},
		{TEXT("CenterColor"), CenterColor},
		{TEXT("EndRange"), ScalarOption(TEXT("OutlineEndRange"), 0.25f)}})
	{
		auto& Input = Core->Inputs.AddDefaulted_GetRef();
		Input.InputName = Pair.Key;
		Input.Input.Expression = Pair.Value;
	}
	Core->Code = TEXT(
		"float2 P = UV - 0.5; float R = length(P);\n"
		"float PX = max(max(length(ddx(UV)), length(ddy(UV))), 0.000001);\n"
		"float Outer = 0.5; float Inner = Outer - max(0.5 - 0.5/max(Size,1.001), 5.0*PX);\n"
		"float A = frac(atan2(P.y,P.x)/6.28318530718+1.0);\n"
		"float Phase = frac(A*max(Steps,1));\n"
		"float SegmentEdge = min(Phase-Gap,1-Gap-Phase)*6.28318530718*R/max(Steps,1);\n"
		"float RadialEdge = min(Outer-R, R-Inner);\n"
		"float Edge = lerp(RadialEdge,min(RadialEdge,SegmentEdge),Segmented) / PX;\n"
		"float Coverage = saturate(Edge+0.5); float Body = saturate(Edge-min(Outline,(Outer-Inner)/PX*0.35)+0.5);\n"
		"float ArcPX = 6.28318530718*max(R,PX)/PX;\n"
		"float Left = (Percent-A)*ArcPX;\n"
		"float Amount = Percent >= 1 ? 1 : (Percent > 0 ? saturate(Left+0.5) : 0);\n"
		"float SegmentPosition = saturate((Phase-Gap)/max(1-2*Gap,0.00001));\n"
		"float EndDistance = min(SegmentPosition,1-SegmentPosition);\n"
		"float EndLight = 1-smoothstep(0,max(EndRange,0.01),EndDistance);\n"
		"float3 Rim = lerp(OutlineColor.rgb,lerp(CenterColor.rgb,OutlineColor.rgb,EndLight),Segmented*OutlineGradient);\n"
		"float3 Empty = lerp(float3(0.018,0.024,0.035), Track.rgb, 0.55);\n"
		"float Inward = smoothstep(0,1,saturate((R-Inner)/max(Outer-Inner,PX)));\n"
		"float Cap = Percent > 0 && Percent < 1 ? saturate(1-abs(Left)/1.25) : 0;\n"
		"float Tip = Percent > 0 && Percent < 1 && Left >= 0 ? TipStrength*pow(saturate(1-Left/max(TipWidth,1)),2) : 0;\n"
		"float3 Active = lerp(Fill.rgb*lerp(0.52,1.0,Inward), float3(1,1,1), saturate(0.36*Cap+Tip));\n"
		"float3 Color = lerp(Empty, Active, Amount);\n"
		"float Border = max(Coverage-Body,0)*saturate(OutlineOpacity);\n"
		"float BodyAlpha = Body*lerp(1,Amount,saturate(TransparentTrack));\n"
		"float Alpha = Border+BodyAlpha;\n"
		"return float4((Rim*Border+Color*BodyAlpha)/max(Alpha,0.00001),Alpha);");
	auto* Alpha = NewObject<UMaterialExpressionComponentMask>(M);
	for (const auto& Input : Halo->Inputs)
	{
		if (Input.InputName == TEXT("Alpha"))
		{
			if (auto* Existing = Cast<UMaterialExpressionComponentMask>(Input.Input.Expression);
				Existing && Existing->Input.Expression == Core) Alpha = Existing;
		}
	}
	if (!M->GetExpressionCollection().Expressions.Contains(Alpha))
	{
		Alpha->Material = M;
		M->GetExpressionCollection().AddExpression(Alpha);
	}
	Alpha->Input.Connect(0, Core);
	Alpha->R = Alpha->G = Alpha->B = false;
	Alpha->A = true;
	for (auto& Input : Sweep->Inputs)
	{
		if (Input.InputName == TEXT("Base"))
		{
			Input.Input.Connect(0, Core);
			Input.Input.SetMask(1, 1, 1, 1, 0);
		}
	}
	for (auto& Input : Halo->Inputs)
	{
		if (Input.InputName == TEXT("Alpha")) Input.Input.Connect(0, Alpha);
	}
	M->PostEditChange();
	return Save(M) ? TEXT("OK: continuous ring saved") : TEXT("ERROR: ring save failed");
}

FString UBuffDurationRingAuthoringLibrary::InstallBuffIconBackground()
{
	auto* DA = LoadObject<UBuffDurationRingSettings>(nullptr, TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing.DA_BuffDurationRing"));
	auto* HUD = LoadObject<UWidgetBlueprint>(nullptr, *(FString(Folder) + TEXT("/WBP_PlayerHUD_sdm.WBP_PlayerHUD_sdm")));
	auto* Tree = HUD ? HUD->WidgetTree.Get() : nullptr;
	auto* Icon = Tree ? Cast<UImage>(Tree->FindWidget(TEXT("BuffStatusIcon"))) : nullptr;
	auto* Overlay = Icon ? Cast<UOverlay>(Icon->GetParent()) : nullptr;
	auto* IconSlot = Icon ? Cast<UOverlaySlot>(Icon->Slot) : nullptr;
	if (!DA || !Overlay || !IconSlot) return TEXT("ERROR: missing buff HUD");
	auto* Material = NewAsset<UMaterial>(FString(Folder) + TEXT("/Materials/M_UI_BuffIconBackground"));
	if (!Material->GetEditorOnlyData()->EmissiveColor.Expression)
	{
		Material->MaterialDomain = MD_UI;
		Material->BlendMode = BLEND_Translucent;
		auto* UV = Expression<UMaterialExpressionTextureCoordinate>(Material, -600, 0);
		auto* Color = Expression<UMaterialExpressionVectorParameter>(Material, -600, 200);
		Color->ParameterName = TEXT("BackgroundColor");
		Color->DefaultValue = FLinearColor::Black;
		auto* Opacity = Expression<UMaterialExpressionScalarParameter>(Material, -600, 400);
		Opacity->ParameterName = TEXT("BackgroundOpacity");
		Opacity->DefaultValue = 0.35f;
		auto* Disk = Expression<UMaterialExpressionCustom>(Material, -100, 0);
		Disk->Description = TEXT("Flat translucent circular icon background");
		Disk->OutputType = CMOT_Float4;
		Disk->Inputs.Reset();
		for (const auto& Pair : TArray<TPair<FName, UMaterialExpression*>>{
			{TEXT("UV"), UV}, {TEXT("Color"), Color}, {TEXT("Opacity"), Opacity}})
		{
			auto& Input = Disk->Inputs.AddDefaulted_GetRef();
			Input.InputName = Pair.Key;
			Input.Input.Expression = Pair.Value;
		}
		Disk->Code = TEXT("float R=length(UV-0.5); float AA=max(fwidth(R),0.00001); return float4(Color.rgb,saturate(Opacity)*saturate((0.5-R)/AA+0.5));");
		auto* Alpha = Expression<UMaterialExpressionComponentMask>(Material, 200, 200);
		Alpha->Input.Connect(0, Disk);
		Alpha->R = Alpha->G = Alpha->B = false;
		Alpha->A = true;
		Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Disk);
		Material->GetEditorOnlyData()->Opacity.Connect(0, Alpha);
		Material->PostEditChange();
	}
	auto* Disk = Cast<UMaterialExpressionCustom>(Material->GetEditorOnlyData()->EmissiveColor.Expression);
	if (!Disk) return TEXT("ERROR: unexpected background material");
	if (Disk->Inputs.Num() == 3)
	{
		for (const auto& Pair : TArray<TPair<FName, float>>{
			{TEXT("BackgroundDiameter"), DA->IconBackgroundDiameter},
			{TEXT("BackgroundFeather"), 0.0f},
			{TEXT("BackgroundGradient"), 0.0f},
			{TEXT("BackgroundCenterRatio"), 0.6f}})
		{
			auto* Parameter = Expression<UMaterialExpressionScalarParameter>(Material, -600, 600);
			Parameter->ParameterName = Pair.Key;
			Parameter->DefaultValue = Pair.Value;
			auto& Input = Disk->Inputs.AddDefaulted_GetRef();
			Input.InputName = Pair.Key;
			Input.Input.Connect(0, Parameter);
		}
		auto* Outer = Expression<UMaterialExpressionVectorParameter>(Material, -600, 800);
		Outer->ParameterName = TEXT("BackgroundOuterColor");
		Outer->DefaultValue = DA->IconBackgroundOuterColor;
		auto& Input = Disk->Inputs.AddDefaulted_GetRef();
		Input.InputName = TEXT("BackgroundOuterColor");
		Input.Input.Connect(0, Outer);
	}
	bool bHasOuterOpacity = false;
	for (const auto& Input : Disk->Inputs)
	{
		if (Input.InputName == TEXT("BackgroundOuterOpacity")) bHasOuterOpacity = true;
	}
	if (!bHasOuterOpacity)
	{
		DA->IconBackgroundOuterOpacity = DA->IconBackgroundOpacity;
		auto* Parameter = Expression<UMaterialExpressionScalarParameter>(Material, -600, 1000);
		Parameter->ParameterName = TEXT("BackgroundOuterOpacity");
		Parameter->DefaultValue = DA->IconBackgroundOpacity;
		auto& Input = Disk->Inputs.AddDefaulted_GetRef();
		Input.InputName = TEXT("BackgroundOuterOpacity");
		Input.Input.Connect(0, Parameter);
	}
	bool bHasMiddleColor = false;
	for (const auto& Input : Disk->Inputs)
	{
		if (Input.InputName == TEXT("BackgroundMiddleColor")) bHasMiddleColor = true;
	}
	if (!bHasMiddleColor)
	{
		const FLinearColor Middle = FMath::Lerp(DA->IconBackgroundColor, DA->IconBackgroundOuterColor, 0.5f);
		DA->IconBackgroundMiddleColor = Middle;
		auto* Parameter = Expression<UMaterialExpressionVectorParameter>(Material, -600, 1200);
		Parameter->ParameterName = TEXT("BackgroundMiddleColor");
		Parameter->DefaultValue = Middle;
		auto& Input = Disk->Inputs.AddDefaulted_GetRef();
		Input.InputName = TEXT("BackgroundMiddleColor");
		Input.Input.Connect(0, Parameter);
	}
	bool bHasMiddlePosition = false;
	for (const auto& Input : Disk->Inputs)
	{
		if (Input.InputName == TEXT("BackgroundMiddlePosition")) bHasMiddlePosition = true;
	}
	if (!bHasMiddlePosition)
	{
		// Preserve the previous halfway point between the central plateau and the outer edge.
		DA->IconBackgroundMiddlePosition = (1.0f + FMath::Clamp(DA->IconBackgroundCenterRatio, 0.0f, 0.95f)) * 50.0f;
		auto* Parameter = Expression<UMaterialExpressionScalarParameter>(Material, -600, 1400);
		Parameter->ParameterName = TEXT("BackgroundMiddlePosition");
		Parameter->DefaultValue = DA->IconBackgroundMiddlePosition / 100.0f;
		auto& Input = Disk->Inputs.AddDefaulted_GetRef();
		Input.InputName = TEXT("BackgroundMiddlePosition");
		Input.Input.Connect(0, Parameter);
	}
	Disk->Code = TEXT("float Radius=max(BackgroundDiameter*0.5,0.5);"
		"float Feather=max(BackgroundFeather,0)*saturate(BackgroundGradient);"
		"float R=length((UV-0.5)*(2*Radius+2*Feather)); float AA=max(fwidth(R),0.0001);"
		"float Mix=smoothstep(Radius*saturate(BackgroundCenterRatio),Radius,max(R,0))*saturate(BackgroundGradient);"
		"float Mask=Feather>0 ? 1-smoothstep(Radius,Radius+Feather,R) : saturate((Radius-R)/AA+0.5);"
		"float Position=clamp(BackgroundMiddlePosition,0.01,0.99);"
		"float Start=min(saturate(BackgroundCenterRatio),Position-0.001);"
		"float Radial=R/Radius;"
		"float3 Inner=lerp(Color.rgb,BackgroundMiddleColor.rgb,smoothstep(Start,Position,Radial));"
		"float3 Gradient=lerp(Inner,BackgroundOuterColor.rgb,smoothstep(Position,1,Radial));"
		"Gradient=lerp(Color.rgb,Gradient,saturate(BackgroundGradient));"
		"return float4(Gradient,lerp(saturate(Opacity),saturate(BackgroundOuterOpacity),Mix)*Mask);");
	Material->PostEditChange();
	auto* Background = Cast<UImage>(Tree->FindWidget(TEXT("BuffIconBackground")));
	if (!Background)
	{
		Background = Tree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BuffIconBackground"));
		HUD->OnVariableAdded(Background->GetFName());
		auto* Slot = Overlay->AddChildToOverlay(Background);
		Slot->SetHorizontalAlignment(IconSlot->GetHorizontalAlignment());
		Slot->SetVerticalAlignment(IconSlot->GetVerticalAlignment());
		Slot->SetPadding(IconSlot->GetPadding());
		Background->SetVisibility(ESlateVisibility::Collapsed);
		Overlay->ShiftChild(0, Background);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(HUD);
		if (!Compile(HUD)) return TEXT("ERROR: HUD compile failed");
	}
	const bool bFirstInstall = !DA->IconBackgroundMaterial;
	DA->IconBackgroundMaterial = Material;
	if (bFirstInstall) DA->bShowIconBackground = true;
	return Save(Material) && Save(HUD) && Save(DA) ? TEXT("OK: icon background saved") : TEXT("ERROR: background save failed");
}

FString UBuffDurationRingAuthoringLibrary::CreateWhiteBuffIconTexture()
{
	UTexture2D* Original = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/QuakeLike_Base/SDM/Symbol/버프_아이콘1.버프_아이콘1"));
	if (!Original || Original->Source.GetFormat() != TSF_BGRA8)
		return TEXT("ERROR: original BGRA8 icon source is required");
	TArray64<uint8> Pixels;
	if (!Original->Source.GetMipData(Pixels, 0)) return TEXT("ERROR: cannot read original source pixels");
	const int64 Width = Original->Source.GetSizeX();
	const int64 Height = Original->Source.GetSizeY();
	if (Pixels.Num() != Width * Height * 4) return TEXT("ERROR: unexpected source pixel layout");
	const FString Path = TEXT("/Game/QuakeLike_Base/SDM/Symbol/T_BuffIcon1_White");
	UTexture2D* White = nullptr;
	if (FPackageName::DoesPackageExist(Path))
	{
		White = LoadObject<UTexture2D>(nullptr, *(Path + TEXT(".T_BuffIcon1_White")));
		if (!White) return TEXT("ERROR: destination exists but is not a texture");
		TArray64<uint8> Existing;
		if (White->Source.GetFormat() != TSF_BGRA8 || White->Source.GetSizeX() != Width
			|| White->Source.GetSizeY() != Height || !White->Source.GetMipData(Existing, 0) || Existing.Num() != Pixels.Num())
			return TEXT("ERROR: existing destination differs; not overwritten");
		for (int64 I = 0; I < Pixels.Num(); I += 4)
			if (Existing[I] != 255 || Existing[I + 1] != 255 || Existing[I + 2] != 255 || Existing[I + 3] != Pixels[I + 3])
				return TEXT("ERROR: existing destination is not the matching white copy; not overwritten");
		return TEXT("OK: verified existing ") + White->GetPathName();
	}
	// Work on the copy only. Preserve every alpha sample, including antialiased edges and holes.
	for (int64 I = 0; I < Pixels.Num(); I += 4) Pixels[I] = Pixels[I + 1] = Pixels[I + 2] = 255;
	White = NewAsset<UTexture2D>(Path);
	White->Source.Init(static_cast<int32>(Width), static_cast<int32>(Height), 1, 1, TSF_BGRA8, Pixels.GetData());
	White->SRGB = Original->SRGB;
	White->CompressionSettings = Original->CompressionSettings;
	White->CompressionNoAlpha = false;
	White->LODGroup = Original->LODGroup;
	White->MipGenSettings = Original->MipGenSettings;
	White->Filter = Original->Filter;
	White->NeverStream = Original->NeverStream;
	White->PostEditChange();
	if (!Save(White)) return TEXT("ERROR: white texture save failed");
	return TEXT("OK: created ") + White->GetPathName();
}

FString UBuffDurationRingAuthoringLibrary::InstallBuffDurationRing()
{
	UWidgetBlueprint* Source = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/CompShooterHUD/UI/Content/Crosshairs/ProgressBar/WB_ProgressBar.WB_ProgressBar"));
	UWidgetBlueprint* HUD = LoadObject<UWidgetBlueprint>(nullptr,
		TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_PlayerHUD_sdm.WBP_PlayerHUD_sdm"));
	if (!Source || !Source->WidgetTree || !HUD || !HUD->WidgetTree) return TEXT("ERROR: source widget or HUD missing");
	auto* Icon = Cast<UImage>(HUD->WidgetTree->FindWidget(TEXT("BuffStatusIcon")));
	auto* Text = Cast<UTextBlock>(HUD->WidgetTree->FindWidget(TEXT("BuffRemainingTimeText")));
	auto* Column = Cast<UVerticalBox>(HUD->WidgetTree->FindWidget(TEXT("BuffStatusColumn")));
	if (!Icon || !Text || !Column) return TEXT("ERROR: existing buff HUD widgets missing");
	UTexture2D* IconTexture = Cast<UTexture2D>(Icon->GetBrush().GetResourceObject());
	if (!IconTexture) IconTexture = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/QuakeLike_Base/SDM/Symbol/버프_아이콘1.버프_아이콘1"));
	if (!IconTexture) return TEXT("ERROR: original icon texture missing");
	UMaterial* Parent = LoadObject<UMaterial>(nullptr, TEXT("/Game/CompShooterHUD/Materials/ProgressBar/M_PB_Base.M_PB_Base"));
	if (!Parent) return TEXT("ERROR: purchased circle material missing");
	auto* MI = NewAsset<UMaterialInstanceConstant>(FString(Folder) + TEXT("/Materials/MI_BuffDurationRing"));
	MI->SetParentEditorOnly(Parent);
	MI->SetScalarParameterValueEditorOnly(TEXT("Size"), 1.06f);
	MI->SetScalarParameterValueEditorOnly(TEXT("Steps"), 800.0f);
	MI->SetScalarParameterValueEditorOnly(TEXT("Percent"), 0.65f);
	MI->PostEditChange();
	UMaterial* BlurMaterial = IconMaterial(IconTexture);
	const FString DAPath = TEXT("/Game/QuakeLike_1_0/Data/UI/DA_BuffDurationRing");
	auto* Settings = NewAsset<UBuffDurationRingSettings>(DAPath);
	Settings->RingMaterial = MI;
	Settings->IconMaterial = BlurMaterial;
	if (!Settings->IconTexture) Settings->IconTexture = IconTexture;

	const FString RingPath = FString(Folder) + TEXT("/WBP_BuffDurationRing");
	auto* RingBP = LoadObject<UWidgetBlueprint>(nullptr, *(RingPath + TEXT(".WBP_BuffDurationRing")));
	if (!RingBP)
	{
		RingBP = Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"))
			.Get().DuplicateAsset(TEXT("WBP_BuffDurationRing"), Folder, Source));
		if (!RingBP) return TEXT("ERROR: ring duplicate failed");
		// Prune only the copy. No original timer/interface/HUD delegate can reach this display.
		TArray<UEdGraph*> Graphs;
		for (UEdGraph* Graph : RingBP->UbergraphPages) Graphs.Add(Graph);
		for (UEdGraph* Graph : RingBP->FunctionGraphs) Graphs.Add(Graph);
		for (UEdGraph* Graph : RingBP->MacroGraphs) Graphs.Add(Graph);
		for (UEdGraph* Graph : RingBP->DelegateSignatureGraphs) Graphs.Add(Graph);
		for (UEdGraph* Graph : Graphs) FBlueprintEditorUtils::RemoveGraph(RingBP, Graph, EGraphRemoveFlags::MarkTransient);
		RingBP->ImplementedInterfaces.Empty();
		RingBP->NewVariables.Empty();
		RingBP->Animations.Empty();
		RingBP->Bindings.Empty();
		RingBP->WidgetVariableNameToGuidMap.Empty();
		RingBP->LastEditedDocuments.Empty();
		RingBP->ParentClass = UBuffDurationRingWidget::StaticClass();
		UImage* OriginalCircle = CastChecked<UImage>(Source->WidgetTree->FindWidget(TEXT("CircleImg")));
		UWidgetTree* OldTree = RingBP->WidgetTree;
		OldTree->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		RingBP->WidgetTree = NewObject<UWidgetTree>(RingBP, TEXT("WidgetTree"), RF_Transactional);
		auto* Box = RingBP->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RingSizeBox"));
		Box->SetWidthOverride(Settings->Diameter); Box->SetHeightOverride(Settings->Diameter);
		Box->bIsVariable = true;
		RingBP->WidgetTree->RootWidget = Box;
		auto* Circle = DuplicateObject<UImage>(OriginalCircle, RingBP->WidgetTree, TEXT("CircleImg"));
		Circle->Slot = nullptr;
		Circle->bIsVariable = true;
		Circle->SetBrushFromMaterial(MI);
		Box->AddChild(Circle);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(RingBP);
	}
	if (!Compile(RingBP)) return TEXT("ERROR: ring blueprint compile failed");
	auto* RingDefault = CastChecked<UBuffDurationRingWidget>(RingBP->GeneratedClass->GetDefaultObject());
	RingDefault->Settings = Settings;
	if (FProperty* Tick = FindFProperty<FProperty>(RingDefault->GetClass(), TEXT("TickFrequency")))
		Tick->ImportText_Direct(TEXT("Never"), Tick->ContainerPtrToValuePtr<void>(RingDefault), RingDefault, PPF_None);

	HUD->Modify(); HUD->WidgetTree->Modify();
	auto* Overlay = Cast<UOverlay>(HUD->WidgetTree->FindWidget(TEXT("BuffStatusOverlay")));
	if (!Overlay)
	{
		Overlay = HUD->WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BuffStatusOverlay"));
		Overlay->bIsVariable = true;
		HUD->OnVariableAdded(Overlay->GetFName());
		Icon->RemoveFromParent(); Text->RemoveFromParent();
		UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Overlay);
		Slot->SetHorizontalAlignment(HAlign_Center);
	}
	auto* Ring = Cast<UBuffDurationRingWidget>(HUD->WidgetTree->FindWidget(TEXT("BuffDurationRing")));
	if (!Ring)
	{
		Ring = HUD->WidgetTree->ConstructWidget<UBuffDurationRingWidget>(RingBP->GeneratedClass.Get(), TEXT("BuffDurationRing"));
		Ring->bIsVariable = true;
		HUD->OnVariableAdded(Ring->GetFName());
	}
	// Render order: ring under the softened icon; crisp text above both.
	Centre(Overlay, Ring); Centre(Overlay, Icon); Centre(Overlay, Text);
	Ring->Settings = Settings;
	Icon->SetDesiredSizeOverride(Settings->IconSize);
	Icon->SetRenderOpacity(Settings->IconOpacity);
	Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);
	Text->SetText(FText::FromString(TEXT("30s")));
	FSlateFontInfo Font = Text->GetFont(); Font.Size = Settings->TimeFontSize;
	Font.OutlineSettings.OutlineSize = Settings->TimeOutlineSize;
	Text->SetFont(Font);
	Column->SetVisibility(ESlateVisibility::Collapsed);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(HUD);
	UEdGraph* EventGraph = FBlueprintEditorUtils::FindEventGraph(HUD);
	UK2Node_CallFunction* Update = nullptr;
	for (UEdGraphNode* Node : EventGraph->Nodes)
		if (auto* Call = Cast<UK2Node_CallFunction>(Node); Call &&
			(Call->FunctionReference.GetMemberName() == TEXT("UpdateBuffRemainingTimeText") ||
			 Call->FunctionReference.GetMemberName() == TEXT("UpdateBuffStatusUI"))) { Update = Call; break; }
	if (!Update) return TEXT("ERROR: existing HUD refresh call missing");
	Update->Modify();
	Update->SetFromFunction(UBuffPresentationComponent::StaticClass()->FindFunctionByName(
		GET_FUNCTION_NAME_CHECKED(UBuffPresentationComponent, UpdateBuffStatusUI)));
	Update->ReconstructNode();
	const UEdGraphSchema* Schema = EventGraph->GetSchema();
	for (const auto& Pair : TArray<TPair<FName, FName>>{{TEXT("BuffDurationRing"), TEXT("DurationRing")}, {TEXT("BuffStatusColumn"), TEXT("StatusGroup")}})
	{
		auto* Get = GetVariable(HUD, EventGraph, Pair.Key, Pair.Key == TEXT("BuffDurationRing") ? 600 : 720);
		if (!Schema->TryCreateConnection(Get->GetValuePin(), Update->FindPinChecked(Pair.Value))) return TEXT("ERROR: HUD pin connection failed");
	}
	FBlueprintEditorUtils::MarkBlueprintAsModified(HUD);
	if (!Compile(HUD)) return TEXT("ERROR: player HUD compile failed; HUD not saved");
	for (UObject* Asset : TArray<UObject*>{BlurMaterial, MI, Settings, RingBP, HUD})
		if (!Save(Asset)) return TEXT("ERROR: asset save failed: ") + Asset->GetPathName();
	return TEXT("OK: saved DA_BuffDurationRing, MI_BuffDurationRing, M_UI_BuffIconBlur, WBP_BuffDurationRing and WBP_PlayerHUD_sdm. Purchased sources untouched.");
}

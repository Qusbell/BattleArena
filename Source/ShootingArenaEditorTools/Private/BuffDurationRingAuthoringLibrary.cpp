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

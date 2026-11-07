#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Buff/BuffPresentationComponent.h"
#include "UI/BuffDurationRing.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "AssetCompilingManager.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "DataTableUtils.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformMisc.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "MaterialShared.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Serialization/BufferArchive.h"
#include "Slate/WidgetRenderer.h"
#include "ShaderCompiler.h"
#include "Tests/AutomationCommon.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffDurationRingTest, "ShootingArena.UI.BuffDurationRing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FBuffDurationRingTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Context;
	if (!Context.CreateTestWorld(EWorldType::Game)) return false;
	UWorld* World = Context.GetTestWorld();
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
	UGameInstance* GI = World->GetGameInstance();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
	ON_SCOPE_EXIT { GI->RemoveLocalPlayer(Player); };
	AddExpectedError(TEXT("저장된 Area Graph가 없습니다"), EAutomationExpectedErrorFlags::Contains, 1);
	if (!Context.BeginPlayInTestWorld()) return false;
	APlayerController* PC = World->SpawnActor<APlayerController>();
	PC->SetPlayer(Player);
	ACharacter* Pawn = World->SpawnActor<ACharacter>();
	PC->Possess(Pawn);
	UClass* BuffClass = LoadClass<UBuffPresentationComponent>(nullptr, TEXT("/Game/QuakeLike_1_0/Buff/BPC_Buff.BPC_Buff_C"));
	UClass* RingClass = LoadClass<UBuffDurationRingWidget>(nullptr,
		TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_BuffDurationRing.WBP_BuffDurationRing_C"));
	if (!TestNotNull(TEXT("Existing gameplay component"), BuffClass) || !TestNotNull(TEXT("Saved purchase-derived ring"), RingClass)) return false;
	auto* Buff = NewObject<UBuffPresentationComponent>(Pawn, BuffClass);
	Pawn->AddInstanceComponent(Buff); Buff->RegisterComponent();
	auto* Ring = CreateWidget<UBuffDurationRingWidget>(PC, RingClass);
	if (!TestNotNull(TEXT("Ring instance"), Ring) || !TestNotNull(TEXT("Independent ring DA"), Ring->Settings.Get())) return false;
	Ring->Settings = DuplicateObject<UBuffDurationRingSettings>(Ring->Settings, GetTransientPackage());
	auto* Style = Ring->Settings.Get();
	Style->Diameter = 212.0f; Style->IconBlurStrength = 4.0f; Style->IconOpacity = 0.18f;
	Style->Position = FVector2D(25.0, -90.0);
	Style->TimeFontSize = 44; Style->bShowSecondsSuffix = true;
	Style->bIconFlashEnabled = false;
	Style->IconColor = FLinearColor(0.8f, 0.3f, 0.1f, 1.0f);
	UTexture2D* OriginalTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/QuakeLike_Base/SDM/Symbol/버프_아이콘1.버프_아이콘1"));
	UTexture2D* WhiteTexture = Style->IconTexture;
	if (!TestNotNull(TEXT("Original icon source"), OriginalTexture) || !TestNotNull(TEXT("White icon source"), WhiteTexture)) return false;
	TestTrue(TEXT("White icon is a separate texture"), WhiteTexture != OriginalTexture);
	TArray64<uint8> OriginalPixels, WhitePixels;
	if (!OriginalTexture->Source.GetMipData(OriginalPixels, 0) || !WhiteTexture->Source.GetMipData(WhitePixels, 0)) return false;
	TestEqual(TEXT("White icon preserves width"), WhiteTexture->Source.GetSizeX(), OriginalTexture->Source.GetSizeX());
	TestEqual(TEXT("White icon preserves height"), WhiteTexture->Source.GetSizeY(), OriginalTexture->Source.GetSizeY());
	if (!TestEqual(TEXT("White icon preserves source byte count"), WhitePixels.Num(), OriginalPixels.Num())) return false;
	bool bWhiteRGB = true, bExactAlpha = true;
	for (int64 I = 0; I < WhitePixels.Num(); I += 4)
	{
		bWhiteRGB &= WhitePixels[I] == 255 && WhitePixels[I+1] == 255 && WhitePixels[I+2] == 255;
		bExactAlpha &= WhitePixels[I+3] == OriginalPixels[I+3];
	}
	TestTrue(TEXT("White icon has neutral RGB in every source pixel"), bWhiteRGB);
	TestTrue(TEXT("White icon preserves the exact original alpha mask"), bExactAlpha);
	UImage* Icon = NewObject<UImage>(PC);
	UTextBlock* Text = NewObject<UTextBlock>(PC);
	UCanvasPanel* Canvas = NewObject<UCanvasPanel>(PC);
	UOverlay* Group = NewObject<UOverlay>(PC);
	UCanvasPanelSlot* Position = Canvas->AddChildToCanvas(Group);
	for (UWidget* Child : TArray<UWidget*>{Ring, Icon, Text})
	{
		UOverlaySlot* Slot = Group->AddChildToOverlay(Child);
		Slot->SetHorizontalAlignment(HAlign_Center); Slot->SetVerticalAlignment(VAlign_Center);
	}
	const TSharedRef<SWidget> Slate = Group->TakeWidget();
	auto Refresh = [&] { UBuffPresentationComponent::UpdateBuffStatusUI(Pawn, Text, Icon, Ring, Group); };
	auto Grant = [&](const TCHAR* FunctionName, const TCHAR* RowParameter, FName Row, double Deadline = 0.0)
	{
		UFunction* Function = Buff->FindFunction(FunctionName);
		if (!TestNotNull(TEXT("Original grant function"), Function)) return false;
		FStructOnScope Args(Function);
		FindFProperty<FNameProperty>(Function, RowParameter)->SetPropertyValue_InContainer(Args.GetStructMemory(), Row);
		if (auto* End = FindFProperty<FDoubleProperty>(Function, TEXT("EndTime"))) End->SetPropertyValue_InContainer(Args.GetStructMemory(), Deadline);
		Buff->ProcessEvent(Function, Args.GetStructMemory());
		return TestTrue(TEXT("Original gameplay grant succeeds"), FindFProperty<FBoolProperty>(Function, TEXT("Success"))->GetPropertyValue_InContainer(Args.GetStructMemory()));
	};
	Refresh();
	TestTrue(TEXT("Missing active buff hides the whole group"), Group->GetVisibility() == ESlateVisibility::Collapsed);
	// Read the independent gameplay data: designers may tune the Quad duration.
	UDataTable* GameplayTable = Buff->BuffDataTable;
	if (!TestNotNull(TEXT("Gameplay buff table"), GameplayTable)) return false;
	const uint8* QuadRow = GameplayTable->FindRowUnchecked(TEXT("Buff.Multi.Atk"));
	if (!TestNotNull(TEXT("Quad gameplay row"), QuadRow)) return false;
	double QuadDuration = 0.0;
	for (TFieldIterator<FNumericProperty> It(GameplayTable->GetRowStruct()); It; ++It)
	{
		if (DataTableUtils::GetPropertyExportName(*It) != TEXT("Duration")) continue;
		const void* Value = It->ContainerPtrToValuePtr<void>(QuadRow);
		QuadDuration = It->IsFloatingPoint() ? It->GetFloatingPointPropertyValue(Value) : static_cast<double>(It->GetSignedIntPropertyValue(Value));
		break;
	}
	if (!TestTrue(TEXT("Quad has a positive configured duration"), FMath::IsFinite(QuadDuration) && QuadDuration > 0.0)) return false;
	const FString QuadSeconds = FString::Printf(TEXT("%ds"), FMath::CeilToInt(QuadDuration));
	if (!Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk"))) return false;
	Refresh();
	TestEqual(TEXT("Normal grant starts full"), Buff->GetPresentationProgress(), 1.0f);
	TestEqual(TEXT("Centre uses existing numeric seconds"), Text->GetText().ToString(), QuadSeconds);
	TestEqual(TEXT("Independent DA positions the group"), Position->GetPosition(), Style->Position);
	if (!TestNotNull(TEXT("Cached radial MID"), Ring->RingMID.Get()) || !TestNotNull(TEXT("Icon-only MID"), Ring->IconMID.Get())) return false;
	TestEqual(TEXT("Icon opacity is independent of ring alpha"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("IconOpacity")), 0.18f);
	TestEqual(TEXT("Icon blur reaches only its material"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("BlurPixels")), 4.0f);
	TestEqual(TEXT("Text stays fully opaque"), Text->GetRenderOpacity(), 1.0f);
	TestEqual(TEXT("Ring size is DA-driven"), Ring->RingSizeBox->GetWidthOverride(), 212.0f);
	UMaterialInstanceDynamic* OriginalRingMID = Ring->RingMID;
	UMaterialInstanceDynamic* OriginalIconMID = Ring->IconMID;
	for (int32 I = 0; I < 50; ++I) Refresh();
	TestTrue(TEXT("Updates never recreate either material"), Ring->RingMID == OriginalRingMID && Ring->IconMID == OriginalIconMID);
	Context.TickTestWorld(2.0f);
	TestTrue(TEXT("Progress decreases with existing server time"), Buff->GetPresentationProgress() < 1.0f);
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk")); Refresh();
	TestEqual(TEXT("Reacquisition uses refreshed gameplay deadline"), Buff->GetPresentationProgress(), 1.0f);
	const double End = World->GetGameState()->GetServerWorldTimeSeconds() + 12.0;
	Grant(TEXT("ApplyDroppedBuff"), TEXT("Row Name"), TEXT("Buff.Multi.Def"), End); Refresh();
	TestTrue(TEXT("Dropped 12/30 seconds preserves forty percent"), FMath::IsNearlyEqual(Buff->GetPresentationProgress(), 0.4f));
	TestEqual(TEXT("Dropped centre time is not reset"), Text->GetText().ToString(), FString(TEXT("12s")));
	TestTrue(TEXT("Protection changes the shared ring colour"), Ring->RingMID->K2_GetVectorParameterValue(TEXT("FillColor")).Equals(Style->ProtectionColor));
	TestTrue(TEXT("Icon colour is independent of the buff ring"), Ring->IconMID->K2_GetVectorParameterValue(TEXT("IconTint")).Equals(Style->IconColor));
	Style->IconColor.A = 0.5f; Refresh();
	TestEqual(TEXT("Icon colour alpha multiplies only icon opacity"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("IconOpacity")), 0.09f);
	TestTrue(TEXT("Icon alpha preserves ring colour"), Ring->RingMID->K2_GetVectorParameterValue(TEXT("FillColor")).Equals(Style->ProtectionColor));
	Style->IconColor.A = 1.0f; Refresh();
	// Flash phase is derived from the same remaining time; no timer or frame-time accumulation.
	Style->bIconFlashEnabled = true;
	Style->IconFlashStrength = 0.5f; Style->IconFlashInterval = 1.0f; Style->IconFlashDuration = 0.15f;
	TestEqual(TEXT("Second boundary starts a subtle flash"), Ring->GetIconFlashMultiplier(12.0), 1.5f);
	TestTrue(TEXT("Flash eases down after the number changes"), Ring->GetIconFlashMultiplier(11.95) > 1.0f && Ring->GetIconFlashMultiplier(11.95) < 1.5f);
	TestEqual(TEXT("Flash returns to normal within its duration"), Ring->GetIconFlashMultiplier(11.8), 1.0f);
	Ring->ApplyStatus(12.0, 0.4f, false, Text, Icon);
	FLinearColor PeakTint = Style->IconColor * 1.5f; PeakTint.A = 1.0f;
	TestTrue(TEXT("Flash reaches only the icon material"), Ring->IconMID->K2_GetVectorParameterValue(TEXT("IconTint")).Equals(PeakTint));
	TestTrue(TEXT("Flash preserves the ring colour"), Ring->RingMID->K2_GetVectorParameterValue(TEXT("FillColor")).Equals(Style->ProtectionColor));
	TestEqual(TEXT("Flash preserves icon opacity"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("IconOpacity")), 0.18f);
	TestEqual(TEXT("Flash preserves crisp number opacity"), Text->GetRenderOpacity(), 1.0f);
	Style->IconFlashInterval = 2.0f;
	TestEqual(TEXT("Two-second interval flashes at even remaining seconds"), Ring->GetIconFlashMultiplier(12.0), 1.5f);
	TestEqual(TEXT("Two-second interval skips the intervening second"), Ring->GetIconFlashMultiplier(11.0), 1.0f);
	Style->IconFlashInterval = 0.5f;
	TestEqual(TEXT("Half-second interval is supported"), Ring->GetIconFlashMultiplier(11.5), 1.5f);
	Style->IconFlashStrength = 0.0f;
	TestEqual(TEXT("Zero strength disables the flash"), Ring->GetIconFlashMultiplier(12.0), 1.0f);
	Style->IconFlashStrength = 2.0f;
	TestEqual(TEXT("Designer strength controls peak brightness"), Ring->GetIconFlashMultiplier(12.0), 3.0f);
	Style->bIconFlashEnabled = false;
	TestEqual(TEXT("Unchecked flash returns normal brightness"), Ring->GetIconFlashMultiplier(12.0), 1.0f);
	Ring->ApplyStatus(12.0, 0.4f, false, Text, Icon);
	TestTrue(TEXT("Unchecking immediately restores icon tint"), Ring->IconMID->K2_GetVectorParameterValue(TEXT("IconTint")).Equals(Style->IconColor));
	Style->bIconFlashEnabled = true;
	Style->IconFlashInterval = 0.0f; Style->IconFlashDuration = NAN; Style->IconFlashStrength = NAN;
	TestTrue(TEXT("Invalid flash settings remain finite"), FMath::IsFinite(Ring->GetIconFlashMultiplier(12.0)));
	TestEqual(TEXT("Expiry never leaves a flash"), Ring->GetIconFlashMultiplier(0.0), 1.0f);
	Style->bIconFlashEnabled = false;
	Style->bShowSecondsSuffix = false; Refresh();
	TestEqual(TEXT("Designer can display numbers without the suffix"), Text->GetText().ToString(), FString(TEXT("12")));
	Style->bShowSecondsSuffix = true;
	Style->bTimeWarningFlashEnabled = true;
	Style->TimeWarningStartSeconds = 5.0f;
	Style->TimeWarningFlashInterval = 0.5f;
	Style->TimeWarningColor = FLinearColor::Red;
	auto NumberColor = [&] { return Text->GetColorAndOpacity().GetSpecifiedColor(); };
	Ring->ApplyStatus(5.001, 0.2f, false, Text, Icon);
	TestTrue(TEXT("Number stays normal before the warning threshold"), NumberColor().Equals(Style->TimeColor));
	Ring->ApplyStatus(5.0, 0.2f, false, Text, Icon);
	TestTrue(TEXT("Exactly five seconds starts with a red number"), NumberColor().Equals(FLinearColor::Red));
	Ring->ApplyStatus(4.9, 0.2f, true, Text, Icon);
	TestTrue(TEXT("Quad receives the same red number warning"), NumberColor().Equals(FLinearColor::Red));
	TestTrue(TEXT("Number warning preserves Quad ring colour"), Ring->RingMID->K2_GetVectorParameterValue(TEXT("FillColor")).Equals(Style->QuadColor));
	TestTrue(TEXT("Number warning preserves icon tint"), Ring->IconMID->K2_GetVectorParameterValue(TEXT("IconTint")).Equals(Style->IconColor));
	TestEqual(TEXT("Blink keeps the number readable and fully opaque"), Text->GetRenderOpacity(), 1.0f);
	Ring->ApplyStatus(4.7, 0.2f, false, Text, Icon);
	TestTrue(TEXT("Second half of the blink restores normal number colour"), NumberColor().Equals(Style->TimeColor));
	Ring->ApplyStatus(4.5, 0.2f, false, Text, Icon);
	TestTrue(TEXT("Next full period returns to red"), NumberColor().Equals(FLinearColor::Red));
	Style->bTimeWarningFlashEnabled = false;
	Ring->ApplyStatus(4.9, 0.2f, false, Text, Icon);
	TestTrue(TEXT("Unchecking warning restores normal colour immediately"), NumberColor().Equals(Style->TimeColor));
	Style->bTimeWarningFlashEnabled = true; Style->TimeWarningStartSeconds = 3.0f;
	Ring->ApplyStatus(4.9, 0.2f, false, Text, Icon);
	TestTrue(TEXT("DA warning threshold edit applies"), NumberColor().Equals(Style->TimeColor));
	Style->TimeWarningColor = FLinearColor::Yellow;
	Ring->ApplyStatus(3.0, 0.1f, false, Text, Icon);
	TestTrue(TEXT("DA warning colour edit applies"), NumberColor().Equals(FLinearColor::Yellow));
	Style->TimeWarningStartSeconds = 5.0f; Style->TimeWarningFlashInterval = 1.0f;
	Ring->ApplyStatus(4.6, 0.2f, false, Text, Icon);
	TestTrue(TEXT("DA blink period edit applies"), NumberColor().Equals(FLinearColor::Yellow));
	Style->TimeWarningStartSeconds = NAN; Style->TimeWarningFlashInterval = NAN;
	Style->TimeWarningColor.R = NAN;
	TestTrue(TEXT("Invalid warning settings fall back to finite red"), Ring->GetTimeDisplayColor(4.9).Equals(FLinearColor::Red));
	TestTrue(TEXT("Expiry does not retain a warning"), Ring->GetTimeDisplayColor(0.0).Equals(Style->TimeColor));
	Style->TimeWarningStartSeconds = 5.0f; Style->TimeWarningFlashInterval = 0.5f;
	Style->TimeWarningColor = FLinearColor::Red;
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Add.Atk"));
	Refresh();
	TestTrue(TEXT("Reacquisition clears the red warning using the new deadline"), NumberColor().Equals(Style->TimeColor));
	// The test world clamps a single frame's delta; advance actual frames rather than one 15s tick.
	for (int32 I = 0; I < 150; ++I) Context.TickTestWorld(0.1f);
	Refresh();
	TestTrue(TEXT("Sixty-second DT row uses its own denominator"), FMath::IsNearlyEqual(Buff->GetPresentationProgress(), 0.75f, 0.001f));

	// Render the real UMG/material composition; never save a test world or designer asset.
	const FString PreviewDir = FPaths::ProjectSavedDir() / TEXT("Automation/BuffDurationRing");
	IFileManager::Get().MakeDirectory(*PreviewDir, true);
	auto Capture = [&](const TCHAR* Name, bool bVerifyPixels = true)
	{
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		// Synchronous offscreen draws have no normal engine-frame boundary to push MID uniforms.
		Ring->RingMID->GetRenderProxy()->CacheUniformExpressions_GameThread(true);
		Ring->IconMID->GetRenderProxy()->CacheUniformExpressions_GameThread(true);
		ENQUEUE_RENDER_COMMAND(FlushBuffRingTestUniforms)([](FRHICommandListImmediate& RHICmdList)
		{
			FMaterialRenderProxy::UpdateDeferredCachedUniformExpressions(RHICmdList);
		});
		FlushRenderingCommands();
		if (FMaterialResource* Resource = Style->IconMaterial->GetMaterial()->GetMaterialResource(GMaxRHIFeatureLevel))
			TestEqual(TEXT("Icon blur shader compiles without fallback"), Resource->GetCompileErrors().Num(), 0);
		Group->ForceLayoutPrepass();
		FWidgetRenderer Renderer(true);
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(256.0, 256.0));
		FlushRenderingCommands();
		// Warm the Slate material handles once before capturing the target.
		if (Target) Renderer.DrawWidget(Target, Slate, FVector2D(256.0, 256.0), 0.0f);
		FlushRenderingCommands();
		TArray<FColor> Pixels;
		if (Target && Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels))
		{
			int32 ColouredPixels = 0;
			for (const FColor& Pixel : Pixels)
				if (FMath::Abs(static_cast<int32>(Pixel.G) - Pixel.R) > 12 || FMath::Abs(static_cast<int32>(Pixel.B) - Pixel.R) > 12) ++ColouredPixels;
			if (bVerifyPixels) TestTrue(FString::Printf(TEXT("%s contains the coloured ring, not only the white text"), Name), ColouredPixels > 100);
		}
		FBufferArchive Bytes;
		if (Target && FImageUtils::ExportRenderTarget2DAsPNG(Target, Bytes))
			TestTrue(TEXT("Rendered preview saved"), FFileHelper::SaveArrayToFile(Bytes, *(PreviewDir / Name)));
		else AddError(TEXT("Could not render the ring preview"));
	};
	// Prime Slate's dynamic material resources before the three verified captures.
	// A real game has intervening frames; this fixture renders all states synchronously.
	Capture(TEXT("_MaterialWarmup.png"), false);
	Capture(TEXT("Quad_75percent.png"));
	Grant(TEXT("ApplyDroppedBuff"), TEXT("Row Name"), TEXT("Buff.Multi.Def"), World->GetGameState()->GetServerWorldTimeSeconds() + 12.0);
	Style->IconBlurStrength = 2.0f; Style->IconOpacity = 0.22f;
	Refresh(); Capture(TEXT("Protection_40percent.png"));
	Style->Diameter = 148.0f; Style->IconBlurStrength = 0.0f; Style->IconOpacity = 0.65f;
	Style->IconSize = FVector2D(116.0, 116.0); Style->TimeFontSize = 32;
	Style->Position = FVector2D(-35.0, -120.0);
	Refresh(); Capture(TEXT("DA_size148_sharp_icon.png"));
	TestEqual(TEXT("Live DA size edit applies"), Ring->RingSizeBox->GetWidthOverride(), 148.0f);
	TestEqual(TEXT("Live DA blur zero removes icon blur"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("BlurPixels")), 0.0f);
	TestEqual(TEXT("Live DA font edit applies"), Text->GetFont().Size, 32.0f);
	TestEqual(TEXT("Live DA position edit applies"), Position->GetPosition(), Style->Position);
	Style->IconOpacity = -1.0f; Style->IconBlurStrength = NAN; Style->Diameter = NAN;
	Refresh();
	TestEqual(TEXT("Invalid blur has a finite fallback"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("BlurPixels")), 2.0f);
	TestEqual(TEXT("Negative opacity clamps to transparent"), Ring->IconMID->K2_GetScalarParameterValue(TEXT("IconOpacity")), 0.0f);
	TestEqual(TEXT("Invalid diameter has a finite fallback"), Ring->RingSizeBox->GetWidthOverride(), 212.0f);
	UBuffPresentationComponent::UpdateBuffStatusUI(nullptr, Text, Icon, Ring, Group);
	TestTrue(TEXT("Pawn loss hides group and clears progress"), Group->GetVisibility() == ESlateVisibility::Collapsed && Ring->AppliedProgress == 0.0f);
	Refresh();
	UFunction* Expire = Buff->FindFunction(TEXT("OnBuffTimerFinished"));
	if (!TestNotNull(TEXT("Original expiry event"), Expire)) return false;
	Buff->ProcessEvent(Expire, nullptr); Refresh();
	TestTrue(TEXT("Gameplay expiry hides all three elements"), Group->GetVisibility() == ESlateVisibility::Collapsed
		&& Icon->GetVisibility() == ESlateVisibility::Collapsed && Text->GetVisibility() == ESlateVisibility::Collapsed
		&& Ring->GetVisibility() == ESlateVisibility::Collapsed);
	TestEqual(TEXT("Expired progress is zero"), Buff->GetPresentationProgress(), 0.0f);
	Grant(TEXT("ApplyBuff"), TEXT("rowName"), TEXT("Buff.Multi.Atk")); Refresh();
	UFunction* Death = Buff->FindFunction(TEXT("OnOwnerDeath"));
	FStructOnScope DeathArgs(Death); Buff->ProcessEvent(Death, DeathArgs.GetStructMemory()); Refresh();
	TestTrue(TEXT("Death removes the indicator"), Group->GetVisibility() == ESlateVisibility::Collapsed);
	return !HasAnyErrors();
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuffHUDStyleTest, "ShootingArena.UI.BuffHUDStyle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter | EAutomationTestFlags::NonNullRHI)

bool FBuffHUDStyleTest::RunTest(const FString& Parameters)
{
    FTestWorldWrapper Context;
    if (!Context.CreateTestWorld(EWorldType::Game)) return false;
    UWorld* World = Context.GetTestWorld();
    UGameInstance* GI = World->GetGameInstance();
    ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
    GI->AddLocalPlayer(Player, FPlatformMisc::GetPlatformUserForUserIndex(0));
    ON_SCOPE_EXIT { GI->RemoveLocalPlayer(Player); };
    APlayerController* PC = World->SpawnActor<APlayerController>(); PC->SetPlayer(Player);
    UClass* Class = LoadClass<UBuffDurationRingWidget>(nullptr, TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_BuffDurationRing.WBP_BuffDurationRing_C"));
    auto* Ring = CreateWidget<UBuffDurationRingWidget>(PC, Class);
    if (!TestNotNull(TEXT("Saved buff ring"), Ring) || !TestNotNull(TEXT("Saved ring settings"), Ring->Settings.Get())) return false;
    Ring->Settings = DuplicateObject<UBuffDurationRingSettings>(Ring->Settings, GetTransientPackage());
    auto* Style = Ring->Settings.Get();
    // Compare the actual displayed purchased HUD numerals, not the other root-HUD placeholders.
    auto SavedText = [this](const TCHAR* ClassPath,const TCHAR* Name)->UTextBlock*
    {
        auto* BP = Cast<UWidgetBlueprintGeneratedClass>(LoadClass<UUserWidget>(nullptr,ClassPath));
        if(!TestNotNull(TEXT("Saved HUD widget class"),BP) || !TestNotNull(TEXT("Saved HUD widget tree"),BP->GetWidgetTreeArchetype())) return nullptr;
        return Cast<UTextBlock>(BP->GetWidgetTreeArchetype()->FindWidget(Name));
    };
    UTextBlock* HealthNumber = SavedText(TEXT("/Game/CompShooterHUD/UI/Content/HealthBar/Content/WB_HealthBar.WB_HealthBar_C"),TEXT("Text_Value"));
    UTextBlock* ArmorNumber = SavedText(TEXT("/Game/CompShooterHUD/UI/Content/ArmorBar/Content/WB_ArmorBar.WB_ArmorBar_C"),TEXT("Text_Value"));
    UTextBlock* BuffNumber = SavedText(TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI/WBP_PlayerHUD_sdm.WBP_PlayerHUD_sdm_C"),TEXT("BuffRemainingTimeText"));
    if(!TestNotNull(TEXT("Displayed health number"),HealthNumber) || !TestNotNull(TEXT("Displayed armor number"),ArmorNumber) || !TestNotNull(TEXT("Saved buff number"),BuffNumber)) return false;
    TestTrue(TEXT("Buff font style exactly matches displayed health and armor"),BuffNumber->GetFont()==HealthNumber->GetFont() && BuffNumber->GetFont()==ArmorNumber->GetFont());
    TestEqual(TEXT("Black outline is disabled in the DA"),Style->TimeOutlineSize,0);
    TestEqual(TEXT("DA keeps the matching HUD point size"),float(Style->TimeFontSize),HealthNumber->GetFont().Size);
    TestTrue(TEXT("Buff shadow offset matches both HUD values"),BuffNumber->GetShadowOffset()==HealthNumber->GetShadowOffset() && BuffNumber->GetShadowOffset()==ArmorNumber->GetShadowOffset());
    TestTrue(TEXT("Buff shadow color/opacity matches the HUD"),BuffNumber->GetShadowColorAndOpacity().Equals(HealthNumber->GetShadowColorAndOpacity()) && BuffNumber->GetShadowColorAndOpacity().Equals(ArmorNumber->GetShadowColorAndOpacity()));
    UTextBlock* RuntimeNumber=NewObject<UTextBlock>(PC);
    RuntimeNumber->SetFont(BuffNumber->GetFont());RuntimeNumber->SetShadowOffset(BuffNumber->GetShadowOffset());RuntimeNumber->SetShadowColorAndOpacity(BuffNumber->GetShadowColorAndOpacity());
    Ring->ApplyStatus(10.0,0.5f,true,RuntimeNumber,nullptr);
    TestTrue(TEXT("Runtime refresh retains the HUD font without restoring an outline"),RuntimeNumber->GetFont()==HealthNumber->GetFont());
    TestTrue(TEXT("Runtime refresh retains the drop shadow"),RuntimeNumber->GetShadowOffset()==HealthNumber->GetShadowOffset() && RuntimeNumber->GetShadowColorAndOpacity().Equals(HealthNumber->GetShadowColorAndOpacity()));
    TestEqual(TEXT("HUD has five health-style blocks"), Style->RingSteps, 5.0f);
    TestTrue(TEXT("Quad and Protection share the purple palette"), Style->QuadColor.Equals(Style->ProtectionColor));
    TestTrue(TEXT("White number and red expiry warning remain"), Style->TimeColor.Equals(FLinearColor::White)
        && Style->bTimeWarningFlashEnabled && Style->TimeWarningColor.Equals(FLinearColor::Red));
    TestEqual(TEXT("A dedicated copy implements the ring"), Style->RingMaterial->GetMaterial()->GetPathName(),
        FString(TEXT("/Game/QuakeLike_Base/SDM/UI/Combat_UI/Materials/M_UI_BuffDurationRing_HUD.M_UI_BuffDurationRing_HUD")));
    Style->bFillEdgeGlowEnabled = false; // Preserve the baseline geometry/contrast captures.
    const TSharedRef<SWidget> Slate = Ring->TakeWidget();
    UImage* Image = Cast<UImage>(Ring->GetWidgetFromName(TEXT("CircleImg")));
    if (!TestNotNull(TEXT("Existing circle image"), Image)) return false;
    Ring->ApplyStatus(10.0, 1.0f, true, nullptr, nullptr);
    UMaterialInstanceDynamic* MID = Image->GetDynamicMaterial();
    if (!TestNotNull(TEXT("Ring uses a dynamic material"), MID)) return false;
    FAssetCompilingManager::Get().FinishAllCompilation();
    if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
    if (FMaterialResource* Resource = Style->RingMaterial->GetMaterial()->GetMaterialResource(GMaxRHIFeatureLevel))
        TestEqual(TEXT("Ring compiles without a fallback shader"), Resource->GetCompileErrors().Num(), 0);
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("Automation/BuffHUDStyle");
    IFileManager::Get().MakeDirectory(*Dir, true);
    auto IsFill = [](const FColor& C) { return C.R > 100 && C.B > 75 && C.R > C.G + 40 && C.B > C.G + 40; };
    auto Luminance = [](const FColor& C) { const FLinearColor L(C); return 0.2126f*L.R + 0.7152f*L.G + 0.0722f*L.B; };
    for (int32 D : {160, 240, 320})
    {
        Style->Diameter = static_cast<float>(D);
        FWidgetRenderer Renderer(true);
        int32 PreviousCount = MAX_int32;
        for (float Percent : {1.0f, 0.8f, 0.59f, 0.5f, 0.2f, 0.01f, 0.0f})
        {
            Ring->ApplyStatus(Percent > 0.0f ? 10.0 : 0.0, Percent, true, nullptr, nullptr);
            MID->GetRenderProxy()->CacheUniformExpressions_GameThread(true);
            ENQUEUE_RENDER_COMMAND(FlushBuffStyleUniforms)([](FRHICommandListImmediate& RHICmdList)
            { FMaterialRenderProxy::UpdateDeferredCachedUniformExpressions(RHICmdList); });
            FlushRenderingCommands(); Ring->ForceLayoutPrepass();
            auto* Target = Renderer.DrawWidget(Slate, FVector2D(D,D));
            FlushRenderingCommands();
            Renderer.DrawWidget(Target, Slate, FVector2D(D,D), 0.0f); FlushRenderingCommands();
            TArray<FColor> Pixels;
            if (!Target || !Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)) { AddError(TEXT("GPU capture failed")); return false; }
            int32 Count = 0;
            for (const FColor& C : Pixels) Count += IsFill(C) ? 1 : 0;
            const FString Name = FString::Printf(TEXT("Ring_%d_%03d.png"), D, FMath::RoundToInt(Percent*100));
            FBufferArchive PNG;
            TestTrue(TEXT("Ring preview encodes"), FImageUtils::ExportRenderTarget2DAsPNG(Target, PNG));
            TestTrue(TEXT("Ring preview saves"), FFileHelper::SaveArrayToFile(PNG, *(Dir / Name)));
            AddInfo(FString::Printf(TEXT("%s: purple pixels=%d"), *Name, Count));
            TestTrue(TEXT("Gauge decreases inside blocks as well as at their boundaries"), Count < PreviousCount);
            PreviousCount = Count;
            TestTrue(TEXT("The centre remains transparent"), Pixels[(D/2)*D+D/2].A == 0);
            if (Percent > 0.0f) TestTrue(TEXT("Remaining fill is visible"), Count > 0);
            else TestEqual(TEXT("Expiry leaves no ring pixels"), Count, 0);
            if (Percent == 1.0f)
            {
                const float Radius = 0.25f*D*(1.0f+1.0f/Style->RingSize);
                int32 Starts=0; bool Previous=false, First=false;
                for (int32 I=0; I<3600; ++I)
                {
                    const float A = I*2.0f*PI/3600.0f;
                    const int32 X=FMath::Clamp(FMath::RoundToInt(D*0.5f+Radius*FMath::Cos(A)),0,D-1);
                    const int32 Y=FMath::Clamp(FMath::RoundToInt(D*0.5f+Radius*FMath::Sin(A)),0,D-1);
                    const bool Current = IsFill(Pixels[Y*D+X]);
                    if (I==0) First=Current;
                    if (Current && !Previous) ++Starts;
                    Previous=Current;
                }
                if (First && Previous) --Starts;
                TestEqual(TEXT("Exactly five circular health-style blocks"), Starts, 5);
            }
            if (Percent == 0.5f)
            {
                float Fill=0.0f, Empty=0.0f; int32 FillN=0, EmptyN=0;
                const float Radius=0.25f*D*(1.0f+1.0f/Style->RingSize);
                for(int32 I=0;I<360;++I)
                {
                    const float A=I*2.0f*PI/360.0f;
                    const int32 X=FMath::Clamp(FMath::RoundToInt(D*0.5f+Radius*FMath::Cos(A)),0,D-1);
                    const int32 Y=FMath::Clamp(FMath::RoundToInt(D*0.5f+Radius*FMath::Sin(A)),0,D-1);
                    const FColor& C=Pixels[Y*D+X];
                    if(IsFill(C)){Fill+=Luminance(C);++FillN;}
                    else if(C.R>30 && C.R<80 && C.B>40 && C.B<90 && C.A>200){Empty+=Luminance(C);++EmptyN;}
                }
                TestTrue(TEXT("Both fill and empty regions are sampled"), FillN>0 && EmptyN>0);
                const float Contrast=(Fill/FMath::Max(FillN,1)+0.05f)/(Empty/FMath::Max(EmptyN,1)+0.05f);
                TestTrue(TEXT("Filled versus exhausted region contrast is at least 3:1"), Contrast>=3.0f);
                AddInfo(FString::Printf(TEXT("%dpx fill/empty contrast %.3f:1"),D,Contrast));
            }
            TestTrue(TEXT("Updates reuse the same MID"), Image->GetDynamicMaterial()==MID);
        }
    }
    Style->Diameter = 240.0f;
    Style->FillEdgeGlowStrength = 0.4f; Style->FillEdgeGlowWidth = 10.0f;
    FWidgetRenderer GlowRenderer(true);
    auto CaptureGlow = [&](const TCHAR* Name, float Progress)
    {
        Ring->ApplyStatus(Progress > 0.0f ? 10.0 : 0.0, Progress, true, nullptr, nullptr);
        MID->GetRenderProxy()->CacheUniformExpressions_GameThread(true);
        ENQUEUE_RENDER_COMMAND(FlushGlowUniforms)([](FRHICommandListImmediate& RHICmdList)
        { FMaterialRenderProxy::UpdateDeferredCachedUniformExpressions(RHICmdList); });
        FlushRenderingCommands(); Ring->ForceLayoutPrepass();
        auto* Target = GlowRenderer.DrawWidget(Slate,FVector2D(240,240)); FlushRenderingCommands();
        TArray<FColor> Pixels;
        if (!Target) { AddError(TEXT("Glow preview target unavailable")); return Pixels; }
        GlowRenderer.DrawWidget(Target,Slate,FVector2D(240,240),0.0f); FlushRenderingCommands();
        if(!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels)) AddError(TEXT("Glow GPU readback failed"));
        FBufferArchive PNG;
        if(FImageUtils::ExportRenderTarget2DAsPNG(Target,PNG)) TestTrue(TEXT("Glow A/B preview saved"),FFileHelper::SaveArrayToFile(PNG,*(Dir/Name)));
        return Pixels;
    };
    Style->bFillEdgeGlowEnabled = false;
    const auto Off = CaptureGlow(TEXT("EdgeGlow_OFF_50.png"),0.5f);
    TestEqual(TEXT("DA checkbox disables material glow"),MID->K2_GetScalarParameterValue(TEXT("EdgeGlowStrength")),0.0f);
    Style->bFillEdgeGlowEnabled = true;
    const auto On = CaptureGlow(TEXT("EdgeGlow_ON_50.png"),0.5f);
    TestEqual(TEXT("DA strength reaches material"),MID->K2_GetScalarParameterValue(TEXT("EdgeGlowStrength")),0.4f);
    TestEqual(TEXT("DA width reaches material"),MID->K2_GetScalarParameterValue(TEXT("EdgeGlowWidth")),10.0f);
    if(Off.Num()!=240*240 || On.Num()!=Off.Num()) return false;
    int32 Changed=0; float MaxDistance=0.0f;
    const FVector2D Tip(120.0,120.0+60.0*(1.0+1.0/Style->RingSize));
    for(int32 I=0;I<Off.Num();++I)
    {
        const FColor A=Off[I],B=On[I];
        if(FMath::Abs(int32(A.R)-B.R)>3 || FMath::Abs(int32(A.G)-B.G)>3 || FMath::Abs(int32(A.B)-B.B)>3 || FMath::Abs(int32(A.A)-B.A)>3)
        {
            ++Changed;MaxDistance=FMath::Max(MaxDistance,float(FVector2D::Distance(Tip,FVector2D(I%240,I/240))));
        }
    }
    TestTrue(TEXT("Enabling glow visibly changes the endpoint"),Changed>5);
    TestTrue(TEXT("Glow affects only a small local area"),Changed<240*240/40 && MaxDistance<35.0f);
    TestEqual(TEXT("Glow leaves the centre transparent"),On[120*240+120].A,uint8(0));
    AddInfo(FString::Printf(TEXT("Endpoint glow changed %d pixels; furthest change %.2fpx from tip"),Changed,MaxDistance));
    Style->FillEdgeGlowWidth=20.0f; CaptureGlow(TEXT("EdgeGlow_Wide_50.png"),0.5f);
    TestEqual(TEXT("DA width edits apply live"),MID->K2_GetScalarParameterValue(TEXT("EdgeGlowWidth")),20.0f);
    Style->bFillEdgeGlowEnabled=false;
    const auto Restored=CaptureGlow(TEXT("EdgeGlow_Restored_OFF_50.png"),0.5f);
    TestTrue(TEXT("Disabling restores the exact original pixels"),Restored==Off);
    Style->bFillEdgeGlowEnabled=true;Style->FillEdgeGlowStrength=0.0f;
    TestTrue(TEXT("Zero strength equals unchecked"),CaptureGlow(TEXT("EdgeGlow_Zero_50.png"),0.5f)==Off);
    Style->FillEdgeGlowStrength=0.4f;Style->FillEdgeGlowWidth=10.0f;
    for(float Pct:{0.3f,0.8f}) CaptureGlow(Pct==0.3f?TEXT("EdgeGlow_ON_30.png"):TEXT("EdgeGlow_ON_80.png"),Pct);
    const auto FullOn=CaptureGlow(TEXT("EdgeGlow_Full_ON.png"),1.0f);
    Style->bFillEdgeGlowEnabled=false;
    TestTrue(TEXT("A full ring never flashes at the wrap seam"),CaptureGlow(TEXT("EdgeGlow_Full_OFF.png"),1.0f)==FullOn);
    Style->bFillEdgeGlowEnabled=true;Style->FillEdgeGlowStrength=NAN;Style->FillEdgeGlowWidth=-5.0f;
    CaptureGlow(TEXT("EdgeGlow_SafeDefaults.png"),0.5f);
    TestEqual(TEXT("Invalid brightness has a finite fallback"),MID->K2_GetScalarParameterValue(TEXT("EdgeGlowStrength")),0.4f);
    TestEqual(TEXT("Width is clamped"),MID->K2_GetScalarParameterValue(TEXT("EdgeGlowWidth")),1.0f);
    TestTrue(TEXT("Switching glow reuses the existing MID"),Image->GetDynamicMaterial()==MID);
    return !HasAnyErrors();
}

#endif

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/ShooterNicknameComponent.h"
#include "Camera/ShooterOutlineComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "SceneView.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/IConsoleManager.h"
#if WITH_EDITOR
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterDisplayRegressionTest, "ShootingArena.CharacterDisplay.DamageCollisionAndVisibleMeshes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterDisplayRegressionTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false)
		.RequiresHitProxies(false).CreatePhysicsScene(true).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &Values);
	if (!TestNotNull(TEXT("Isolated regression world"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	APlayerController* PC = World->SpawnActor<APlayerController>();
	APawn* Shooter = World->SpawnActor<APawn>();
	UCapsuleComponent* Root = NewObject<UCapsuleComponent>(Shooter);
	Shooter->SetRootComponent(Root);
	Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Root->SetHiddenInGame(true, false); // Actual BP_ShooterBase: hidden capsule, visible child meshes.
	Root->RegisterComponent();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Engine test mesh"), Cube)) { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); return false; }
	auto AddPart = [&](FVector Location)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Shooter);
		Mesh->SetupAttachment(Root);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetRelativeLocation(Location);
		Mesh->SetRelativeScale3D(FVector(0.2));
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->RegisterComponent();
		return Mesh;
	};
	UStaticMeshComponent* Head = AddPart(FVector(500, 0, 180));
	TestFalse(TEXT("Collision root is hidden"), Root->IsVisible());
	TestTrue(TEXT("Child mesh remains visible under hidden collision root"), Head->IsVisible());
	AddPart(FVector(500, 60, 120)); // Arm outside the central capsule.
	AddPart(FVector(500, -20, 30));
	UBoxComponent* CapsuleStandIn = NewObject<UBoxComponent>(Shooter);
	CapsuleStandIn->SetupAttachment(Root);
	CapsuleStandIn->SetRelativeLocation(FVector(500, 0, 100));
	CapsuleStandIn->SetBoxExtent(FVector(30, 30, 100));
	CapsuleStandIn->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleStandIn->SetCollisionResponseToAllChannels(ECR_Block);
	CapsuleStandIn->RegisterComponent();
	UShooterNicknameComponent* Nickname = NewObject<UShooterNicknameComponent>(PC);
	Nickname->RegisterComponent();
	Nickname->ShooterClass = APawn::StaticClass();
	TestEqual(TEXT("Actual damage capsule resolves nickname without mesh physics"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector), Shooter);
	TestNull(TEXT("Visible arm outside actual collision must not resolve nickname"),
		Nickname->FindAimTarget(FVector(0, 60, 120), FVector::ForwardVector));
	CapsuleStandIn->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	TestEqual(TEXT("RailGun overlap damage capsule resolves despite no blocking hit"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector), Shooter);
	CapsuleStandIn->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestNull(TEXT("Disabled damage collision must not resolve nickname"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector));
	CapsuleStandIn->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CapsuleStandIn->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	TestNull(TEXT("Damage channel Ignore must not resolve visible body"),
		Nickname->FindAimTarget(FVector(0, 0, 180), FVector::ForwardVector));
	Nickname->TraceChannel = ECC_Camera;
	TestEqual(TEXT("Matching an alternate weapon channel resolves damage collision"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector), Shooter);
	Nickname->TraceChannel = ECC_GameTraceChannel2;
	CapsuleStandIn->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	AActor* Weapon = World->SpawnActor<AActor>();
	Weapon->SetOwner(Shooter);
	Weapon->Tags.Add(TEXT("DeathCamWeapon"));
	UStaticMeshComponent* WeaponMesh = NewObject<UStaticMeshComponent>(Weapon);
	Weapon->SetRootComponent(WeaponMesh);
	WeaponMesh->SetStaticMesh(Cube);
	WeaponMesh->SetRelativeScale3D(FVector(0.2));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WeaponMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WeaponMesh->RegisterComponent();
	Weapon->AttachToActor(Shooter, FAttachmentTransformRules::KeepWorldTransform);
	Weapon->SetActorLocation(FVector(400, 100, 120));
	TestNull(TEXT("Weapon-only hit never promotes the weapon to its shooter owner"),
		Nickname->FindAimTarget(FVector(0, 100, 120), FVector::ForwardVector));
	Weapon->SetActorLocation(FVector(400, 0, 80));
	TestNull(TEXT("Blocking weapon collision prevents a body behind it being selected"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestEqual(TEXT("NoCollision weapon leaves the real body collision targetable"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector), Shooter);
	Weapon->Destroy();
	AActor* Wall = World->SpawnActor<AActor>();
	UBoxComponent* WallMesh = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(WallMesh);
	WallMesh->SetBoxExtent(FVector(10, 200, 200));
	WallMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WallMesh->SetCollisionResponseToAllChannels(ECR_Block);
	WallMesh->RegisterComponent();
	Wall->SetActorLocation(FVector(250, 0, 100));
	TestNull(TEXT("Wall blocks nickname before damage collision"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector));
	Wall->Destroy();
	TestEqual(TEXT("Removing obstruction restores damage collision target"),
		Nickname->FindAimTarget(FVector(0, 0, 80), FVector::ForwardVector), Shooter);
	UStaticMeshComponent* FirstPersonPart = AddPart(FVector(500, 100, 180));
	FirstPersonPart->SetOnlyOwnerSee(true);
	UStaticMeshComponent* HiddenPart = AddPart(FVector(500, 150, 180));
	HiddenPart->SetVisibility(false);
	Head->SetOwnerNoSee(true);

	UShooterOutlineComponent* Outline = NewObject<UShooterOutlineComponent>(PC);
	Outline->RegisterComponent();
	Outline->ShooterClass = APawn::StaticClass();
	TestTrue(TEXT("Controller owner is hidden by engine default"), PC->IsHidden());
	Outline->RefreshShooters();
	TestEqual(TEXT("Only camera-visible parts receive outlines; remote FP and hidden meshes excluded"), Outline->Proxies.Num(), 3);
	for (const FShooterOutlineProxy& Proxy : Outline->Proxies)
	{
		TestEqual(TEXT("Proxy belongs to visible source actor"), Proxy.Mesh->GetOwner(), static_cast<AActor*>(Shooter));
		TestFalse(TEXT("Proxy is not hidden by controller ownership"), Proxy.Mesh->GetOwner()->IsHidden());
		TestTrue(TEXT("Proxy writes CustomDepth"), !!Proxy.Mesh->bRenderCustomDepth);
		TestFalse(TEXT("Proxy stays out of main pass"), !!Proxy.Mesh->bRenderInMainPass);
		TestEqual(TEXT("Proxy retains source OwnerNoSee"), !!Proxy.Mesh->bOwnerNoSee, !!Proxy.Source->bOwnerNoSee);
		TestEqual(TEXT("Proxy retains source OnlyOwnerSee"), !!Proxy.Mesh->bOnlyOwnerSee, !!Proxy.Source->bOnlyOwnerSee);
		TestNotEqual(TEXT("Remote first-person source is excluded"), Proxy.Source.Get(), static_cast<UMeshComponent*>(FirstPersonPart));
	}
	FSceneViewFamilyContext Family(FSceneViewFamily::ConstructionValues(nullptr, World->Scene, FEngineShowFlags(ESFIM_Game)));
	FSceneViewInitOptions ViewOptions;
	ViewOptions.ViewFamily = &Family;
	ViewOptions.SetViewRectangle(FIntRect(0, 0, 256, 256));
	FSceneView View(ViewOptions);
	View.HiddenPrimitives.Add(Head->GetPrimitiveSceneId());
	View.ShowOnlyPrimitives.Emplace();
	View.ShowOnlyPrimitives->Add(Head->GetPrimitiveSceneId());
	Outline->ApplyViewVisibility(View);
	for (const FShooterOutlineProxy& Proxy : Outline->Proxies)
		if (Proxy.Source == Head)
		{
			TestTrue(TEXT("Camera hidden-component list also hides its outline proxy"), View.HiddenPrimitives.Contains(Proxy.Mesh->GetPrimitiveSceneId()));
			TestTrue(TEXT("Camera show-only list includes matching outline proxy"), View.ShowOnlyPrimitives->Contains(Proxy.Mesh->GetPrimitiveSceneId()));
		}
	Head->SetOnlyOwnerSee(true);
	Outline->UpdateProxies();
	for (const FShooterOutlineProxy& Proxy : Outline->Proxies)
		if (Proxy.Source == Head) TestFalse(TEXT("Runtime owner-only change disables the foreign outline immediately"), !!Proxy.Mesh->bRenderCustomDepth);
	Head->SetOnlyOwnerSee(false);
	TestFalse(TEXT("Original mesh CustomDepth remains unchanged"), !!Head->bRenderCustomDepth);
	TestEqual(TEXT("Original collision response remains Ignore"),
		Head->GetCollisionResponseToChannel(ECC_GameTraceChannel2), ECR_Ignore);
	Outline->SetOutlineEnabled(false);
	TestEqual(TEXT("Disabling outlines removes proxies"), Outline->Proxies.Num(), 0);

	// Actual project mesh: target the existing capsule, never invent mesh collision.
	APawn* SteelShooter = World->SpawnActor<APawn>();
	UCapsuleComponent* SteelRoot = NewObject<UCapsuleComponent>(SteelShooter);
	SteelShooter->SetRootComponent(SteelRoot);
	SteelRoot->InitCapsuleSize(34, 88);
	SteelRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SteelRoot->SetCollisionResponseToAllChannels(ECR_Ignore);
	SteelRoot->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
	SteelRoot->SetHiddenInGame(true, false);
	SteelRoot->RegisterComponent();
	SteelShooter->SetActorLocation(FVector(1000, 1000, 88));
	USkeletalMeshComponent* SteelMesh = NewObject<USkeletalMeshComponent>(SteelShooter);
	SteelMesh->SetupAttachment(SteelRoot);
	SteelMesh->SetRelativeLocation(FVector(0, 0, -88));
	SteelMesh->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/ParagonSteel/Characters/Heroes/Steel/Meshes/NoShield_Steel.NoShield_Steel")));
	SteelMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SteelMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	SteelMesh->RegisterComponent();
	SteelMesh->RefreshBoneTransforms();
	TestNull(TEXT("Real NoShield_Steel source has no PhysicsAsset"), SteelMesh->GetPhysicsAsset());
	{
		TestEqual(TEXT("Real Steel resolves nickname through existing damage capsule"),
			Nickname->FindAimTarget(FVector(0, 1000, 88), FVector::ForwardVector), SteelShooter);
		TestNull(TEXT("Outside real Steel damage capsule does not invent body collision"),
			Nickname->FindAimTarget(FVector(0, 1060, 88), FVector::ForwardVector));
		TestNull(TEXT("Original PhysicsAsset stays unchanged"), SteelMesh->GetPhysicsAsset());
		Outline->SetOutlineEnabled(true);
		Outline->RefreshShooters();
		bool bFoundSteelProxy = false;
		for (const FShooterOutlineProxy& Proxy : Outline->Proxies)
			bFoundSteelProxy |= Proxy.Source == SteelMesh;
		TestTrue(TEXT("Real Steel below hidden capsule receives outline proxy"), bFoundSteelProxy);
		Outline->SetOutlineEnabled(false);
		if (!GUsingNullRHI)
		{
			UShooterOutlineSettings* Settings = LoadObject<UShooterOutlineSettings>(nullptr,
				TEXT("/Game/QuakeLike_1_0/Data/Player/DA_ShooterOutline_Default.DA_ShooterOutline_Default"));
			// Keep the render probe independent of the gameplay Blueprint dependency graph.
			Outline->PrepareMaterial(Settings->Material);
			Outline->StencilValue = Settings->StencilValue;
			Outline->ShooterClass = APawn::StaticClass();
			FShooterOutlineStyle Style = Outline->GetOutlineStyle();
			Style.bEnabled = true;
			Style.Color = FLinearColor(1, 0, 1, 1);
			Style.Thickness = 5;
			Style.Opacity = 1;
			Outline->SetOutlineStyle(Style);
			Outline->RefreshShooters();
			Outline->UpdateProxies();
			SteelMesh->ShowMaterialSection(0, 0, false, 0);
			Outline->UpdateProxies();
			for (const FShooterOutlineProxy& Proxy : Outline->Proxies)
				if (Proxy.Source == SteelMesh)
					TestFalse(TEXT("Hidden source material section stays hidden in outline"),
						CastChecked<USkeletalMeshComponent>(Proxy.Mesh)->IsMaterialSectionShown(0, 0));
			SteelMesh->ShowAllMaterialSections(0);
			Outline->UpdateProxies();
#if WITH_EDITOR
			FAssetCompilingManager::Get().FinishAllCompilation();
			if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
#endif
			FMaterialResource* MaterialResource = Settings->Material->GetMaterialResource(World->GetFeatureLevel());
			AddInfo(FString::Printf(TEXT("Material feature level %d, shader map %d, complete %d"),
				(int32)World->GetFeatureLevel(), MaterialResource && MaterialResource->GetGameThreadShaderMap(),
				MaterialResource && MaterialResource->IsGameThreadShaderMapComplete()));
			SteelMesh->RefreshBoneTransforms();
			SteelMesh->MarkRenderStateDirty();
			for (const FShooterOutlineProxy& Proxy : Outline->Proxies) Proxy.Mesh->MarkRenderStateDirty();
			AActor* CaptureActor = World->SpawnActor<AActor>();
			USceneCaptureComponent2D* Capture = NewObject<USceneCaptureComponent2D>(CaptureActor);
			CaptureActor->SetRootComponent(Capture);
			Capture->bCaptureEveryFrame = false;
			Capture->bCaptureOnMovement = false;
			Capture->bAlwaysPersistRenderingState = true;
			Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
			Capture->FOVAngle = 60;
			Capture->ShowFlags.SetEditor(false);
			Capture->ShowFlags.SetGame(true);
			Capture->TextureTarget = NewObject<UTextureRenderTarget2D>(CaptureActor);
			Capture->TextureTarget->InitCustomFormat(256, 256, PF_B8G8R8A8, false);
			Capture->TextureTarget->UpdateResourceImmediate();
			Capture->PostProcessSettings.AddBlendable(Outline->Material, 1);
			Capture->RegisterComponent();
			Capture->SetVisibility(true);
			Capture->SetHiddenInGame(false);
			Capture->HideActorComponents(CaptureActor); // Exclude the editor-only camera proxy from its own view.
			CaptureActor->SetActorLocation(SteelMesh->Bounds.Origin - FVector(500, 0, 0));
			AddInfo(FString::Printf(TEXT("Steel transform %s, bounds %s, extent %s; capture %s"),
				*SteelMesh->GetComponentLocation().ToString(), *SteelMesh->Bounds.Origin.ToString(),
				*SteelMesh->Bounds.BoxExtent.ToString(), *Capture->GetComponentLocation().ToString()));
			AddInfo(FString::Printf(TEXT("Capture visible %d, world scene %d, CustomDepth cvar %d"),
				Capture->IsVisible(), !!World->Scene, IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"))->GetInt()));
			TArray<FColor> WithoutOutline, WithOutline;
			auto SaveCapture = [&](const TCHAR* File)
			{
				Capture->CaptureScene(); FlushRenderingCommands();
				TArray<FColor> Pixels;
				Capture->TextureTarget->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
				TArray64<uint8> PNG;
				FImageUtils::PNGCompressImageArray(256, 256, Pixels, PNG);
				FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / File));
			};
			Capture->PostProcessSettings.WeightedBlendables.Array.Empty();
			SaveCapture(TEXT("CharacterDisplaySceneProbe.png"));
			Capture->PostProcessSettings.AddBlendable(Outline->Material, 1);
			Outline->Material->SetScalarParameterValue(TEXT("OutlineOpacity"), 0);
			Capture->CaptureScene();
			FlushRenderingCommands();
			TestTrue(TEXT("Read capture without outline"), Capture->TextureTarget->GameThread_GetRenderTargetResource()->ReadPixels(WithoutOutline));
			{
				TArray64<uint8> PNG;
				FImageUtils::PNGCompressImageArray(256, 256, WithoutOutline, PNG);
				FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / TEXT("CharacterDisplayWithoutOutlineProbe.png")));
			}
			Outline->Material->SetScalarParameterValue(TEXT("OutlineOpacity"), 1);
			Capture->CaptureScene();
			FlushRenderingCommands();
			TestTrue(TEXT("Read capture with outline"), Capture->TextureTarget->GameThread_GetRenderTargetResource()->ReadPixels(WithOutline));
			int32 MagentaPixels = 0;
			for (int32 Index = 0; Index < WithOutline.Num() && Index < WithoutOutline.Num(); ++Index)
			{
				const FColor Color = WithOutline[Index];
				if (Color.R > 100 && Color.B > 100 && Color.G < 60 && Color != WithoutOutline[Index]) ++MagentaPixels;
			}
			TestTrue(FString::Printf(TEXT("Actual Steel renders visible magenta outline (%d pixels)"), MagentaPixels), MagentaPixels > 10);
			AddInfo(FString::Printf(TEXT("Outline render probe: %d magenta pixels"), MagentaPixels));
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(256, 256, WithOutline, PNG);
			FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir() / TEXT("CharacterDisplayOutlineProbe.png")));
			SteelMesh->SetRenderCustomDepth(true);
			SteelMesh->SetCustomDepthStencilValue(2);
			SaveCapture(TEXT("CharacterDisplaySourceStencilProbe.png"));
			Capture->PostProcessSettings.WeightedBlendables.Array.Empty();
			if (UMaterialInterface* StencilDebug = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Engine/BufferVisualization/CustomStencil.CustomStencil")))
			{
#if WITH_EDITOR
				if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
#endif
				Capture->PostProcessSettings.AddBlendable(StencilDebug, 1);
				SaveCapture(TEXT("CharacterDisplayStencilDebugProbe.png"));
			}
			Outline->SetOutlineEnabled(false);
		}
	}
	Nickname->SetNicknameEnabled(false);
	TestFalse(TEXT("Nickname OFF remains independent from outline settings"), Nickname->IsNicknameEnabled());
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	return true;
}

#endif

#include "DressSwapTool.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Docking/TabManager.h"
#include "ScopedTransaction.h"
#include "Textures/SlateIcon.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/StaticMeshComponent.h"
#include "ToolMenus.h"

namespace
{
	const FName DressSwapTabName(TEXT("ShootingArenaDressSwap"));

	class SDressSwapPanel final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDressSwapPanel) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& Args)
		{
			FContentBrowserModule& ContentBrowser =
				FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

			FAssetPickerConfig PickerConfig;
			PickerConfig.Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
			PickerConfig.bAllowNullSelection = false;
			PickerConfig.bAllowDragging = false;
			PickerConfig.bFocusSearchBoxWhenOpened = true;
			PickerConfig.InitialAssetViewType = EAssetViewType::List;
			PickerConfig.Filter.bRecursiveClasses = true;
			PickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SDressSwapPanel::OnAssetSelected);

			FAssetPickerConfig MaterialPickerConfig;
			MaterialPickerConfig.Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
			MaterialPickerConfig.Filter.bRecursiveClasses = true;
			MaterialPickerConfig.bAllowNullSelection = false;
			MaterialPickerConfig.bAllowDragging = false;
			MaterialPickerConfig.bFocusSearchBoxWhenOpened = true;
			MaterialPickerConfig.InitialAssetViewType = EAssetViewType::List;
			MaterialPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SDressSwapPanel::OnMaterialSelected);

			ChildSlot
			[
				SNew(SBorder)
				.Padding(10.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("레벨에서 대상 액터를 선택한 뒤, 아래 목록에서 적용할 Static Mesh를 고르세요.")))
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0, 0, 4, 0)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("메시 교체")))
							.OnClicked(this, &SDressSwapPanel::ShowMeshMode)
						]
						+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(4, 0, 0, 0)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("머티리얼 교체")))
							.OnClicked(this, &SDressSwapPanel::ShowMaterialMode)
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.0f).MinHeight(220.0f)
					[
						SAssignNew(AssetTypeSwitcher, SWidgetSwitcher)
						+ SWidgetSwitcher::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().FillHeight(1.0f)
							[
								ContentBrowser.Get().CreateAssetPicker(PickerConfig)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)
							[
								SAssignNew(SelectedMeshText, STextBlock)
								.Text(FText::FromString(TEXT("교체할 메시: 선택 안 됨")))
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("선택 액터에 메시 적용")))
								.IsEnabled_Lambda([this]() { return SelectedMesh.IsValid(); })
								.OnClicked(this, &SDressSwapPanel::ApplyMeshReplacement)
							]
						]
						+ SWidgetSwitcher::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().FillHeight(1.0f)
							[
								ContentBrowser.Get().CreateAssetPicker(MaterialPickerConfig)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)
							[
								SAssignNew(SelectedMaterialText, STextBlock)
								.Text(FText::FromString(TEXT("적용할 머티리얼: 선택 안 됨")))
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
							[
								SNew(SButton)
								.Text(FText::FromString(TEXT("선택 액터의 모든 슬롯에 적용")))
								.IsEnabled_Lambda([this]() { return SelectedMaterial.IsValid(); })
								.OnClicked(this, &SDressSwapPanel::ApplyMaterialReplacement)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
					[
						SAssignNew(StatusText, STextBlock)
						.Text(FText::FromString(TEXT("레벨에서 Static Mesh 컴포넌트가 하나인 액터를 선택하세요.")))
						.AutoWrapText(true)
					]
				]
			];
		}

	private:
		void OnAssetSelected(const FAssetData& AssetData)
		{
			SelectedMesh = Cast<UStaticMesh>(AssetData.GetAsset());
			SelectedMeshText->SetText(SelectedMesh.IsValid()
				? FText::Format(FText::FromString(TEXT("교체할 메시: {0}")), FText::FromString(SelectedMesh->GetPathName()))
				: FText::FromString(TEXT("교체할 메시: 선택 안 됨")));
		}

		void OnMaterialSelected(const FAssetData& AssetData)
		{
			SelectedMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
			SelectedMaterialText->SetText(SelectedMaterial.IsValid()
				? FText::Format(FText::FromString(TEXT("적용할 머티리얼: {0}")), FText::FromString(SelectedMaterial->GetPathName()))
				: FText::FromString(TEXT("적용할 머티리얼: 선택 안 됨")));
		}

		FReply ShowMeshMode()
		{
			AssetTypeSwitcher->SetActiveWidgetIndex(0);
			StatusText->SetText(FText::FromString(TEXT("메시 교체: Static Mesh를 고르고 레벨 액터에 적용하세요.")));
			return FReply::Handled();
		}

		FReply ShowMaterialMode()
		{
			AssetTypeSwitcher->SetActiveWidgetIndex(1);
			StatusText->SetText(FText::FromString(TEXT("머티리얼 교체: Material 또는 Material Instance를 골라 모든 슬롯에 적용하세요.")));
			return FReply::Handled();
		}

		FReply ApplyMeshReplacement()
		{
			if (!GEditor || !SelectedMesh.IsValid())
			{
				StatusText->SetText(FText::FromString(TEXT("먼저 교체할 Static Mesh를 고르세요.")));
				return FReply::Handled();
			}

			USelection* Selection = GEditor->GetSelectedActors();
			if (!Selection || Selection->Num() == 0)
			{
				StatusText->SetText(FText::FromString(TEXT("레벨에서 선택된 액터가 없습니다.")));
				return FReply::Handled();
			}

			struct FReplacementTarget
			{
				AActor* Actor = nullptr;
				UStaticMeshComponent* Component = nullptr;
			};
			TArray<FReplacementTarget> Targets;
			int32 Skipped = 0;

			for (FSelectionIterator It(*Selection); It; ++It)
			{
				AActor* Actor = Cast<AActor>(*It);
				if (!Actor)
				{
					++Skipped;
					continue;
				}

				TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
				Actor->GetComponents(MeshComponents);
				if (MeshComponents.Num() != 1 || !MeshComponents[0]->GetStaticMesh())
				{
					++Skipped;
					continue;
				}

				Targets.Add({ Actor, MeshComponents[0] });
			}

			if (Targets.IsEmpty())
			{
				StatusText->SetText(FText::Format(
					FText::FromString(TEXT("교체된 액터가 없습니다. {0}개를 건너뛰었습니다. 대상은 Static Mesh 컴포넌트가 정확히 하나여야 합니다.")),
					FText::AsNumber(Skipped)));
				return FReply::Handled();
			}

			const FScopedTransaction Transaction(FText::FromString(TEXT("선택한 Static Mesh 교체")));
			int32 Replaced = 0;
			for (const FReplacementTarget& Target : Targets)
			{
				Target.Actor->Modify();
				Target.Component->Modify();
				Target.Component->SetStaticMesh(SelectedMesh.Get());
				Target.Component->PostEditChange();
				Target.Actor->MarkPackageDirty();
				++Replaced;
			}

			StatusText->SetText(FText::Format(
				FText::FromString(TEXT("{0}개 액터 적용 완료, {1}개 건너뜀. Ctrl+Z로 되돌릴 수 있습니다.")),
				FText::AsNumber(Replaced), FText::AsNumber(Skipped)));
			return FReply::Handled();
		}

		FReply ApplyMaterialReplacement()
		{
			if (!GEditor || !SelectedMaterial.IsValid())
			{
				StatusText->SetText(FText::FromString(TEXT("먼저 적용할 머티리얼을 고르세요.")));
				return FReply::Handled();
			}

			USelection* Selection = GEditor->GetSelectedActors();
			if (!Selection || Selection->Num() == 0)
			{
				StatusText->SetText(FText::FromString(TEXT("레벨에서 선택된 액터가 없습니다.")));
				return FReply::Handled();
			}

			struct FMaterialTarget
			{
				AActor* Actor = nullptr;
				UStaticMeshComponent* Component = nullptr;
				int32 SlotCount = 0;
			};
			TArray<FMaterialTarget> Targets;
			int32 Skipped = 0;

			for (FSelectionIterator It(*Selection); It; ++It)
			{
				AActor* Actor = Cast<AActor>(*It);
				if (!Actor)
				{
					++Skipped;
					continue;
				}

				TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
				Actor->GetComponents(MeshComponents);
				if (MeshComponents.Num() != 1 || !MeshComponents[0]->GetStaticMesh() || MeshComponents[0]->GetNumMaterials() == 0)
				{
					++Skipped;
					continue;
				}

				Targets.Add({ Actor, MeshComponents[0], MeshComponents[0]->GetNumMaterials() });
			}

			if (Targets.IsEmpty())
			{
				StatusText->SetText(FText::Format(
					FText::FromString(TEXT("적용된 액터가 없습니다. {0}개를 건너뛰었습니다. 대상은 Static Mesh와 머티리얼 슬롯이 있어야 합니다.")),
					FText::AsNumber(Skipped)));
				return FReply::Handled();
			}

			const FScopedTransaction Transaction(FText::FromString(TEXT("선택 액터 머티리얼 교체")));
			int32 Applied = 0;
			for (const FMaterialTarget& Target : Targets)
			{
				Target.Actor->Modify();
				Target.Component->Modify();
				for (int32 SlotIndex = 0; SlotIndex < Target.SlotCount; ++SlotIndex)
				{
					Target.Component->SetMaterial(SlotIndex, SelectedMaterial.Get());
				}
				Target.Component->PostEditChange();
				Target.Actor->MarkPackageDirty();
				++Applied;
			}

			StatusText->SetText(FText::Format(
				FText::FromString(TEXT("{0}개 액터의 머티리얼 슬롯에 적용 완료, {1}개 건너뜀. Ctrl+Z로 되돌릴 수 있습니다.")),
				FText::AsNumber(Applied), FText::AsNumber(Skipped)));
			return FReply::Handled();
		}

		TWeakObjectPtr<UStaticMesh> SelectedMesh;
		TWeakObjectPtr<UMaterialInterface> SelectedMaterial;
		TSharedPtr<SWidgetSwitcher> AssetTypeSwitcher;
		TSharedPtr<STextBlock> SelectedMeshText;
		TSharedPtr<STextBlock> SelectedMaterialText;
		TSharedPtr<STextBlock> StatusText;
	};
}

void FDressSwapTool::Register()
{
	if (bRegistered)
	{
		return;
	}

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DressSwapTabName,
		FOnSpawnTab::CreateRaw(this, &FDressSwapTool::CreateTab))
		.SetDisplayName(FText::FromString(TEXT("에셋 입혀보기")))
		.SetTooltipText(FText::FromString(TEXT("선택한 레벨 액터에 Static Mesh를 빠르게 적용합니다.")))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	ToolMenusStartupCallbackHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]()
	{
		FToolMenuOwnerScoped Owner(this);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Window"));
		FToolMenuSection& Section = Menu->FindOrAddSection(TEXT("ShootingArenaTools"));
		Section.AddMenuEntry(
			TEXT("OpenDressSwap"),
			FText::FromString(TEXT("에셋 입혀보기")),
			FText::FromString(TEXT("선택한 레벨 액터에 Static Mesh를 적용합니다.")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				FGlobalTabmanager::Get()->TryInvokeTab(DressSwapTabName);
			})));
	}));

	bRegistered = true;
}

void FDressSwapTool::Unregister()
{
	if (!bRegistered)
	{
		return;
	}

	UToolMenus::UnRegisterStartupCallback(ToolMenusStartupCallbackHandle);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DressSwapTabName);
	bRegistered = false;
}

TSharedRef<SDockTab> FDressSwapTool::CreateTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SDressSwapPanel)
		];
}

#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"

#include "Engine/Blueprint.h"

#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"

#include "FileHelpers.h"

#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "Misc/MessageDialog.h"
#include "Misc/ScopedSlowTask.h"

#include "Editor.h"
#include "DetailLayoutBuilder.h"
#include "IDetailCustomization.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "UI/MatchTimerStyle.h"
#include "UObject/StrongObjectPtr.h"
#include "DressSwapTool.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"


DEFINE_LOG_CATEGORY_STATIC(LogBPRefreshTool, Log, All);

namespace
{
    void SyncMapNameFormatting(UObject* Object, FPropertyChangedEvent& Event)
    {
        static const FName Left(TEXT("MapName_1"));
        static const FName Right(TEXT("MapName_2"));
        UTextBlock* Label = Cast<UTextBlock>(Object);
        if (!Label || (Label->GetFName() != Left && Label->GetFName() != Right) || GIsTransacting)
            return;

        FProperty* Property = Event.MemberProperty ? Event.MemberProperty : Event.Property;
        static const TSet<FName> FormattingProperties = {
            TEXT("Font"), TEXT("ColorAndOpacity"), TEXT("ShadowOffset"), TEXT("ShadowColorAndOpacity"),
            TEXT("Justification"), TEXT("Margin"), TEXT("LineHeightPercentage"), TEXT("bApplyLineHeightToBottomLine"),
            TEXT("AutoWrapText"), TEXT("WrapTextAt"), TEXT("WrappingPolicy"), TEXT("TextOverflowPolicy"),
            TEXT("TextTransformPolicy"), TEXT("ShapedTextOptions"), TEXT("MinDesiredWidth"),
            TEXT("bSimpleTextMode"), TEXT("RenderOpacity"), TEXT("RenderTransform"), TEXT("RenderTransformPivot")
        };
        if (!Property || !FormattingProperties.Contains(Property->GetFName()))
            return;

        const UWidgetTree* Tree = Label->GetTypedOuter<UWidgetTree>();
        if (!Tree)
            return;

        UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Tree->GetOuter());
        UUserWidget* Preview = Cast<UUserWidget>(Tree->GetOuter());
        if (Preview && Preview->IsDesignTime())
            Blueprint = Cast<UWidgetBlueprint>(Preview->GetClass()->ClassGeneratedBy);

        static const FName PackageName(TEXT("/Game/QuakeLike_1_0/GameFlow/UI/WBP_MapSelect_SDM"));
        if (!Blueprint || Blueprint->GetOutermost()->GetFName() != PackageName || !Blueprint->WidgetTree)
            return;

        const FName PeerName = Label->GetFName() == Left ? Right : Left;
        const auto CopyProperty = [Label, Property](UTextBlock* Peer, bool bTemplate)
        {
            if (!Peer || Peer->GetClass() != Label->GetClass() || Property->Identical_InContainer(Label, Peer))
                return false;
            if (bTemplate)
                Peer->Modify();
            Property->CopyCompleteValue_InContainer(Peer, Label);
            Peer->SynchronizeProperties();
            return true;
        };

        // UMG commits interactive edits from the preview to the template on release.
        if (!(Event.ChangeType & EPropertyChangeType::Interactive)
            && CopyProperty(Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(PeerName)), true))
        {
            Blueprint->Status = BS_Dirty;
            Blueprint->MarkPackageDirty();
        }
        if (Preview)
            CopyProperty(Cast<UTextBlock>(Preview->GetWidgetFromName(PeerName)), false);
    }

    class FMatchTimerStyleDetails : public IDetailCustomization
    {
    public:
        virtual void CustomizeDetails(IDetailLayoutBuilder& Layout) override
        {
            const auto Font = Layout.GetProperty(GET_MEMBER_NAME_CHECKED(UMatchTimerStyle, Font));
            const auto Outline = Font->GetChildHandle(GET_MEMBER_NAME_CHECKED(FSlateFontInfo, OutlineSettings));

            const auto SetTooltip = [](const IPropertyHandle& Parent, FName Property, const FText& Tooltip)
            {
                if (const auto Child = Parent.GetChildHandle(Property))
                {
                    Child->SetToolTipText(Tooltip);
                }
            };

            SetTooltip(*Font, "FontObject", NSLOCTEXT("MatchTimerStyleDetails", "FontObject", "타이머에 사용할 글꼴 에셋을 선택합니다."));
            SetTooltip(*Font, "TypefaceFontName", NSLOCTEXT("MatchTimerStyleDetails", "Typeface", "선택한 글꼴의 서체를 지정합니다. 서체에 따라 글자 굵기가 달라집니다."));
            SetTooltip(*Font, "Size", NSLOCTEXT("MatchTimerStyleDetails", "Size", "타이머 글자의 크기입니다. 기본값 45는 HP 숫자와 같습니다."));
            SetTooltip(*Font, "LetterSpacing", NSLOCTEXT("MatchTimerStyleDetails", "LetterSpacing", "글자 사이의 간격을 조절합니다."));
            SetTooltip(*Font, "SkewAmount", NSLOCTEXT("MatchTimerStyleDetails", "SkewAmount", "글자를 기울이는 정도를 조절합니다."));
            SetTooltip(*Font, "bForceMonospaced", NSLOCTEXT("MatchTimerStyleDetails", "Monospacing", "타이머 숫자는 시간에 따라 너비가 달라지지 않도록 항상 같은 폭으로 표시합니다."));
            SetTooltip(*Font, "MonospacedWidth", NSLOCTEXT("MatchTimerStyleDetails", "MonospacedWidth", "타이머 숫자의 고정 폭은 선택한 글꼴과 크기에 맞춰 자동으로 계산합니다."));
            SetTooltip(*Font, "FontMaterial", NSLOCTEXT("MatchTimerStyleDetails", "FontMaterial", "글자에 적용할 머티리얼을 선택합니다."));
            SetTooltip(*Font, "bMaterialIsStencil", NSLOCTEXT("MatchTimerStyleDetails", "MaterialIsStencil", "글자 모양을 머티리얼에서 직접 처리하도록 합니다."));
            SetTooltip(*Font, "OutlineSettings", NSLOCTEXT("MatchTimerStyleDetails", "OutlineSettings", "글자 외곽선의 두께, 색상, 머티리얼을 설정합니다."));
            if (Outline)
            {
                SetTooltip(*Outline, "OutlineSize", NSLOCTEXT("MatchTimerStyleDetails", "OutlineSize", "글자 외곽선의 두께를 조절합니다."));
                SetTooltip(*Outline, "bMiteredCorners", NSLOCTEXT("MatchTimerStyleDetails", "MiteredCorners", "외곽선 모서리를 뾰족하게 표시합니다. 끄면 모서리가 둥글게 표시됩니다."));
                SetTooltip(*Outline, "bSeparateFillAlpha", NSLOCTEXT("MatchTimerStyleDetails", "SeparateFillAlpha", "글자 안쪽과 외곽선의 불투명도를 따로 적용합니다."));
                SetTooltip(*Outline, "bApplyOutlineToDropShadows", NSLOCTEXT("MatchTimerStyleDetails", "ApplyOutlineToDropShadows", "글자에 사용한 외곽선을 그림자에도 적용합니다."));
                SetTooltip(*Outline, "OutlineMaterial", NSLOCTEXT("MatchTimerStyleDetails", "OutlineMaterial", "외곽선에 적용할 머티리얼을 선택합니다."));
                SetTooltip(*Outline, "OutlineColor", NSLOCTEXT("MatchTimerStyleDetails", "OutlineColor", "글자 외곽선의 색상을 설정합니다."));
            }
        }
    };
}

class FShootingArenaEditorToolsModule : public IModuleInterface
{
public:

    virtual void StartupModule() override
    {
        // Uncooked Standalone also needs the reflected animation graph classes,
        // but authoring menus and Blueprint refresh commands belong to the editor.
        if (!GIsEditor)
        {
            return;
        }

        MapNameFormattingHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddStatic(&SyncMapNameFormatting);

        DressSwapTool = MakeUnique<FDressSwapTool>();
        DressSwapTool->Register();

        FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor").RegisterCustomClassLayout(
            TEXT("MatchTimerStyle"),
            FOnGetDetailCustomizationInstance::CreateLambda([] { return MakeShared<FMatchTimerStyleDetails>(); }));

        RefreshBlueprintsCommand =
            MakeUnique<FAutoConsoleCommand>(
                TEXT("ShootingArena.RefreshBlueprints"),

                TEXT(
                    "Refresh, compile and save Blueprint assets.\n"
                    "Usage:\n"
                    "  ShootingArena.RefreshBlueprints\n"
                    "  ShootingArena.RefreshBlueprints /Game/SomeFolder\n"
                    "  ShootingArena.RefreshBlueprints /Game/SomeFolder resaveall"
                ),

                FConsoleCommandWithArgsDelegate::CreateRaw(
                    this,
                    &FShootingArenaEditorToolsModule::RefreshBlueprints
                ),

                ECVF_Default
            );
    }


    virtual void ShutdownModule() override
    {
        FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(MapNameFormattingHandle);
        if (GIsEditor)
        {
            if (auto* PropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
            {
                PropertyEditor->UnregisterCustomClassLayout(TEXT("MatchTimerStyle"));
            }
        }

        if (DressSwapTool)
        {
            DressSwapTool->Unregister();
            DressSwapTool.Reset();
        }
        RefreshBlueprintsCommand.Reset();
    }


private:

    TUniquePtr<FAutoConsoleCommand> RefreshBlueprintsCommand;
    TUniquePtr<FDressSwapTool> DressSwapTool;
    FDelegateHandle MapNameFormattingHandle;


    void RefreshBlueprints(const TArray<FString>& Args)
    {
        const bool bUnattended = FApp::IsUnattended();

        // PIE 중에는 실행 금지
        if (GEditor && GEditor->PlayWorld != nullptr)
        {
            UE_LOG(
                LogBPRefreshTool,
                Error,
                TEXT("Cannot refresh Blueprints while PIE is running.")
            );

            return;
        }


        // ------------------------------------------------------------
        // Arguments
        //
        // 기본:
        //   ShootingArena.RefreshBlueprints
        //
        // 특정 폴더:
        //   ShootingArena.RefreshBlueprints /Game/QuakeLike_1_0
        //
        // 강제 재저장:
        //   ShootingArena.RefreshBlueprints /Game/QuakeLike_1_0 resaveall
        // ------------------------------------------------------------

        FString RootPath = TEXT("/Game");
        bool bForceResaveAll = false;

        for (const FString& Arg : Args)
        {
            if (Arg.StartsWith(TEXT("/")))
            {
                RootPath = Arg;
            }
            else if (Arg.Equals(TEXT("resaveall"), ESearchCase::IgnoreCase))
            {
                bForceResaveAll = true;
            }
        }


        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("============================================================")
        );

        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("Blueprint refresh started. RootPath = %s, ForceResave = %s"),
            *RootPath,
            bForceResaveAll ? TEXT("true") : TEXT("false")
        );


        // ------------------------------------------------------------
        // Asset Registry
        // ------------------------------------------------------------

        FAssetRegistryModule& AssetRegistryModule =
            FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
                TEXT("AssetRegistry")
            );

        IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();


        FARFilter Filter;

        Filter.PackagePaths.Add(FName(*RootPath));

        Filter.ClassPaths.Add(
            UBlueprint::StaticClass()->GetClassPathName()
        );

        Filter.bRecursivePaths = true;
        Filter.bRecursiveClasses = true;


        TArray<FAssetData> AssetDataList;

        AssetRegistry.GetAssets(
            Filter,
            AssetDataList
        );


        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("Found %d Blueprint assets."),
            AssetDataList.Num()
        );


        if (AssetDataList.IsEmpty())
        {
            UE_LOG(
                LogBPRefreshTool,
                Warning,
                TEXT("No Blueprints found under %s."),
                *RootPath
            );

            return;
        }


        // ------------------------------------------------------------
        // 확인창
        // ------------------------------------------------------------

        const FString ConfirmationMessage = FString::Printf(
            TEXT(
                "%d Blueprint assets will be refreshed and compiled.\n\n"
                "Root: %s\n"
                "Force Resave: %s\n\n"
                "Continue?"
            ),
            AssetDataList.Num(),
            *RootPath,
            bForceResaveAll ? TEXT("Yes") : TEXT("No")
        );


        if (!bUnattended && FMessageDialog::Open(
                EAppMsgType::YesNo,
                FText::FromString(ConfirmationMessage)
            ) != EAppReturnType::Yes)
        {
            UE_LOG(
                LogBPRefreshTool,
                Warning,
                TEXT("Cancelled by user.")
            );

            return;
        }


        // ------------------------------------------------------------
        // Blueprint Load
        //
        // TStrongObjectPtr를 사용해서 일괄 컴파일 도중 GC가 발생하더라도
        // Blueprint 객체가 살아 있도록 유지합니다.
        // ------------------------------------------------------------

        TArray<TStrongObjectPtr<UBlueprint>> Blueprints;

        Blueprints.Reserve(AssetDataList.Num());


        for (const FAssetData& AssetData : AssetDataList)
        {
            UObject* Asset = AssetData.GetAsset();

            UBlueprint* Blueprint = Cast<UBlueprint>(Asset);

            if (!Blueprint)
            {
                UE_LOG(
                    LogBPRefreshTool,
                    Warning,
                    TEXT("Failed to load Blueprint: %s"),
                    *AssetData.GetObjectPathString()
                );

                continue;
            }

            Blueprints.Emplace(Blueprint);
        }


        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("Loaded %d Blueprint assets."),
            Blueprints.Num()
        );


        // 3단계 작업이므로 대략 * 3
        FScopedSlowTask SlowTask(
            Blueprints.Num() * 3.0f,
            FText::FromString(TEXT("Refreshing Blueprints..."))
        );

        SlowTask.MakeDialog(!bUnattended);


        // ------------------------------------------------------------
        // PASS 1
        //
        // Refresh All Nodes
        // ------------------------------------------------------------

        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("PASS 1: Refresh All Nodes")
        );


        for (const TStrongObjectPtr<UBlueprint>& BlueprintPtr : Blueprints)
        {
            if (SlowTask.ShouldCancel())
            {
                UE_LOG(
                    LogBPRefreshTool,
                    Warning,
                    TEXT("Cancelled. Nothing will be saved.")
                );

                return;
            }


            UBlueprint* Blueprint = BlueprintPtr.Get();

            if (!Blueprint)
            {
                continue;
            }


            SlowTask.EnterProgressFrame(
                1.0f,
                FText::FromString(Blueprint->GetPathName())
            );


            UE_LOG(
                LogBPRefreshTool,
                Verbose,
                TEXT("Refreshing: %s"),
                *Blueprint->GetPathName()
            );


            FBlueprintEditorUtils::RefreshAllNodes(Blueprint);
        }


        // ------------------------------------------------------------
        // PASS 2
        //
        // Compile
        //
        // SkipSave를 사용합니다.
        // 컴파일 중 Save On Compile 설정 때문에 일부 BP만 먼저 저장되는
        // 상황을 방지합니다.
        // ------------------------------------------------------------

        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("PASS 2: Compile Blueprints")
        );


        int32 CompileErrorCount = 0;


        for (const TStrongObjectPtr<UBlueprint>& BlueprintPtr : Blueprints)
        {
            if (SlowTask.ShouldCancel())
            {
                UE_LOG(
                    LogBPRefreshTool,
                    Warning,
                    TEXT("Cancelled. Nothing will be saved.")
                );

                return;
            }


            UBlueprint* Blueprint = BlueprintPtr.Get();

            if (!Blueprint)
            {
                continue;
            }


            SlowTask.EnterProgressFrame(
                1.0f,
                FText::FromString(Blueprint->GetPathName())
            );


            FCompilerResultsLog Results;

            Results.bSilentMode = true;


            FKismetEditorUtilities::CompileBlueprint(
                Blueprint,
                EBlueprintCompileOptions::SkipSave,
                &Results
            );


            if (Results.NumErrors > 0 || Blueprint->Status == BS_Error)
            {
                ++CompileErrorCount;

                UE_LOG(
                    LogBPRefreshTool,
                    Error,
                    TEXT(
                        "Compile FAILED: %s "
                        "(Errors=%d, Warnings=%d)"
                    ),
                    *Blueprint->GetPathName(),
                    Results.NumErrors,
                    Results.NumWarnings
                );
            }
            else
            {
                UE_LOG(
                    LogBPRefreshTool,
                    Verbose,
                    TEXT(
                        "Compile OK: %s "
                        "(Warnings=%d)"
                    ),
                    *Blueprint->GetPathName(),
                    Results.NumWarnings
                );
            }
        }


        // 하나라도 컴파일 실패하면 자동 저장하지 않음
        if (CompileErrorCount > 0)
        {
            UE_LOG(
                LogBPRefreshTool,
                Error,
                TEXT(
                    "%d Blueprint(s) failed to compile. "
                    "Automatic save has been aborted."
                ),
                CompileErrorCount
            );

            if (!bUnattended)
            {
                FMessageDialog::Open(
                    EAppMsgType::Ok,
                    FText::FromString(
                        FString::Printf(
                            TEXT(
                                "%d Blueprint(s) failed to compile.\n\n"
                                "Nothing was automatically saved.\n"
                                "Check Output Log for details."
                            ),
                            CompileErrorCount
                        )
                    )
                );
            }

            return;
        }


        // ------------------------------------------------------------
        // PASS 3
        //
        // User Defined Struct 등의 최신 상태를 저장 전에 보장
        // ------------------------------------------------------------

        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("PASS 3: Prepare Blueprints for save")
        );


        TArray<UPackage*> PackagesToSave;


        for (const TStrongObjectPtr<UBlueprint>& BlueprintPtr : Blueprints)
        {
            if (SlowTask.ShouldCancel())
            {
                UE_LOG(
                    LogBPRefreshTool,
                    Warning,
                    TEXT("Cancelled. Nothing will be saved.")
                );

                return;
            }


            UBlueprint* Blueprint = BlueprintPtr.Get();

            if (!Blueprint)
            {
                continue;
            }


            SlowTask.EnterProgressFrame(
                1.0f,
                FText::FromString(Blueprint->GetPathName())
            );


            // // 중요:
            // // User Defined Struct 최신화 문제와 직접 관계된 UE 함수
            // FBlueprintEditorUtils::RecompileBeforeSaveIfNeeded(
            //     Blueprint
            // );


            // resaveall 옵션이면 변경 여부와 관계없이 패키지 재저장
            if (bForceResaveAll)
            {
                Blueprint->MarkPackageDirty();
            }


            PackagesToSave.AddUnique(
                Blueprint->GetOutermost()
            );
        }


        // RecompileBeforeSaveIfNeeded 이후 상태 재확인
        for (const TStrongObjectPtr<UBlueprint>& BlueprintPtr : Blueprints)
        {
            UBlueprint* Blueprint = BlueprintPtr.Get();

            if (Blueprint && Blueprint->Status == BS_Error)
            {
                UE_LOG(
                    LogBPRefreshTool,
                    Error,
                    TEXT(
                        "Blueprint entered error state before save: %s. "
                        "Save aborted."
                    ),
                    *Blueprint->GetPathName()
                );

                return;
            }
        }


        // ------------------------------------------------------------
        // SAVE
        // ------------------------------------------------------------

        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT(
                "Saving dirty packages... "
                "Packages considered: %d"
            ),
            PackagesToSave.Num()
        );


        const bool bSaveSucceeded =
            UEditorLoadingAndSavingUtils::SavePackages(
                PackagesToSave,
                true       // Dirty package만 저장
            );


        if (!bSaveSucceeded)
        {
            UE_LOG(
                LogBPRefreshTool,
                Error,
                TEXT("One or more packages failed to save.")
            );

            if (!bUnattended)
            {
                FMessageDialog::Open(
                    EAppMsgType::Ok,
                    FText::FromString(
                        TEXT(
                            "One or more Blueprint packages failed to save.\n"
                            "Check Output Log."
                        )
                    )
                );
            }

            return;
        }


        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("Blueprint refresh completed successfully.")
        );

        UE_LOG(
            LogBPRefreshTool,
            Display,
            TEXT("============================================================")
        );


        if (!bUnattended)
        {
            FMessageDialog::Open(
                EAppMsgType::Ok,
                FText::FromString(
                    FString::Printf(
                        TEXT(
                            "Blueprint refresh completed.\n\n"
                            "Processed: %d\n"
                            "Root: %s\n\n"
                            "Check Git status before committing."
                        ),
                        Blueprints.Num(),
                        *RootPath
                    )
                )
            );
        }
    }
};


IMPLEMENT_MODULE(
    FShootingArenaEditorToolsModule,
    ShootingArenaEditorTools
)

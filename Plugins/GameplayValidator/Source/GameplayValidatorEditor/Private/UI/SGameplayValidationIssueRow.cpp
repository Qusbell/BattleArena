#include "UI/SGameplayValidationIssueRow.h"

#include "GameFramework/Actor.h"
#include "Editor.h"
#include "GameplayValidationSubsystem.h"
#include "Integration/GameplayValidatorPathLinkBridge.h"
#include "ScopedTransaction.h"
#include "Utils/GameplayValidationEditorUtils.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"

void SGameplayValidationIssueRow::Construct(
    const FArguments& InArgs,
    const TSharedRef<STableViewBase>& OwnerTableView)
{
    Issue = InArgs._Issue;
    SMultiColumnTableRow<TSharedPtr<FGameplayValidationIssue>>::Construct(
        FSuperRowType::FArguments().Padding(2.0f),
        OwnerTableView);
}

TSharedRef<SWidget> SGameplayValidationIssueRow::GenerateWidgetForColumn(const FName& ColumnName)
{
    if (!Issue.IsValid())
    {
        return SNew(STextBlock).Text(FText::GetEmpty());
    }

    if (ColumnName == TEXT("Rule"))
    {
        return SNew(STextBlock).Text(FText::FromName(Issue->RuleId));
    }

    if (ColumnName == TEXT("Category"))
    {
        const FString Category = Issue->SlotId == TEXT("Default")
            ? Issue->ProviderId.ToString()
            : FString::Printf(TEXT("%s / %s"), *Issue->ProviderId.ToString(), *Issue->SlotId.ToString());
        return SNew(STextBlock).Text(FText::FromString(Category));
    }

    if (ColumnName == TEXT("Actor"))
    {
        return SNew(STextBlock).Text(GetActorText());
    }

    if (ColumnName == TEXT("Message"))
    {
        return SNew(STextBlock)
            .Text(Issue->Message)
            .AutoWrapText(true)
            .ToolTipText(Issue->Suggestion);
    }

    if (ColumnName == TEXT("Focus"))
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GameplayValidator", "FocusButton", "Focus"))
                .IsEnabled_Lambda([this] { return Issue.IsValid() && Issue->TargetActor.IsValid(); })
                .OnClicked(this, &SGameplayValidationIssueRow::HandleFocusClicked)
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            .Padding(4.0f, 0.0f, 0.0f, 0.0f)
            [
                SNew(SButton)
                .Text(NSLOCTEXT("GameplayValidator", "AttachButton", "Attach"))
                .ToolTipText(NSLOCTEXT("GameplayValidator", "AttachButtonTooltip", "이 PathLink의 Attach To Ground를 실행하고 검사 결과를 갱신합니다."))
                .Visibility(Issue->ProviderId == TEXT("PathLink") ? EVisibility::Visible : EVisibility::Collapsed)
                .IsEnabled(this, &SGameplayValidationIssueRow::CanAttach)
                .OnClicked(this, &SGameplayValidationIssueRow::HandleAttachClicked)
            ];
    }

    return SNew(STextBlock).Text(FText::GetEmpty());
}

FReply SGameplayValidationIssueRow::HandleFocusClicked()
{
    FGameplayValidationEditorUtils::FocusActor(Issue.IsValid() ? Issue->TargetActor.Get() : nullptr);
    return FReply::Handled();
}

bool SGameplayValidationIssueRow::CanAttach() const
{
    return GEditor && !GEditor->PlayWorld
        && Issue.IsValid() && Issue->ProviderId == TEXT("PathLink")
        && FGameplayValidatorPathLinkBridge::CanAttachToGround(Issue->TargetActor.Get());
}

FReply SGameplayValidationIssueRow::HandleAttachClicked()
{
    if (!CanAttach())
    {
        return FReply::Handled();
    }

    {
        // AttachToGround 내부의 Modify()를 Undo/Redo 트랜잭션으로 묶습니다.
        const FScopedTransaction Transaction(NSLOCTEXT("GameplayValidator", "AttachPathLinkTransaction", "Attach PathLink To Ground"));
        FGameplayValidatorPathLinkBridge::TryAttachToGround(Issue->TargetActor.Get());
    }

    GEditor->RedrawLevelEditingViewports();
    if (UGameplayValidationSubsystem* Subsystem = GEditor->GetEditorSubsystem<UGameplayValidationSubsystem>())
    {
        Subsystem->ValidateCurrentLevel();
    }
    return FReply::Handled();
}

FText SGameplayValidationIssueRow::GetActorText() const
{
    if (!Issue.IsValid() || !Issue->TargetActor.IsValid())
    {
        return NSLOCTEXT("GameplayValidator", "ActorDeleted", "<Actor Deleted>");
    }

    return FText::FromString(Issue->TargetActor->GetActorLabel());
}

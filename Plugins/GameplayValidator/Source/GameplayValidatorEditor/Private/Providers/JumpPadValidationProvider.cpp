#include "Providers/JumpPadValidationProvider.h"

#include "Utils/GameplayValidationReflectionUtils.h"
#include "Utils/GameplayValidationUtils.h"

const FName FJumpPadValidationProvider::ProviderId(TEXT("JumpPad"));
const FName FJumpPadValidationProvider::ApexTimeSlotId(TEXT("ApexTime"));
const FName FJumpPadValidationProvider::LaunchAngleSlotId(TEXT("LaunchAngle"));

namespace JumpPadBindings
{
    // 기획자가 레벨에 배치한 뒤 직접 지정하는 값만 Contract로 검사합니다.
    const TArray<FName> TargetPointNames = { TEXT("Target Point"), TEXT("TargetPoint") };

}

FText FJumpPadValidationProvider::GetDisplayName() const
{
    return NSLOCTEXT("GameplayValidator", "JumpPadProvider", "JumpPad");
}

void FJumpPadValidationProvider::GetTargetSlots(TArray<FGameplayValidationTargetSlot>& OutSlots) const
{
    FGameplayValidationTargetSlot& ApexTime = OutSlots.AddDefaulted_GetRef();
    ApexTime.SlotId = ApexTimeSlotId;
    ApexTime.DisplayName = NSLOCTEXT("GameplayValidator", "ApexTimeJumpPadClass", "ApexTime JumpPad");

    FGameplayValidationTargetSlot& LaunchAngle = OutSlots.AddDefaulted_GetRef();
    LaunchAngle.SlotId = LaunchAngleSlotId;
    LaunchAngle.DisplayName = NSLOCTEXT("GameplayValidator", "LaunchAngleJumpPadClass", "LaunchAngle JumpPad");
}

bool FJumpPadValidationProvider::ValidateTargetClass(
    const FName SlotId,
    UClass* TargetClass,
    FText& OutReason) const
{
    if (!IGameplayValidationProvider::ValidateTargetClass(SlotId, TargetClass, OutReason))
    {
        return false;
    }

    // 현재는 Target Point 연결만 검사합니다. 이후 패드별 궤적값 규칙은
    // ValidateJumpPadTargetPoint와 분리된 별도 메서드로 안전하게 추가할 수 있습니다.
    if (!FGameplayValidationReflectionUtils::HasProperty(TargetClass, JumpPadBindings::TargetPointNames))
    {
        OutReason = NSLOCTEXT(
            "GameplayValidator",
            "JumpPadClassContractMismatch",
            "JumpPad 슬롯에는 Target Point를 가진 JumpPad Blueprint를 등록해야 합니다.");
        return false;
    }

    OutReason = FText::GetEmpty();
    return true;
}

void FJumpPadValidationProvider::Validate(
    const FGameplayValidationContext& Context,
    TArray<FGameplayValidationIssue>& OutIssues)
{
    ValidateJumpPadTargetPoint(Context, ApexTimeSlotId, OutIssues);
    ValidateJumpPadTargetPoint(Context, LaunchAngleSlotId, OutIssues);
}

void FJumpPadValidationProvider::ValidateJumpPadTargetPoint(
    const FGameplayValidationContext& Context,
    const FName SlotId,
    TArray<FGameplayValidationIssue>& OutIssues) const
{
    const TArray<TWeakObjectPtr<AActor>>& Actors = Context.GetActors(ProviderId, SlotId);

    for (const TWeakObjectPtr<AActor>& WeakActor : Actors)
    {
        AActor* JumpPad = WeakActor.Get();
        if (!IsValid(JumpPad))
        {
            continue;
        }

        AActor* TargetPoint = FGameplayValidationReflectionUtils::GetActorProperty(
            JumpPad,
            JumpPadBindings::TargetPointNames);

        // JUMPPAD_001: 기획자가 반드시 연결해야 하는 Target Point 누락
        if (!IsValid(TargetPoint))
        {
            FGameplayValidationUtils::AddIssue(
                OutIssues,
                TEXT("JUMPPAD_001"),
                ProviderId,
                SlotId,
                EGameplayValidationSeverity::Error,
                JumpPad,
                NSLOCTEXT("GameplayValidator", "JumpPadMissingTarget", "Target Point가 지정되지 않았습니다."),
                NSLOCTEXT("GameplayValidator", "JumpPadMissingTargetFix", "JumpPad가 이동시킬 Target Point Actor를 지정하세요."));
        }
        else
        {
            // JUMPPAD_002: 자기 자신을 Target으로 잘못 지정
            if (TargetPoint == JumpPad)
            {
                FGameplayValidationUtils::AddIssue(
                    OutIssues,
                    TEXT("JUMPPAD_002"),
                    ProviderId,
                    SlotId,
                    EGameplayValidationSeverity::Error,
                    JumpPad,
                    NSLOCTEXT("GameplayValidator", "JumpPadSelfTarget", "JumpPad 자기 자신이 Target Point로 지정되어 있습니다."),
                    NSLOCTEXT("GameplayValidator", "JumpPadSelfTargetFix", "별도의 Target Point Actor를 지정하세요."));
            }
            // JUMPPAD_003: Target은 있지만 이동 의미가 거의 없는 배치
            else if (JumpPad->GetActorLocation().Equals(TargetPoint->GetActorLocation(), 1.0f))
            {
                FGameplayValidationUtils::AddIssue(
                    OutIssues,
                    TEXT("JUMPPAD_003"),
                    ProviderId,
                    SlotId,
                    EGameplayValidationSeverity::Warning,
                    JumpPad,
                    NSLOCTEXT("GameplayValidator", "JumpPadSameLocation", "JumpPad와 Target Point가 사실상 같은 위치에 있습니다."),
                    NSLOCTEXT("GameplayValidator", "JumpPadSameLocationFix", "Target Point의 배치 위치가 의도한 위치인지 확인하세요."),
                    TargetPoint);
            }

        }
    }
}

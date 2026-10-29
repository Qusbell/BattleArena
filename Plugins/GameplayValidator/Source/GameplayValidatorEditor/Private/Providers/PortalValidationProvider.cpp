#include "Providers/PortalValidationProvider.h"

#include "Utils/GameplayValidationReflectionUtils.h"
#include "Utils/GameplayValidationUtils.h"

const FName FPortalValidationProvider::ProviderId(TEXT("Portal"));
const FName FPortalValidationProvider::SeamlessSlotId(TEXT("Seamless"));

namespace PortalBindings
{
    const TArray<FName> ExitTargetNames = { TEXT("exitTarget"), TEXT("Exit Target") };
    const TArray<FName> TeleportEnabledNames = { TEXT("bTeleportEnabled"), TEXT("Teleport Enabled") };
}

FText FPortalValidationProvider::GetDisplayName() const
{
    return NSLOCTEXT("GameplayValidator", "PortalProvider", "Portal");
}

void FPortalValidationProvider::GetTargetSlots(TArray<FGameplayValidationTargetSlot>& OutSlots) const
{
    FGameplayValidationTargetSlot& Seamless = OutSlots.AddDefaulted_GetRef();
    Seamless.SlotId = SeamlessSlotId;
    Seamless.DisplayName = NSLOCTEXT("GameplayValidator", "SeamlessPortalClass", "Seamless Portal");
}

bool FPortalValidationProvider::ValidateTargetClass(
    const FName SlotId,
    UClass* TargetClass,
    FText& OutReason) const
{
    if (!IGameplayValidationProvider::ValidateTargetClass(SlotId, TargetClass, OutReason))
    {
        return false;
    }

    // SeamlessPortalActor가 상속받은 활성화 여부와 출구 연결값만 검사합니다.
    if (!FGameplayValidationReflectionUtils::HasProperty(TargetClass, PortalBindings::ExitTargetNames)
        || !FGameplayValidationReflectionUtils::HasProperty(TargetClass, PortalBindings::TeleportEnabledNames))
    {
        OutReason = NSLOCTEXT(
            "GameplayValidator",
            "PortalClassContractMismatch",
            "Seamless Portal 슬롯에는 Exit Target과 Teleport Enabled를 가진 Actor를 등록해야 합니다.");
        return false;
    }

    OutReason = FText::GetEmpty();
    return true;
}

void FPortalValidationProvider::Validate(
    const FGameplayValidationContext& Context,
    TArray<FGameplayValidationIssue>& OutIssues)
{
    for (const TWeakObjectPtr<AActor>& WeakActor : Context.GetActors(ProviderId, SeamlessSlotId))
    {
        if (AActor* Actor = WeakActor.Get(); IsValid(Actor))
        {
            ValidatePortalActor(SeamlessSlotId, Actor, OutIssues);
        }
    }
}

void FPortalValidationProvider::ValidatePortalActor(
    const FName SlotId,
    AActor* Portal,
    TArray<FGameplayValidationIssue>& OutIssues) const
{
    bool bTeleportEnabled = false;
    FGameplayValidationReflectionUtils::GetBoolProperty(Portal, PortalBindings::TeleportEnabledNames, bTeleportEnabled);

    // 비활성 포탈은 실제 이동을 수행하지 않으므로 Exit Target을 요구하지 않습니다.
    if (!bTeleportEnabled)
    {
        return;
    }

    AActor* ExitTarget = FGameplayValidationReflectionUtils::GetActorProperty(Portal, PortalBindings::ExitTargetNames);

    // PORTAL_001: 활성 포탈의 출구 대상 누락
    if (!IsValid(ExitTarget))
    {
        FGameplayValidationUtils::AddIssue(
            OutIssues,
            TEXT("PORTAL_001"),
            ProviderId,
            SlotId,
            EGameplayValidationSeverity::Error,
            Portal,
            NSLOCTEXT("GameplayValidator", "PortalMissingTarget", "Teleport Enabled가 켜져 있지만 Exit Target이 지정되지 않았습니다."),
            NSLOCTEXT("GameplayValidator", "PortalMissingTargetFix", "이 Portal이 연결될 출구 Actor를 지정하세요."));
        return;
    }

}

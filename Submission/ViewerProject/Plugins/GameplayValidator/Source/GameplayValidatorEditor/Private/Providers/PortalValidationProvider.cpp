#include "Providers/PortalValidationProvider.h"

#include "Utils/GameplayValidationReflectionUtils.h"
#include "Utils/GameplayValidationUtils.h"

const FName FPortalValidationProvider::ProviderId(TEXT("Portal"));
const FName FPortalValidationProvider::OneWaySlotId(TEXT("OneWay"));

namespace PortalBindings
{
    const TArray<FName> ExitTargetNames = { TEXT("exitTarget"), TEXT("Exit Target") };
    const TArray<FName> TeleportEnabledNames = { TEXT("bTeleportEnabled"), TEXT("Teleport Enabled") };
    const TArray<FName> TeleportDataAssetNames = { TEXT("teleportDA"), TEXT("Teleport DA") };
    const TArray<FName> LaunchAngleNames = { TEXT("launchAngle"), TEXT("Launch Angle") };
    const TArray<FName> LaunchPowerNames = { TEXT("launchPower"), TEXT("Launch Power") };
}

FText FPortalValidationProvider::GetDisplayName() const
{
    return NSLOCTEXT("GameplayValidator", "PortalProvider", "Portal");
}

void FPortalValidationProvider::GetTargetSlots(TArray<FGameplayValidationTargetSlot>& OutSlots) const
{
    FGameplayValidationTargetSlot& OneWay = OutSlots.AddDefaulted_GetRef();
    OneWay.SlotId = OneWaySlotId;
    OneWay.DisplayName = NSLOCTEXT("GameplayValidator", "OneWayPortalClass", "OneWay Teleport Portal");
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

    // 코드 포탈의 런타임 계약: 활성화 여부, 출구 Actor, 공용 Teleport DataAsset.
    if (!FGameplayValidationReflectionUtils::HasProperty(TargetClass, PortalBindings::ExitTargetNames)
        || !FGameplayValidationReflectionUtils::HasProperty(TargetClass, PortalBindings::TeleportEnabledNames)
        || !FGameplayValidationReflectionUtils::HasProperty(TargetClass, PortalBindings::TeleportDataAssetNames))
    {
        OutReason = NSLOCTEXT(
            "GameplayValidator",
            "PortalClassContractMismatch",
            "Portal 슬롯에는 exitTarget, Teleport Enabled, Teleport DA를 가진 코드 포탈 Actor를 등록해야 합니다.");
        return false;
    }

    OutReason = FText::GetEmpty();
    return true;
}

void FPortalValidationProvider::Validate(
    const FGameplayValidationContext& Context,
    TArray<FGameplayValidationIssue>& OutIssues)
{
    TSet<const UObject*> ValidatedDataAssets;
    for (const TWeakObjectPtr<AActor>& WeakActor : Context.GetActors(ProviderId, OneWaySlotId))
    {
        if (AActor* Actor = WeakActor.Get(); IsValid(Actor))
        {
            ValidatePortalActor(OneWaySlotId, Actor, ValidatedDataAssets, OutIssues);
        }
    }
}

void FPortalValidationProvider::ValidatePortalActor(
    const FName SlotId,
    AActor* Portal,
    TSet<const UObject*>& ValidatedDataAssets,
    TArray<FGameplayValidationIssue>& OutIssues) const
{
    bool bTeleportEnabled = false;
    FGameplayValidationReflectionUtils::GetBoolProperty(Portal, PortalBindings::TeleportEnabledNames, bTeleportEnabled);

    UObject* TeleportDataAsset = FGameplayValidationReflectionUtils::GetObjectProperty(
        Portal,
        PortalBindings::TeleportDataAssetNames);

    if (!IsValid(TeleportDataAsset))
    {
        FGameplayValidationUtils::AddIssue(
            OutIssues,
            TEXT("PORTAL_003"),
            ProviderId,
            SlotId,
            EGameplayValidationSeverity::Error,
            Portal,
            NSLOCTEXT("GameplayValidator", "PortalMissingDataAsset", "Teleport DA가 지정되지 않았습니다."),
            NSLOCTEXT("GameplayValidator", "PortalMissingDataAssetFix", "DA_PortalInfo_Default를 Teleport DA에 지정하세요."));
    }
    else if (!ValidatedDataAssets.Contains(TeleportDataAsset))
    {
        ValidatedDataAssets.Add(TeleportDataAsset);
        ValidateTeleportDataAsset(SlotId, Portal, TeleportDataAsset, OutIssues);
    }

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

    // PORTAL_002: 자기 자신을 출구 대상으로 잘못 연결한 경우
    if (ExitTarget == Portal)
    {
        FGameplayValidationUtils::AddIssue(
            OutIssues,
            TEXT("PORTAL_002"),
            ProviderId,
            SlotId,
            EGameplayValidationSeverity::Error,
            Portal,
            NSLOCTEXT("GameplayValidator", "PortalSelfTarget", "Portal 자기 자신이 출구 대상으로 지정되어 있습니다."),
            NSLOCTEXT("GameplayValidator", "PortalSelfTargetFix", "다른 출구 Actor를 지정하세요."),
            ExitTarget);
    }
}

void FPortalValidationProvider::ValidateTeleportDataAsset(
    const FName SlotId,
    AActor* Portal,
    UObject* DataAsset,
    TArray<FGameplayValidationIssue>& OutIssues) const
{
    double LaunchAngle = 0.0;
    if (FGameplayValidationReflectionUtils::GetNumericProperty(DataAsset, PortalBindings::LaunchAngleNames, LaunchAngle)
        && LaunchAngle <= 0.0)
    {
        FGameplayValidationUtils::AddIssue(
            OutIssues,
            TEXT("PORTAL_004"),
            ProviderId,
            SlotId,
            EGameplayValidationSeverity::Error,
            Portal,
            NSLOCTEXT("GameplayValidator", "PortalInvalidLaunchAngle", "Teleport DA의 Launch Angle이 0 이하입니다."),
            NSLOCTEXT("GameplayValidator", "PortalInvalidLaunchAngleFix", "DA_PortalInfo_Default의 Launch Angle을 0보다 크게 설정하세요."));
    }

    double LaunchPower = 0.0;
    if (FGameplayValidationReflectionUtils::GetNumericProperty(DataAsset, PortalBindings::LaunchPowerNames, LaunchPower)
        && LaunchPower <= 0.0)
    {
        FGameplayValidationUtils::AddIssue(
            OutIssues,
            TEXT("PORTAL_005"),
            ProviderId,
            SlotId,
            EGameplayValidationSeverity::Error,
            Portal,
            NSLOCTEXT("GameplayValidator", "PortalInvalidLaunchPower", "Teleport DA의 Launch Power가 0 이하입니다."),
            NSLOCTEXT("GameplayValidator", "PortalInvalidLaunchPowerFix", "DA_PortalInfo_Default의 Launch Power를 0보다 크게 설정하세요."));
    }
}

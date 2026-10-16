#pragma once

#include "CoreMinimal.h"
#include "Core/GameplayValidationProvider.h"

class FPortalValidationProvider final : public IGameplayValidationProvider
{
public:
    static const FName ProviderId;
    static const FName OneWaySlotId;

    virtual FName GetProviderId() const override { return ProviderId; }
    virtual FText GetDisplayName() const override;
    virtual void GetTargetSlots(TArray<FGameplayValidationTargetSlot>& OutSlots) const override;
    virtual bool ValidateTargetClass(FName SlotId, UClass* TargetClass, FText& OutReason) const override;
    virtual void Validate(const FGameplayValidationContext& Context, TArray<FGameplayValidationIssue>& OutIssues) override;

private:
    void ValidatePortalActor(
        FName SlotId,
        AActor* Portal,
        TSet<const UObject*>& ValidatedDataAssets,
        TArray<FGameplayValidationIssue>& OutIssues) const;
    void ValidateTeleportDataAsset(FName SlotId, AActor* Portal, UObject* DataAsset, TArray<FGameplayValidationIssue>& OutIssues) const;
};

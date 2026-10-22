#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PathLinkCacheEditorSubsystem.generated.h"

/** No placed actor or manual bake step is required. */
UCLASS()
class UPathLinkCacheEditorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    void PrepareForPlay(bool bSimulating);
    void PrepareForSave(UWorld* World, class FObjectPreSaveContext Context);
    bool bPreparing = false;
};

#include "PathLinkCacheEditorSubsystem.h"
#include "AI/PathLink/PathLinkSubsystem.h"
#include "Editor.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "UObject/ObjectSaveContext.h"

namespace
{
    void BuildRoutingCache(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
    {
        if (!Args.IsEmpty()) { Output.Logf(TEXT("Usage: PathLink.BuildRoutingCache")); return; }
        if (!GEditor || GEditor->PlayWorld || IsRunningCommandlet())
        { Output.Logf(TEXT("Stop PIE/Simulate before building the editor map's routing cache.")); return; }
        UWorld* World = GEditor->GetEditorWorldContext().World();
        auto* Subsystem = World ? World->GetSubsystem<UPathLinkSubsystem>() : nullptr;
        if (!Subsystem || World->WorldType != EWorldType::Editor)
        { Output.Logf(TEXT("No editor map is available.")); return; }
        const bool bBuilt = Subsystem->PrepareRoutingCacheForEditor(true);
        Output.Logf(TEXT("[PathLink] %s: %s%s"), bBuilt ? TEXT("Build complete") : TEXT("Build unavailable"),
            *Subsystem->GetRoutingCacheStatus(), bBuilt ? TEXT(" Save the map to persist the cache.") : TEXT(""));
    }

    FAutoConsoleCommandWithWorldArgsAndOutputDevice BuildRoutingCacheCommand(
        TEXT("PathLink.BuildRoutingCache"),
        TEXT("Force rebuild the current editor map's PathLink routing cache. Stop PIE first, then save the map. Does not rebuild NavMesh."),
        FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&BuildRoutingCache));
}

void UPathLinkCacheEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FEditorDelegates::PreBeginPIE.AddUObject(this, &ThisClass::PrepareForPlay);
    FEditorDelegates::PreSaveWorldWithContext.AddUObject(this, &ThisClass::PrepareForSave);
}

void UPathLinkCacheEditorSubsystem::Deinitialize()
{
    FEditorDelegates::PreBeginPIE.RemoveAll(this);
    FEditorDelegates::PreSaveWorldWithContext.RemoveAll(this);
    Super::Deinitialize();
}

void UPathLinkCacheEditorSubsystem::PrepareForPlay(bool bSimulating)
{
    if (bPreparing || !GEditor || IsRunningCommandlet()) return;
    TGuardValue<bool> Guard(bPreparing, true);
    if (UWorld* World = GEditor->GetEditorWorldContext().World())
        if (auto* Subsystem = World->GetSubsystem<UPathLinkSubsystem>()) Subsystem->PrepareRoutingCacheForEditor();
}

void UPathLinkCacheEditorSubsystem::PrepareForSave(UWorld* World, FObjectPreSaveContext Context)
{
    if (bPreparing || IsRunningCommandlet() || Context.IsCooking() || !World || World->WorldType != EWorldType::Editor) return;
    TGuardValue<bool> Guard(bPreparing, true);
    if (auto* Subsystem = World->GetSubsystem<UPathLinkSubsystem>()) Subsystem->PrepareRoutingCacheForEditor();
}

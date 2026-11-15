#include "PreparePathLinkCachesCommandlet.h"

#include "AI/PathLink/PathLinkSubsystem.h"
#include "AI/NavigationSystemBase.h"
#include "EditorWorldUtils.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Weapon/CascadeEffectPoolSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogPreparePathLinkCaches, Log, All);

int32 UPreparePathLinkCachesCommandlet::Main(const FString& Params)
{
    const auto& MatchMaps = GetDefault<UCascadeEffectPoolSettings>()->MatchMaps;
    if (MatchMaps.IsEmpty())
    {
        UE_LOG(LogPreparePathLinkCaches, Error, TEXT("MatchMaps is empty."));
        return 1;
    }

    const auto WorldInit = UWorld::InitializationValues()
        .RequiresHitProxies(false)
        .ShouldSimulatePhysics(false)
        .CreateNavigation(true)
        .CreateAISystem(false)
        .AllowAudioPlayback(false);

    TSet<FString> PreparedMaps;
    for (const TSoftObjectPtr<UWorld>& Map : MatchMaps)
    {
        const FString PackageName = Map.ToSoftObjectPath().GetLongPackageName();
        if (PreparedMaps.Contains(PackageName))
        {
            continue;
        }

        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetMapPackageExtension())
            || !IFileManager::Get().FileExists(*Filename))
        {
            UE_LOG(LogPreparePathLinkCaches, Error, TEXT("Match map file not found: %s"), *Map.ToString());
            return 1;
        }

        UE_LOG(LogPreparePathLinkCaches, Display, TEXT("Preparing routing cache: %s"), *PackageName);
        FScopedEditorWorld EditorWorld(Map, WorldInit);
        UWorld* World = EditorWorld.GetWorld();
        if (!World)
        {
            UE_LOG(LogPreparePathLinkCaches, Error, TEXT("Could not initialize %s."), *PackageName);
            return 1;
        }
        FNavigationSystem::AddNavigationSystemToWorld(*World, FNavigationSystemRunMode::EditorMode);
        auto* PathLink = World->GetSubsystem<UPathLinkSubsystem>();
        if (!PathLink || !PathLink->PrepareRoutingCacheForEditor())
        {
            const FString Reason = PathLink ? PathLink->GetRoutingCacheStatus() : TEXT("PathLink subsystem unavailable.");
            UE_LOG(LogPreparePathLinkCaches, Error, TEXT("Could not prepare %s: %s"), *PackageName, *Reason);
            return 1;
        }

        UPackage* Package = World->GetOutermost();
        if (Package->IsDirty())
        {
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Standalone;
            if (!UPackage::SavePackage(Package, World, *Filename, SaveArgs))
            {
                UE_LOG(LogPreparePathLinkCaches, Error, TEXT("Could not save the routing cache to %s"), *Filename);
                return 1;
            }
        }

        PreparedMaps.Add(PackageName);
        UE_LOG(LogPreparePathLinkCaches, Display, TEXT("Routing cache ready: %s"), *PackageName);
    }

    return 0;
}

using UnrealBuildTool;

public class ShootingArenaEditorTools : ModuleRules
{
    public ShootingArenaEditorTools( ReadOnlyTargetRules Target )
        : base( Target )
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Migration helpers use file-local names; unity amalgamation makes them collide.
        bUseUnity = false;
        // Graph node authoring classes live in uncooked packages; their runtime structs are in ShootingArena.
        OverridePackageType = PackageOverrideType.GameUncookedOnly;

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "EditorSubsystem",
                "AssetRegistry",
                "AssetTools",
                "KismetCompiler",
                "Slate",
                "SlateCore",
                "LevelEditor",
                "ContentBrowser",
                "ToolMenus",
                "BlueprintGraph",
                "UMG",
                "UMGEditor",
                "ShootingArena",
                "AnimGraph",
                "Json"
            }
        );
    }
}

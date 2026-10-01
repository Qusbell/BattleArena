using UnrealBuildTool;

public class ShootingArenaEditorTools : ModuleRules
{
    public ShootingArenaEditorTools( ReadOnlyTargetRules Target )
        : base( Target )
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Graph node authoring classes live in uncooked packages; their runtime structs are in ShootingArena.
        OverridePackageType = PackageOverrideType.GameUncookedOnly;

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "AssetRegistry",
                "Slate",
                "SlateCore",
                "LevelEditor",
                "ContentBrowser",
                "ToolMenus",
                "BlueprintGraph",
                "ShootingArena",
                "AnimGraph",
                "Json"
            }
        );
    }
}

using UnrealBuildTool;

public class PluginOptimizer : ModuleRules
{
    public PluginOptimizer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        /* Este plugin só existe no Editor. */
        if (!Target.bBuildEditor)
        {
            PrecompileForTargets = PrecompileTargetsType.None;
            return;
        }

        PrivateDependencyModuleNames.AddRange(new[]
        {
            /* Núcleo */
            "Core",
            "CoreUObject",
            "Engine",

            /* UI / Editor */
            "Slate",
            "SlateCore",
            "InputCore",     // <-- ADICIONADO: define EKeys
            "UnrealEd",
            "LevelEditor",
            "ToolMenus",

            /* Infraestrutura */
            "Projects",        // IProjectManager
            "AssetRegistry",
            "ApplicationCore",
            "AppFramework"
        });

        PrecompileForTargets = PrecompileTargetsType.Editor;
    }
}

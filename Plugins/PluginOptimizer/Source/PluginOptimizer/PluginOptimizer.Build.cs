using UnrealBuildTool;

public class PluginOptimizer : ModuleRules
{
    public PluginOptimizer(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Só compila em targets com Editor
        if (!Target.bBuildEditor)
        {
            return;                     // nada a fazer em Game/Shipping
        }

        // 1. Dependências QUE PRECISAM ficar públicas
        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UnrealEd",
            "LevelEditor",
            "ToolMenus",
            "Projects",
            "AssetRegistry"
        });

        // 2. Se usar ApplicationCore/AppFramework coloque aqui também
        PublicDependencyModuleNames.AddRange(new[]
        {
            "ApplicationCore",
            "AppFramework"
        });

        // 3. Opcional – gera só no Editor (evita tentar pré-compilar)
        //    Remova se quiser o benefício dos binários pré-compilados.
        PrecompileForTargets = PrecompileTargetsType.None;
    }
}

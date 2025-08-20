#include "PluginOptimizerCommands.h"

#define LOCTEXT_NAMESPACE "FPluginOptimizerCommands"

void FPluginOptimizerCommands::RegisterCommands()
{
    UI_COMMAND(DetectUnusedPlugins,
        "Detect Unnecessary Plugins",
        "Scan project and list potentially unused plugins.",
        EUserInterfaceActionType::Button,
        FInputChord());
}

#undef LOCTEXT_NAMESPACE

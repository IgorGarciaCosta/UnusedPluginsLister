#pragma once

#include "Framework/Commands/Commands.h"
#include "Styling/AppStyle.h"

class FPluginOptimizerCommands : public TCommands<FPluginOptimizerCommands>
{
public:
    FPluginOptimizerCommands()
        : TCommands<FPluginOptimizerCommands>(
            TEXT("PluginOptimizer"),
            NSLOCTEXT("Contexts", "PluginOptimizer", "Plugin Optimizer"),
            NAME_None,
            FAppStyle::GetAppStyleSetName())
    {}

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> DetectUnusedPlugins;
};

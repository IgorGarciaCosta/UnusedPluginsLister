#pragma once

#include "Modules/ModuleManager.h"

class FUICommandList;
class UToolMenu;

class FPluginOptimizerModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void AddMenuEntry(UToolMenu* Menu);
    void AddToolbarEntry(UToolMenu* Toolbar);
    void OnDetectUnusedPlugins();
    void ShowResultsPopup(const TArray<FString>& Candidates,
        int32 EnabledCount,
        int32 UsedCount);

private:
    TSharedPtr<FUICommandList> PluginCommands;
};

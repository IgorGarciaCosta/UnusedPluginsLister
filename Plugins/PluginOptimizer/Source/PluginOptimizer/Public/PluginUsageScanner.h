#pragma once

#include "CoreMinimal.h"

struct FPluginScanResult
{
    TArray<FString> EnabledPlugins;
    TArray<FString> UsedPlugins;
};

struct FPluginUsageScanner
{
    // Performs the scan and fills OutResult (runs entirely in editor thread)
    static void Scan(FPluginScanResult& OutResult);
};

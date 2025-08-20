#include "PluginUsageScanner.h"

#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "Modules/ModuleManager.h"

namespace
{
	// Converte “/Script/Module.Class”  ->  Module  ->  Plugin
	void ConsiderClassPath(const FString& ClassPath,
		const TMap<FString, FString>& ModuleToPlugin,
		TSet<FString>& OutUsed)
	{
		FString Left, Right;
		if (!ClassPath.Split(TEXT("."), &Left, &Right)) return;

		const FString Prefix(TEXT("/Script/"));
		if (!Left.StartsWith(Prefix)) return;

		const FString ModuleName = Left.Mid(Prefix.Len());
		if (const FString* Plug = ModuleToPlugin.Find(ModuleName))
			OutUsed.Add(*Plug);
	}
}

void FPluginUsageScanner::Scan(FPluginScanResult& Out)
{
	// ------------------ AssetRegistry ------------------
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AR = ARM.Get();
	AR.WaitForCompletion();

	// ------------------ plugins habilitados ------------
	TArray<TSharedRef<IPlugin>> Plugins = IPluginManager::Get().GetEnabledPlugins();
	for (const TSharedRef<IPlugin>& P : Plugins) Out.EnabledPlugins.Add(P->GetName());

	// módulo -> plugin
	TMap<FString, FString> ModuleToPlugin;
	for (const TSharedRef<IPlugin>& P : Plugins)
	{
		for (const FModuleDescriptor& M : P->GetDescriptor().Modules)
			ModuleToPlugin.Add(M.Name.ToString(), P->GetName());
	}

	TSet<FString> Used;

	// ------------------ 1) Classes usadas em assets ----
	TArray<FAssetData> GameAssets;
	AR.GetAssetsByPath("/Game", GameAssets, true);

	for (const FAssetData& AD : GameAssets)
	{
		// classe do asset (diferença 5.0 vs 5.1+)
		FString ClassPath;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 1
		ClassPath = AD.AssetClass.ToString();
#else
		ClassPath = AD.AssetClassPath.ToString();
#endif
		ConsiderClassPath(ClassPath, ModuleToPlugin, Used);

		auto CheckTag = [&](FName Tag)
			{
				FString Val;
				if (AD.GetTagValue(Tag, Val))
					ConsiderClassPath(Val, ModuleToPlugin, Used);
			};

		CheckTag("NativeParentClass");
		CheckTag("ParentClass");
		CheckTag("GeneratedClass");

		FString Interfaces;
		if (AD.GetTagValue("ImplementedInterfaces", Interfaces))
		{
			const FString Marker(TEXT("/Script/"));
			int32 Pos = 0;
			while ((Pos = Interfaces.Find(Marker, ESearchCase::IgnoreCase, ESearchDir::FromStart, Pos)) != INDEX_NONE)
			{
				int32 End = Interfaces.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromStart, Pos);
				FString Slice = (End != INDEX_NONE) ? Interfaces.Mid(Pos, End - Pos) : Interfaces.Mid(Pos);
				ConsiderClassPath(Slice, ModuleToPlugin, Used);
				Pos = (End == INDEX_NONE) ? Pos + Marker.Len() : End;
			}
		}
	}

	// ------------------ 2) Assets de plugin referenciados por /Game ----
	for (const TSharedRef<IPlugin>& P : Plugins)
	{
		// 5.0-5.5: FName   |   5.6: FString
		auto MountPath = P->GetMountedAssetPath();
		FString MountStr = FString(MountPath);		// ctor aceita FName ou FString
		if (MountStr.IsEmpty()) continue;

		TArray<FAssetData> PlgAssets;
		AR.GetAssetsByPath(FName(*MountStr), PlgAssets, true);

		for (const FAssetData& PAD : PlgAssets)
		{
			TArray<FName> RefPkgs;
			AR.GetReferencers(
				PAD.PackageName,
				RefPkgs,
				UE::AssetRegistry::EDependencyCategory::Package,
				UE::AssetRegistry::EDependencyQuery::Hard | UE::AssetRegistry::EDependencyQuery::Soft);

			for (const FName& Ref : RefPkgs)
			{
				if (Ref.ToString().StartsWith("/Game"))
				{
					Used.Add(P->GetName());
					break;
				}
			}
			if (Used.Contains(P->GetName())) break;
		}
	}

	// ------------------ 3) módulos carregados no editor ---------------
	for (const TPair<FString, FString>& Pair : ModuleToPlugin)
	{
		if (FModuleManager::Get().IsModuleLoaded(*Pair.Key))
			Used.Add(Pair.Value);
	}

	Used.Sort([](const FString& A, const FString& B) { return A < B; });
	Out.UsedPlugins = Used.Array();
}

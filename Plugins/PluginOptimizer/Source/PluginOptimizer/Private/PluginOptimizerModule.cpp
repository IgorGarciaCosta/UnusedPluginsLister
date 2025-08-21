#include "PluginOptimizerModule.h"
#include "PluginOptimizerCommands.h"
#include "PluginUsageScanner.h"
#include "SPluginOptimizerDialog.h"

#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Framework/Commands/UICommandList.h"
#include "Styling/AppStyle.h"

#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FPluginOptimizerModule"

void FPluginOptimizerModule::StartupModule()
{
	FPluginOptimizerCommands::Register();

	PluginCommands = MakeShared<FUICommandList>();
	PluginCommands->MapAction(
		FPluginOptimizerCommands::Get().DetectUnusedPlugins,
		FExecuteAction::CreateRaw(this, &FPluginOptimizerModule::OnDetectUnusedPlugins));

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FPluginOptimizerModule::RegisterMenus));
}

void FPluginOptimizerModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FPluginOptimizerCommands::Unregister();
	PluginCommands.Reset();
}

void FPluginOptimizerModule::RegisterMenus()
{
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
		AddMenuEntry(Menu);

	if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
		AddToolbarEntry(Toolbar);
}

void FPluginOptimizerModule::AddMenuEntry(UToolMenu* Menu)
{
	FToolMenuSection& Section =
		Menu->AddSection("PluginOptimizer", LOCTEXT("PluginOptimizerSection", "Plugin Optimizer"));
	Section.AddMenuEntryWithCommandList(
		FPluginOptimizerCommands::Get().DetectUnusedPlugins, PluginCommands);
}

void FPluginOptimizerModule::AddToolbarEntry(UToolMenu* Toolbar)
{
	FToolMenuSection& Section = Toolbar->FindOrAddSection("Settings");

	FToolMenuEntry Entry =
		FToolMenuEntry::InitToolBarButton(FPluginOptimizerCommands::Get().DetectUnusedPlugins);
	Entry.SetCommandList(PluginCommands);

	Section.AddEntry(Entry);
}

/* ---------------- comando principal ---------------- */
void FPluginOptimizerModule::OnDetectUnusedPlugins()
{
	FPluginScanResult Result;
	FPluginUsageScanner::Scan(Result);

	TSet<FString> Enabled(Result.EnabledPlugins);
	TSet<FString> Used(Result.UsedPlugins);

	TArray<FString> Candidates;
	for (const FString& Name : Enabled)
		if (!Used.Contains(Name))
			Candidates.Add(Name);

	Candidates.Sort();

	/* cria janela */
	TSharedRef<SWindow> Win = SNew(SWindow)
		.Title(LOCTEXT("PluginOptimizerTitle", "Plugin Optimizer"))
		.ClientSize(FVector2D(700, 500));

	Win->SetContent(
		SNew(SPluginOptimizerDialog)
		.Candidates(Candidates)
		.EnabledCount(Enabled.Num())
		.UsedCount(Used.Num())
	);

	FSlateApplication::Get().AddWindow(Win);
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FPluginOptimizerModule, PluginOptimizer)

#include "PluginOptimizerModule.h"
#include "PluginOptimizerCommands.h"
#include "PluginUsageScanner.h"

#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Framework/Commands/UICommandList.h"
#include "Styling/AppStyle.h"

#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FPluginOptimizerModule"

void FPluginOptimizerModule::StartupModule()
{
	// ­­­­­­­­­ registrar comandos
	FPluginOptimizerCommands::Register();

	PluginCommands = MakeShared<FUICommandList>();
	PluginCommands->MapAction(
		FPluginOptimizerCommands::Get().DetectUnusedPlugins,
		FExecuteAction::CreateRaw(this, &FPluginOptimizerModule::OnDetectUnusedPlugins));

	// sistema de menus está ativo em todas as versões 5.x
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
	{
		AddMenuEntry(Menu);
	}
	if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
	{
		AddToolbarEntry(Toolbar);
	}
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

void FPluginOptimizerModule::OnDetectUnusedPlugins()
{
	FPluginScanResult Result;
	FPluginUsageScanner::Scan(Result);

	TSet<FString> Enabled(Result.EnabledPlugins);
	TSet<FString> Used(Result.UsedPlugins);

	TArray<FString> Candidates;
	for (const FString& Name : Enabled)
	{
		if (!Used.Contains(Name))
			Candidates.Add(Name);
	}
	Candidates.Sort();

	ShowResultsPopup(Candidates, Enabled.Num(), Used.Num());
}

void FPluginOptimizerModule::ShowResultsPopup(const TArray<FString>& Candidates,
	int32 EnabledCount,
	int32 UsedCount)
{
	FString Header = FString::Printf(TEXT("Enabled: %d | Used: %d | Potentially Unused: %d\n\n"),
		EnabledCount, UsedCount, Candidates.Num());

	FString Body;
	if (Candidates.IsEmpty())
	{
		Body = TEXT("No potentially unused plugins were found.\n");
	}
	else
	{
		Body = TEXT("You may be able to disable the following plugins:\n\n");
		for (const FString& Name : Candidates) Body += TEXT(" - ") + Name + TEXT("\n");
	}
	Body += TEXT("\nAlways run a Cook/Package after disabling to validate.\n");

	TSharedRef<SWindow> Win = SNew(SWindow)
		.Title(LOCTEXT("PluginOptimizerTitle", "Plugin Optimizer"))
		.ClientSize(FVector2D(650, 450));

	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8)
		[SNew(STextBlock).Text(FText::FromString(Header)).AutoWrapText(true)]
		+ SVerticalBox::Slot().FillHeight(1).Padding(8)
		[SNew(SBorder)
		[SNew(SScrollBox)
		+ SScrollBox::Slot()
		[SNew(STextBlock).Text(FText::FromString(Body)).AutoWrapText(true)]]]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(8)
		[SNew(SButton)
		.Text(LOCTEXT("Close", "Close"))
		.OnClicked_Lambda([Win]() { Win->RequestDestroyWindow(); return FReply::Handled(); })];

	Win->SetContent(VBox);
	FSlateApplication::Get().AddWindow(Win);
}

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FPluginOptimizerModule, PluginOptimizer)

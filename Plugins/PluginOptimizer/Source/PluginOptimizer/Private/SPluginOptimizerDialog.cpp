#include "SPluginOptimizerDialog.h"

#include "Interfaces/IProjectManager.h"
#include "Misc/ConfigCacheIni.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SPluginOptimizerDialog"

/* ------------------------------------------------------------------ */
/*  Preferência “não mostrar de novo”                                 */
/* ------------------------------------------------------------------ */
static const TCHAR* CFG_SECTION = TEXT("PluginOptimizer");
static const TCHAR* CFG_KEY = TEXT("SuppressRestartPopup");
static bool bSuppressRestartPopup = false;

/* ------------------------------------------------------------------ */
/*  CONSTRUTOR                                                         */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::Construct(const FArguments& InArgs)
{
	/* carrega preferência do .ini */
	{
		bool bTmp = false;
		GConfig->GetBool(CFG_SECTION, CFG_KEY, bTmp, GEditorPerProjectIni);
		bSuppressRestartPopup = bTmp;
	}

	EnabledCnt = InArgs._EnabledCount;
	UsedCnt = InArgs._UsedCount;

	for (const FString& Name : InArgs._Candidates)
		Items.Add(MakeShared<FString>(Name));

	ChildSlot
		[
			SNew(SVerticalBox)

				/* ---------------- cabeçalho ---------------- */
				+ SVerticalBox::Slot().AutoHeight().Padding(6)
				[
					SNew(SHorizontalBox)

						/* texto contador */
						+ SHorizontalBox::Slot().FillWidth(1)
						[
							SAssignNew(HeaderText, STextBlock).AutoWrapText(true)
						]

						/* botão Select (toggle) */
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SNew(SButton)
								.Text(LOCTEXT("Select", "Select"))
								.OnClicked(this, &SPluginOptimizerDialog::OnSelectClicked)
						]

						/* botão Disable (visível só em select-mode) */
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SAssignNew(DisableSelectedBtn, SButton)
								.Text(LOCTEXT("DisableTop", "Disable"))
								.Visibility(EVisibility::Collapsed)
								.OnClicked(this, &SPluginOptimizerDialog::OnDisableSelectedClicked)
						]

						/* botão Select All / Deselect All */
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SAssignNew(SelectAllBtn, SButton)
								.Text_Lambda([this]()
									{
										return (Selected.Num() == Items.Num() && Items.Num() > 0)
											? LOCTEXT("DeselectAll", "Deselect All")
											: LOCTEXT("SelectAll", "Select All");
									})
								.Visibility(EVisibility::Collapsed)
								.OnClicked(this, &SPluginOptimizerDialog::OnSelectAllClicked)
						]
				]

				/* ---------------- lista -------------------- */
				+ SVerticalBox::Slot().FillHeight(1).Padding(6)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FString>>)
						.ListItemsSource(&Items)
						.OnGenerateRow(this, &SPluginOptimizerDialog::OnGenerateRow)
						.SelectionMode(ESelectionMode::None)
				]
		];

	RefreshHeader();
}

/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::RefreshHeader()
{
	const int32 Potential = Items.Num();
	FString Txt = FString::Printf(TEXT("Enabled: %d | Used: %d | Potentially Unused: %d"),
		EnabledCnt, UsedCnt, Potential);
	HeaderText->SetText(FText::FromString(Txt));
}

/* ------------------------------------------------------------------ */
/*  LINHA DA LISTA – checkbox na direita                              */
/* ------------------------------------------------------------------ */
TSharedRef<ITableRow> SPluginOptimizerDialog::OnGenerateRow(
	TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FString>>, Owner)
		[
			SNew(SHorizontalBox)

				/* nome do plugin */
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(STextBlock).Text(FText::FromString(*Item))
				]

				/* botão Disable individual */
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(SButton)
						.Text(LOCTEXT("DisableRow", "Disable"))
						.Visibility_Lambda([this]() { return bSelectMode ? EVisibility::Collapsed : EVisibility::Visible; })
						.OnClicked_Lambda([this, Item]()
							{
								DisableOne(*Item);
								return FReply::Handled();
							})
				]

				/* checkbox (aparece no lugar do botão) */
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SCheckBox)
						.Visibility_Lambda([this]() { return bSelectMode ? EVisibility::Visible : EVisibility::Collapsed; })
						.IsChecked(this, &SPluginOptimizerDialog::IsItemChecked, Item)
						.OnCheckStateChanged(this, &SPluginOptimizerDialog::OnCheckboxChanged, Item)
				]
		];
}

/* ------------------------------------------------------------------ */
/*  BOTÕES DO TOPO                                                     */
/* ------------------------------------------------------------------ */
FReply SPluginOptimizerDialog::OnSelectClicked()
{
	bSelectMode = !bSelectMode;
	Selected.Empty();

	const EVisibility SelVis = bSelectMode ? EVisibility::Visible : EVisibility::Collapsed;
	SelectAllBtn->SetVisibility(SelVis);
	DisableSelectedBtn->SetVisibility(SelVis);

	ListView->RequestListRefresh();
	return FReply::Handled();
}

FReply SPluginOptimizerDialog::OnSelectAllClicked()
{
	const bool bAllSelected = (Selected.Num() == Items.Num() && Items.Num() > 0);

	Selected.Empty();
	if (!bAllSelected)
	{
		for (const TSharedPtr<FString>& It : Items) Selected.Add(*It);
	}

	ListView->RequestListRefresh();
	SelectAllBtn->Invalidate(EInvalidateWidget::Layout);
	return FReply::Handled();
}

FReply SPluginOptimizerDialog::OnDisableSelectedClicked()
{
	if (Selected.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("NoneSelected", "No plugins selected."));
		return FReply::Handled();
	}

	const TArray<FString> ToDisable = Selected.Array();
	DisableMultiple(ToDisable);
	return FReply::Handled();
}

/* ------------------------------------------------------------------ */
/*  CHECKBOX                                                          */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::OnCheckboxChanged(ECheckBoxState State, TSharedPtr<FString> Item)
{
	if (State == ECheckBoxState::Checked)  Selected.Add(*Item);
	else                                   Selected.Remove(*Item);
}

ECheckBoxState SPluginOptimizerDialog::IsItemChecked(TSharedPtr<FString> Item) const
{
	return Selected.Contains(*Item) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

/* ------------------------------------------------------------------ */
/*  DESATIVAR 1 PLUGIN                                                */
/* ------------------------------------------------------------------ */
bool SPluginOptimizerDialog::DisableOne(const FString& PluginName)
{
	IProjectManager& ProjMgr = IProjectManager::Get();
	FText Fail;

	if (!ProjMgr.SetPluginEnabled(PluginName, false, Fail))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("DisableFailed", "Failed to disable plugin:\n{0}\n\n{1}"),
				FText::FromString(PluginName), Fail));
		return false;
	}

	ProjMgr.SaveCurrentProjectToDisk(Fail);

	Items.RemoveAll([&](const TSharedPtr<FString>& Ptr) { return *Ptr == PluginName; });
	Selected.Remove(PluginName);
	RefreshHeader();
	ListView->RequestListRefresh();

	ShowRestartPopup();
	return true;
}

/* ------------------------------------------------------------------ */
/*  DESATIVAR VÁRIOS PLUGINS                                          */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::DisableMultiple(const TArray<FString>& ToDisable)
{
	for (const FString& P : ToDisable)
		DisableOne(P);
	ShowRestartPopup();
}

/* ------------------------------------------------------------------ */
/*  POP-UP “Restart required”                                         */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::ShowRestartPopup()
{
	if (bSuppressRestartPopup)
		return;

	TSharedPtr<SCheckBox> Check;
	TSharedRef<SWindow> Win = SNew(SWindow)
		.Title(LOCTEXT("RestartPopupTitle", "Plugin Optimizer"))
		.ClientSize(FVector2D(400, 120))
		.SupportsMinimize(false)
		.SupportsMaximize(false);

	Win->SetContent(
		SNew(SVerticalBox)

		+ SVerticalBox::Slot().FillHeight(1).Padding(10)
		[
			SNew(STextBlock)
				.AutoWrapText(true)
				.Text(LOCTEXT("RestartPopupMsg",
					"Plugin disabled.\nPlease restart the editor to finish unloading."))
		]

		+ SVerticalBox::Slot().AutoHeight().Padding(10, 0, 10, 10)
		[
			SAssignNew(Check, SCheckBox)
				.Content()
				[
					SNew(STextBlock).Text(LOCTEXT("DontShowAgain", "Don't show this again"))
				]
		]

		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0, 0, 10, 10)
		[
			SNew(SButton)
				.Text(LOCTEXT("OK", "OK"))
				.OnClicked_Lambda([Win, Check]()
					{
						bSuppressRestartPopup = Check->IsChecked();
						GConfig->SetBool(CFG_SECTION, CFG_KEY, bSuppressRestartPopup, GEditorPerProjectIni);
						Win->RequestDestroyWindow();
						return FReply::Handled();
					})
		]);

	FSlateApplication::Get().AddModalWindow(Win, nullptr);
}

#undef LOCTEXT_NAMESPACE

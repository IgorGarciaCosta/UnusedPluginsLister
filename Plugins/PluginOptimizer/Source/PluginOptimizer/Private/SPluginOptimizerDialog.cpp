#include "SPluginOptimizerDialog.h"

#include "Interfaces/IProjectManager.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SPluginOptimizerDialog"

/* ------------------------------------------------------------------ */
/*  CONSTRUTOR                                                         */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::Construct(const FArguments& InArgs)
{
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

						/* botão Select All (visível só em select-mode) */
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SAssignNew(SelectAllBtn, SButton)
								.Text(LOCTEXT("SelectAll", "Select All"))
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
/*  LINHA DA LISTA  – agora com visibilidade dinâmica                  */
/* ------------------------------------------------------------------ */
TSharedRef<ITableRow> SPluginOptimizerDialog::OnGenerateRow(
	TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FString>>, Owner)
		[
			SNew(SHorizontalBox)

				/* checkbox (mostra/oculta via Visibility) */
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					SNew(SCheckBox)
						.Visibility_Lambda([this]() { return bSelectMode ? EVisibility::Visible : EVisibility::Collapsed; })
						.IsChecked(this, &SPluginOptimizerDialog::IsItemChecked, Item)
						.OnCheckStateChanged(this, &SPluginOptimizerDialog::OnCheckboxChanged, Item)
				]

				/* nome do plugin */
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(STextBlock).Text(FText::FromString(*Item))
				]

				/* botão Disable individual (esconde em select-mode) */
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
		];
}


/* ------------------------------------------------------------------ */
/*  BOTÕES DO TOPO                                                     */
/* ------------------------------------------------------------------ */
FReply SPluginOptimizerDialog::OnSelectClicked()
{
	bSelectMode = !bSelectMode;
	Selected.Empty();

	/* visibilidade de botões */
	const EVisibility SelVis = bSelectMode ? EVisibility::Visible : EVisibility::Collapsed;
	SelectAllBtn->SetVisibility(SelVis);
	DisableSelectedBtn->SetVisibility(SelVis);

	ListView->RequestListRefresh();
	return FReply::Handled();
}

FReply SPluginOptimizerDialog::OnSelectAllClicked()
{
	Selected.Empty();
	for (const TSharedPtr<FString>& It : Items) Selected.Add(*It);
	ListView->RequestListRefresh();
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

	/* cria array estável para iterar (Selected será modificado) */
	TArray<FString> ToDisable = Selected.Array();
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
/*  DESATIVAR 1                                                       */
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
	return true;
}

/* ------------------------------------------------------------------ */
/*  DESATIVAR VÁRIOS                                                  */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::DisableMultiple(const TArray<FString>& ToDisable)
{
	for (const FString& P : ToDisable)
		DisableOne(P);

	FMessageDialog::Open(EAppMsgType::Ok,
		LOCTEXT("RestartRequired", "Selected plugins disabled.\nPlease restart the Editor to finish unloading."));
}

#undef LOCTEXT_NAMESPACE

#include "SPluginOptimizerDialog.h"

#include "Interfaces/IProjectManager.h"
#include "Interfaces/IPluginManager.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"

#define LOCTEXT_NAMESPACE "SPluginOptimizerDialog"

/* ------------------------------------------------------------------ */
/*  CONSTRUTOR                                                        */
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

				/* -------------- cabeçalho ---------------- */
				+ SVerticalBox::Slot().AutoHeight().Padding(6)
				[
					SNew(SHorizontalBox)

						+ SHorizontalBox::Slot().FillWidth(1)
						[
							SAssignNew(HeaderText, STextBlock).AutoWrapText(true)
						]

						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SNew(SButton)
								.Text(LOCTEXT("Select", "Select"))
								.OnClicked(this, &SPluginOptimizerDialog::OnSelectClicked)
						]

						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SAssignNew(SelectAllBtn, SButton)
								.Text(LOCTEXT("SelectAll", "Select All"))
								.Visibility(EVisibility::Collapsed)
								.OnClicked(this, &SPluginOptimizerDialog::OnSelectAllClicked)
						]
				]

				/* -------------- lista -------------------- */
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
/*  HEADER                                                            */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::RefreshHeader()
{
	const int32 Potential = Items.Num();
	FString Txt = FString::Printf(TEXT("Enabled: %d | Used: %d | Potentially Unused: %d"),
		EnabledCnt, UsedCnt, Potential);
	HeaderText->SetText(FText::FromString(Txt));
}

/* ------------------------------------------------------------------ */
/*  GERA LINHA DA LISTA                                               */
/* ------------------------------------------------------------------ */
TSharedRef<ITableRow> SPluginOptimizerDialog::OnGenerateRow(
	TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FString>>, Owner)
		[
			SNew(SHorizontalBox)

				/* checkbox (aparece só no select-mode) */
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0)
				[
					bSelectMode
						? StaticCastSharedRef<SWidget>(
							SNew(SCheckBox)
							.IsChecked(this, &SPluginOptimizerDialog::IsItemChecked, InItem)
							.OnCheckStateChanged(this, &SPluginOptimizerDialog::OnCheckboxChanged, InItem))
						: SNullWidget::NullWidget
				]

				/* nome do plugin */
				+ SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center).Padding(4, 0)
				[
					SNew(STextBlock).Text(FText::FromString(*InItem))
				]

				/* botão Disable (só fora do select-mode) */
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4, 0)
				[
					!bSelectMode
						? StaticCastSharedRef<SWidget>(
							SNew(SButton)
							.Text(LOCTEXT("Disable", "Disable"))
							.OnClicked_Lambda([this, InItem]()
								{
									DisablePlugin(*InItem);
									return FReply::Handled();
								}))
						: SNullWidget::NullWidget
				]
		];
}

/* ------------------------------------------------------------------ */
/*  BOTÕES DO TOPO                                                    */
/* ------------------------------------------------------------------ */
FReply SPluginOptimizerDialog::OnSelectClicked()
{
	bSelectMode = !bSelectMode;
	Selected.Empty();

	SelectAllBtn->SetVisibility(bSelectMode ? EVisibility::Visible : EVisibility::Collapsed);
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

/* ------------------------------------------------------------------ */
/*  CHECKBOX                                                          */
/* ------------------------------------------------------------------ */
void SPluginOptimizerDialog::OnCheckboxChanged(ECheckBoxState NewState, TSharedPtr<FString> Item)
{
	if (NewState == ECheckBoxState::Checked)  Selected.Add(*Item);
	else                                      Selected.Remove(*Item);
}

ECheckBoxState SPluginOptimizerDialog::IsItemChecked(TSharedPtr<FString> Item) const
{
	return Selected.Contains(*Item) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

/* ------------------------------------------------------------------ */
/*  DESATIVAR PLUGIN                                                  */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/*  DESATIVAR PLUGIN – versão que força refresh e avisa               */
/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/*  DESATIVAR PLUGIN – versão final                                   */
/* ------------------------------------------------------------------ */
bool SPluginOptimizerDialog::DisablePlugin(const FString& PluginName)
{
	IProjectManager& ProjMgr = IProjectManager::Get();
	FText Fail;

	// 1. Marca como disabled no .uproject
	if (!ProjMgr.SetPluginEnabled(PluginName, false, Fail))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("DisableFailed", "Failed to disable plugin:\n{0}\n\n{1}"),
				FText::FromString(PluginName), Fail));
		return false;
	}

	// 2. Salva o projeto
	ProjMgr.SaveCurrentProjectToDisk(Fail);   // ignoramos falha aqui; já avisamos antes

	// 3. Atualiza UI
	Items.RemoveAll([&](const TSharedPtr<FString>& Ptr) { return *Ptr == PluginName; });
	Selected.Remove(PluginName);
	RefreshHeader();
	ListView->RequestListRefresh();

	// 4. Feedback
	FMessageDialog::Open(EAppMsgType::Ok,
		LOCTEXT("RestartRequired", "Plugin disabled.\nPlease restart the Editor to finish unloading."));

	return true;
}

#undef LOCTEXT_NAMESPACE

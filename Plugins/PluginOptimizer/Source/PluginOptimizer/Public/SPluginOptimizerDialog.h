#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

/* ------------------------------------------------------------------ */
/*  Janela principal do “Plugin Optimizer”                            */
/* ------------------------------------------------------------------ */
class SPluginOptimizerDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPluginOptimizerDialog) {}
		SLATE_ARGUMENT(TArray<FString>, Candidates)
		SLATE_ARGUMENT(int32, EnabledCount)
		SLATE_ARGUMENT(int32, UsedCount)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	/* ---------- geração de linhas ---------- */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> Item,
		const TSharedRef<STableViewBase>& OwnerTable);

	/* ---------- botões do topo ------------- */
	FReply OnSelectClicked();
	FReply OnSelectAllClicked();
	FReply OnDisableSelectedClicked();

	/* ---------- checkbox linhas ------------ */
	void   OnCheckboxChanged(ECheckBoxState State, TSharedPtr<FString> Item);
	ECheckBoxState IsItemChecked(TSharedPtr<FString> Item) const;

	/* ---------- lógica --------------------- */
	bool DisableOne(const FString& PluginName);      // usado pelo modo item
	void DisableMultiple(const TArray<FString>& ToDisable);

	void RefreshHeader();

	/* ---------- dados ---------------------- */
	TArray<TSharedPtr<FString>> Items;   // plugins ainda listados
	TSet<FString>               Selected;
	bool                        bSelectMode = false;

	int32 EnabledCnt = 0;
	int32 UsedCnt = 0;

	/* ---------- widgets -------------------- */
	TSharedPtr<SListView<TSharedPtr<FString>>> ListView;
	TSharedPtr<STextBlock>                     HeaderText;
	TSharedPtr<SButton>                        SelectAllBtn;
	TSharedPtr<SButton>                        DisableSelectedBtn;
};

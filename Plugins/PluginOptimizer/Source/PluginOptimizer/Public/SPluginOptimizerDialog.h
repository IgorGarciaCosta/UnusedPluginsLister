#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

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
	/* ---------- callbacks ---------- */
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FString> InItem,
		const TSharedRef<STableViewBase>& OwnerTable);

	FReply OnSelectClicked();
	FReply OnSelectAllClicked();
	void   OnCheckboxChanged(ECheckBoxState NewState, TSharedPtr<FString> Item);

	bool DisablePlugin(const FString& PluginName);
	void RefreshHeader();

	ECheckBoxState IsItemChecked(TSharedPtr<FString> Item) const;

	/* ---------- dados ---------- */
	TArray<TSharedPtr<FString>> Items;     // linhas da lista
	TSet<FString>               Selected;  // seleção em modo select
	bool                        bSelectMode = false;

	int32 EnabledCnt = 0;
	int32 UsedCnt = 0;

	/* ---------- widgets ---------- */
	TSharedPtr<SListView<TSharedPtr<FString>>> ListView;
	TSharedPtr<STextBlock>                     HeaderText;
	TSharedPtr<SButton>                        SelectAllBtn;
};

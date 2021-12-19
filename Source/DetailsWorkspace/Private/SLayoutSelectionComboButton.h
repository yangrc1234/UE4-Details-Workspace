#pragma once

class SLayoutSelectionComboButton : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnLayoutOperation, FString);

	SLATE_BEGIN_ARGS(SLayoutSelectionComboButton) {}
		SLATE_EVENT(FOnLayoutOperation, OnLayoutDeleteClicked)
		SLATE_EVENT(FOnLayoutOperation, OnLayoutSelected)
		SLATE_EVENT(FSimpleDelegate, OnRenameLayout)
		SLATE_EVENT(FSimpleDelegate, OnCreateNewLayout)
		SLATE_EVENT(FSimpleDelegate, OnCreateNewLayoutByCopying)
		SLATE_ATTRIBUTE(FText, SelectedLayoutName)
	SLATE_END_ARGS()

	void Construct(const FArguments& Arguments);
	FText GetLabel() const;

private:
	TSharedPtr<SComboButton> ComboButton;
	FSimpleDelegate OnRenameLayout;
	FSimpleDelegate OnCreateNewLayout;
	FSimpleDelegate OnCreateNewLayoutByCopying;
	FOnLayoutOperation OnLayoutDeleteClicked;
	FOnLayoutOperation OnLayoutSelected;
	TAttribute<FText> OnSelectedLayoutName;
	TSharedPtr<SSearchBox> SearchBox;
	TSharedPtr<SBox> LayoutsRoot;
	struct FLayoutRowItem : TSharedFromThis<FLayoutRowItem>
	{
		FLayoutRowItem(FString InLayoutName) {	LayoutName = InLayoutName;	}
		FString LayoutName;
	}; 
	TArray< TSharedPtr<FLayoutRowItem> > LayoutListItems;
	typedef SListView< TSharedPtr< FLayoutRowItem > > SLayoutListView;
	
	void OnRowItemSelected(TSharedPtr<FLayoutRowItem> Layout, ESelectInfo::Type SelectInfo);

	FReply OnLayoutDeleteButton(FString LayoutName);

	TSharedRef<ITableRow> OnGenerateLayoutItemRow(TSharedPtr<FLayoutRowItem> InItem,
	                                              const TSharedRef<STableViewBase>& OwnerTable);
	void RefreshLayoutList();

	TSharedRef<SWidget> OnGetLayoutSelectMenuContent();
};
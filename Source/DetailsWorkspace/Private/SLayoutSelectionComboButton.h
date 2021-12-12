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

private:
	TSharedPtr<SComboButton> ComboButton;
	FSimpleDelegate OnRenameLayout;
	FSimpleDelegate OnCreateNewLayout;
	FSimpleDelegate OnCreateNewLayoutByCopying;
	FOnLayoutOperation OnLayoutDeleteClicked;
	FOnLayoutOperation OnLayoutSelected;
  
	struct FLayoutRowItem : TSharedFromThis<FLayoutRowItem>
	{
		FLayoutRowItem(FString InLayoutName) {	LayoutName = InLayoutName;	}
		FString LayoutName;
	}; 
	typedef SListView< TSharedPtr< FLayoutRowItem > > SLayoutListView;
	TArray< TSharedPtr<FLayoutRowItem> > LayoutListItems;

	void OnRowItemSelected(TSharedPtr<FLayoutRowItem> Layout, ESelectInfo::Type SelectInfo);

	FReply OnLayoutDeleteButton(FString LayoutName);

	TSharedRef<ITableRow> OnGenerateLayoutItemRow(TSharedPtr<FLayoutRowItem> InItem,
	                                              const TSharedRef<STableViewBase>& OwnerTable);

	TSharedRef<SWidget> OnGetLayoutSelectMenuContent();
};
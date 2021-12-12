#include "SLayoutSelectionComboButton.h"


#include "DetailsWorkspaceProfile.h"
#include "Slate.h"
#include "SlateCore.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLayoutSelectionComboButton::Construct(const FArguments& Arguments)
{
	ChildSlot[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()[
			SAssignNew(ComboButton, SComboButton)
                    .OnGetMenuContent(
				                                     FOnGetContent::CreateSP(
					                                     this,
					                                     &SLayoutSelectionComboButton::OnGetLayoutSelectMenuContent)
			                                     )
                    .ButtonContent()
			[
				SNew(STextBlock).Text(Arguments._SelectedLayoutName)
				                .MinDesiredWidth(150.0f)
			]
			.VAlign(VAlign_Center)
		]
		.AutoWidth()
	];

	OnRenameLayout = Arguments._OnRenameLayout;
	OnCreateNewLayout = Arguments._OnCreateNewLayout;
	OnCreateNewLayoutByCopying = Arguments._OnCreateNewLayoutByCopying;
	OnLayoutDeleteClicked = Arguments._OnLayoutDeleteClicked;
	OnLayoutSelected = Arguments._OnLayoutSelected;
}


END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SLayoutSelectionComboButton::OnRowItemSelected(TSharedPtr<FLayoutRowItem> Layout, ESelectInfo::Type SelectInfo)
{
	OnLayoutSelected.ExecuteIfBound(Layout->LayoutName);
	ComboButton->SetIsOpen(false);
}

FReply SLayoutSelectionComboButton::OnLayoutDeleteButton(FString LayoutName)
{
	OnLayoutDeleteClicked.ExecuteIfBound(LayoutName);
	return FReply::Handled();
}

TSharedRef<ITableRow> SLayoutSelectionComboButton::OnGenerateLayoutItemRow(TSharedPtr<FLayoutRowItem> InItem,
                                                                           const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow<TSharedPtr<FLayoutRowItem> >, OwnerTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()[
				  SNew(STextBlock).Text(FText::FromString(InItem->LayoutName))
			  ]
			  .HAlign(HAlign_Left)
			  .FillWidth(1.0f)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
                .Text(LOCTEXT("Delete", "Delete"))
                .OnClicked(FOnClicked::CreateSP(this, &SLayoutSelectionComboButton::OnLayoutDeleteButton,
                                                InItem->LayoutName))
                .ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Delete"))
				]
			]
			.HAlign(HAlign_Right)
		];
}

TSharedRef<SWidget> SLayoutSelectionComboButton::OnGetLayoutSelectMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

	// First area, list all available layout items, and corresponding "copy" and "delete" button.
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("NewLayout", "Create New Layout"));
	MenuBuilder.AddMenuEntry(
        LOCTEXT("New", "New"),
        FText::GetEmpty(),
        FSlateIcon(),
        FUIAction(OnCreateNewLayout)
    );
	MenuBuilder.AddMenuEntry(
        LOCTEXT("CopyCurrent", "Copy Current"),
        FText::GetEmpty(),
        FSlateIcon(),
        FUIAction(OnCreateNewLayoutByCopying)
    );
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("CurrentLayout", "Current Layout"));
	MenuBuilder.AddMenuEntry(
        LOCTEXT("Rename", "Rename"),
        FText::GetEmpty(),
        FSlateIcon(),
        FUIAction(OnRenameLayout)
    );
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("Layouts", "Layouts"));
	{
		TSharedPtr<SLayoutListView> ListView;
		SAssignNew(ListView, SLayoutListView)
                .OnGenerateRow(
                                                 SLayoutListView::FOnGenerateRow::CreateSP(
                                                     this, &SLayoutSelectionComboButton::OnGenerateLayoutItemRow))
                .ListItemsSource(&LayoutListItems)
                .OnSelectionChanged(
                                                 SLayoutListView::FOnSelectionChanged::CreateSP(
                                                     this, &SLayoutSelectionComboButton::OnRowItemSelected))
                .SelectionMode(ESelectionMode::Type::Single);

		auto LocalCollection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
		TArray<FString> LayoutNames;
		LocalCollection->WorkspaceLayouts.GetKeys(LayoutNames);

		LayoutListItems.Empty();
		for (auto& LayoutName : LayoutNames)
		{
			LayoutListItems.Add(MakeShared<FLayoutRowItem>(LayoutName));
		}

		MenuBuilder.AddWidget(ListView.ToSharedRef(), FText::GetEmpty(), false);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

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
            .ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
            .OnGetMenuContent(
                 FOnGetContent::CreateSP(
                     this,
                     &SLayoutSelectionComboButton::OnGetLayoutSelectMenuContent)
             )
            .ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &SLayoutSelectionComboButton::GetLabel)
				.MinDesiredWidth(150.0f)
				.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
			]
			.VAlign(VAlign_Center)
		]
		.AutoWidth()
	];

	OnSelectedLayoutName = Arguments._SelectedLayoutName;
	OnRenameLayout = Arguments._OnRenameLayout;
	OnCreateNewLayout = Arguments._OnCreateNewLayout;
	OnCreateNewLayoutByCopying = Arguments._OnCreateNewLayoutByCopying;
	OnLayoutDeleteClicked = Arguments._OnLayoutDeleteClicked;
	OnLayoutSelected = Arguments._OnLayoutSelected;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FText SLayoutSelectionComboButton::GetLabel() const
{
	return FText::Format(LOCTEXT("CurrentLayoutLabelFormat", "Layout: {0}"), OnSelectedLayoutName.Get());
}

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


void SLayoutSelectionComboButton::RefreshLayoutList()
{
	TSharedRef<SLayoutListView> ListView = 
	SNew(SLayoutListView)
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
		if (SearchBox->GetText().IsEmptyOrWhitespace() || LayoutName.Contains(SearchBox->GetText().ToString()))
			LayoutListItems.Add(MakeShared<FLayoutRowItem>(LayoutName));
	}

	LayoutsRoot->SetContent(ListView);
}

TSharedRef<SWidget> SLayoutSelectionComboButton::OnGetLayoutSelectMenuContent()
{
	FMenuBuilder MenuBuilder(true, nullptr, nullptr, true, &FCoreStyle::Get(), false);
	
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
		MenuBuilder.AddWidget(
			SAssignNew(SearchBox, SSearchBox)
			.OnTextChanged_Lambda([this](const FText& Val)
			{
				SearchBox->SetText(Val);
				RefreshLayoutList();
			}),
			FText::GetEmpty(),
			true,
			false
		);

		SAssignNew(LayoutsRoot, SBox);
		MenuBuilder.AddWidget(LayoutsRoot.ToSharedRef(), FText::GetEmpty(), false);

		RefreshLayoutList();
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

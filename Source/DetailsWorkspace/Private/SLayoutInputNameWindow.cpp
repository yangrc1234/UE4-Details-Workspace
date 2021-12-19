#include "SLayoutInputNameWindow.h"


#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SLayoutNameInputWindow::Construct(const FArguments& Args)
{
	SWindow::Construct(SWindow::FArguments()
	                   .Title(Args._Title)
	                   .ClientSize(Args._ClientSize)
	);

	OnConfirmed = Args._OnConfirmed;

	TSharedPtr<SEditableTextBox> Input;
	
	SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()[
			  SAssignNew(Input, SEditableTextBox)
                .HintText(LOCTEXT("NewLayoutNameEditableTextHintText", "New layout name.."))
                .OnTextChanged(FOnTextChanged::CreateSP(this, &SLayoutNameInputWindow::OnTextChanged))
                .Text(this, &SLayoutNameInputWindow::GetText)
		  ]
		  .AutoHeight()
		  .HAlign(HAlign_Center)
		  .Padding(0.0f, 5.0f)
		+ SVerticalBox::Slot()[
			  SNew(SButton)
                .OnClicked(FOnClicked::CreateSP(this, &SLayoutNameInputWindow::OnConfirmClicked))
                .Text(Args._ButtonText)
		  ]
		  .AutoHeight()
		  .HAlign(HAlign_Center)
		  .Padding(0.0f, 5.0f)
	);

	GEditor->EditorAddModalWindow(SharedThis(this));
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SLayoutNameInputWindow::OnConfirmClicked()
{
	OnConfirmed.ExecuteIfBound(InputContent);
	RequestDestroyWindow();
	return FReply::Handled();
}

/** Overridden from SWidget: Called when a key is pressed down - capturing copy */
FReply SLayoutNameInputWindow::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	if (InKeyEvent.GetKey() == EKeys::Enter)
	{
		OnConfirmClicked();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

#undef LOCTEXT_NAMESPACE

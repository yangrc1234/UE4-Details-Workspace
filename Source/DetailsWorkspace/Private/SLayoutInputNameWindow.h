#pragma once
#include "Slate.h"
#include "SlateCore.h"

class SLayoutNameInputWindow : public SWindow
{
public:
	DECLARE_DELEGATE_OneParam(FOnNameInputConfirmed, FText);
	
	SLATE_BEGIN_ARGS(SLayoutNameInputWindow)
	{}
	SLATE_ATTRIBUTE( FText, Title )
    SLATE_ATTRIBUTE( FText, ButtonText )
    SLATE_ARGUMENT( FVector2D, ClientSize )    
    SLATE_EVENT( FOnNameInputConfirmed, OnConfirmed )
    SLATE_END_ARGS()
	
    FText GetText() const
	{
		return InputContent;
	}
	
	void Construct(const FArguments& Args);

private:
	FReply OnConfirmClicked();

	void OnTextChanged(const FText& Input)
	{
		InputContent = Input;
	}
	FOnNameInputConfirmed OnConfirmed;
	FText InputContent;
};


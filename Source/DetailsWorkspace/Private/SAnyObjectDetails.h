#pragma once
#include "DetailsWorkspaceProfile.h"
#include "Slate.h"
#include "SlateCore.h"

class SAnyObjectDetails : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAnyObjectDetails)
	{}
	SLATE_ATTRIBUTE(bool, AutoInspectPIE)
	SLATE_END_ARGS()

	FReply OnSelectObjectClicked();

	FText GetHintText() const;

	void Construct(const FArguments& InArgs, FTabId InTabID);

	UObject* GetObject(bool bPIE) { return ObjectValue.Resolve(bPIE); }

	UObject* GetObjectAuto();

	void SetObjectLazyPtr(FDetailsWorkspaceObservedItem Value);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	FTabId TabID;

private:
	TWeakObjectPtr<UObject> CurrentWatching;
	TAttribute<bool> bAutoInspectPIE;
	FDetailsWorkspaceObservedItem ObjectValue;
	TSharedPtr<class IDetailsView> DetailsView;
	bool bLoaded = false;
};


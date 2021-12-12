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
	bool OnPropertyVisible(const FPropertyAndParent& PropertyAndParent);
	FReply OnCategoriesClicked();

	void Construct(const FArguments& InArgs, FTabId InTabID);

	UObject* GetObject(bool bPIE) { return ObjectValue.Resolve(bPIE); }

	UObject* GetObjectAuto();
	void OnCategoryFilterCheckStateChanged(ECheckBoxState State, FString Category);

	ECheckBoxState OnGetCategoryFilterCheckState(FString Category) const;
	void SetObjectLazyPtr(FDetailsWorkspaceObservedItem Value);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	FTabId TabID;

private:
	bool bCategoryFilterVisibility = false;
	TSharedPtr<SBorder> CategoryFilterRoot;
	TArray<FString> AvailableCategories;
	TSet<FString> HiddenCategories;
	TWeakObjectPtr<UObject> CurrentWatching;
	TAttribute<bool> bAutoInspectPIE;
	FDetailsWorkspaceObservedItem ObjectValue;
	TSharedPtr<class IDetailsView> DetailsView;
	bool bLoaded = false;
};


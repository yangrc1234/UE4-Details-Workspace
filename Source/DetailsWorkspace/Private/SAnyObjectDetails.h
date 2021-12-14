#pragma once
#include "DetailsWorkspaceProfile.h"
#include "ISequencer.h"
#include "Slate.h"
#include "SlateCore.h"

class SAnyObjectDetails : public SCompoundWidget
{

public:
	SLATE_BEGIN_ARGS(SAnyObjectDetails)
	{}
	SLATE_ATTRIBUTE(bool, AutoInspectPIE)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FTabId InTabID);
	~SAnyObjectDetails();

	inline static void RegisterSequencer(TSharedRef<ISequencer> Sequencer);

	UObject* GetObject(bool bPIE) { return ObjectValue.Resolve(bPIE); }
	UObject* GetObjectAuto();
	
	void SetObserveItem(FDetailsWorkspaceObservedItem Value);
	const FDetailsWorkspaceObservedItem& GetObserveItem() const { return ObjectValue; }

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	FTabId TabID;

private:

	FReply OnSelectObjectClicked();
	FText GetHintText() const;
	FReply OnCategorySettingsClicked();
	void OnCategoryFilterCheckStateChanged(ECheckBoxState State, FName Category);
	ECheckBoxState OnGetCategoryFilterCheckState(FName Category) const;
	FLinearColor CategorySettingLabelColor(FName Category) const;
	EVisibility CategorySettingLabelVisibility(FName Cateogry) const;
	void OnGetCategoryNames(TArray<FName> Val);

	TSharedPtr<SBorder> CategoryFilterRoot;
	TArray<FName> AvailableCategories;
	TWeakObjectPtr<UObject> CurrentWatching;
	TAttribute<bool> bAutoInspectPIE;
	FDetailsWorkspaceObservedItem ObjectValue;
	TSharedPtr<class IDetailsView> DetailsView;
	bool bAvailableCategoriesDirty = true;
};


#pragma once
#include "DetailsWorkspaceProfile.h"

class SAnyObjectDetails;

class DETAILSWORKSPACE_API SDetailsWorkspace : public SDockTab  
{
public:
	SLATE_BEGIN_ARGS(SDetailsWorkspace)
	{}
	SLATE_END_ARGS()

	~SDetailsWorkspace();
	void Construct(const FArguments& Args, FString InstanceName, bool bLoadInstanceLastLayout);
	void ResetLayoutToInitial();
	void RestoreFromLocalUserLayout(FString LayoutName);
	void SpawnNewDetailWidgetForObject(UObject* InObject);
	bool IsObservingObject(UObject* Object) const;
	void SwitchLayout(FString TargetLayoutName, bool bSaveBeforeSwitching = true);
	void CreateNewLayout(FText LayoutName);
	void CreateNewLayoutByCopyingCurrent(FText LayoutName);
	void CreateNewLayoutWithDialog();
	void CreateNewLayoutByCopyingWithDialog();

protected:
	TSharedRef<SDockTab> CreateDocKTabWithDetailView(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> CreateWelcomeTab(const FSpawnTabArgs& Args);
	void Restore(const FDetailsWorkspaceLayout& Profile);
	void DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget);
	bool EnableAutoPIE() const;
	EVisibility DropAreaVisibility() const;
	FText GetLabel() const;

	TSharedPtr<FTabManager> TabManager;
	TWeakPtr<SDockTab> ParentTab;
	FString InstanceName;
	FString WorkingLayoutName;
	FDetailsWorkspaceLayout LoadedProfile;

	bool OnRecognizeObserveObjectDrop(TSharedPtr<FDragDropOperation> Operation) const;
	void SaveLayout();
	FName GetUnusedTabTypeName() const;

private:
	bool IsNotObservingObject(UObject* Object) const
	{
		return !IsObservingObject(Object);
	}
	void EnsureRegisterDetailTabSpawner(FName TabID);
	void DeleteLayoutWithDialog(FString LayoutName);
	FReply OnObserveObjectDrop(TSharedPtr<FDragDropOperation> Op);
	void OnDetailTabClosed(TSharedRef<SDockTab> Tab, TWeakPtr<SAnyObjectDetails> Detail);
	EVisibility DeveloperTextVisibility() const;
	TWeakPtr<SDetailsWorkspace> Root;
	TArray<TWeakPtr<SAnyObjectDetails>> SpawnedDetails;
	TSharedRef<SWidget> CreateConfigArea(const FDetailsWorkspaceInstanceSettings* Settings);
	FReply CopyCurrentLayoutStringToClipboard();
	TSharedPtr<SBorder> DockingTabsContainer;
	void DoPersistVisualState();
	FText OnGetCurrentLayoutName() const;

	void RenameCurrentLayout(FText NewName);
	void CreateRenameCurrentLayoutWindow();

	TSharedPtr<SCheckBox> DeveloperModeCheckerbox;
	TSharedPtr<SCheckBox> AutoPIECheckBox;
	TSharedPtr<SCompoundWidget> LayoutSelectComboButton;
	TSharedPtr<class SSubObjectAddArea> SubObjectAddArea;
	TSharedPtr<SExpandableArea> ConfigArea;
};

TSharedRef<SDetailsWorkspace> CreateDetailsWorkSpace(FString InstanceName, bool bLoadInstanceLastLayout);

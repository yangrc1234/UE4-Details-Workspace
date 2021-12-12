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

	// Reset to initial state. (Welcome tab)
	// This will erase current layout data.  
	void ResetLayoutToInitial();

	// Restore layout from layout name.
	// Will erase current layout data.
	void RestoreFromLocalUserLayout(FString LayoutName);

	// Create new tab for InObject.  
	void SpawnNewDetailWidgetForObject(UObject* InObject);

	// Check if Object has corresponding tab.
	bool IsObservingObject(UObject* Object) const;

	// Switch to another layout.  
	void SwitchLayout(FString TargetLayoutName, bool bSaveBeforeSwitching = true);

	// Create new layout using LayoutName.
	void CreateNewLayout(FString LayoutName);

	// Create new layout using LayoutName.
	// Current layout data will be copied over.
	void CreateNewLayoutByCopyingCurrent(FString LayoutName);

	// Create new layout, offering user a dialog to input layout name. 
	void CreateNewLayoutWithDialog();
	
	// Create new layout, offering user a dialog to input layout name.
	// Current layout data will be copied over.
	void CreateNewLayoutByCopyingWithDialog();

	// Rename current layout. 
	void RenameCurrentLayout(FString NewName);
protected:
	TSharedRef<SDockTab> CreateDocKTabWithDetailView(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> CreateWelcomeTab(const FSpawnTabArgs& Args);
	void Restore(const FDetailsWorkspaceLayout& Profile);
	void DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget);
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

	void CreateRenameCurrentLayoutWindow();

	TSharedPtr<SCheckBox> DeveloperModeCheckerbox;
	TSharedPtr<SCheckBox> AutoPIECheckBox;
	TSharedPtr<SCompoundWidget> LayoutSelectComboButton;
	TSharedPtr<class SSubObjectAddArea> SubObjectAddArea;
	TSharedPtr<SExpandableArea> ConfigArea;
};

TSharedRef<SDetailsWorkspace> CreateDetailsWorkSpace(FString InstanceName, bool bLoadInstanceLastLayout);

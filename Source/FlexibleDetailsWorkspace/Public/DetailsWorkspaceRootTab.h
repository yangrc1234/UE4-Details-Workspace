#pragma once

#include "DetailsWorkspaceRootTab.generated.h"

UCLASS(MinimalAPI, hidecategories = Object, collapsecategories)
class UDetailsWorkspaceProfileCollectionFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

    //~ Begin UFactory Interface
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	FString GetDefaultNewAssetName() const override
	{
		return FString(TEXT("DetailsWorkspaceProfileCollection"));
	}
	//~ Begin UFactory Interface	
};

USTRUCT()
struct FDetailWorkspaceObservedItem
{
	GENERATED_BODY()

	static FDetailWorkspaceObservedItem From(UObject* Object)
	{
		FDetailWorkspaceObservedItem Result;
		if (Object->IsDefaultSubobject())
		{
			Result.bIsDefaultSubObject = true;
			Result.SubObjectName = Object->GetFName();
			Result.SavedObject = Object->GetOuter();
		}
		else
		{
			Result.SavedObject = Object;
			Result.bIsDefaultSubObject= false;
			Result.SubObjectName = NAME_None;
		}
		return Result;
	}

	UObject* Resolve()
	{
		if (!SavedObject.Get())
			return nullptr;
		
		if (!ResolvedObject.IsValid())
		{
			if (!bIsDefaultSubObject)
			{
				ResolvedObject = SavedObject.Get();
			}
			else
			{
				ResolvedObject = SavedObject->GetDefaultSubobjectByName(SubObjectName);
			}
		}
		return ResolvedObject.Get();
	}

	TWeakObjectPtr<UObject> ResolvedObject;

	UPROPERTY(EditAnywhere)
	TLazyObjectPtr<UObject> SavedObject;

	UPROPERTY(EditAnywhere)
	bool bIsDefaultSubObject;

	UPROPERTY(EditAnywhere)
	FName SubObjectName;
};

USTRUCT()
struct FDetailsWorkspaceLayout
{
	GENERATED_BODY()

	UPROPERTY()
	FString SavedLevelName;

	UPROPERTY(EditAnywhere)
	FString LayoutString;

	UPROPERTY(EditAnywhere)
	TMap<FName, FDetailWorkspaceObservedItem> References;
};

UCLASS()
class UDetailsWorkspaceProfile : public UObject
{
	GENERATED_BODY()
public:
	//Key is layout id.  
	UPROPERTY(EditAnywhere)
	TMap<FString, FDetailsWorkspaceLayout> WorkspaceLayouts;
	
	// Remember what layout is on, when any instance is closed.  
	UPROPERTY(EditAnywhere)
	TMap<FString, FString> InstanceLastLayout;

	static UDetailsWorkspaceProfile* GetOrCreateLocalUserProfile();
    
};

class FLEXIBLEDETAILSWORKSPACE_API SDetailsWorkspaceRootTab : public SDockTab  
{
public:
	SLATE_BEGIN_ARGS(SDetailsWorkspaceRootTab)
	{}
	SLATE_END_ARGS()

	~SDetailsWorkspaceRootTab();
	void Construct(const FArguments& Args, TSharedPtr<SWindow> Window, FString InstanceName, bool bLoadInstanceLastLayout);
	void ResetLayoutToInitial();
	void RestoreFromLocalUserLayout(FString LayoutName);
	void SpawnNewDetailWidgetForObject(UObject* InObject);
	bool IsObservingObject(UObject* Object) const;
	void SwitchLayout(FString TargetLayoutName, bool bSaveBeforeSwitching = true);
	void CreateNewLayout(FText LayoutName);
	void CreateNewLayoutByCopyingCurrent(FText LayoutName);

protected:
	TSharedRef<SDockTab> CreateDocKTabWithDetailView(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> CreateWelcomeTab(const FSpawnTabArgs& Args);
	void Restore(const FDetailsWorkspaceLayout& Profile);
	void DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget);
	EVisibility DropAreaVisibility() const;

	TSharedPtr<FTabManager> TabManager;
	TWeakPtr<SDockTab> ParentTab;
	FString InstanceName;
	FString WorkingLayoutName;
	FDetailsWorkspaceLayout LoadedProfile;

	
	bool OnRecognizeObserveObjectDrop(TSharedPtr<FDragDropOperation> Operation) const;
	void SaveLayout(FString LayoutName);
	void SaveLayoutWithDialog(FString LayoutName);
	FName GetUnusedTabTypeName() const;

private:
	bool IsNotObservingObject(UObject* Object) const
	{
		return !IsObservingObject(Object);
	}
	void EnsureRegisterDetailTabSpawner(FName TabID);
	void OnLayoutMenuSelected(const FText& Layout);
	void DeleteLayoutWithDialog(FString LayoutName);
	FReply OnObserveObjectDrop(TSharedPtr<FDragDropOperation> Op);
	TWeakPtr<SDetailsWorkspaceRootTab> Root;
	TArray<TWeakPtr<class SAnyObjectDetails>> SpawnedDetails;
	TSharedPtr<SBorder> DockingTabsContainer;
	void DoPersistVisualState();
	void CreateNewLayoutWithDialog();
	void CreateNewLayoutByCopyingWithDialog();
	FText OnGetCurrentLayoutName() const;

	void RenameCurrentLayout(FText NewName);
	void CreateRenameCurrentLayoutWindow();

	TSharedPtr<SComboButton> LayoutSelectComboButton;
	TSharedPtr<class SSubObjectAddArea> SubObjectAddArea;
};

TSharedRef<SDetailsWorkspaceRootTab> CreateDetailsWorkSpace(TSharedPtr<SWindow> Window, FString InstanceName, bool bLoadInstanceLastLayout);

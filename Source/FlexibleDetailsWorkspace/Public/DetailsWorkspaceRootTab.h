#pragma once

#include "DetailsWorkspaceRootTab.generated.h"

struct FInspectItem
{
	FLazyObjectPtr Object;
	TSharedPtr<IDetailsView> DetailsView;
};

class SAnyObjectDetails : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAnyObjectDetails)
	{}
	SLATE_END_ARGS()
	
    void Construct(const FArguments& InArgs, FTabId InTabID);

public:
	TLazyObjectPtr<UObject> GetLazyObjectPtr() const {return LazyObjectRef; }
	void SetObjectLazyPtr(TLazyObjectPtr<UObject> Value);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	FTabId TabID;
private:
	TLazyObjectPtr<UObject> LazyObjectRef;
	TSharedPtr<class IDetailsView> DetailsView;
	bool bLoaded = false;
};

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

class SDetailsWorkspaceRootTab : public SDockTab  
{
public:
	FReply OnNewTabClicked();

	SLATE_BEGIN_ARGS(SDetailsWorkspaceRootTab)
	{}
	SLATE_END_ARGS()

	~SDetailsWorkspaceRootTab();
	void Construct(const FArguments& Args, TSharedPtr<SWindow> Window, FString InstanceName, bool bLoadInstanceLastLayout);
	
	void RestoreFromLocalUserLayout(FString LayoutName);
	void Restore(const FDetailsWorkspaceLayout& Profile);
	void DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget);

protected:
	FDetailsWorkspaceLayout LoadedProfile;
	TSharedRef<SDockTab> CreateDocKTabWithDetailView(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> CreateWelcomeTab(const FSpawnTabArgs& Args);
	TSharedPtr<SDockTab> AllocateNewTab(FTabId &OutTabID);
	EVisibility AddActorComponentsButtonVisibility() const;
	FReply OnAddActorComponentsClicked();
	FReply OnAddObjectButtonClicked();
	EVisibility AddObjectButtonVisibility() const;
	FText PendingObservedObjectName() const;
	TSharedRef<SWidget> CreateAddObjectToObserveArea();
	
	TSharedPtr<FTabManager> TabManager;
	TWeakObjectPtr<AActor> PickekdActor;
	TArray<FInspectItem> Items;
	TWeakPtr<SDockTab> ParentTab;
	FString InstanceName;
	FString WorkingLayoutName;

	// The objeect dropped in drop area. 
	TWeakObjectPtr<UObject> PendingObservedObject; 
	
	bool OnRecognizeObserveObjectDrop(TSharedPtr<FDragDropOperation> Operation);
	void SaveLayout(FString LayoutName);
	void SaveLayoutWithDialog(FString LayoutName);
	void SpawnNewDetailWidgetForObject(UObject* InObject);
	FName GetUnusedTabTypeName() const;

private:
	TSharedRef<SWidget> OnAddTargetsMenu();
	FReply OnObserveObjectDrop(TSharedPtr<FDragDropOperation> Op);
	TWeakPtr<SDetailsWorkspaceRootTab> Root;
	TArray<TWeakPtr<SAnyObjectDetails>> SpawnedDetails;
	TSharedPtr<SBorder> DockingTabsContainer;
	void DoPersistVisualState();
	FReply OnCreateNewLayoutDialogConfirmed(FString LayoutName);
	void CreateNewLayoutWithDialog();
	FReply OnLayoutDeleteButton(FString LayoutName);
	void SwitchLayout(FString TargetLayoutName, bool bSaveBeforeSwitching = true);
	TSharedRef<SWidget> OnGetLayoutSelectMenuContent();
	FText OnGetCurrentLayoutName() const;
};


TSharedRef<SDockTab> CreateDetailsWorkSpace(TSharedPtr<SWindow> Window, FString InstanceName, bool bLoadInstanceLastLayout);

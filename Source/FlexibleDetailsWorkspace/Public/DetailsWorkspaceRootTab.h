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

	static FDetailWorkspaceObservedItem From(UObject* Object);

	UObject* Resolve(bool bTryResolvePIECounterPart)
	{
		if (!SavedObject.Get())
			return nullptr;

		auto Actor = Cast<AActor>(SavedObject.Get());
		if (Actor && bTryResolvePIECounterPart && ensure(GEditor->IsPlayingSessionInEditor()))
		{
			if (!ResolvedObjectPIE.IsValid())
			{
				Actor = EditorUtilities::GetSimWorldCounterpartActor(Actor);
				if (ensure(Actor))
				{
					if (!bIsDefaultSubObject)
					{
						ResolvedObjectPIE = Actor;
					}
					else
					{
						ResolvedObjectPIE = Actor->GetDefaultSubobjectByName(SubObjectName);
					}
				}
			}
			return ResolvedObjectPIE.Get();
		}
		else
		{
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
	}

	TWeakObjectPtr<UObject> ResolvedObject = nullptr;
	TWeakObjectPtr<UObject> ResolvedObjectPIE = nullptr;

	UPROPERTY(EditAnywhere)
	TLazyObjectPtr<UObject> SavedObject = nullptr;

	UPROPERTY(EditAnywhere)
	bool bIsDefaultSubObject = false;

	UPROPERTY(EditAnywhere)
	FName SubObjectName = NAME_None;
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

class SAnyObjectDetails;

class FLEXIBLEDETAILSWORKSPACE_API SDetailsWorkspaceRootTab : public SDockTab  
{
public:
	SLATE_BEGIN_ARGS(SDetailsWorkspaceRootTab)
	{}
	SLATE_END_ARGS()

	~SDetailsWorkspaceRootTab();
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
	TWeakPtr<SDetailsWorkspaceRootTab> Root;
	TArray<TWeakPtr<SAnyObjectDetails>> SpawnedDetails;
	TSharedRef<SWidget> CreateConfigArea();
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

TSharedRef<SDetailsWorkspaceRootTab> CreateDetailsWorkSpace(FString InstanceName, bool bLoadInstanceLastLayout);

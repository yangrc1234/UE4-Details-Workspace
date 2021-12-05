#include "DetailsWorkspaceRootTab.h"
#include "SlateOptMacros.h"
#include "FilterableActorPicker.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

// Some expectations:
// For end user:
// 1. Tab can exist without actual object to observe.
//		a) A tab contains multiple objects lazy reference. Once any object is loaded, the tab shows its details.  
// 2. User can drag-n-drop an object into a tab.	
// 3. Tab layout and its watching object is saved, in per-user per-project ini. (like content browser)
//
// If layout doesn't share between levels, will user accept?
//	Of course. A layout should probably be considered level-local.
//	It's kinda meaningless to share tabs layout between levels.
//	
//
// Where do we store layout data.
// 1. In level. No, it will pollute the level. 
// 2. In developer folder as asset. 
// 3. In editor-ini.
//
//  
// 
// For programmer:
// 1. Can create an instance from code. Can assign what objects to observe in which tab from code.

// How to manage tab?
// 1. Tab 

//FLayoutSaveRestore::LoadFromConfig(GEditorLayoutIni, StandaloneDefaultLayout);
//AActor* GetSimWorldCounterpartActor( AActor* Actor )
//SDropTarget

/*
TSharedRef<SWidget> SDetailsWorkspaceRootTab::CreateHeaderLine()
{
	// What does header should contain?
	// 1. Workspace save/load area:
	//		A drop-down, to select any saved workspace,
	//		save new/save/load/delete, to do corresponding action to selected workspace.
	// 2. Create new tab.
	//		A single button. Once clicked, a new tab is created.

	TSharedPtr<SFilterableActorPicker> ActorPicker;
	
	SAssignNew(ActorPicker, SFilterableActorPicker)
    .OnSetObject_Lambda([this](const FAssetData& AssetData) -> void
    {
        if (AssetData.IsValid())
        {
        	this->PickekdActor = AssetData.GetAsset();
        }
    })
    .OnShouldFilterAsset_Lambda([&](const FAssetData& AssetData) -> bool
    {
        return !!Cast<AActor>(AssetData.GetAsset());
    })
    .ActorAssetData_Lambda([&]() -> FAssetData
    {
        return FAssetData(PickekdActor.Get(), true);
    });
	
	return SNew(SHorizontalBox)
	+ SHorizontalBox::Slot()[
		ActorPicker.ToSharedRef()
    ]
	+ SHorizontalBox::Slot()[
		SNew(SButton).Text(LOCTEXT("AddActor", "Add Actor"))
		.OnClicked_Lambda([]()
		{
			// Create tab view for actor.  
		})
    ];
}
*/

TSharedRef<SWidget> CreateContent(TSharedRef<SDockTab> Root, TArray<FInspectItem> InspectItems)
{
	//Create tab layout for InspectItems.
	auto TabManager = FGlobalTabmanager::Get()->NewTabManager(Root);
	
	return SNew(STextBlock).Text(LOCTEXT("TestContent", "Test Content"));	
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAnyObjectDetails::Construct(const FArguments& InArgs, UObject* InObject)
{
	SetObject(InObject);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
}

FReply FDetailsWorkspaceRootTab::OnNewTabClicked()
{
	auto NewTab = AllocateNewTab();
	return FReply::Handled();
}

void FDetailsWorkspaceRootTab::Init()
{
	Root.Pin()->SetContent
    (
        SNew( SVerticalBox )
        +SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock).Text(LOCTEXT("Test", "test"))
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SButton)
            .Text(LOCTEXT("Test1", "test1"))
            .OnClicked(FOnClicked::CreateSP(this, &FDetailsWorkspaceRootTab::OnNewTabClicked))
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock).Text(LOCTEXT("Test", "test"))
        ]
    );
}

FDetailsWorkspaceRootTab::FDetailsWorkspaceRootTab()
{
	auto Tab = SNew(SDetailsWorkspaceRootTab);
	Tab->Handler = SharedThis(this);
	Root = Tab;
	TabManager = FGlobalTabmanager::Get()->NewTabManager(Tab);
}

FDetailsWorkspaceRootTab::~FDetailsWorkspaceRootTab()
{
	UE_LOG(LogTemp, Log, TEXT("FDetailsWorkspaceRootTab Destructed"));
}

TSharedRef<SDetailsWorkspaceRootTab> FDetailsWorkspaceRootTab::GetWidget()
{
	return Root.Pin().ToSharedRef();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SDockTab> FDetailsWorkspaceRootTab::CreateDocKTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab);
}

TSharedPtr<SDockTab> FDetailsWorkspaceRootTab::AllocateNewTab()
{
	int Index = 0;
	while (true)
	{
		auto TabID = FName(FString::Printf(TEXT("Tab%d"), Index));
		const bool bRegistered = TabManager->HasTabSpawner(TabID);

		if (bRegistered)
		{
			const bool bOpened = TabManager->FindExistingLiveTab(TabID) != nullptr;
			if (bOpened)
			{
				continue;
			}
			else
			{
				return TabManager->TryInvokeTab(TabID);
			}
		}
		else
		{
			TabManager->RegisterTabSpawner(TabID, FOnSpawnTab::CreateSP(this, &FDetailsWorkspaceRootTab::CreateDocKTab));
			TabManager->TryInvokeTab(TabID);
		}
	}	
}

TSharedRef<SDockTab> CreateDetailsWorkSpace()
{
	auto RootTab = MakeShared<FDetailsWorkspaceRootTab>();
	RootTab->Init();
	return RootTab->GetWidget();
}

#undef LOCTEXT_NAMESPACE
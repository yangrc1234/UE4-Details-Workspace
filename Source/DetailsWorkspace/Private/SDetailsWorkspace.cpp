#include "SDetailsWorkspace.h"

#include "DetailsWorkspaceProfile.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "IContentBrowserSingleton.h"
#include "SAnyObjectDetails.h"
#include "SDropTarget.h"
#include "SLayoutInputNameWindow.h"
#include "DragAndDrop/ActorDragDropOp.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "HAL/PlatformApplicationMisc.h"
#include "SLayoutSelectionComboButton.h"
#include "SSubObjectAddArea.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

static FName WelcomePageTabID = TEXT("Welcome");
static FName NormalTabID = TEXT("Normal");

static TSharedRef<FTabManager::FLayout> InitialLayout(int InitialDetailTabCount = 16)
{
	auto t = FTabManager::NewPrimaryArea();
	t->Split(
        FTabManager::NewStack()
        ->AddTab(WelcomePageTabID, ETabState::OpenedTab)
	);

	// We add some "closed" tabs.
	// Though at this moment, tab spawner is not registered, this still works.
	// And it allows new detail tab to be added inside the panel, which feels good.  
	for(int i = 0; i < InitialDetailTabCount;i++)
	{
		t->Split(
            FTabManager::NewStack()
            ->AddTab(FName(FString::Printf(TEXT("%s%d"), *NormalTabID.ToString(), i)), ETabState::ClosedTab)
        );
	}
	
	return FTabManager::NewLayout(TEXT("InitialLayout"))
        ->AddArea
        (
			t
        );
}


TSharedRef<SWidget> SDetailsWorkspace::CreateConfigArea(const FDetailsWorkspaceInstanceSettings* Settings)
{
	SAssignNew(ConfigArea, SExpandableArea)
		.HeaderContent()[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()[
				SNew(STextBlock).Text(LOCTEXT("Config", "Config"))
			]
			.VAlign(VAlign_Center)
			+ SHorizontalBox::Slot()[
			    SAssignNew(SubObjectAddArea, SSubObjectAddArea)
                .OnAddObjectConfirmed(SSubObjectAddArea::FAddObjectConfirmed::CreateSP(this, &SDetailsWorkspace::SpawnNewDetailWidgetForObject))
                .OnVerifyObjectAddable(SSubObjectAddArea::FVerifyObjectAddable::CreateSP(this, &SDetailsWorkspace::IsNotObservingObject))
            ]
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Top)
            .AutoWidth()
		]
		.BodyContent()[
			
		SNew(SGridPanel)
            .FillColumn(0, 0.2f)
            .FillColumn(1, 0.8f)
            + SGridPanel::Slot(0, 0)[
                SNew(STextBlock).Text(LOCTEXT("LayoutLabel", "Layout"))
            ].Padding(2.0f).VAlign(VAlign_Center)
            + SGridPanel::Slot(1, 0)[	
				SAssignNew(LayoutSelectComboButton, SLayoutSelectionComboButton)
	            .OnLayoutDeleteClicked(this, &SDetailsWorkspace::DeleteLayoutWithDialog)
	            .OnCreateNewLayout(this, &SDetailsWorkspace::CreateNewLayoutWithDialog)
	            .OnCreateNewLayoutByCopying(this, &SDetailsWorkspace::CreateNewLayoutByCopyingWithDialog)
	            .OnRenameLayout(this, &SDetailsWorkspace::CreateRenameCurrentLayoutWindow)
	            .SelectedLayoutName(this, &SDetailsWorkspace::OnGetCurrentLayoutName)
	            .OnLayoutSelected(this, &SDetailsWorkspace::SwitchLayout, true)
	        ].Padding(2.0f).VAlign(VAlign_Center)
	        
            + SGridPanel::Slot(0, 2)[
                SNew(STextBlock).Text(LOCTEXT("AutoSwitchPIE", "Auto Switch to PIE Object"))
            ].Padding(2.0f).VAlign(VAlign_Center)
            + SGridPanel::Slot(1, 2)[
                SAssignNew(AutoPIECheckBox, SCheckBox)
            ].Padding(2.0f).VAlign(VAlign_Center)

            + SGridPanel::Slot(0, 3)[
                SNew(STextBlock).Text(LOCTEXT("EnableDeveloperMode", "Enable Developer Mode"))
            ].Padding(2.0f).VAlign(VAlign_Center)
            + SGridPanel::Slot(1, 3)[
                SAssignNew(DeveloperModeCheckerbox, SCheckBox)
            ].Padding(2.0f).VAlign(VAlign_Center)
            
            + SGridPanel::Slot(0, 4).ColumnSpan(2)[
            	SNew(SButton)
            	.OnClicked(FOnClicked::CreateSP(this, &SDetailsWorkspace::CopyCurrentLayoutStringToClipboard))
            	[
					SNew(STextBlock).Text(LOCTEXT("CopyLayoutStringToClipboard", "Copy Layout String To Clipboard"))
                ]
            ].Padding(2.0f).VAlign(VAlign_Center)
        ];

	if (Settings)
	{
		AutoPIECheckBox->SetIsChecked(Settings->bAutoPIE);
		DeveloperModeCheckerbox->SetIsChecked(Settings->bDeveloperMode);
	}
	AutoPIECheckBox->SetIsChecked(true);
	ConfigArea->SetExpanded(false);
	
	return ConfigArea.ToSharedRef();
}

FReply SDetailsWorkspace::CopyCurrentLayoutStringToClipboard()
{
	auto String = TabManager->PersistLayout()->ToString();
	FPlatformApplicationMisc::ClipboardCopy( *String );
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LayoutStringCopied", "Layout string copied"));
	return FReply::Handled();
}

void SDetailsWorkspace::DoPersistVisualState()
{
	auto LocalCollection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	// Remember opened layout name. 
	if (LocalCollection)
	{
		if (!InstanceName.TrimStartAndEnd().IsEmpty())
		{
			LocalCollection->Modify();
			auto& t = LocalCollection->InstanceSettings.FindOrAdd(InstanceName);
			t.LastLayout = WorkingLayoutName;
			t.bDeveloperMode = DeveloperModeCheckerbox->IsChecked();
			t.bAutoPIE = DeveloperModeCheckerbox->IsChecked();
		}
	}

	// Save the layout.  
	if (!WorkingLayoutName.TrimStartAndEnd().IsEmpty())
		SaveLayout();
}

void SDetailsWorkspace::CreateNewLayout(FString LayoutName)
{
	if (LayoutName.TrimStartAndEnd().IsEmpty())
		return;
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	Collection->WorkspaceLayouts.Add(LayoutName, FDetailsWorkspaceLayout());
	WorkingLayoutName = LayoutName;

	ResetLayoutToInitial();
}

void SDetailsWorkspace::CreateNewLayoutByCopyingCurrent(FString LayoutName)
{
	if (LayoutName.TrimStartAndEnd().IsEmpty())
		return;
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	FDetailsWorkspaceLayout Value;
	DumpCurrentLayout(Value);
	
	Collection->WorkspaceLayouts.Add(LayoutName, Value);
	WorkingLayoutName = LayoutName;
}
 
void SDetailsWorkspace::SwitchLayout(FString TargetLayoutName, bool bSaveBeforeSwitching)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();

	auto Found = Collection->WorkspaceLayouts.Find(TargetLayoutName);
	if (!ensure(Found))
	{
		return;
	}

	if (bSaveBeforeSwitching)
	{
		SaveLayout();
	}

	Restore(*Found);
	WorkingLayoutName = TargetLayoutName;
}

bool SDetailsWorkspace::IsObservingObject(UObject* Object) const
{
	for(auto Detail : SpawnedDetails)
	{
		auto Pinned = Detail.Pin();
		if (Pinned && Pinned->GetObject(false) == Object)
		{
			return true;
		}
	}
	return false;
}

void SDetailsWorkspace::CreateRenameCurrentLayoutWindow()
{
	auto Window = SNew(SLayoutNameInputWindow)
        .Title(LOCTEXT("RenameLayoutDialogTitle", "Input new name for layout"))
        .ClientSize(FVector2D(300, 120))
        .ButtonText(LOCTEXT("Rename", "Rename"))
        .OnConfirmed(SLayoutNameInputWindow::FOnNameInputConfirmed::CreateSP(this, &SDetailsWorkspace::RenameCurrentLayout));
}

void SDetailsWorkspace::DeleteLayoutWithDialog(FString LayoutName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	if (EAppReturnType::Ok == FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteLayoutDialogContent", "Are you sure to delete the layout?")))
	{
		Collection->WorkspaceLayouts.Remove(LayoutName);
		if (WorkingLayoutName == LayoutName)
		{
			ResetLayoutToInitial();
			WorkingLayoutName = TEXT("");
		}
	}
}

void SDetailsWorkspace::CreateNewLayoutWithDialog()
{
	auto Window = SNew(SLayoutNameInputWindow)
        .Title(LOCTEXT("CreateNewLayoutDialogTitle", "Input name for new layout"))
        .ClientSize(FVector2D(300, 120))
        .ButtonText(LOCTEXT("Add", "Add"))
        .OnConfirmed(SLayoutNameInputWindow::FOnNameInputConfirmed::CreateSP(this, &SDetailsWorkspace::CreateNewLayout));	
}

void SDetailsWorkspace::CreateNewLayoutByCopyingWithDialog()
{
	auto Window = SNew(SLayoutNameInputWindow)
        .Title(LOCTEXT("CreateNewLayoutDialogTitle", "Input name for new layout"))
        .ClientSize(FVector2D(300, 120))
        .ButtonText(LOCTEXT("Add", "Add"))
        .OnConfirmed(SLayoutNameInputWindow::FOnNameInputConfirmed::CreateSP(this, &SDetailsWorkspace::CreateNewLayoutByCopyingCurrent));
}
void SDetailsWorkspace::RenameCurrentLayout(FString NewName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();

	if (auto Current = Collection->WorkspaceLayouts.Find(WorkingLayoutName))
	{
		auto Copy = *Current;
		Collection->Modify();
		Collection->WorkspaceLayouts.Remove(WorkingLayoutName);
		Collection->WorkspaceLayouts.Add(NewName, Copy);
		// Directly save the file.
		// It's stupid to ask user to save a transient file.  
		FEditorFileUtils::PromptForCheckoutAndSave({Collection->GetPackage()}, true, false);
	}

	WorkingLayoutName = NewName;
}

FText SDetailsWorkspace::OnGetCurrentLayoutName() const
{
	return FText::FromString(WorkingLayoutName);
}

EVisibility SDetailsWorkspace::DropAreaVisibility() const
{
    return FSlateApplication::Get().IsDragDropping() && OnRecognizeObserveObjectDrop(FSlateApplication::Get().GetDragDroppingContent()) ? EVisibility::Visible : EVisibility::Hidden;
}

FText SDetailsWorkspace::GetLabel() const
{
	return FText::FromString(WorkingLayoutName);
}

void SDetailsWorkspace::Construct(const FArguments& Args, FString InInstanceName, bool bLoadInstanceLastLayout)
{
	this->InstanceName = InInstanceName;
	
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	FDetailsWorkspaceInstanceSettings* Settings = nullptr;
	if (Collection)
	{
		Settings = Collection->InstanceSettings.Find(InstanceName);
	}
	
	SDockTab::Construct(SDockTab::FArguments()
		.TabRole(ETabRole::NomadTab)
		.Label(this, &SDetailsWorkspace::GetLabel)
	);

	SetOnPersistVisualState(FOnPersistVisualState::CreateSP(this, &SDetailsWorkspace::DoPersistVisualState));
	
	TabManager = FGlobalTabmanager::Get()->NewTabManager(SharedThis(this));
	TabManager
	->RegisterTabSpawner(WelcomePageTabID, FOnSpawnTab::CreateSP(this, &SDetailsWorkspace::CreateWelcomeTab))
	.SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId&){ return TSharedPtr<SDockTab>(); }));

	
	auto DropArea = SNew(SOverlay)
		.Visibility(this, &SDetailsWorkspace::DropAreaVisibility)
        + SOverlay::Slot()[
            SNew(STextBlock)
            .Text(LOCTEXT("DropActorHereToViewDetails", "Drop Actor Here To View Details"))
            .Visibility(EVisibility::SelfHitTestInvisible)
        ]
        + SOverlay::Slot()[
	        SNew(SDropTarget)
			    .OnIsRecognized(SDropTarget::FVerifyDrag::CreateSP(this, &SDetailsWorkspace::OnRecognizeObserveObjectDrop))
			    .OnAllowDrop(SDropTarget::FVerifyDrag::CreateSP(this, &SDetailsWorkspace::OnRecognizeObserveObjectDrop))
			    .OnDrop(SDropTarget::FOnDrop::CreateSP(this, &SDetailsWorkspace::OnObserveObjectDrop))
        ];

	SetContent
    (
		SNew(SOverlay)
		+ SOverlay::Slot()[
		    SNew( SVerticalBox )
		    +SVerticalBox::Slot()
            [
            	SNew(SHorizontalBox)
            	+ SHorizontalBox::Slot()[
                    CreateConfigArea(Settings)
                ]
            	.HAlign(HAlign_Fill)
                .FillWidth(1.0f)
            ]
            .Padding(2.0f)
            .AutoHeight()
		    +SVerticalBox::Slot()
		    .FillHeight( 1.0f )
		    [
				SAssignNew(DockingTabsContainer, SBorder)
				.BorderImage(FEditorStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
		    ]
	    ]
	    + SOverlay::Slot() [
			DropArea
	    ]
    );

	
	WorkingLayoutName = "Default";
	if (bLoadInstanceLastLayout)
	{
		if (Settings)
		{
			WorkingLayoutName = Settings->LastLayout;
		}
	}

	if (Collection && Collection->WorkspaceLayouts.Find(WorkingLayoutName))
	{
		RestoreFromLocalUserLayout(WorkingLayoutName);
	}
	else
	{
		ResetLayoutToInitial();
	}
}

void SDetailsWorkspace::ResetLayoutToInitial()
{
	TabManager->CloseAllAreas();
	TSharedRef<FTabManager::FLayout> Layout = InitialLayout();
	DockingTabsContainer->SetContent(
        TabManager->RestoreFrom(Layout, GetParentWindow()).ToSharedRef()
    );
}

SDetailsWorkspace::~SDetailsWorkspace()
{
	UE_LOG(LogTemp, Log, TEXT("FDetailsWorkspaceRootTab Destructed"));
}

void SDetailsWorkspace::RestoreFromLocalUserLayout(FString LayoutName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	if (Collection)
	{
		auto Layout = Collection->WorkspaceLayouts.Find(LayoutName);
		if (Layout)
		{
			Restore(*Layout);
		}
	}
}

void SDetailsWorkspace::Restore(const FDetailsWorkspaceLayout& Profile)
{
	LoadedProfile = Profile;
	auto Layout = FTabManager::FLayout::NewFromString(Profile.LayoutString);
	if (!Layout)
	{
		UE_LOG(LogTemp, Warning, TEXT("Restore failed due to layout parsing failure."));
		return;
	}

	TabManager->CloseAllAreas();
	SpawnedDetails.Empty();

	for(auto& t : Profile.References)
	{
		EnsureRegisterDetailTabSpawner(t.Key);
	}

	DockingTabsContainer->SetContent(
		TabManager->RestoreFrom(Layout.ToSharedRef(), GetParentWindow()).ToSharedRef()
	);
}

void SDetailsWorkspace::SaveLayout()
{
	auto LayoutName = WorkingLayoutName;
	if (LayoutName.TrimStartAndEnd().IsEmpty())
		return;
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	Collection->Modify();
	
	auto& Layout = Collection->WorkspaceLayouts.FindOrAdd(LayoutName);
	DumpCurrentLayout(Layout);

	// Directly save the file.
	// It's stupid to ask user to save a transient file.  
	FEditorFileUtils::PromptForCheckoutAndSave({Collection->GetPackage()}, true, false);
}

void SDetailsWorkspace::DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget)
{
	OutTarget.LayoutString = TabManager->PersistLayout()->ToString();
	for(auto& Tab: SpawnedDetails)
	{
		auto Pinned = Tab.Pin();
		if (Pinned && Pinned->GetObject(false) && TabManager->FindExistingLiveTab(Tab.Pin()->TabID))
		{
			auto& Item = Pinned->GetObserveItem();
			OutTarget.References.Add(Pinned->TabID.TabType, Item);
		}
	}
}

void SDetailsWorkspace::OnDetailTabClosed(TSharedRef<SDockTab> Tab, TWeakPtr<SAnyObjectDetails> Detail)
{
	SpawnedDetails.Remove(Detail);
}

EVisibility SDetailsWorkspace::DeveloperTextVisibility() const
{
	return DeveloperModeCheckerbox->IsChecked() ? EVisibility::SelfHitTestInvisible : EVisibility::Hidden;
}

TSharedRef<SDockTab> SDetailsWorkspace::CreateDocKTabWithDetailView(const FSpawnTabArgs& Args)
{
	auto ID = Args.GetTabId();
	auto NewObjectDetailWidget =
		SNew(SAnyObjectDetails, ID)
		.AutoInspectPIE(AutoPIECheckBox.ToSharedRef(), &SCheckBox::IsChecked)
	;
	
	auto WeakPtrToDetailWidget = TWeakPtr<SAnyObjectDetails>(NewObjectDetailWidget);
	auto Tab =
        SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
		.OnTabClosed(FOnTabClosedCallback::CreateSP(this, &SDetailsWorkspace::OnDetailTabClosed, WeakPtrToDetailWidget))
		.Label_Lambda(
			[WeakPtrToDetailWidget]()
			{
				auto Widget = WeakPtrToDetailWidget.Pin();
				if (!Widget)
					return FText::GetEmpty();
				auto Object = Widget->GetObjectAuto();
				if (Object)
				{
					return FText::FromString(GetPrettyNameForDetailsWorkspaceObject(Object));
				}
				else
				{
					return FText::FromString(TEXT("Empty"));
				}
			}
		);

	Tab->SetContent(
		SNew(SOverlay)
		+ SOverlay::Slot()[
			NewObjectDetailWidget
        ]
        + SOverlay::Slot()[
        	SNew(STextBlock)
        	.Text(FText::FromString(ID.ToString()))
        	.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
        	.ShadowOffset(FVector2D(1.0f, 1.0f))
        	.Visibility(this, &SDetailsWorkspace::DeveloperTextVisibility)
        ].HAlign(HAlign_Center).VAlign(VAlign_Center)
    );
	
	SpawnedDetails.Add(NewObjectDetailWidget);

	if (LoadedProfile.References.Find(ID.TabType))
	{
		auto Item = *LoadedProfile.References.Find(ID.TabType);
		NewObjectDetailWidget->SetObserveItem(Item);
	}

	return Tab;
}

TSharedRef<SDockTab> SDetailsWorkspace::CreateWelcomeTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("WelcomeTabLabel", "Welcome"))
		.Content()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("WelcomeTabContent",
                	"Drop any actor/asset here to start.\n"
                	"If actor/asset has subobject(Component), a button will appear for choosing in config.\n"
                	"\n"
                    "You can freely re-arrange created tabs in this panel.\n"
                	"\n"
                	"Switch layout in config.\n"
                    "\n"
                    "Layout will be saved on you closing the window, or layout switched.\n"
                    "\n"
                    "You can close this Welcome tab. It doesn't do anything.\n"
                ))
            ].HAlign(HAlign_Center).VAlign(VAlign_Center)
		];
}

bool SDetailsWorkspace::OnRecognizeObserveObjectDrop(TSharedPtr<FDragDropOperation> Operation) const
{
	return Operation->IsOfType<FActorDragDropOp>() || Operation->IsOfType<FAssetDragDropOp>();
}

static bool HasDefaultSubObject(UObject* Object)
{
	TArray<UObject*> Objects;
	Object->GetDefaultSubobjects(Objects);
	return  Objects.Num() != 0;
}

FReply SDetailsWorkspace::OnObserveObjectDrop(TSharedPtr<FDragDropOperation> Op)
{
	if (!ensure(OnRecognizeObserveObjectDrop(Op)))
		return FReply::Unhandled();

	auto DealObject = [this](UObject* Object)
	{
		if (!Object)
			return;
		SpawnNewDetailWidgetForObject(Object);
		/*
		if (HasDefaultSubObject(Object))
		{
			SubObjectAddArea->PendingObservedObject = Object;
			ConfigArea->SetExpanded(true);	//So user could see the add button.  
		}
		else
		{
			SpawnNewDetailWidgetForObject(Object);
		}*/
	};
	
	if (Op->IsOfType<FActorDragDropOp>())
	{
		TSharedPtr<FActorDragDropOp> ActorDragDropOperation = StaticCastSharedPtr<FActorDragDropOp>(Op);

		for (auto& Actor : ActorDragDropOperation->Actors)
		{
			DealObject(Actor.Get());			
		}
	}
	else if (Op->IsOfType<FAssetDragDropOp>())
	{
		TSharedPtr<FAssetDragDropOp> AssetDragDropOperation = StaticCastSharedPtr<FAssetDragDropOp>(Op);
		for (auto& Asset : AssetDragDropOperation->GetAssets())
		{
			DealObject(Asset.GetAsset());
		}
	}
	return FReply::Handled();
}

FName SDetailsWorkspace::GetUnusedTabTypeName() const
{
	int Index = 0;
	// Find an available TabID, or a closed tab.  
	while (true)
	{
		auto TabType = FName(FString::Printf(TEXT("%s%d"), *NormalTabID.ToString(), Index));
		
		const bool bOpened = TabManager->FindExistingLiveTab(TabType) != nullptr;
		if (bOpened)
		{
			Index++;
		}
		else
		{
			return TabType;
		}
	}
}

void SDetailsWorkspace::EnsureRegisterDetailTabSpawner(FName TabTypeID)
{
	if (!TabManager->HasTabSpawner(TabTypeID))
	{
		auto& TabSpawner = TabManager->RegisterTabSpawner(TabTypeID, FOnSpawnTab::CreateSP(this, &SDetailsWorkspace::CreateDocKTabWithDetailView));

		// SetReuseTabMethod is required:
		// By default, FTabManager doesn't allow multi same TabTypeID exists same time.
		// And during switching layout, this could happen, causing switching failure. (Even if we call TabManager->CloseAllAreas(), the actual close is delayed, so the tab can't be released)
		// Bind any thing to SetReuseTabMethod let FTabManager to ignore this check.    
		TabSpawner.SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId&)
        {
            return TSharedPtr<SDockTab>();
        }));
	}
}

void SDetailsWorkspace::SpawnNewDetailWidgetForObject(UObject* InObject)
{
	if (!ensure(InObject))
	{
		return;
	}

	if (IsObservingObject(InObject))
	{
		return;
	}
	
	auto TabTypeID = GetUnusedTabTypeName();
	EnsureRegisterDetailTabSpawner(TabTypeID);
	LoadedProfile.References.Add(TabTypeID, FDetailsWorkspaceObservedItem::From(InObject));
	TabManager->TryInvokeTab(TabTypeID);
}


TSharedRef<SDetailsWorkspace> CreateDetailsWorkSpace(FString InstanceName, bool bLoadInstanceLastLayout)
{
	return SNew(SDetailsWorkspace, InstanceName, bLoadInstanceLastLayout);
}

#undef LOCTEXT_NAMESPACE

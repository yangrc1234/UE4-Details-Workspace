#include "DetailsWorkspaceRootTab.h"


#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "SlateOptMacros.h"
#include "FilterableActorPicker.h"
#include "IContentBrowserSingleton.h"
#include "SDropTarget.h"
#include "DragAndDrop/ActorDragDropOp.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

TSharedRef<SWidget> CreateContent(TSharedRef<SDockTab> Root, TArray<FInspectItem> InspectItems)
{
	//Create tab layout for InspectItems.
	auto TabManager = FGlobalTabmanager::Get()->NewTabManager(Root);
	
	return SNew(STextBlock).Text(LOCTEXT("TestContent", "Test Content"));	
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAnyObjectDetails::Construct(const FArguments& InArgs, FTabId InTabID)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ComponentsAndActorsUseNameArea;
	TabID = InTabID;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	ChildSlot[
		SNew(SOverlay)
        + SOverlay::Slot()[
			SNew(STextBlock)
			.Text_Lambda(
				[Self = SharedThis(this)]()
				{
					if (Self->GetLazyObjectPtr().IsNull())
					{
						return LOCTEXT("DetailObjectDestroyedHint", "Object is detsroyed. Close the tab");
					}
					else if (!Self->GetLazyObjectPtr().IsValid())
					{
						return LOCTEXT("DetailObjectNotLoaded", "Object is not loaded yet.");
					}
					else
					{
						return FText::GetEmpty();
					}
				}
			)
        ]
		+ SOverlay::Slot()[
            DetailsView->AsShared()
        ]
	];
}

void SAnyObjectDetails::SetObjectLazyPtr(TLazyObjectPtr<UObject> Value)
{
	LazyObjectRef = Value;
	DetailsView->SetObject(Value.Get());
}

void SAnyObjectDetails::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	if (bLoaded != GetLazyObjectPtr().IsValid())
	{
		bLoaded = GetLazyObjectPtr().IsValid();
		DetailsView->SetObject(GetLazyObjectPtr().Get());
	}
}

UObject* UDetailsWorkspaceProfileCollectionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
                                                                     EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDetailsWorkspaceProfile>(InParent, Class, Name, Flags);
}

static FName WelcomePageTabID = TEXT("Welcome");
static FName NormalTabID = TEXT("Normal");

static TSharedRef<FTabManager::FLayout> InitialLayout()
{
	return FTabManager::NewLayout(TEXT("TestLayout"))
        ->AddArea
        (
            FTabManager::NewPrimaryArea()
            ->Split
            (
                FTabManager::NewStack()
                ->SetForegroundTab(NormalTabID)
                ->AddTab(WelcomePageTabID, ETabState::OpenedTab)
            )
        );
}

void SDetailsWorkspaceRootTab::DoPersistVisualState()
{
	auto LocalCollection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	if (!ensure(!InstanceName.TrimStartAndEnd().IsEmpty() && !WorkingLayoutName.TrimStartAndEnd().IsEmpty()))
		return;

	// Remember opened layout name. 
	if (LocalCollection)
	{
		LocalCollection->Modify();
		LocalCollection->InstanceLastLayout.FindOrAdd(InstanceName, WorkingLayoutName);
	}

	// Save the actual layout.  
	SaveLayout(WorkingLayoutName);
}

FReply SDetailsWorkspaceRootTab::OnCreateNewLayoutDialogConfirmed(FString LayoutName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	Collection->WorkspaceLayouts.Add(LayoutName, FDetailsWorkspaceLayout());
	return FReply::Handled();
}

static FString GetPrettyNameForObject(UObject* Object)
{
	check(Object);
	FString Result;
	if (auto Actor = Cast<AActor>(Object))
		Result = Actor->GetActorLabel();
	else
		Result = Object->GetName();
	if (Object->HasAnyFlags(RF_Transient))
	{
		Result = TEXT("(Transient)") + Result;
	}
	return Result;
}
 
void SDetailsWorkspaceRootTab::CreateNewLayoutWithDialog()
{
	TSharedRef<SWindow> WidgetWindow = SNew(SWindow)
        .Title(LOCTEXT("CreateNewLayoutDialogTitle", "Input name for new dialog"))
        .ClientSize(FVector2D(500, 600));

	static FText InputContent;
	
	WidgetWindow->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()[
			SNew(SEditableTextBox)
			.OnTextCommitted_Lambda([](const FText& Value, ETextCommit::Type)
			{
				InputContent = Value;
			})
			.Text_Lambda([](){return InputContent;})
		]
		+ SVerticalBox::Slot()[
			SNew(SButton)
			.OnClicked(FOnClicked::CreateSP(this, &SDetailsWorkspaceRootTab::OnCreateNewLayoutDialogConfirmed, InputContent.ToString()))
        ]
	);

	GEditor->EditorAddModalWindow(WidgetWindow);
}

FReply SDetailsWorkspaceRootTab::OnLayoutDeleteButton(FString LayoutName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	if (EAppMsgType::Ok == FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteLayoutDialogContent", "Are you sure to delete the layout?")))
	{
		Collection->WorkspaceLayouts.Remove(LayoutName);
		if (WorkingLayoutName == LayoutName)
		{
			WorkingLayoutName = TEXT("");
		}
	}
	return FReply::Handled();
}

void SDetailsWorkspaceRootTab::SwitchLayout(FString TargetLayoutName, bool bSaveBeforeSwitching)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();

	auto Found = Collection->WorkspaceLayouts.Find(TargetLayoutName);
	if (!ensure(Found))
	{
		return;
	}

	if (bSaveBeforeSwitching)
	{
		SaveLayout(TargetLayoutName);
	}

	Restore(*Found);
}

TSharedRef<SWidget> SDetailsWorkspaceRootTab::OnAddTargetsMenu()
{
	if (!ensure(PendingObservedObject.Get()))	// button should be hidden.
		return SNew(SBorder);
	
	FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddThisObject", "Add This Object"),
		FText::GetEmpty(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SDetailsWorkspaceRootTab::SpawnNewDetailWidgetForObject, PendingObservedObject.Get()))
	);

	TArray<UObject*> SubObjects;
	PendingObservedObject.Get()->GetDefaultSubobjects(SubObjects);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("SubObjects", "Sub Objects"));
	for(auto& t : SubObjects)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Name"), FText::FromString(GetPrettyNameForObject(t)));
		
		MenuBuilder.AddMenuEntry(
			FText::Format(LOCTEXT("AddSubObjectFormat", "Add {Name}"), Arguments),
            FText::GetEmpty(),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateSP(this, &SDetailsWorkspaceRootTab::SpawnNewDetailWidgetForObject, t))
        );
	}
	
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> SDetailsWorkspaceRootTab::OnGetLayoutSelectMenuContent()
{	
	FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

	// First area, list all available layout items, and corresponding "copy" and "delete" button.
	MenuBuilder.BeginSection(NAME_None, LOCTEXT("NewLayout", "Create New Layout"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("New","New"),
		LOCTEXT("CreateNewLayoutTooltip", "Create new layout"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateSP(this, &SDetailsWorkspaceRootTab::CreateNewLayoutWithDialog))
	);
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("Layouts", "Layouts"));
	{
		auto LocalCollection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
		TArray<FString> LayoutNames;
		LocalCollection->WorkspaceLayouts.GetKeys(LayoutNames);
		TSharedPtr<SScrollBox> ContentBox;
		SAssignNew(ContentBox, SScrollBox);
		for(auto& LayoutName : LayoutNames)
		{
			ContentBox->AddSlot()[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
                        SNew(STextBlock).Text(FText::FromString(LayoutName))
                    ]
                    + SOverlay::Slot()
                    [
						SNew(SButton).OnClicked_Lambda(
							[Self = SharedThis(this), LayoutName]()
							{
								Self->SwitchLayout(LayoutName, true);
								return FReply::Handled();
							}
						)
					]	
				]
				.FillWidth(1.0f)
				+ SHorizontalBox::Slot()[
					SNew(SButton)
					.Text(LOCTEXT("Delete", "Delete"))
					.OnClicked(FOnClicked::CreateSP(this, &SDetailsWorkspaceRootTab::OnLayoutDeleteButton, LayoutName))
                ]
			];
		}

		MenuBuilder.AddWidget(
			ContentBox.ToSharedRef(), FText::GetEmpty(), true);
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

FText SDetailsWorkspaceRootTab::OnGetCurrentLayoutName() const
{
	return FText::FromString(WorkingLayoutName);
}

EVisibility SDetailsWorkspaceRootTab::AddActorComponentsButtonVisibility() const
{
	if (!PendingObservedObject.Get() || !PendingObservedObject->IsA(AActor::StaticClass()))
		return EVisibility::Collapsed;
	return EVisibility::Visible;
}

FReply SDetailsWorkspaceRootTab::OnAddActorComponentsClicked()
{
	if (!ensure(PendingObservedObject.Get()) || !ensure(PendingObservedObject->IsA(AActor::StaticClass())))
		return FReply::Handled();

	auto Actor = Cast<AActor>(PendingObservedObject);
	for (auto& t : Actor->GetComponents())
	{
		SpawnNewDetailWidgetForObject(t);
	}
	return FReply::Handled();
}

FReply SDetailsWorkspaceRootTab::OnAddObjectButtonClicked()
{
	if (!ensure(PendingObservedObject.Get()))
		return FReply::Handled();

	SpawnNewDetailWidgetForObject(PendingObservedObject.Get());
	return FReply::Handled();
}

EVisibility SDetailsWorkspaceRootTab::AddObjectButtonVisibility() const
{
	return PendingObservedObject.Get() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SDetailsWorkspaceRootTab::PendingObservedObjectName() const
{
	if ( PendingObservedObject.Get())
	{
		return FText::FromString(GetPrettyNameForObject(PendingObservedObject.Get()));
	}
	else
	{
		return FText::GetEmpty();	
	}
}

TSharedRef<SWidget> SDetailsWorkspaceRootTab::CreateAddObjectToObserveArea()
{
	auto DropTarget = SNew(SDropTarget)
        .OnIsRecognized(SDropTarget::FVerifyDrag::CreateSP(this, &SDetailsWorkspaceRootTab::OnRecognizeObserveObjectDrop))
        .OnAllowDrop(SDropTarget::FVerifyDrag::CreateSP(this, &SDetailsWorkspaceRootTab::OnRecognizeObserveObjectDrop))
        .OnDrop(SDropTarget::FOnDrop::CreateSP(this, &SDetailsWorkspaceRootTab::OnObserveObjectDrop));
	
	auto DropArea = SNew(SOverlay)	
		+ SOverlay::Slot()[
	        SNew(STextBlock)
	        .Text(LOCTEXT("DropActorHereToViewDetails", "Drop Actor Here To View Details"))
	    ]
		+ SOverlay::Slot()[
			DropTarget
		];

	auto AddComponentsButton =
        SNew(SButton)
        .Text(LOCTEXT("AddActorComponentsButton", "Add Components"))
        .OnClicked(FOnClicked::CreateSP(this, &SDetailsWorkspaceRootTab::OnAddActorComponentsClicked))
        .Visibility(this, &SDetailsWorkspaceRootTab::AddActorComponentsButtonVisibility);

	auto AddObjectButton = SNew(SComboButton)
		.OnGetMenuContent(FOnGetContent::CreateSP(this, &SDetailsWorkspaceRootTab::OnAddTargetsMenu))
		.ButtonContent()[
			SNew(STextBlock)
			.Text(LOCTEXT("AddButtonLabel", "Add.."))
		]
        .Visibility(this, &SDetailsWorkspaceRootTab::AddObjectButtonVisibility);
/*
	auto AddObjectButton =
        SNew(SButton)
        .Text(LOCTEXT("AddObjectButton", "Add Object"))
        .OnClicked(FOnClicked::CreateSP(this, &SDetailsWorkspaceRootTab::OnAddObjectButtonClicked))
        .Visibility(this, &SDetailsWorkspaceRootTab::AddObjectButtonVisibility);
*/
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot()[
		DropArea
	]
	+ SVerticalBox::Slot()[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(STextBlock)
			.Text(this, &SDetailsWorkspaceRootTab::PendingObservedObjectName)
		]
	   /* + SHorizontalBox::Slot()
	    [
	        AddComponentsButton
	    ]*/
		+ SHorizontalBox::Slot()
	    [
			AddObjectButton
	    ]
	];
}

void SDetailsWorkspaceRootTab::Construct(const FArguments& Args, TSharedPtr<SWindow> Window, FString InInstanceName, bool bLoadInstanceLastLayout)
{
	this->InstanceName = InInstanceName;
	
	SDockTab::Construct(SDockTab::FArguments()
		.TabRole(ETabRole::MajorTab)
	);

	SetOnPersistVisualState(FOnPersistVisualState::CreateSP(this, &SDetailsWorkspaceRootTab::DoPersistVisualState));
	
	TabManager = FGlobalTabmanager::Get()->NewTabManager(SharedThis(this));
	TabManager->RegisterTabSpawner(WelcomePageTabID, FOnSpawnTab::CreateSP(this, &SDetailsWorkspaceRootTab::CreateWelcomeTab));

	SetContent
    (
	    SNew( SVerticalBox )
	    +SVerticalBox::Slot()
	    .AutoHeight()
	    [
	    	SNew(SHorizontalBox)
	    	+ SHorizontalBox::Slot()[
			    SNew(SComboButton)
		        .OnGetMenuContent(
		            FOnGetContent::CreateSP(this, &SDetailsWorkspaceRootTab::OnGetLayoutSelectMenuContent)
		        )
		        .ButtonContent()[
	                SNew(SHorizontalBox)
	                +SHorizontalBox::Slot()
	                .FillWidth(1)
	                .VAlign(VAlign_Center)
	                [
	                    // Show the name of the asset or actor
	                    SNew(STextBlock)
	                    .Text(this, &SDetailsWorkspaceRootTab::OnGetCurrentLayoutName)
	                ]
		        ]
	        ]
	        + SHorizontalBox::Slot()[
	        	CreateAddObjectToObserveArea()
	        ]
	    ]
	    +SVerticalBox::Slot()
	    .FillHeight( 1.0f )
	    [
			SAssignNew(DockingTabsContainer, SBorder)
	    ]
    );

	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	WorkingLayoutName = "Default";
	if (bLoadInstanceLastLayout)
	{
		if (Collection)
		{
			if (auto FoundLastLayoutName = Collection->InstanceLastLayout.Find(InstanceName))
			{
				WorkingLayoutName = *FoundLastLayoutName;
			}
		}
	}

	if (Collection && Collection->WorkspaceLayouts.Find(WorkingLayoutName))
	{
		RestoreFromLocalUserLayout(WorkingLayoutName);
	}
	else
	{
		TSharedRef<FTabManager::FLayout> Layout = InitialLayout();
		DockingTabsContainer->SetContent(
            TabManager->RestoreFrom(Layout, Window).ToSharedRef()
        );
	}
}

UDetailsWorkspaceProfile* UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile()
{
	auto Folder = FPaths::Combine(TEXT("/Game/Developers/"), FPaths::GameUserDeveloperFolderName());
	FString AssetName = TEXT("DetailsWorkspaceLayoutProfile");

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();

	auto LoadProfile = LoadObject<UDetailsWorkspaceProfile>(nullptr, *(Folder / AssetName));

	if (!LoadProfile)
	{
		LoadProfile = Cast<UDetailsWorkspaceProfile>(
            AssetToolsModule.Get().CreateAsset(
            AssetName
                , Folder
                , UDetailsWorkspaceProfile::StaticClass()
                , Cast<UFactory>(UDetailsWorkspaceProfileCollectionFactory::StaticClass()->GetDefaultObject()))
            );
		if (!LoadProfile)
		{
			UE_LOG(LogTemp, Error,  TEXT("Create DWLayoutCollection failed"));
		}
	}

	return LoadProfile;
}

FReply SDetailsWorkspaceRootTab::OnNewTabClicked()
{
	//auto NewTab = AllocateNewTab();
	return FReply::Handled();
}

SDetailsWorkspaceRootTab::~SDetailsWorkspaceRootTab()
{
	UE_LOG(LogTemp, Log, TEXT("FDetailsWorkspaceRootTab Destructed"));
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SDetailsWorkspaceRootTab::RestoreFromLocalUserLayout(FString LayoutName)
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

void SDetailsWorkspaceRootTab::Restore(const FDetailsWorkspaceLayout& Profile)
{
	LoadedProfile = Profile;
	auto Layout = FTabManager::FLayout::NewFromString(Profile.LayoutString);
	if (!Layout)
	{
		UE_LOG(LogTemp, Warning, TEXT("Restore failed due to layout parsing failure."));
		return;
	}

	for(auto& t : Profile.References)
	{
		if (TabManager->HasTabSpawner(t.Key))
		{
			TabManager->UnregisterTabSpawner(t.Key);
		}
		TabManager->RegisterTabSpawner(t.Key, FOnSpawnTab::CreateSP(this, &SDetailsWorkspaceRootTab::CreateDocKTabWithDetailView));		
	}

	DockingTabsContainer->SetContent(
		TabManager->RestoreFrom(Layout.ToSharedRef(), GetParentWindow()).ToSharedRef()
	);
}

UDetailsWorkspaceProfileCollectionFactory::UDetailsWorkspaceProfileCollectionFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	SupportedClass = UDetailsWorkspaceProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = false;
}

void SDetailsWorkspaceRootTab::SaveLayout(FString LayoutName)
{
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

void SDetailsWorkspaceRootTab::SaveLayoutWithDialog(FString LayoutName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	if (!Collection)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveProfileFailed", "Failed to save profile. Check your disk size/write permission"));
		return;
	}

	if (Collection->WorkspaceLayouts.Contains(LayoutName))
	{
		if (EAppMsgType::Ok != FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("OverwriteProfileConfirmDialogContent", "Overwrite existing layout?")))
		{
			return;
		}
	}

	SaveLayout(LayoutName);
}

void SDetailsWorkspaceRootTab::DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget)
{
	OutTarget.LayoutString = TabManager->PersistLayout()->ToString();
	for(auto& Tab: SpawnedDetails)
	{
		auto Pinned = Tab.Pin();
		if (Pinned && !Pinned->GetLazyObjectPtr().IsNull() && TabManager->FindExistingLiveTab(Tab.Pin()->TabID))
		{
			auto Object = Pinned->GetLazyObjectPtr().Get();
			if (!Object->HasAnyFlags(RF_Transient))
				OutTarget.References.Add(Pinned->TabID.TabType, FDetailWorkspaceObservedItem::From(Object));
		}
	}
}

TSharedRef<SDockTab> SDetailsWorkspaceRootTab::CreateDocKTabWithDetailView(const FSpawnTabArgs& Args)
{
	auto ID = Args.GetTabId();
	auto NewObjectDetailWidget = SNew(SAnyObjectDetails, ID);
	auto WeakPtrToDetailWidget = TWeakPtr<SAnyObjectDetails>(NewObjectDetailWidget);
	auto Tab =
        SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
		.Label_Lambda(
			[WeakPtrToDetailWidget]()
			{
				auto Widget = WeakPtrToDetailWidget.Pin();
				if (!Widget)
					return FText::GetEmpty();
				auto Object = Widget->GetLazyObjectPtr();
				if (Object.IsValid())
				{
					return FText::FromString(GetPrettyNameForObject(Object.Get()));
				}else
				{
					return FText::FromString(TEXT("Empty"));
				}
			}
		);

	Tab->SetContent(
        NewObjectDetailWidget
    );
	
	SpawnedDetails.Add(NewObjectDetailWidget);

	if (LoadedProfile.References.Find(ID.TabType))
	{
		auto WeakPtr = *LoadedProfile.References.Find(ID.TabType);
		NewObjectDetailWidget->SetObjectLazyPtr(WeakPtr.Resolve());
	}

	return Tab;
}

TSharedRef<SDockTab> SDetailsWorkspaceRootTab::CreateWelcomeTab(const FSpawnTabArgs& Args)
{	
	return SNew(SDockTab)
		.TabRole(ETabRole::PanelTab)
		.Label(LOCTEXT("WelcomeTabLabel", "Welcome"))
		.Content()[
			SNew(SOverlay)
		    + SOverlay::Slot()[
		        SNew(STextBlock)
		        .Text(LOCTEXT("DropActorHereToViewDetails", "Drop Actor Here To View Details"))
		    ]
		];	
}

TSharedPtr<SDockTab> SDetailsWorkspaceRootTab::AllocateNewTab(FTabId &OutTabID)
{
	return TabManager->TryInvokeTab(GetUnusedTabTypeName());		
}

bool SDetailsWorkspaceRootTab::OnRecognizeObserveObjectDrop(TSharedPtr<FDragDropOperation> Operation)
{
	return Operation->IsOfType<FActorDragDropOp>();
}

FReply SDetailsWorkspaceRootTab::OnObserveObjectDrop(TSharedPtr<FDragDropOperation> Op)
{
	if (!OnRecognizeObserveObjectDrop(Op))
		return FReply::Unhandled();
	TSharedPtr<FActorDragDropOp> ActorDragDropOperation = StaticCastSharedPtr<FActorDragDropOp>(Op);

	for (auto& Actor : ActorDragDropOperation->Actors)
	{
		if (Actor.IsValid())
		{
			PendingObservedObject = Actor;
			//SpawnNewDetailWidgetForObject(Actor.Get());
		}
	}
	return FReply::Handled();
}

FName SDetailsWorkspaceRootTab::GetUnusedTabTypeName() const
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

void SDetailsWorkspaceRootTab::SpawnNewDetailWidgetForObject(UObject* InObject)
{
	if (!ensure(InObject))
	{
		return;
	}
	
	auto TabTypeID = GetUnusedTabTypeName();

	if (TabManager->HasTabSpawner(TabTypeID))
	{
		TabManager->UnregisterTabSpawner(TabTypeID);
	}
	TabManager->RegisterTabSpawner(TabTypeID, FOnSpawnTab::CreateSP(this, &SDetailsWorkspaceRootTab::CreateDocKTabWithDetailView));
	
	LoadedProfile.References.Add(TabTypeID, FDetailWorkspaceObservedItem::From(InObject));
	TabManager->TryInvokeTab(TabTypeID);
}


TSharedRef<SDockTab> CreateDetailsWorkSpace(TSharedPtr<SWindow> Window, FString InstanceName, bool bLoadInstanceLastLayout)
{
	return SNew(SDetailsWorkspaceRootTab, Window, InstanceName, bLoadInstanceLastLayout);
}

#undef LOCTEXT_NAMESPACE

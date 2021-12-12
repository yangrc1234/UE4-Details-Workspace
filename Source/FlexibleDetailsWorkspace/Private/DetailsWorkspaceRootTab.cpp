#include "DetailsWorkspaceRootTab.h"

#include "Widgets/Views/SListView.h"
#include "AssetToolsModule.h"
#include "FileHelpers.h"
#include "SlateOptMacros.h"
#include "FilterableActorPicker.h"
#include "IContentBrowserSingleton.h"
#include "SDropTarget.h"
#include "DragAndDrop/ActorDragDropOp.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SGridPanel.h"
#include "HAL/PlatformApplicationMisc.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"


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
	if (Object->GetWorld() && Object->GetWorld()->WorldType == EWorldType::PIE)
	{
		Result = TEXT("(PIE)") + Result;
	}
	return Result;
}

class SLayoutNameInputWindow : public SWindow
{
public:
	DECLARE_DELEGATE_OneParam(FOnNameInputConfirmed, FText);
	
	SLATE_BEGIN_ARGS(SLayoutNameInputWindow)
	{}
	SLATE_ATTRIBUTE( FText, Title )
    SLATE_ATTRIBUTE( FText, ButtonText )
    SLATE_ARGUMENT( FVector2D, ClientSize )    
    SLATE_EVENT( FOnNameInputConfirmed, OnConfirmed )
    SLATE_END_ARGS()
	
    FText GetText() const
	{
		return InputContent;
	}
	
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& Args)
	{
		SWindow::Construct(SWindow::FArguments()
		.Title(Args._Title)
		.ClientSize(Args._ClientSize)	
		);
	
		OnConfirmed = Args._OnConfirmed;
		
		SetContent(
	        SNew(SVerticalBox)
	        + SVerticalBox::Slot()[
	            SNew(SEditableTextBox)
	            .HintText(LOCTEXT("NewLayoutNameEditableTextHintText", "New layout name.."))
	            .OnTextChanged(FOnTextChanged::CreateSP(this, &SLayoutNameInputWindow::OnTextChanged))
	            .Text( this, &SLayoutNameInputWindow::GetText)
	        ]
	        .AutoHeight()
	        .HAlign(HAlign_Center)
	        .Padding(0.0f, 5.0f)
	        + SVerticalBox::Slot()[
	            SNew(SButton)
	            .OnClicked(FOnClicked::CreateSP(this, &SLayoutNameInputWindow::OnConfirmClicked))
	            .Text(Args._ButtonText)
	        ]
	        .AutoHeight()
	        .HAlign(HAlign_Center)
	        .Padding(0.0f, 5.0f)
	    );
		
		GEditor->EditorAddModalWindow(SharedThis(this));	
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:
	FReply OnConfirmClicked()
	{
		OnConfirmed.ExecuteIfBound(InputContent);
		RequestDestroyWindow();
		return FReply::Handled();
	}
	void OnTextChanged(const FText& Input)
	{
		InputContent = Input;
	}
	FOnNameInputConfirmed OnConfirmed;
	FText InputContent;
};


class SAnyObjectDetails : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAnyObjectDetails)
	{}
	SLATE_ATTRIBUTE(bool, AutoInspectPIE)
	SLATE_END_ARGS()

	FReply OnSelectObjectClicked()
	{
		auto Object = ObjectValue.Resolve(GEditor->IsPlayingSessionInEditor());
		if (Object)
		{
			GEditor->SelectNone(false, true, false);
			if (Object->IsA(AActor::StaticClass()))
			{
				GEditor->SelectActor(Cast<AActor>(Object), true, true);
			}
			else if (Object->IsA(UActorComponent::StaticClass()))
			{
				auto Component = Cast<UActorComponent>(Object);
				GEditor->SelectActor(Component->GetOwner(), true, true);
				GEditor->SelectComponent(Component, true, true);
			}
			else
			{
				TArray<UObject*> Temp;
				Temp.Add(Object);
				GEditor->SyncBrowserToObjects(Temp);
			}
		}
		return FReply::Handled();
	}

	FText GetHintText() const
	{
		if (ObjectValue.SavedObject.IsNull())
		{
			return LOCTEXT("DetailObjectDestroyedHint", "Object is detsroyed. Close the tab");
		}
		else if (!ObjectValue.SavedObject.IsValid())
		{
			return LOCTEXT("DetailObjectNotLoaded", "Object is not loaded yet.");
		}
		else
		{
			return FText::GetEmpty();
		}
	}
	
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
    void Construct(const FArguments& InArgs, FTabId InTabID)
	{
		bAutoInspectPIE = InArgs._AutoInspectPIE;
		
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		FDetailsViewArgs DetailsViewArgs;
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
		DetailsViewArgs.bShowPropertyMatrixButton = false;
		TabID = InTabID;
		DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
		ChildSlot
        [
            SNew(SOverlay)
            + SOverlay::Slot()[
                SNew(STextBlock)
                .Text(this, &SAnyObjectDetails::GetHintText)
                .Visibility(EVisibility::SelfHitTestInvisible)
            ].HAlign(HAlign_Center).VAlign(VAlign_Center)
            + SOverlay::Slot()[
            	SNew(SVerticalBox)
            	+ SVerticalBox::Slot()[
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()[
						SNew(SButton)
						.OnClicked(FOnClicked::CreateSP(this, &SAnyObjectDetails::OnSelectObjectClicked))
						.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
						[
							SNew(SImage)
	                        .Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
	                    ]
	                ]
                    .AutoWidth()
                ]
                .AutoHeight()
            	+ SVerticalBox::Slot()[
					DetailsView->AsShared()
                ]
            ]
        ];
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	UObject* GetObject(bool bPIE) { return ObjectValue.Resolve(bPIE); }

	UObject* GetObjectAuto()
	{
		const bool bWantPIE = bAutoInspectPIE.Get() && GEditor->IsPlayingSessionInEditor();
		return ObjectValue.Resolve(bWantPIE);
	}
	
	void SetObjectLazyPtr(FDetailWorkspaceObservedItem Value)
	{
		ObjectValue = Value;
	}
	
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		auto Expected = GetObjectAuto();

		if (CurrentWatching != Expected)
		{
			CurrentWatching = Expected;
			DetailsView->SetObject(Expected);
		}
	}
	FTabId TabID;

private:
	TWeakObjectPtr<UObject> CurrentWatching;
	TAttribute<bool> bAutoInspectPIE;
	FDetailWorkspaceObservedItem ObjectValue;
	TSharedPtr<class IDetailsView> DetailsView;
	bool bLoaded = false;
};


class SLayoutSelectionComboButton : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnLayoutOperation, FString);

	SLATE_BEGIN_ARGS(SLayoutSelectionComboButton) {}
		SLATE_EVENT(FOnLayoutOperation, OnLayoutDeleteClicked)
		SLATE_EVENT(FOnLayoutOperation, OnLayoutSelected)
		SLATE_EVENT(FSimpleDelegate, OnRenameLayout)
		SLATE_EVENT(FSimpleDelegate, OnCreateNewLayout)
		SLATE_EVENT(FSimpleDelegate, OnCreateNewLayoutByCopying)
		SLATE_ATTRIBUTE(FText, SelectedLayoutName)
	SLATE_END_ARGS()

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& Arguments)
	{
		ChildSlot[
			SNew(SHorizontalBox)
	        + SHorizontalBox::Slot()[
                SAssignNew(ComboButton,  SComboButton)
                    .OnGetMenuContent(
                        FOnGetContent::CreateSP(this, &SLayoutSelectionComboButton::OnGetLayoutSelectMenuContent)
                    )
                    .ButtonContent()
                    [
                        SNew(STextBlock).Text(Arguments._SelectedLayoutName)
                        .MinDesiredWidth(150.0f)
                    ]
                    .VAlign(VAlign_Center)
            ]
	        .AutoWidth()
		];

		OnRenameLayout = Arguments._OnRenameLayout;
		OnCreateNewLayout = Arguments._OnCreateNewLayout;
		OnCreateNewLayoutByCopying = Arguments._OnCreateNewLayoutByCopying;
		OnLayoutDeleteClicked = Arguments._OnLayoutDeleteClicked;
		OnLayoutSelected = Arguments._OnLayoutSelected;	
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:
	TSharedPtr<SComboButton> ComboButton;
	FSimpleDelegate OnRenameLayout;
	FSimpleDelegate OnCreateNewLayout;
	FSimpleDelegate OnCreateNewLayoutByCopying;
	FOnLayoutOperation OnLayoutDeleteClicked;
	FOnLayoutOperation OnLayoutSelected;
  
	struct FLayoutRowItem : TSharedFromThis<FLayoutRowItem>
	{
		FLayoutRowItem(FString InLayoutName) {	LayoutName = InLayoutName;	}
		FString LayoutName;
	}; 
	typedef SListView< TSharedPtr< FLayoutRowItem > > SLayoutListView;
	TArray< TSharedPtr<FLayoutRowItem> > LayoutListItems;	
	
	void OnRowItemSelected( TSharedPtr<FLayoutRowItem > Layout, ESelectInfo::Type SelectInfo )
	{
		OnLayoutSelected.ExecuteIfBound(Layout->LayoutName);
		ComboButton->SetIsOpen(false);
	}

	FReply OnLayoutDeleteButton(FString LayoutName)
	{
		OnLayoutDeleteClicked.ExecuteIfBound(LayoutName);
		return FReply::Handled();
	}

	TSharedRef<ITableRow> OnGenerateLayoutItemRow( TSharedPtr<FLayoutRowItem> InItem, const TSharedRef<STableViewBase>& OwnerTable )
	{
		return
        SNew(STableRow<TSharedPtr<FLayoutRowItem> >, OwnerTable)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()[
                SNew(STextBlock).Text(FText::FromString(InItem->LayoutName))
            ]
            .HAlign(HAlign_Left)
            .FillWidth(1.0f)
            + SHorizontalBox::Slot()
            [
                SNew(SButton)
                .Text(LOCTEXT("Delete", "Delete"))
                .OnClicked(FOnClicked::CreateSP(this, &SLayoutSelectionComboButton::OnLayoutDeleteButton, InItem->LayoutName))
                .ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
                [
                    SNew( SImage )
                    .Image( FEditorStyle::GetBrush("PropertyWindow.Button_Delete") )
                ]
            ]
            .HAlign(HAlign_Right)
        ];
	}
	
	TSharedRef<SWidget> OnGetLayoutSelectMenuContent()
	{
		FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

		// First area, list all available layout items, and corresponding "copy" and "delete" button.
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("NewLayout", "Create New Layout"));
		MenuBuilder.AddMenuEntry(
	        LOCTEXT("New","New"),
	        FText::GetEmpty(),
	        FSlateIcon(),
	        FUIAction(OnCreateNewLayout)
	    );
		MenuBuilder.AddMenuEntry(
	        LOCTEXT("CopyCurrent","Copy Current"),
	        FText::GetEmpty(),
	        FSlateIcon(),
	        FUIAction(OnCreateNewLayoutByCopying)
	    );
		MenuBuilder.EndSection();

		MenuBuilder.BeginSection(NAME_None, LOCTEXT("CurrentLayout", "Current Layout"));
		MenuBuilder.AddMenuEntry(
	        LOCTEXT("Rename","Rename"),
	        FText::GetEmpty(),
	        FSlateIcon(),
	        FUIAction(OnRenameLayout)
	    );
		MenuBuilder.EndSection();
		
		MenuBuilder.BeginSection(NAME_None, LOCTEXT("Layouts", "Layouts"));
		{
			TSharedPtr<SLayoutListView> ListView;
			SAssignNew(ListView, SLayoutListView)
				.OnGenerateRow(SLayoutListView::FOnGenerateRow::CreateSP(this, &SLayoutSelectionComboButton::OnGenerateLayoutItemRow))
				.ListItemsSource(&LayoutListItems)
				.OnSelectionChanged(SLayoutListView::FOnSelectionChanged::CreateSP(this, &SLayoutSelectionComboButton::OnRowItemSelected))
				.SelectionMode(ESelectionMode::Type::Single);
				
			auto LocalCollection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
			TArray<FString> LayoutNames;
			LocalCollection->WorkspaceLayouts.GetKeys(LayoutNames);

			LayoutListItems.Empty();
			for(auto& LayoutName : LayoutNames)
			{
				LayoutListItems.Add(MakeShared<FLayoutRowItem>(LayoutName));	
			}

			MenuBuilder.AddWidget(ListView.ToSharedRef(), FText::GetEmpty(), false);
		}
		MenuBuilder.EndSection();
		
		return MenuBuilder.MakeWidget();
	}
};

class SSubObjectAddArea : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FAddObjectConfirmed, UObject*)
	DECLARE_DELEGATE_RetVal_OneParam(bool, FVerifyObjectAddable, UObject*)

	SLATE_BEGIN_ARGS(SSubObjectAddArea)
	{}
	SLATE_EVENT(FAddObjectConfirmed, OnAddObjectConfirmed)
	SLATE_EVENT(FVerifyObjectAddable, OnVerifyObjectAddable)

	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		OnAddObjectConfirmed = Args._OnAddObjectConfirmed;
		OnVerifyObjectAddable = Args._OnVerifyObjectAddable;
		
		ChildSlot[
            SNew(SHorizontalBox)
            .Visibility(this, &SSubObjectAddArea::AddObjectButtonVisibility)
            + SHorizontalBox::Slot()
            [
	            SNew(SComboButton)
			        .OnGetMenuContent(FOnGetContent::CreateSP(this, &SSubObjectAddArea::CreateDetailForObjectMenu))
			        .ButtonContent()
			        [
			            SNew(STextBlock)
			            .Text(LOCTEXT("AddButtonLabel", "Add.."))
			        ]
            ]
            .AutoWidth()
		];
	}
	
	TWeakObjectPtr<UObject> PendingObservedObject;
private:
	
	FAddObjectConfirmed OnAddObjectConfirmed;
	FVerifyObjectAddable OnVerifyObjectAddable;
	
	EVisibility AddObjectButtonVisibility() const
	{
		return PendingObservedObject.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	FText OnGetPendingObservedObjectLabel() const
	{
		if ( PendingObservedObject.Get())
		{
			return FText::Format(LOCTEXT("ChooseSubobjectToAdd", "Choose Subobject to add: {0}"), FText::FromString(GetPrettyNameForObject(PendingObservedObject.Get())));
		}
		else
		{
			return FText::GetEmpty();	
		}
	}

	void SpawnNewDetailWidgetForObject(UObject* Object)
	{
		OnAddObjectConfirmed.ExecuteIfBound(Object);
	}

	TSharedRef<SWidget> CreateDetailForObjectMenu()
	{
		if (!ensure(PendingObservedObject.Get()))	// button should be hidden.
			return SNew(SBorder);
	
		FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

		if (!OnVerifyObjectAddable.IsBound() || OnVerifyObjectAddable.Execute(PendingObservedObject.Get()))
		{
			MenuBuilder.AddMenuEntry(
                LOCTEXT("AddThisObject", "Add This Object"),
                FText::GetEmpty(),
                FSlateIcon(),
                FUIAction(FExecuteAction::CreateSP(this, &SSubObjectAddArea::SpawnNewDetailWidgetForObject, PendingObservedObject.Get()))
            );
		}

		TArray<UObject*> SubObjects;
		PendingObservedObject.Get()->GetDefaultSubobjects(SubObjects);

		MenuBuilder.BeginSection(NAME_None, LOCTEXT("SubObjects", "Sub Objects"));
		for(auto& t : SubObjects)
		{
			if (!OnVerifyObjectAddable.IsBound() || OnVerifyObjectAddable.Execute(t))
			{
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("Name"), FText::FromString(GetPrettyNameForObject(t)));
			
				MenuBuilder.AddMenuEntry(
	                FText::Format(LOCTEXT("AddSubObjectFormat", "Add {Name}"), Arguments),
	                FText::GetEmpty(),
	                FSlateIcon(),
	                FUIAction(FExecuteAction::CreateSP(this, &SSubObjectAddArea::SpawnNewDetailWidgetForObject, t))
	            );
			}
		}
	
		MenuBuilder.EndSection();

		return MenuBuilder.MakeWidget();
	}
};

UObject* UDetailsWorkspaceProfileCollectionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
                                                                     EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDetailsWorkspaceProfile>(InParent, Class, Name, Flags);
}

FDetailWorkspaceObservedItem FDetailWorkspaceObservedItem::From(UObject* Object)
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
		Result.bIsDefaultSubObject = false;
		Result.SubObjectName = NAME_None;
	}
	return Result;
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


TSharedRef<SWidget> SDetailsWorkspaceRootTab::CreateConfigArea()
{
	SAssignNew(ConfigArea, SExpandableArea)
		.HeaderContent()[
			SNew(STextBlock).Text(LOCTEXT("Config", "Config"))
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
	            .OnLayoutDeleteClicked(this, &SDetailsWorkspaceRootTab::DeleteLayoutWithDialog)
	            .OnCreateNewLayout(this, &SDetailsWorkspaceRootTab::CreateNewLayoutWithDialog)
	            .OnCreateNewLayoutByCopying(this, &SDetailsWorkspaceRootTab::CreateNewLayoutByCopyingWithDialog)
	            .OnRenameLayout(this, &SDetailsWorkspaceRootTab::CreateRenameCurrentLayoutWindow)
	            .SelectedLayoutName(this, &SDetailsWorkspaceRootTab::OnGetCurrentLayoutName)
	            .OnLayoutSelected(this, &SDetailsWorkspaceRootTab::SwitchLayout, true)
	        ].Padding(2.0f).VAlign(VAlign_Center)
	        
            + SGridPanel::Slot(0, 1)[
                SNew(STextBlock).Text(LOCTEXT("AutoSwitchPIE", "Auto Switch to PIE Object"))
            ].Padding(2.0f).VAlign(VAlign_Center)
            + SGridPanel::Slot(1, 1)[
                SAssignNew(AutoPIECheckBox, SCheckBox)
            ].Padding(2.0f).VAlign(VAlign_Center)

            + SGridPanel::Slot(0, 2)[
            	SNew(STextBlock).Text(LOCTEXT("ObjectToAdd", "Object To Add: "))
            ].Padding(2.0f).VAlign(VAlign_Center)
            + SGridPanel::Slot(1, 2)[
                SAssignNew(SubObjectAddArea, SSubObjectAddArea)
                .OnAddObjectConfirmed(SSubObjectAddArea::FAddObjectConfirmed::CreateSP(this, &SDetailsWorkspaceRootTab::SpawnNewDetailWidgetForObject))
                .OnVerifyObjectAddable(SSubObjectAddArea::FVerifyObjectAddable::CreateSP(this, &SDetailsWorkspaceRootTab::IsNotObservingObject))
            ].Padding(2.0f).VAlign(VAlign_Center)

            + SGridPanel::Slot(0, 3)[
                SNew(STextBlock).Text(LOCTEXT("EnableDeveloperMode", "Enable Developer Mode"))
            ].Padding(2.0f).VAlign(VAlign_Center)
            + SGridPanel::Slot(1, 3)[
                SAssignNew(DeveloperModeCheckerbox, SCheckBox)
            ].Padding(2.0f).VAlign(VAlign_Center)
            
            + SGridPanel::Slot(0, 4)[
            	SNew(SButton)
            	.OnClicked(FOnClicked::CreateSP(this, &SDetailsWorkspaceRootTab::CopyCurrentLayoutStringToClipboard))
            	[
					SNew(STextBlock).Text(LOCTEXT("CopyLayoutStringToClipboard", "Copy Layout String To Clipboard"))
                ]
            ].Padding(2.0f).VAlign(VAlign_Center)
        ];
	AutoPIECheckBox->SetIsChecked(true);
	ConfigArea->SetExpanded(false);
	
	return ConfigArea.ToSharedRef();
}

FReply SDetailsWorkspaceRootTab::CopyCurrentLayoutStringToClipboard()
{
	auto String = TabManager->PersistLayout()->ToString();
	FPlatformApplicationMisc::ClipboardCopy( *String );
	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("LayoutStringCopied", "Layout string copied"));
	return FReply::Handled();
}

void SDetailsWorkspaceRootTab::DoPersistVisualState()
{
	auto LocalCollection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	// Remember opened layout name. 
	if (LocalCollection)
	{
		if (!InstanceName.TrimStartAndEnd().IsEmpty())
		{
			LocalCollection->Modify();
			LocalCollection->InstanceLastLayout.FindOrAdd(InstanceName) = WorkingLayoutName;
		}
	}

	// Save the layout.  
	if (!WorkingLayoutName.TrimStartAndEnd().IsEmpty())
		SaveLayout();
}

void SDetailsWorkspaceRootTab::CreateNewLayout(FText LayoutName)
{
	if (LayoutName.ToString().TrimStartAndEnd().IsEmpty())
		return;
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	Collection->WorkspaceLayouts.Add(LayoutName.ToString(), FDetailsWorkspaceLayout());
	WorkingLayoutName = LayoutName.ToString();

	ResetLayoutToInitial();
}

void SDetailsWorkspaceRootTab::CreateNewLayoutByCopyingCurrent(FText LayoutName)
{
	if (LayoutName.ToString().TrimStartAndEnd().IsEmpty())
		return;
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	
	FDetailsWorkspaceLayout Value;
	DumpCurrentLayout(Value);
	
	Collection->WorkspaceLayouts.Add(LayoutName.ToString(), Value);
	WorkingLayoutName = LayoutName.ToString();
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
		SaveLayout();
	}

	Restore(*Found);
	WorkingLayoutName = TargetLayoutName;
}

bool SDetailsWorkspaceRootTab::IsObservingObject(UObject* Object) const
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

void SDetailsWorkspaceRootTab::CreateRenameCurrentLayoutWindow()
{
	auto Window = SNew(SLayoutNameInputWindow)
        .Title(LOCTEXT("RenameLayoutDialogTitle", "Input new name for layout"))
        .ClientSize(FVector2D(300, 120))
        .ButtonText(LOCTEXT("Rename", "Rename"))
        .OnConfirmed(SLayoutNameInputWindow::FOnNameInputConfirmed::CreateSP(this, &SDetailsWorkspaceRootTab::RenameCurrentLayout));
}

void SDetailsWorkspaceRootTab::DeleteLayoutWithDialog(FString LayoutName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();
	if (EAppReturnType::Ok == FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteLayoutDialogContent", "Are you sure to delete the layout?")))
	{
		Collection->WorkspaceLayouts.Remove(LayoutName);
		if (WorkingLayoutName == LayoutName)
		{
			WorkingLayoutName = TEXT("");
		}
	}
}

void SDetailsWorkspaceRootTab::CreateNewLayoutWithDialog()
{
	auto Window = SNew(SLayoutNameInputWindow)
        .Title(LOCTEXT("CreateNewLayoutDialogTitle", "Input name for new layout"))
        .ClientSize(FVector2D(300, 120))
        .ButtonText(LOCTEXT("Add", "Add"))
        .OnConfirmed(SLayoutNameInputWindow::FOnNameInputConfirmed::CreateSP(this, &SDetailsWorkspaceRootTab::CreateNewLayout));	
}

void SDetailsWorkspaceRootTab::CreateNewLayoutByCopyingWithDialog()
{
	auto Window = SNew(SLayoutNameInputWindow)
        .Title(LOCTEXT("CreateNewLayoutDialogTitle", "Input name for new layout"))
        .ClientSize(FVector2D(300, 120))
        .ButtonText(LOCTEXT("Add", "Add"))
        .OnConfirmed(SLayoutNameInputWindow::FOnNameInputConfirmed::CreateSP(this, &SDetailsWorkspaceRootTab::CreateNewLayoutByCopyingCurrent));
}
void SDetailsWorkspaceRootTab::RenameCurrentLayout(FText NewName)
{
	auto Collection = UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile();

	if (auto Current = Collection->WorkspaceLayouts.Find(WorkingLayoutName))
	{
		auto Copy = *Current;
		Collection->Modify();
		Collection->WorkspaceLayouts.Remove(WorkingLayoutName);
		Collection->WorkspaceLayouts.Add(NewName.ToString(), Copy);
		// Directly save the file.
		// It's stupid to ask user to save a transient file.  
		FEditorFileUtils::PromptForCheckoutAndSave({Collection->GetPackage()}, true, false);
	}

	WorkingLayoutName = NewName.ToString();
}

FText SDetailsWorkspaceRootTab::OnGetCurrentLayoutName() const
{
	return FText::FromString(WorkingLayoutName);
}

EVisibility SDetailsWorkspaceRootTab::DropAreaVisibility() const
{
    return FSlateApplication::Get().IsDragDropping() && OnRecognizeObserveObjectDrop(FSlateApplication::Get().GetDragDroppingContent()) ? EVisibility::Visible : EVisibility::Hidden;
}

FText SDetailsWorkspaceRootTab::GetLabel() const
{
	return FText::FromString(WorkingLayoutName);
}

void SDetailsWorkspaceRootTab::Construct(const FArguments& Args, FString InInstanceName, bool bLoadInstanceLastLayout)
{
	this->InstanceName = InInstanceName;
	
	SDockTab::Construct(SDockTab::FArguments()
		.TabRole(ETabRole::NomadTab)
		.Label(this, &SDetailsWorkspaceRootTab::GetLabel)
	);

	SetOnPersistVisualState(FOnPersistVisualState::CreateSP(this, &SDetailsWorkspaceRootTab::DoPersistVisualState));
	
	TabManager = FGlobalTabmanager::Get()->NewTabManager(SharedThis(this));
	TabManager
	->RegisterTabSpawner(WelcomePageTabID, FOnSpawnTab::CreateSP(this, &SDetailsWorkspaceRootTab::CreateWelcomeTab))
	.SetReuseTabMethod(FOnFindTabToReuse::CreateLambda([](const FTabId&){ return TSharedPtr<SDockTab>(); }));

	
	auto DropArea = SNew(SOverlay)
		.Visibility(this, &SDetailsWorkspaceRootTab::DropAreaVisibility)
        + SOverlay::Slot()[
            SNew(STextBlock)
            .Text(LOCTEXT("DropActorHereToViewDetails", "Drop Actor Here To View Details"))
            .Visibility(EVisibility::SelfHitTestInvisible)
        ]
        + SOverlay::Slot()[
	        SNew(SDropTarget)
			    .OnIsRecognized(SDropTarget::FVerifyDrag::CreateSP(this, &SDetailsWorkspaceRootTab::OnRecognizeObserveObjectDrop))
			    .OnAllowDrop(SDropTarget::FVerifyDrag::CreateSP(this, &SDetailsWorkspaceRootTab::OnRecognizeObserveObjectDrop))
			    .OnDrop(SDropTarget::FOnDrop::CreateSP(this, &SDetailsWorkspaceRootTab::OnObserveObjectDrop))
        ];

	SetContent
    (
		SNew(SOverlay)
		+ SOverlay::Slot()[
		    SNew( SVerticalBox )
		    +SVerticalBox::Slot()
            [
	        	CreateConfigArea()
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
		ResetLayoutToInitial();
	}
}

void SDetailsWorkspaceRootTab::ResetLayoutToInitial()
{
	TabManager->CloseAllAreas();
	TSharedRef<FTabManager::FLayout> Layout = InitialLayout();
	DockingTabsContainer->SetContent(
        TabManager->RestoreFrom(Layout, GetParentWindow()).ToSharedRef()
    );
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

SDetailsWorkspaceRootTab::~SDetailsWorkspaceRootTab()
{
	UE_LOG(LogTemp, Log, TEXT("FDetailsWorkspaceRootTab Destructed"));
}

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

UDetailsWorkspaceProfileCollectionFactory::UDetailsWorkspaceProfileCollectionFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	SupportedClass = UDetailsWorkspaceProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = false;
}

void SDetailsWorkspaceRootTab::SaveLayout()
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

void SDetailsWorkspaceRootTab::DumpCurrentLayout(FDetailsWorkspaceLayout& OutTarget)
{
	OutTarget.LayoutString = TabManager->PersistLayout()->ToString();
	for(auto& Tab: SpawnedDetails)
	{
		auto Pinned = Tab.Pin();
		if (Pinned && Pinned->GetObject(false) && TabManager->FindExistingLiveTab(Tab.Pin()->TabID))
		{
			auto Object = Pinned->GetObject(false);
			if (!Object->HasAnyFlags(RF_Transient))
				OutTarget.References.Add(Pinned->TabID.TabType, FDetailWorkspaceObservedItem::From(Object));
		}
	}
}

bool SDetailsWorkspaceRootTab::EnableAutoPIE() const
{
	return AutoPIECheckBox->IsChecked();	
}

void SDetailsWorkspaceRootTab::OnDetailTabClosed(TSharedRef<SDockTab> Tab, TWeakPtr<SAnyObjectDetails> Detail)
{
	SpawnedDetails.Remove(Detail);
}

EVisibility SDetailsWorkspaceRootTab::DeveloperTextVisibility() const
{
	return DeveloperModeCheckerbox->IsChecked() ? EVisibility::SelfHitTestInvisible : EVisibility::Hidden;
}

TSharedRef<SDockTab> SDetailsWorkspaceRootTab::CreateDocKTabWithDetailView(const FSpawnTabArgs& Args)
{
	auto ID = Args.GetTabId();
	auto NewObjectDetailWidget =
		SNew(SAnyObjectDetails, ID)
		.AutoInspectPIE(this, &SDetailsWorkspaceRootTab::EnableAutoPIE)
	;
	
	auto WeakPtrToDetailWidget = TWeakPtr<SAnyObjectDetails>(NewObjectDetailWidget);
	auto Tab =
        SNew(SDockTab)
        .TabRole(ETabRole::PanelTab)
		.OnTabClosed(FOnTabClosedCallback::CreateSP(this, &SDetailsWorkspaceRootTab::OnDetailTabClosed, WeakPtrToDetailWidget))
		.Label_Lambda(
			[WeakPtrToDetailWidget]()
			{
				auto Widget = WeakPtrToDetailWidget.Pin();
				if (!Widget)
					return FText::GetEmpty();
				auto Object = Widget->GetObjectAuto();
				if (Object)
				{
					return FText::FromString(GetPrettyNameForObject(Object));
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
        	.Visibility(this, &SDetailsWorkspaceRootTab::DeveloperTextVisibility)
        ].HAlign(HAlign_Center).VAlign(VAlign_Center)
    );
	
	SpawnedDetails.Add(NewObjectDetailWidget);

	if (LoadedProfile.References.Find(ID.TabType))
	{
		auto WeakPtr = *LoadedProfile.References.Find(ID.TabType);
		NewObjectDetailWidget->SetObjectLazyPtr(WeakPtr);
	}

	return Tab;
}

TSharedRef<SDockTab> SDetailsWorkspaceRootTab::CreateWelcomeTab(const FSpawnTabArgs& Args)
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
                    "You can freely re-arrange created tabs in this panel."
                	"\n"
                	"Switch layout in config.\n"
                ))
            ].HAlign(HAlign_Center).VAlign(VAlign_Center)
		];
}

bool SDetailsWorkspaceRootTab::OnRecognizeObserveObjectDrop(TSharedPtr<FDragDropOperation> Operation) const
{
	return Operation->IsOfType<FActorDragDropOp>() || Operation->IsOfType<FAssetDragDropOp>();
}

static bool HasDefaultSubObject(UObject* Object)
{
	TArray<UObject*> Objects;
	Object->GetDefaultSubobjects(Objects);
	return  Objects.Num() != 0;
}

FReply SDetailsWorkspaceRootTab::OnObserveObjectDrop(TSharedPtr<FDragDropOperation> Op)
{
	if (!ensure(OnRecognizeObserveObjectDrop(Op)))
		return FReply::Unhandled();

	auto DealObject = [this](UObject* Object)
	{
		if (!Object)
			return;
		if (HasDefaultSubObject(Object))
		{
			SubObjectAddArea->PendingObservedObject = Object;
			ConfigArea->SetExpanded(true);	//So user could see the add button.  
			
		}
		else
		{
			SpawnNewDetailWidgetForObject(Object);
		}
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

void SDetailsWorkspaceRootTab::EnsureRegisterDetailTabSpawner(FName TabTypeID)
{
	if (!TabManager->HasTabSpawner(TabTypeID))
	{
		auto& TabSpawner = TabManager->RegisterTabSpawner(TabTypeID, FOnSpawnTab::CreateSP(this, &SDetailsWorkspaceRootTab::CreateDocKTabWithDetailView));

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

void SDetailsWorkspaceRootTab::SpawnNewDetailWidgetForObject(UObject* InObject)
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
	LoadedProfile.References.Add(TabTypeID, FDetailWorkspaceObservedItem::From(InObject));
	TabManager->TryInvokeTab(TabTypeID);
}


TSharedRef<SDetailsWorkspaceRootTab> CreateDetailsWorkSpace(FString InstanceName, bool bLoadInstanceLastLayout)
{
	return SNew(SDetailsWorkspaceRootTab, InstanceName, bLoadInstanceLastLayout);
}

#undef LOCTEXT_NAMESPACE

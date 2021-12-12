#include "SAnyObjectDetails.h"

#include "EditorFontGlyphs.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

FReply SAnyObjectDetails::OnSelectObjectClicked()
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

FText SAnyObjectDetails::GetHintText() const
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

bool SAnyObjectDetails::OnPropertyVisible(const FPropertyAndParent& PropertyAndParent)
{
	auto Category = PropertyAndParent.Property.GetMetaData(TEXT("Category"));
	if (HiddenCategories.Contains(Category))
		return false;
	for (auto Parent : PropertyAndParent.ParentProperties)
	{
		if (HiddenCategories.Contains(Parent->GetMetaData(TEXT("Category"))))
			return false;
	}
	return true;
}

void GetAllCategories(const UObject* Object, TArray<FString> &OutResult)
{
	FProperty* Pointer = Object->GetClass()->PropertyLink;	
	
	while(Pointer)
	{
		static const FName CategoryMetaDataKey = TEXT("Category");
		auto Category = Pointer->GetMetaData(CategoryMetaDataKey);
		if (!Category.TrimStartAndEnd().IsEmpty())
			OutResult.AddUnique(Pointer->GetMetaData(CategoryMetaDataKey));		
		Pointer = Pointer->PropertyLinkNext;
	};

	TArray<UObject*> SubObjects;
	Object->CollectDefaultSubobjects(SubObjects);
	for(auto& t : SubObjects)
	{
		GetAllCategories(t, OutResult);
	}
}

FReply SAnyObjectDetails::OnCategoriesClicked()
{
	bCategoryFilterVisibility = !bCategoryFilterVisibility;
	return FReply::Handled();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SAnyObjectDetails::Construct(const FArguments& InArgs, FTabId InTabID)
{
	bAutoInspectPIE = InArgs._AutoInspectPIE;

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(
		"PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	
	TabID = InTabID;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SAnyObjectDetails::OnPropertyVisible));
	
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()[
			SNew(STextBlock)
                .Text(this, &SAnyObjectDetails::GetHintText)
                .Visibility(EVisibility::SelfHitTestInvisible)
		].HAlign(HAlign_Center).VAlign(VAlign_Center)
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			[
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
				
                + SHorizontalBox::Slot()[
                    SNew(SButton)
                    .OnClicked(FOnClicked::CreateSP(this, &SAnyObjectDetails::OnCategoriesClicked))
                    .ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
                    [
						SNew(STextBlock)
                        .TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
                        .Font(FEditorStyle::Get().GetFontStyle("FontAwesome.11"))
                        .Text(FEditorFontGlyphs::Filter)
                        .ColorAndOpacity_Lambda(
                        	[this]()
                        	{
                        		return HiddenCategories.Num() > 0 ? FLinearColor::White : FLinearColor(0.66f, 0.66f, 0.66f, 0.66f);
                        	})
                    ]
                ]
                .AutoWidth()
			]
			.AutoHeight()			
			+ SVerticalBox::Slot()
            [
				SAssignNew(CategoryFilterRoot, SBorder)
				.Visibility_Lambda([this](){ return bCategoryFilterVisibility ? EVisibility::Visible : EVisibility::Collapsed; })
            ]
			.AutoHeight()
			+ SVerticalBox::Slot()[
				DetailsView->AsShared()
			]
		]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

UObject* SAnyObjectDetails::GetObjectAuto()
{
	const bool bWantPIE = bAutoInspectPIE.Get() && GEditor->IsPlayingSessionInEditor();
	return ObjectValue.Resolve(bWantPIE);
}

void SAnyObjectDetails::OnCategoryFilterCheckStateChanged(ECheckBoxState State, FString Category)
{
	if (State == ECheckBoxState::Checked)
	{
		HiddenCategories.Remove(Category);
	}else
	{
		HiddenCategories.Add(Category);
	}
	if (ensure(DetailsView))
		DetailsView->ForceRefresh();
}

ECheckBoxState SAnyObjectDetails::OnGetCategoryFilterCheckState(FString Category) const
{
	if (HiddenCategories.Contains(Category))
	{
		return ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Checked;
	}
}

void SAnyObjectDetails::SetObjectLazyPtr(FDetailsWorkspaceObservedItem Value)
{
	AvailableCategories.Empty();
	ObjectValue = Value;
	if (ObjectValue.Resolve(false))
	{
		GetAllCategories(ObjectValue.Resolve(false), AvailableCategories);
	}
	AvailableCategories.Sort();

	auto Box = SNew(SWrapBox).UseAllottedSize(true);

	for(auto t : AvailableCategories)
	{
		Box->AddSlot()[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
			[
				SNew(SCheckBox)
				.OnCheckStateChanged(this, &SAnyObjectDetails::OnCategoryFilterCheckStateChanged, t)
				.IsChecked(this, &SAnyObjectDetails::OnGetCategoryFilterCheckState, t)
				[
					SNew(STextBlock).Text(FText::FromString(t))
				]
			]
		].Padding(2.0f);
	}

	CategoryFilterRoot->SetContent(
		Box
	);
}

void SAnyObjectDetails::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	auto Expected = GetObjectAuto();

	if (CurrentWatching != Expected)
	{
		CurrentWatching = Expected;
		DetailsView->SetObject(Expected);
	}
}

#undef LOCTEXT_NAMESPACE

#include "SAnyObjectDetails.h"


#include "DetailLayoutBuilder.h"
#include "EditorFontGlyphs.h"
#include "IDetailCustomization.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

namespace
{
	class FCustomDetailsLayout : public IDetailCustomization
	{
	public:
		DECLARE_DELEGATE_RetVal_OneParam(bool, FOnGetCategoryVisibiliy, FName)
		FOnGetCategoryVisibiliy OnGetCategoryVisibiliy;
		
		DECLARE_DELEGATE_OneParam(FOnGetCategories, TArray<FName>)
		FOnGetCategories OnGetCategories;
		
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			TArray<FName> Categories;
			DetailBuilder.GetCategoryNames(Categories);
			if (OnGetCategories.IsBound())
			{
				OnGetCategories.Execute(Categories);
			}
			for(auto t : Categories)
			{
				if (OnGetCategoryVisibiliy.IsBound() && !OnGetCategoryVisibiliy.Execute(t))
				{
					DetailBuilder.HideCategory(t);
				}
			}
		}
	};
}

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

void GetAllCategories(const UObject* Object, TArray<FName> &OutResult)
{
	FProperty* Pointer = Object->GetClass()->PropertyLink;	
	
	while(Pointer)
	{
		static const FName CategoryMetaDataKey = TEXT("Category");
		auto Category = Pointer->GetMetaData(CategoryMetaDataKey);
		if (!Category.TrimStartAndEnd().IsEmpty())
			OutResult.AddUnique(FName(Pointer->GetMetaData(CategoryMetaDataKey)));		
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
	
	DetailsView->RegisterInstancedCustomPropertyLayout(UObject::StaticClass(), FOnGetDetailCustomizationInstance::CreateLambda([this]()
    {
        auto t = MakeShared<FCustomDetailsLayout>();
        t->OnGetCategoryVisibiliy = FCustomDetailsLayout::FOnGetCategoryVisibiliy::CreateLambda([this](FName Cateogry)
        {
            return !ObjectValue.HiddenCategories.Contains(Cateogry);
        });
		t->OnGetCategories = FCustomDetailsLayout::FOnGetCategories::CreateSP(this, &SAnyObjectDetails::OnGetCategoryNames);
			
        return t;
    }));
	
	//DetailsView->SetIsPropertyVisibleDelegate(FIsPropertyVisible::CreateSP(this, &SAnyObjectDetails::OnPropertyVisible));
	
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
                        		return ObjectValue.HiddenCategories.Num() > 0 ? FLinearColor::White : FLinearColor(0.66f, 0.66f, 0.66f, 0.66f);
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
			+ SVerticalBox::Slot()
			[
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

void SAnyObjectDetails::OnCategoryFilterCheckStateChanged(ECheckBoxState State, FName Category)
{
	if (State == ECheckBoxState::Checked)
	{
		ObjectValue.HiddenCategories.Remove(Category);
	}else
	{
		ObjectValue.HiddenCategories.Add(Category);
	}
	if (ensure(DetailsView))
		DetailsView->ForceRefresh();
}

ECheckBoxState SAnyObjectDetails::OnGetCategoryFilterCheckState(FName Category) const
{
	if (ObjectValue.HiddenCategories.Contains(Category))
	{
		return ECheckBoxState::Unchecked;
	}
	else
	{
		return ECheckBoxState::Checked;
	}
}

void SAnyObjectDetails::OnGetCategoryNames(TArray<FName> Val)
{
	if (!bAvailableCategoriesDirty)
		return;
	bAvailableCategoriesDirty = false;
	AvailableCategories = Val;
	auto Box = SNew(SWrapBox).UseAllottedSize(true);

	AvailableCategories.Sort([](const FName& a, const FName& b){return a.FastLess(b);});
	for(auto t : AvailableCategories)
	{
		Box->AddSlot()[
            SNew(SBorder)
            .BorderImage_Lambda([this, t]()
				{
					return GetObserveItem().HiddenCategories.Contains(t) ? FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder") : FEditorStyle::GetBrush("ToolPanel.GroupBorder"); 
				})
            [
                SNew(SCheckBox)
                .OnCheckStateChanged(this, &SAnyObjectDetails::OnCategoryFilterCheckStateChanged, t)
                .IsChecked(this, &SAnyObjectDetails::OnGetCategoryFilterCheckState, t)
                [
                    SNew(STextBlock).Text(FText::FromString(t.ToString()))
                ]
            ]
        ].Padding(2.0f);
	}

	CategoryFilterRoot->SetContent(
        Box
    );
}

void SAnyObjectDetails::SetObserveItem(FDetailsWorkspaceObservedItem Value)
{	
	ObjectValue = Value;
	if (ObjectValue.Resolve(false))
	{
		auto Object = ObjectValue.Resolve(false);
		//GetAllCategories(Object, AvailableCategories);
	}
	
	AvailableCategories.Empty();
	bAvailableCategoriesDirty = true;
	
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

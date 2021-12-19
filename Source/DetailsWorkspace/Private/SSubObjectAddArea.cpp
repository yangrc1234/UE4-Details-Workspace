#include "SSubObjectAddArea.h"

#include "DetailsWorkspaceProfile.h"
#include "Engine/Selection.h"

#define LOCTEXT_NAMESPACE "DetailsWorkSpace"

void SSubObjectAddArea::Construct(const FArguments& Args)
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
	        .ContentPadding(2.0f)
	        .ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &SSubObjectAddArea::OnGetPendingObservedObjectLabel)
				.TextStyle(FEditorStyle::Get(), "FlatButton.DefaultTextStyle")
			]
			.ButtonStyle(FEditorStyle::Get(), "FlatButton.Default")
		]
		.AutoWidth()
	];
}

EVisibility SSubObjectAddArea::AddObjectButtonVisibility() const
{
	return (GEditor->GetSelectedActors()->Num() > 0 || GEditor->GetSelectedComponents()->Num() > 0 || GEditor->GetSelectedObjects()->Num() > 0) ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SSubObjectAddArea::OnGetPendingObservedObjectLabel() const
{
	return LOCTEXT("Add", "Add Selected...");
}

void SSubObjectAddArea::SpawnNewDetailWidgetForObject(UObject* Object)
{
	OnAddObjectConfirmed.ExecuteIfBound(Object);
}

TSharedRef<SWidget> SSubObjectAddArea::CreateDetailForObjectMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

	auto DoForObject = [this, &MenuBuilder](UObject* Object)
	{
		MenuBuilder.BeginSection(NAME_None, FText::FromString(Object->GetName()));
		
		if (!OnVerifyObjectAddable.IsBound() || OnVerifyObjectAddable.Execute(Object))
		{
			MenuBuilder.AddMenuEntry(
				FText::Format(LOCTEXT("AddObjectName", "Add {0}"), FText::FromString(Object->GetName())),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SSubObjectAddArea::SpawnNewDetailWidgetForObject, Object))
			);
		}

		TArray<UObject*> SubObjects;
		Object->GetDefaultSubobjects(SubObjects);

		for (auto& t : SubObjects)
		{
			if (!OnVerifyObjectAddable.IsBound() || OnVerifyObjectAddable.Execute(t))
			{
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("Name"), FText::FromString(GetPrettyNameForDetailsWorkspaceObject(t)));

				MenuBuilder.AddMenuEntry(
					FText::Format(LOCTEXT("AddSubObjectFormat", "Add {Name}"), Arguments),
					FText::GetEmpty(),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateSP(this, &SSubObjectAddArea::SpawnNewDetailWidgetForObject, t))
				);
			}
		}

		MenuBuilder.EndSection();
	};

	TArray<UObject*> Array;
	GEditor->GetSelectedActors()->GetSelectedObjects(Array);
	for(auto Actor : Array)
	{
		DoForObject(Actor);
	}

	GEditor->GetSelectedComponents()->GetSelectedObjects(Array);
	for(auto Obj : Array)
	{
		DoForObject(Obj);
	}

	GEditor->GetSelectedObjects()->GetSelectedObjects(Array);
	for(auto Obj : Array)
	{
		DoForObject(Obj);
	}

	return MenuBuilder.MakeWidget();
}

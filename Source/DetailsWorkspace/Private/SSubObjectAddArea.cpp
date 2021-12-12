#include "SSubObjectAddArea.h"

#include "DetailsWorkspaceProfile.h"

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
			        .ButtonContent()
			[
				SNew(STextBlock)
				.Text(this, &SSubObjectAddArea::OnGetPendingObservedObjectLabel)
			]
		]
		.AutoWidth()
	];
}

EVisibility SSubObjectAddArea::AddObjectButtonVisibility() const
{
	return PendingObservedObject.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
}

FText SSubObjectAddArea::OnGetPendingObservedObjectLabel() const
{
	if (PendingObservedObject.Get())
	{
		return FText::Format(
			LOCTEXT("ChooseSubobjectToAdd", "Choose Subobject to add: {0}"),
			FText::FromString(GetPrettyNameForDetailsWorkspaceObject(PendingObservedObject.Get())));
	}
	else
	{
		return FText::GetEmpty();
	}
}

void SSubObjectAddArea::SpawnNewDetailWidgetForObject(UObject* Object)
{
	OnAddObjectConfirmed.ExecuteIfBound(Object);
}

TSharedRef<SWidget> SSubObjectAddArea::CreateDetailForObjectMenu()
{
	if (!ensure(PendingObservedObject.Get())) // button should be hidden.
		return SNew(SBorder);

	FMenuBuilder MenuBuilder(true, nullptr, nullptr, true);

	if (!OnVerifyObjectAddable.IsBound() || OnVerifyObjectAddable.Execute(PendingObservedObject.Get()))
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AddThisObject", "Add This Object"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SSubObjectAddArea::SpawnNewDetailWidgetForObject,
			                                   PendingObservedObject.Get()))
		);
	}

	TArray<UObject*> SubObjects;
	PendingObservedObject.Get()->GetDefaultSubobjects(SubObjects);

	MenuBuilder.BeginSection(NAME_None, LOCTEXT("SubObjects", "Sub Objects"));
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

	return MenuBuilder.MakeWidget();
}

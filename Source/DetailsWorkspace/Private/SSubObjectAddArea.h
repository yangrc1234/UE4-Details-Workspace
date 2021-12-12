#pragma once

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

	void Construct(const FArguments& Args);

	TWeakObjectPtr<UObject> PendingObservedObject;
private:
	
	FAddObjectConfirmed OnAddObjectConfirmed;
	FVerifyObjectAddable OnVerifyObjectAddable;

	EVisibility AddObjectButtonVisibility() const;

	FText OnGetPendingObservedObjectLabel() const;

	void SpawnNewDetailWidgetForObject(UObject* Object);

	TSharedRef<SWidget> CreateDetailForObjectMenu();
};

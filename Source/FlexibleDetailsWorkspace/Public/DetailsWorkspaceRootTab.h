#pragma once

struct FInspectItem
{
	FLazyObjectPtr Object;
	TSharedPtr<IDetailsView> DetailsView;
};

class SAnyObjectDetails : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SAnyObjectDetails)
	{}
	SLATE_END_ARGS()
	
    void Construct(const FArguments& InArgs, UObject* InObject);

public:
	UObject* GetObject() const {return Object.Get();}
	void SetObject(UObject* Value) { Object = Value; }

private:
	TWeakObjectPtr<UObject> Object;
	TSharedPtr<class IDetailsView> DetailsView;
};

class SDetailsWorkspaceRootTab;

class FDetailsWorkspaceRootTab : public TSharedFromThis<FDetailsWorkspaceRootTab> 
{
public:
	FReply OnNewTabClicked();
	void Init();
	FDetailsWorkspaceRootTab();
	~FDetailsWorkspaceRootTab();
	TSharedRef<SDetailsWorkspaceRootTab> GetWidget();
	TSharedPtr<FTabManager> TabManager;
	TWeakObjectPtr<AActor> PickekdActor;
	TArray<FInspectItem> Items;
	TWeakPtr<SDockTab> ParentTab;

protected:
	TSharedRef<SDockTab> CreateDocKTab(const FSpawnTabArgs& Args);
	TSharedPtr<SDockTab> AllocateNewTab();

private:
	TWeakPtr<SDetailsWorkspaceRootTab> Root;
};

class SDetailsWorkspaceRootTab : public SDockTab
{
public:
	TSharedPtr<FDetailsWorkspaceRootTab> Handler;
};

TSharedRef<SDockTab> CreateDetailsWorkSpace();
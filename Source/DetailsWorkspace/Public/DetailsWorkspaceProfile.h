#pragma once

#include "DetailsWorkspaceProfile.generated.h"

FString GetPrettyNameForDetailsWorkspaceObject(UObject* Object);

UENUM()
enum class EDetailsWorkspaceCategorySettingState
{
	Closed,	// The area is collapsed. 
    PickShowOnly,	// The area is open. When user click an item, only target category is shown. Only non-filtered categories appear for chosen.    
    MultiSelectFilter,	// The area is open. When user click an item, toggles it visibility. in select mode, and corresponding properties visibility.
    NUM
};

USTRUCT()
struct FDetailsWorkspaceCategorySettings
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	EDetailsWorkspaceCategorySettingState SettingsState = EDetailsWorkspaceCategorySettingState::Closed;

	UPROPERTY(EditAnywhere)
	TArray<FName> HiddenCategories;

	UPROPERTY(EditAnywhere)
	FName ShowOnlyCategory = NAME_None;

	bool IsActive()
	{
		return ShowOnlyCategory != NAME_None || HiddenCategories.Num() > 0;
	}
	
	bool ShouldShow(FName Category)
	{
		return !HiddenCategories.Contains(Category)  && (ShowOnlyCategory == NAME_None || ShowOnlyCategory == Category);
	}
};

USTRUCT()
struct FDetailsWorkspaceObservedItem
{
	GENERATED_BODY()

	static FDetailsWorkspaceObservedItem From(UObject* Object);

	UObject* Resolve(bool bTryResolvePIECounterPart);

	UPROPERTY(EditAnywhere)
	TLazyObjectPtr<UObject> SavedObject = nullptr;

	UPROPERTY(EditAnywhere)
	bool bIsDefaultSubObject = false;

	UPROPERTY(EditAnywhere)
	FName SubObjectName = NAME_None;

	UPROPERTY(EditAnywhere)
	FDetailsWorkspaceCategorySettings CategorySettings;

private:
	TWeakObjectPtr<UObject> ResolvedObject = nullptr;
	TWeakObjectPtr<UObject> ResolvedObjectPIE = nullptr;
};

USTRUCT()
struct FDetailsWorkspaceLayout
{
	GENERATED_BODY()

	UPROPERTY()
	FString SavedLevelName;

	UPROPERTY(EditAnywhere)
	FString LayoutString;

	UPROPERTY(EditAnywhere)
	TMap<FName, FDetailsWorkspaceObservedItem> References;
};

USTRUCT()
struct FDetailsWorkspaceInstanceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	bool bAutoPIE;
	
	UPROPERTY(EditAnywhere)
	bool bDeveloperMode;

	UPROPERTY(EditAnywhere)
	FString LastLayout;
};

UCLASS()
class UDetailsWorkspaceProfile : public UObject
{
	GENERATED_BODY()
public:
	//Key is layout id.  
	UPROPERTY(EditAnywhere)
	TMap<FString, FDetailsWorkspaceLayout> WorkspaceLayouts;
	
	// Remember what layout is on, when any instance is closed.  
	UPROPERTY(EditAnywhere)
	TMap<FString, FDetailsWorkspaceInstanceSettings> InstanceSettings;

	static UDetailsWorkspaceProfile* GetOrCreateLocalUserProfile();
    
};

#pragma once

#include "DetailsWorkspaceProfile.generated.h"

FString GetPrettyNameForDetailsWorkspaceObject(UObject* Object);

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
	TArray<FName> HiddenCategories;

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

#pragma once
#include "DetailsWorkspaceProfileCollectionFactory.generated.h"

UCLASS(MinimalAPI, hidecategories = Object, collapsecategories)
class UDetailsWorkspaceProfileCollectionFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

    //~ Begin UFactory Interface
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	FString GetDefaultNewAssetName() const override
	{
		return FString(TEXT("DetailsWorkspaceProfileCollection"));
	}
	//~ Begin UFactory Interface	
};

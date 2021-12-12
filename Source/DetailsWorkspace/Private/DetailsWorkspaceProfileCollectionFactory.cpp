#include "DetailsWorkspaceProfileCollectionFactory.h"

#include "DetailsWorkspaceProfile.h"

UObject* UDetailsWorkspaceProfileCollectionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name,
                                                                     EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDetailsWorkspaceProfile>(InParent, Class, Name, Flags);
}

UDetailsWorkspaceProfileCollectionFactory::UDetailsWorkspaceProfileCollectionFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	SupportedClass = UDetailsWorkspaceProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = false;
}

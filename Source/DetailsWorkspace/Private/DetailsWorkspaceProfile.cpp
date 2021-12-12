#include "DetailsWorkspaceProfile.h"

#include "AssetToolsModule.h"
#include "DetailsWorkspaceProfileCollectionFactory.h"

UObject* FDetailsWorkspaceObservedItem::Resolve(bool bTryResolvePIECounterPart)
{
	if (!SavedObject.Get())
		return nullptr;

	auto Actor = Cast<AActor>(SavedObject.Get());
	if (Actor && bTryResolvePIECounterPart && ensure(GEditor->IsPlayingSessionInEditor()))
	{
		if (!ResolvedObjectPIE.IsValid())
		{
			Actor = EditorUtilities::GetSimWorldCounterpartActor(Actor);
			if (ensure(Actor))
			{
				if (!bIsDefaultSubObject)
				{
					ResolvedObjectPIE = Actor;
				}
				else
				{
					ResolvedObjectPIE = Actor->GetDefaultSubobjectByName(SubObjectName);
				}
			}
		}
		return ResolvedObjectPIE.Get();
	}
	else
	{
		if (!ResolvedObject.IsValid())
		{
			if (!bIsDefaultSubObject)
			{
				ResolvedObject = SavedObject.Get();
			}
			else
			{
				ResolvedObject = SavedObject->GetDefaultSubobjectByName(SubObjectName);
			}
		}

		return ResolvedObject.Get();
	}
}

FString GetPrettyNameForDetailsWorkspaceObject(UObject* Object)
{
	check(Object);
	FString Result;
	if (auto Actor = Cast<AActor>(Object))
		Result = Actor->GetActorLabel();
	else
		Result = Object->GetName();
	if (Object->HasAnyFlags(RF_Transient))
	{
		Result = TEXT("(Transient)") + Result;
	}
	if (Object->GetWorld() && Object->GetWorld()->WorldType == EWorldType::PIE)
	{
		Result = TEXT("(PIE)") + Result;
	}
	return Result;
}

FDetailsWorkspaceObservedItem FDetailsWorkspaceObservedItem::From(UObject* Object)
{
	FDetailsWorkspaceObservedItem Result;
	if (Object->IsDefaultSubobject())
	{
		Result.bIsDefaultSubObject = true;
		Result.SubObjectName = Object->GetFName();
		Result.SavedObject = Object->GetOuter();
	}
	else
	{
		Result.SavedObject = Object;
		Result.bIsDefaultSubObject = false;
		Result.SubObjectName = NAME_None;
	}
	return Result;
}

UDetailsWorkspaceProfile* UDetailsWorkspaceProfile::GetOrCreateLocalUserProfile()
{
	auto Folder = FPaths::Combine(TEXT("/Game/Developers/"), FPaths::GameUserDeveloperFolderName());
	FString AssetName = TEXT("DetailsWorkspaceLayoutProfile");

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();

	auto LoadProfile = LoadObject<UDetailsWorkspaceProfile>(nullptr, *(Folder / AssetName));

	if (!LoadProfile)
	{
		LoadProfile = Cast<UDetailsWorkspaceProfile>(
            AssetToolsModule.Get().CreateAsset(
            AssetName
                , Folder
                , UDetailsWorkspaceProfile::StaticClass()
                , Cast<UFactory>(UDetailsWorkspaceProfileCollectionFactory::StaticClass()->GetDefaultObject()))
            );
		if (!LoadProfile)
		{
			UE_LOG(LogTemp, Error,  TEXT("Create DWLayoutCollection failed"));
		}
	}

	return LoadProfile;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DetailsWorkspaceStyle.h"

#define DETAILS_WORKSPACE_DEFAULT_INSTANCES 4

class FDetailsWorkspaceCommands : public TCommands<FDetailsWorkspaceCommands>
{
public:

	FDetailsWorkspaceCommands()
		: TCommands<FDetailsWorkspaceCommands>(TEXT("DetailsWorkspace"), NSLOCTEXT("Contexts", "DetailsWorkspace", "DetailsWorkspace Plugin"), NAME_None, FDetailsWorkspaceStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TStaticArray<TSharedPtr< FUICommandInfo >, DETAILS_WORKSPACE_DEFAULT_INSTANCES> OpenPluginWindow;
};
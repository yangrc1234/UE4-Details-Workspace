// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "DetailsWorkspaceStyle.h"

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
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
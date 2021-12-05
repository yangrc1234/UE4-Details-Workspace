// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "FlexibleDetailsWorkspaceStyle.h"

class FFlexibleDetailsWorkspaceCommands : public TCommands<FFlexibleDetailsWorkspaceCommands>
{
public:

	FFlexibleDetailsWorkspaceCommands()
		: TCommands<FFlexibleDetailsWorkspaceCommands>(TEXT("FlexibleDetailsWorkspace"), NSLOCTEXT("Contexts", "FlexibleDetailsWorkspace", "FlexibleDetailsWorkspace Plugin"), NAME_None, FFlexibleDetailsWorkspaceStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};
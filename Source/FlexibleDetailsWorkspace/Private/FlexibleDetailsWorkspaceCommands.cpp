// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlexibleDetailsWorkspaceCommands.h"

#define LOCTEXT_NAMESPACE "FFlexibleDetailsWorkspaceModule"

void FFlexibleDetailsWorkspaceCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "FlexibleDetailsWorkspace", "Bring up FlexibleDetailsWorkspace window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE

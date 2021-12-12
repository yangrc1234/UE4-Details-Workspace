// Copyright Epic Games, Inc. All Rights Reserved.

#include "DetailsWorkspaceCommands.h"

#define LOCTEXT_NAMESPACE "FDetailsWorkspaceModule"

void FDetailsWorkspaceCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "DetailsWorkspace", "Bring up DetailsWorkspace window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE

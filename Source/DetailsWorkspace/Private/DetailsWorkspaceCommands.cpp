// Copyright Epic Games, Inc. All Rights Reserved.

#include "DetailsWorkspaceCommands.h"

#define LOCTEXT_NAMESPACE "FDetailsWorkspaceModule"

void FDetailsWorkspaceCommands::RegisterCommands()
{
	UI_COMMAND(
        OpenPluginWindow[0]
        , "Details Workspace 1"
        , "Bring up DetailsWorkspace window"
        , EUserInterfaceActionType::Button
        , FInputChord());
	UI_COMMAND(
        OpenPluginWindow[1]
        , "Details Workspace 2"
        , "Bring up DetailsWorkspace window"
        , EUserInterfaceActionType::Button
        , FInputChord());
	UI_COMMAND(
        OpenPluginWindow[2]
        , "Details Workspace 3"
        , "Bring up DetailsWorkspace window"
        , EUserInterfaceActionType::Button
        , FInputChord());
	UI_COMMAND(
        OpenPluginWindow[3]
        , "Details Workspace 4"
        , "Bring up DetailsWorkspace window"
        , EUserInterfaceActionType::Button
        , FInputChord());
}

#undef LOCTEXT_NAMESPACE

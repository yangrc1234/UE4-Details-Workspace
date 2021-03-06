// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FDetailsWorkspaceModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
private:

	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked(int Index);
	//void RegisterMenus();
	void AddMenuEntry(FMenuBuilder& MenuBuilder);
	TSharedPtr<FExtender> MenuExtender;

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	FDelegateHandle OnSequencerCreatedHandle;
};

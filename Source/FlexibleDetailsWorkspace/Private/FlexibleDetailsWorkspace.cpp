// Copyright Epic Games, Inc. All Rights Reserved.

#include "FlexibleDetailsWorkspace.h"

#include "DetailsWorkspaceRootTab.h"
#include "FlexibleDetailsWorkspaceStyle.h"
#include "FlexibleDetailsWorkspaceCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"

static const FName FlexibleDetailsWorkspaceTabName("FlexibleDetailsWorkspace");

#define LOCTEXT_NAMESPACE "FFlexibleDetailsWorkspaceModule"

void FFlexibleDetailsWorkspaceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FFlexibleDetailsWorkspaceStyle::Initialize();
	FFlexibleDetailsWorkspaceStyle::ReloadTextures();

	FFlexibleDetailsWorkspaceCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FFlexibleDetailsWorkspaceCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FFlexibleDetailsWorkspaceModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FFlexibleDetailsWorkspaceModule::RegisterMenus));
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FlexibleDetailsWorkspaceTabName, FOnSpawnTab::CreateRaw(this, &FFlexibleDetailsWorkspaceModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FFlexibleDetailsWorkspaceTabTitle", "FlexibleDetailsWorkspace"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FFlexibleDetailsWorkspaceModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FFlexibleDetailsWorkspaceStyle::Shutdown();

	FFlexibleDetailsWorkspaceCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FlexibleDetailsWorkspaceTabName);
}

TSharedRef<SDockTab> FFlexibleDetailsWorkspaceModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{

	// How is this going to work?  
	// 1. Object references are stored.
	// 2. An action named "load workspace" is called, when user open the window, or explicitly click a button in the window.  
	// 3. A workspace contains layout, and each node corresponding lazy object pointer.

	//TODO:
	// 1. Create the root tab slate.
	// 2. Root tab slate should allow drag-n-drop objects into it.  

	// Root tab.
	// It doesn't change.
	return CreateDetailsWorkSpace();
}

void FFlexibleDetailsWorkspaceModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FlexibleDetailsWorkspaceTabName);
}

void FFlexibleDetailsWorkspaceModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FFlexibleDetailsWorkspaceCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Settings");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FFlexibleDetailsWorkspaceCommands::Get().OpenPluginWindow));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFlexibleDetailsWorkspaceModule, FlexibleDetailsWorkspace)
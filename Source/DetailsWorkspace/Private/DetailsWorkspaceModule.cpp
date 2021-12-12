// Copyright Epic Games, Inc. All Rights Reserved.

#include "DetailsWorkspaceModule.h"

#include "SDetailsWorkspace.h"
#include "DetailsWorkspaceStyle.h"
#include "DetailsWorkspaceCommands.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"

static const FName DetailsWorkspaceTabName("DetailsWorkspace");

#define LOCTEXT_NAMESPACE "FDetailsWorkspaceModule"

void FDetailsWorkspaceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FDetailsWorkspaceStyle::Initialize();
	FDetailsWorkspaceStyle::ReloadTextures();

	FDetailsWorkspaceCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FDetailsWorkspaceCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FDetailsWorkspaceModule::PluginButtonClicked),
		FCanExecuteAction());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(DetailsWorkspaceTabName, FOnSpawnTab::CreateRaw(this, &FDetailsWorkspaceModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FDetailsWorkspaceTabTitle", "DetailsWorkspace"))
	.SetMenuType(ETabSpawnerMenuType::Hidden);
	

	{
		MenuExtender = MakeShareable(new FExtender);
		MenuExtender->AddMenuExtension("LevelEditor", 
                                            EExtensionHook::After, 
                                            PluginCommands, 
                                            FMenuExtensionDelegate::CreateRaw(this, &FDetailsWorkspaceModule::AddMenuEntry));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FDetailsWorkspaceModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("DetailsWorkspace", TAttribute<FText>(LOCTEXT("DetailsWorkspace", "Details Workspace")));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenDetailsWorkspace", "Open Details Workspace"),
		FText::GetEmpty(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FDetailsWorkspaceModule::PluginButtonClicked))
	);
	MenuBuilder.EndSection();
}

void FDetailsWorkspaceModule::ShutdownModule()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);

	FDetailsWorkspaceStyle::Shutdown();

	FDetailsWorkspaceCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DetailsWorkspaceTabName);
}

TSharedRef<SDockTab> FDetailsWorkspaceModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return CreateDetailsWorkSpace(TEXT("Default"), true);
}

void FDetailsWorkspaceModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DetailsWorkspaceTabName);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDetailsWorkspaceModule, DetailsWorkspace)
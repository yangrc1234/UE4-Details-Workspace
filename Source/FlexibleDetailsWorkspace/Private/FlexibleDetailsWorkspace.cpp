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

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(FlexibleDetailsWorkspaceTabName, FOnSpawnTab::CreateRaw(this, &FFlexibleDetailsWorkspaceModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FFlexibleDetailsWorkspaceTabTitle", "FlexibleDetailsWorkspace"))
	.SetMenuType(ETabSpawnerMenuType::Hidden);
	

	{
		MenuExtender = MakeShareable(new FExtender);
		MenuExtender->AddMenuExtension("LevelEditor", 
                                            EExtensionHook::After, 
                                            PluginCommands, 
                                            FMenuExtensionDelegate::CreateRaw(this, &FFlexibleDetailsWorkspaceModule::AddMenuEntry));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FFlexibleDetailsWorkspaceModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("DetailsWorkspace", TAttribute<FText>(LOCTEXT("DetailsWorkspace", "Details Workspace")));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenDetailsWorkspace", "Open Details Workspace"),
		FText::GetEmpty(),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FFlexibleDetailsWorkspaceModule::PluginButtonClicked))
	);
	MenuBuilder.EndSection();
}

void FFlexibleDetailsWorkspaceModule::ShutdownModule()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);

	FFlexibleDetailsWorkspaceStyle::Shutdown();

	FFlexibleDetailsWorkspaceCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FlexibleDetailsWorkspaceTabName);
}

TSharedRef<SDockTab> FFlexibleDetailsWorkspaceModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return CreateDetailsWorkSpace(TEXT("Default"), true);
}

void FFlexibleDetailsWorkspaceModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FlexibleDetailsWorkspaceTabName);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFlexibleDetailsWorkspaceModule, FlexibleDetailsWorkspace)
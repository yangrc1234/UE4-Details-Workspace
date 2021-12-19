// Copyright Epic Games, Inc. All Rights Reserved.

#include "DetailsWorkspaceModule.h"

#include "SDetailsWorkspace.h"
#include "DetailsWorkspaceStyle.h"
#include "DetailsWorkspaceCommands.h"
#include "ISequencerModule.h"
#include "LevelEditor.h"
#include "SAnyObjectDetails.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FDetailsWorkspaceModule"

static FName GetDefaultTabID(int Index)
{
	const FString DetailsWorkspaceTabID("DetailsWorkspace");
	static bool bInitialized = false;
	static TArray<FName> Result;
	if (!bInitialized)
	{
		for (int i = 0; i < DETAILS_WORKSPACE_DEFAULT_INSTANCES; i++)
		{
			Result.Add(FName(FString::Printf(TEXT("%s%i"), *DetailsWorkspaceTabID, i)));
		}
		bInitialized = true;
	}
	return Result[Index];
}

void FDetailsWorkspaceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FDetailsWorkspaceStyle::Initialize();
	FDetailsWorkspaceStyle::ReloadTextures();

	FDetailsWorkspaceCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	for (int i = 0; i < DETAILS_WORKSPACE_DEFAULT_INSTANCES; i++)
	{
		PluginCommands->MapAction(
			FDetailsWorkspaceCommands::Get().OpenPluginWindow[i],
			FExecuteAction::CreateRaw(this, &FDetailsWorkspaceModule::PluginButtonClicked, i),
			FCanExecuteAction());

		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(GetDefaultTabID(i),
		                                                  FOnSpawnTab::CreateRaw(
			                                                  this, &FDetailsWorkspaceModule::OnSpawnPluginTab))
		                        .SetDisplayName(LOCTEXT("FDetailsWorkspaceTabTitle", "DetailsWorkspace"))
		                        .SetMenuType(ETabSpawnerMenuType::Hidden);
	}

	{
		MenuExtender = MakeShareable(new FExtender);
		MenuExtender->AddMenuExtension("LevelEditor",
		                               EExtensionHook::After,
		                               PluginCommands,
		                               FMenuExtensionDelegate::CreateRaw(this, &FDetailsWorkspaceModule::AddMenuEntry));

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}

	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	OnSequencerCreatedHandle = SequencerModule.RegisterOnSequencerCreated(
		FOnSequencerCreated::FDelegate::CreateStatic(&SAnyObjectDetails::RegisterSequencer));
}

void FDetailsWorkspaceModule::AddMenuEntry(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("DetailsWorkspace", TAttribute<FText>(LOCTEXT("DetailsWorkspace", "Details Workspace")));
	MenuBuilder.AddSubMenu(
		LOCTEXT("DetailsWorkspace", "Details Workspace"),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& InMenuBuilder)
		{
			for (int i = 0; i < 4; i++)
			{
				InMenuBuilder.AddMenuEntry(
					FDetailsWorkspaceCommands::Get().OpenPluginWindow[i]
				);
			}
		}));
	MenuBuilder.EndSection();
}

void FDetailsWorkspaceModule::ShutdownModule()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);

	FDetailsWorkspaceStyle::Shutdown();

	FDetailsWorkspaceCommands::Unregister();

	for (int i = 0; i < DETAILS_WORKSPACE_DEFAULT_INSTANCES; i++)
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GetDefaultTabID(i));
	}

	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	SequencerModule.UnregisterOnSequencerCreated(OnSequencerCreatedHandle);
}

TSharedRef<SDockTab> FDetailsWorkspaceModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return CreateDetailsWorkSpace(SpawnTabArgs.GetTabId().ToString(), true,  true);
}

void FDetailsWorkspaceModule::PluginButtonClicked(int Index)
{
	FGlobalTabmanager::Get()->TryInvokeTab(GetDefaultTabID(Index));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDetailsWorkspaceModule, DetailsWorkspace)

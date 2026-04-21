//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "Delegates/IDelegateInstance.h"
#include "ArticyEditorConsoleCommands.h"
#include "Customizations/ArticyEditorCustomizationManager.h"
#include "Framework/Commands/UICommandList.h"
#include "Slate/SArticyIdProperty.h"


DECLARE_LOG_CATEGORY_EXTERN(LogArticyEditor, Log, All)

DECLARE_MULTICAST_DELEGATE(FOnImportFinished);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCompilationFinished, UArticyImportData*);
DECLARE_MULTICAST_DELEGATE(FOnAssetsGenerated);

/**
 * Enum representing the validity status of an import operation.
 */
enum EImportStatusValidity
{
	Valid, /**< Import is valid. */
	ImportantAssetMissing, /**< Important asset is missing. */
	FileMissing, /**< File is missing. */
	ImportDataAssetMissing /**< Import data asset is missing. */
};

/**
 * Articy Editor Module class for managing customizations and commands for the Articy plugin in Unreal Engine.
 */
class FArticyEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/**
	 * Get the Articy editor module instance.
	 *
	 * @return Reference to the Articy editor module.
	 */
	static inline FArticyEditorModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FArticyEditorModule>(TEXT("ArticyEditor"));
	}

	/**
	 * Get the customization manager for Articy editor.
	 *
	 * @return Shared pointer to the customization manager.
	 */
	TSharedPtr<FArticyEditorCustomizationManager> GetCustomizationManager() const { return CustomizationManager; }

	/**
	 * Retrieve all Articy packages in the project.
	 *
	 * @return An array of Articy packages.
	 */
	TArray<UArticyPackage*> ARTICYEDITOR_API GetPackagesSlow();

	/** Registers the Articy toolbar entry and menu extensions. */
	void RegisterArticyToolbar();
	/** Registers asset type actions for Articy asset types. */
	void RegisterAssetTypeActions();
	/** Registers directories that should be excluded from auto-reimport. */
	void RegisterAutoReimportExclusions();
	/** Registers the Articy editor console commands. */
	void RegisterConsoleCommands();
	/** Registers all default widget extensions. As of this point, the articy button */
	void RegisterDefaultArticyIdPropertyWidgetExtensions() const;
	/** Registers detail customizations for Articy types in the details panel. */
	void RegisterDetailCustomizations() const;
	/** Registers the directory watcher used to detect articy export changes on disk. */
	void RegisterDirectoryWatcher();
	/** Registers the graph pin factory used to render Articy types on Blueprint pins. */
	void RegisterGraphPinFactory() const;
	/** Registers UI commands bound to the plugin's toolbar and menu entries. */
	void RegisterPluginCommands();
	/** Registers the plugin's Project Settings entry. */
	void RegisterPluginSettings() const;
	/** Registers custom dock tabs used by the plugin (e.g. the GV debugger). */
	void RegisterToolTabs();

	/** Unregisters the plugin's Project Settings entry. */
	void UnregisterPluginSettings() const;

	/** Queues an import to be triggered once the editor is ready to do so. */
	void QueueImport();
	/** Returns true if an import has been queued and not yet triggered. */
	bool IsImportQueued();

	/** Delegate to bind custom logic you want to perform after the import has successfully finished */
	FOnCompilationFinished OnCompilationFinished;
	/** Broadcast after the importer has finished generating the articy asset files. */
	FOnAssetsGenerated OnAssetsGenerated;
	/** Broadcast after an import has fully finished (code + assets). */
	FOnImportFinished OnImportFinished;

private:
	/** Opens the Articy X Importer dock tab. */
	void OpenArticyWindow();
	/** Opens the Articy global variables debugger dock tab. */
	void OpenArticyGVDebugger();

	/** Checks the validity of the current import state and returns an EImportStatusValidity describing it. */
	EImportStatusValidity CheckImportStatusValidity() const;
	/**
	 * Callback fired by the directory watcher when files under the generated
	 * code directory change, used to decide whether an import should be queued.
	 *
	 * @param FileChanges The set of file changes reported by the watcher.
	 */
	void OnGeneratedCodeChanged(const TArray<struct FFileChangeData>& FileChanges) const;

	/** Clears any previously queued import request. */
	void UnqueueImport();
	/**
	 * Triggers a queued import if one is pending.
	 *
	 * @param b Whether to actually trigger the import (false clears the queue without importing).
	 */
	void TriggerQueuedImport(bool b);

	// Old tool UI hook callbacks required for UE4
#if ENGINE_MAJOR_VERSION == 4
	/**
	 * Extends the level editor toolbar with the Articy menu entry (UE4 only).
	 *
	 * @param Builder The toolbar builder used to add the extension.
	 */
	void AddToolbarExtension(FToolBarBuilder& Builder);
	/** Builds the Articy tools dropdown menu used in the UE4 toolbar extension. */
	TSharedRef<SWidget> OnGenerateArticyToolsMenu() const;
#endif

	/**
	 * Spawns the Articy X Importer menu dock tab.
	 *
	 * @param SpawnTabArgs The tab spawn arguments forwarded from the tab manager.
	 * @return The spawned dock tab.
	 */
	TSharedRef<class SDockTab> OnSpawnArticyMenuTab(const class FSpawnTabArgs& SpawnTabArgs) const;
	/**
	 * Spawns the Articy global variables debugger dock tab.
	 *
	 * @param SpawnTabArgs The tab spawn arguments forwarded from the tab manager.
	 * @return The spawned dock tab.
	 */
	TSharedRef<class SDockTab> OnSpawnArticyGVDebuggerTab(const class FSpawnTabArgs& SpawnTabArgs) const;

private:
	/** True if an import has been queued via QueueImport and not yet triggered. */
	bool bIsImportQueued = false;
	/** Handle used to clear the queued-import delegate when the import finally runs. */
	FDelegateHandle QueuedImportHandle;
	/** Handle used to unregister the directory watcher callback on shutdown. */
	FDelegateHandle GeneratedCodeWatcherHandle;
	/** Console command registry owned by the module. */
	FArticyEditorConsoleCommands* ConsoleCommands = nullptr;
	/** UI command list bound to the plugin's menu and toolbar entries. */
	TSharedPtr<FUICommandList> PluginCommands;
	/** The CustomizationManager registers and owns all customization factories */
	TSharedPtr<FArticyEditorCustomizationManager> CustomizationManager = nullptr;

	/** The CustomizationManager has ownership of the factories. These here are cached for removal at shutdown */
	TArray<const IArticyIdPropertyWidgetCustomizationFactory*> DefaultArticyRefWidgetCustomizationFactories;
};

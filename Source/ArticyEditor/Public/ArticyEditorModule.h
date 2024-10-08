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

	void RegisterArticyToolbar();
	void RegisterAssetTypeActions();
	void RegisterConsoleCommands();
	/** Registers all default widget extensions. As of this point, the articy button */
	void RegisterDefaultArticyIdPropertyWidgetExtensions() const;
	void RegisterDetailCustomizations() const;
	void RegisterDirectoryWatcher();
	void RegisterGraphPinFactory() const;
	void RegisterPluginCommands();
	void RegisterPluginSettings() const;
	void RegisterToolTabs();

	void UnregisterPluginSettings() const;

	void QueueImport();
	bool IsImportQueued();

	/** Delegate to bind custom logic you want to perform after the import has successfully finished */
	FOnCompilationFinished OnCompilationFinished;
	FOnAssetsGenerated OnAssetsGenerated;
	FOnImportFinished OnImportFinished;

private:
	void OpenArticyWindow();
	void OpenArticyGVDebugger();

	EImportStatusValidity CheckImportStatusValidity() const;
	void OnGeneratedCodeChanged(const TArray<struct FFileChangeData>& FileChanges) const;

	void UnqueueImport();
	void TriggerQueuedImport(bool b);

	// Old tool UI hook callbacks required for UE4
#if ENGINE_MAJOR_VERSION == 4
	void AddToolbarExtension(FToolBarBuilder& Builder);
	TSharedRef<SWidget> OnGenerateArticyToolsMenu() const;
#endif

	TSharedRef<class SDockTab> OnSpawnArticyMenuTab(const class FSpawnTabArgs& SpawnTabArgs) const;
	TSharedRef<class SDockTab> OnSpawnArticyGVDebuggerTab(const class FSpawnTabArgs& SpawnTabArgs) const;

private:
	bool bIsImportQueued = false;
	FDelegateHandle QueuedImportHandle;
	FDelegateHandle GeneratedCodeWatcherHandle;
	FArticyEditorConsoleCommands* ConsoleCommands = nullptr;
	TSharedPtr<FUICommandList> PluginCommands;
	/** The CustomizationManager registers and owns all customization factories */
	TSharedPtr<FArticyEditorCustomizationManager> CustomizationManager = nullptr;

	/** The CustomizationManager has ownership of the factories. These here are cached for removal at shutdown */
	TArray<const IArticyIdPropertyWidgetCustomizationFactory*> DefaultArticyRefWidgetCustomizationFactories;
};

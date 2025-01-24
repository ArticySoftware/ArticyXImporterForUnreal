//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"
#include "ArticyGlobalVariables.h"
#include "Slate/GV/SArticyGlobalVariables.h"

/**
 * @class FArticyGvEditor
 * @brief Editor class for managing and editing Articy Global Variables.
 *
 * This class provides an editor interface for editing Articy Global Variables within Unreal Engine.
 */
class FArticyGvEditor final : public FAssetEditorToolkit, FEditorUndoClient, FNotifyHook, FGCObject
{
public:
	/** Destructor for FArticyGVEditor. */
	virtual ~FArticyGvEditor() override;

	/**
	 * @brief Initializes the Articy Global Variables editor.
	 *
	 * @param Mode The toolkit mode (standalone or world-centric).
	 * @param InitToolkitHost The toolkit host for integration.
	 * @param ObjectToEdit The ArticyGlobalVariables object to be edited.
	 */
	void InitArticyGvEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UArticyGlobalVariables* ObjectToEdit);

	/** FGCObject Interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	/** IToolkit Interface */
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/** FAssetEditorToolkit Interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& TabManager) override;
	virtual FString GetReferencerName() const override;

private:
	/**
	 * @brief Spawns the Articy Global Variables tab.
	 *
	 * @param Args The arguments for spawning the tab.
	 * @return A shared reference to the created SDockTab.
	 */
	TSharedRef<SDockTab> SpawnTab_ArticyGv(const FSpawnTabArgs& Args) const;

	/** The ArticyGlobalVariables object being edited. */
	TWeakObjectPtr<UArticyGlobalVariables> GlobalVariables;

	/** Widget for displaying and interacting with global variables. */
	TSharedPtr<SArticyGlobalVariables> GlobalVariablesWidget;

	/** The name identifier for the Articy Global Variables tab. */
	static const FName ArticyGvTab;
};

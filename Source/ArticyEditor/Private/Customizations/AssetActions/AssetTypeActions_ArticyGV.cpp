//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "Customizations/AssetActions/AssetTypeActions_ArticyGV.h"
#include "Customizations/ArticyGVEditor.h"

/**
 * @brief Opens the editor for the specified Articy global variable assets.
 *
 * This method initializes the ArticyGV editor for each selected ArticyGlobalVariables object.
 *
 * @param InObjects The array of objects to be edited.
 * @param EditWithinLevelEditor An optional toolkit host to edit within the level editor.
 */
void FAssetTypeActions_ArticyGV::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		const auto& ArticyGV = Cast<UArticyGlobalVariables>(*ObjIt);
		if (ArticyGV != nullptr)
		{
			TSharedRef<FArticyGvEditor> Editor(new FArticyGvEditor);
			Editor->InitArticyGvEditor(Mode, EditWithinLevelEditor, ArticyGV);
		}
	}
}

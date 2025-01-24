//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "AssetTypeActions_Base.h"
#include "ArticyGlobalVariables.h"

/**
 * @class FAssetTypeActions_ArticyGv
 * @brief Implements actions for handling Articy global variables in the editor.
 *
 * This class provides the functionality to manage and display Articy global variable assets within the Unreal Editor.
 */
class FAssetTypeActions_ArticyGv final : public FAssetTypeActions_Base
{
public:
	/**
	 * @brief Retrieves the display name for the asset type.
	 *
	 * @return The localized text for the asset type name.
	 */
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ArticyGV", "ArticyGV"); }

	/**
	 * @brief Gets the class supported by this asset type.
	 *
	 * @return The class type supported by this asset action.
	 */
	virtual UClass* GetSupportedClass() const override
	{
		return UArticyGlobalVariables::StaticClass();
	}

	/**
	 * @brief Returns the color associated with this asset type.
	 *
	 * @return The color used to represent this asset type in the editor.
	 */
	virtual FColor GetTypeColor() const override
	{
		return FColor(128, 128, 64);
	}

	/**
	 * @brief Returns the asset categories this asset type belongs to.
	 *
	 * @return The categories this asset type belongs to.
	 */
	virtual uint32 GetCategories() override { return EAssetTypeCategories::None; }

	/**
	 * @brief Opens the editor for the specified Articy global variable assets.
	 *
	 * Initializes the ArticyGV editor for each selected ArticyGlobalVariables object.
	 *
	 * @param InObjects The array of objects to be edited.
	 * @param EditWithinLevelEditor An optional toolkit host to edit within the level editor.
	 */
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
};

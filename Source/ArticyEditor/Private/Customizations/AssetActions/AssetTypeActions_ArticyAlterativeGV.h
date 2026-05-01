//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "AssetTypeActions_Base.h"

/**
 * @class FAssetTypeActions_ArticyAlterativeGV
 * @brief Implements actions for handling Articy alternative global variables in the editor.
 *
 * This class provides the functionality to manage and display Articy alternative global variable assets within the Unreal Editor.
 */
class FAssetTypeActions_ArticyAlterativeGV
	: public FAssetTypeActions_Base
{
public:
	/**
	 * @brief Determines if the asset can be filtered in the content browser.
	 *
	 * @return True if the asset can be filtered, false otherwise.
	 */
	virtual bool CanFilter() override;

	/**
	 * @brief Returns the asset categories this asset type belongs to.
	 *
	 * @return The categories this asset type belongs to.
	 */
	virtual uint32 GetCategories() override;

	/**
	 * @brief Retrieves the display name for the asset type.
	 *
	 * @return The localized text for the asset type name.
	 */
	virtual FText GetName() const override;

	/**
	 * @brief Gets the class supported by this asset type.
	 *
	 * @return The class type supported by this asset action.
	 */
	virtual UClass* GetSupportedClass() const override;

	/**
	 * @brief Returns the color associated with this asset type.
	 *
	 * @return The color used to represent this asset type in the editor.
	 */
	virtual FColor GetTypeColor() const override;
};

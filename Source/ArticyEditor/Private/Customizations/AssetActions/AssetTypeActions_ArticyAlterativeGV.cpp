//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "AssetTypeActions_ArticyAlterativeGV.h"
#include "ArticyAlternativeGlobalVariables.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

/**
 * @brief Determines if the asset can be filtered in the content browser.
 *
 * @return True if the asset can be filtered, false otherwise.
 */
bool FAssetTypeActions_ArticyAlterativeGV::CanFilter()
{
	return true;
}

/**
 * @brief Returns the asset categories this asset type belongs to.
 *
 * @return The categories this asset type belongs to.
 */
uint32 FAssetTypeActions_ArticyAlterativeGV::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

/**
 * @brief Retrieves the display name for the asset type.
 *
 * @return The localized text for the asset type name.
 */
FText FAssetTypeActions_ArticyAlterativeGV::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_AlternativeGlobalVariables", "Alternative Articy Global Variables");
}

/**
 * @brief Gets the class supported by this asset type.
 *
 * @return The class type supported by this asset action.
 */
UClass* FAssetTypeActions_ArticyAlterativeGV::GetSupportedClass() const
{
	return UArticyAlternativeGlobalVariables::StaticClass();
}

/**
 * @brief Returns the color associated with this asset type.
 *
 * @return The color used to represent this asset type in the editor.
 */
FColor FAssetTypeActions_ArticyAlterativeGV::GetTypeColor() const
{
	return FColor(128, 128, 64);
}

#undef LOCTEXT_NAMESPACE

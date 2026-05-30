//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "AssetTypeActions_ArticyImportData.h"
#include "ArticyImportData.h"
#include "EditorFramework/AssetImportData.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

/**
 * @brief Marks the asset as importable.
 *
 * @return Always true.
 */
bool FAssetTypeActions_ArticyImportData::IsImportedAsset() const
{
	return true;
}

/**
 * @brief Resolves source file paths from the asset's import data.
 *
 * @param TypeAssets The selected assets.
 * @param OutSourceFilePaths Populated with the resolved paths.
 */
void FAssetTypeActions_ArticyImportData::GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const
{
	for (UObject* Asset : TypeAssets)
	{
		if (const UArticyImportData* ImportData = Cast<UArticyImportData>(Asset))
		{
			if (ImportData->ImportData)
			{
				ImportData->ImportData->ExtractFilenames(OutSourceFilePaths);
			}
		}
	}
}

/**
 * @brief Returns the asset categories.
 *
 * @return The categories this asset belongs to.
 */
uint32 FAssetTypeActions_ArticyImportData::GetCategories()
{
	return EAssetTypeCategories::Misc;
}

/**
 * @brief Returns the display name.
 *
 * @return The localized asset type name.
 */
FText FAssetTypeActions_ArticyImportData::GetName() const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_ArticyImportData", "Articy Import Data");
}

/**
 * @brief Returns the supported class.
 *
 * @return UArticyImportData::StaticClass().
 */
UClass* FAssetTypeActions_ArticyImportData::GetSupportedClass() const
{
	return UArticyImportData::StaticClass();
}

/**
 * @brief Returns the asset color.
 *
 * @return The color used in the editor.
 */
FColor FAssetTypeActions_ArticyImportData::GetTypeColor() const
{
	return FColor(128, 128, 64);
}

#undef LOCTEXT_NAMESPACE

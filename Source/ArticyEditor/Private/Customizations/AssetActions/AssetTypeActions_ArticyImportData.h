//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "AssetTypeActions_Base.h"

/**
 * @class FAssetTypeActions_ArticyImportData
 * @brief Asset type actions for UArticyImportData. Marks the asset as imported so
 * auto-reimport routes .articyue changes through UArticyJSONFactory.
 */
class FAssetTypeActions_ArticyImportData
	: public FAssetTypeActions_Base
{
public:
	/**
	 * @brief Marks the asset as importable.
	 *
	 * @return Always true.
	 */
	virtual bool IsImportedAsset() const override;

	/**
	 * @brief Resolves source file paths from the asset's import data.
	 *
	 * @param TypeAssets The selected assets.
	 * @param OutSourceFilePaths Populated with the resolved paths.
	 */
	virtual void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths) const override;

	/**
	 * @brief Returns the asset categories.
	 *
	 * @return The categories this asset belongs to.
	 */
	virtual uint32 GetCategories() override;

	/**
	 * @brief Returns the display name.
	 *
	 * @return The localized asset type name.
	 */
	virtual FText GetName() const override;

	/**
	 * @brief Returns the supported class.
	 *
	 * @return UArticyImportData::StaticClass().
	 */
	virtual UClass* GetSupportedClass() const override;

	/**
	 * @brief Returns the asset color.
	 *
	 * @return The color used in the editor.
	 */
	virtual FColor GetTypeColor() const override;
};

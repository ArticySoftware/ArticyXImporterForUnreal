//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyImportData.h"

/**
 * Enumerates the possible results of ensuring that an import data asset is valid.
 */
enum EImportDataEnsureResult
{
	/** Import data found in the asset registry. */
	AssetRegistry,
	/** Import data asset generated successfully. */
	Generation,
	/** Parameter was already valid. */
	Success,
	/** Import data asset was not found or generated. */
	Failure
};

/**
 * FArticyEditorFunctionLibrary provides static functions for handling Articy data imports and asset management.
 */
class ARTICYEDITOR_API FArticyEditorFunctionLibrary
{

public:
	/**
	 * Forces a complete reimport of the Articy data, resetting all hashes and package definitions.
	 *
	 * @param UArticyImportData The Articy import data object to reimport.
	 * @return The number of changes reimported, or -1 if the operation failed.
	 */
	static int32 ForceCompleteReimport(UArticyImportData* = nullptr);

	/**
	 * Reimports changes from the Articy data without resetting all data.
	 *
	 * @param UArticyImportData The Articy import data object to reimport.
	 * @return The number of changes reimported, or -1 if the operation failed.
	 */
	static int32 ReimportChanges(UArticyImportData* = nullptr);

	/**
	 * Regenerates assets from the given Articy import data.
	 *
	 * @param UArticyImportData The Articy import data object from which assets are regenerated.
	 * @return -1 if the operation failed.
	 */
	static int32 RegenerateAssets(UArticyImportData* = nullptr);

	/**
	 * Ensures that the Articy import data asset is valid and available.
	 * Generates a new import data asset if necessary.
	 *
	 * @param UArticyImportData A pointer to a pointer of the Articy import data object.
	 * @return The result of the ensure operation.
	 */
	static EImportDataEnsureResult EnsureImportDataAsset(UArticyImportData**);

private:
	/**
	 * Generates a new Articy import data asset.
	 * Searches for an existing .articyue file in the specified directory and imports it.
	 *
	 * @return A pointer to the generated Articy import data asset, or nullptr if the operation failed.
	 */
	static UArticyImportData* GenerateImportDataAsset();
};

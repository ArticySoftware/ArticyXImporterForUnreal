//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

class UArticyImportData;
struct FArticyGVInfo;
class FString;

/**
 * @class DatabaseGenerator
 * @brief Generates project-specific Articy Database code and assets.
 */
class DatabaseGenerator
{
public:
	/**
	 * @brief Generates code for the Articy database class based on import data.
	 *
	 * @param Data The import data used for code generation.
	 * @param OutFile The output filename for the generated code.
	 */
	static void GenerateCode(const UArticyImportData* Data, FString& OutFile);

	/**
	 * @brief Generates the Articy database asset based on import data.
	 *
	 * @param Data The import data used for asset generation.
	 * @return A pointer to the generated Articy database asset.
	 */
	static class UArticyDatabase* GenerateAsset(const UArticyImportData* Data);
};

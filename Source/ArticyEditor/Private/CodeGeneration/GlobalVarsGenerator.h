//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

class UArticyImportData;
struct FArticyGVInfo;
class FString;

/**
 * @class GlobalVarsGenerator
 * @brief Generates global variables code and assets for Articy import data.
 */
class GlobalVarsGenerator
{
public:
	/**
	 * @brief Generates code for Articy global variables based on import data.
	 *
	 * Creates header files for global variables, organizing them into namespaces as needed.
	 *
	 * @param Data The import data used for code generation.
	 * @param OutFile The output filename for the generated code.
	 */
	static void GenerateCode(const UArticyImportData* Data, FString& OutFile);

	/**
	 * @brief Generates the Articy global variables asset based on import data.
	 *
	 * Creates the project-specific global variables asset for Articy.
	 *
	 * @param Data The import data used for asset generation.
	 */
	static void GenerateAsset(const UArticyImportData* Data);

private:
	GlobalVarsGenerator() {}
	~GlobalVarsGenerator() {}
};

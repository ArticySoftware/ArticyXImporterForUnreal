//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyImportData.h"

/**
 * @class ExpressoScriptsGenerator
 * @brief Generates expresso scripts and methods for Articy import data.
 */
class ExpressoScriptsGenerator
{
public:
	/**
	 * @brief Generates code for the Articy expresso scripts class.
	 *
	 * Manages the code generation process for the expresso scripts, including user methods and global variables.
	 *
	 * @param Data The import data used for code generation.
	 * @param OutFile The output filename for the generated code.
	 */
	static void GenerateCode(const UArticyImportData* Data, FString& OutFile);

	/**
	 * @brief Returns the filename of the generated expresso scripts class (with extension).
	 *
	 * Generates the filename for the expresso scripts based on the import data.
	 *
	 * @param Data The import data used for filename generation.
	 * @return The filename of the generated expresso scripts class.
	 */
	static FString GetFilename(const UArticyImportData* Data);
};

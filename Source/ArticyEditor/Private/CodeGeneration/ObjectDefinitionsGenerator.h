//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

class UArticyImportData;
class FString;

/**
 * @class ObjectDefinitionsGenerator
 * @brief Generates code for object definitions in Articy import data.
 */
class ObjectDefinitionsGenerator
{
public:
	/**
	 * @brief Generates code for Articy object definitions based on import data.
	 *
	 * Creates header files for object types defined in the Articy project.
	 *
	 * @param Data The import data used for code generation.
	 * @param OutFile The output filename for the generated code.
	 */
	static void GenerateCode(UArticyImportData* Data, FString& OutFile);

private:
	ObjectDefinitionsGenerator() {};
	~ObjectDefinitionsGenerator() {};
};

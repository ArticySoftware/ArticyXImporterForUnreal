//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

class UArticyImportData;
class FString;

/**
 * @class InterfacesGenerator
 * @brief Generates interface code for Articy import data.
 */
class InterfacesGenerator
{
public:
	/**
	 * @brief Generates code for Articy interfaces based on import data.
	 *
	 * Creates header files for interfaces associated with Articy features.
	 *
	 * @param Data The import data used for code generation.
	 * @param OutFile The output filename for the generated code.
	 */
	static void GenerateCode(UArticyImportData* Data, FString& OutFile);

private:
	InterfacesGenerator() {};
	~InterfacesGenerator() {};
};

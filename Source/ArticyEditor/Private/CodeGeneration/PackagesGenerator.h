//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

/**
 * @class PackagesGenerator
 * @brief Generates package assets for Articy import data.
 */
class PackagesGenerator
{
public:
	/**
	 * @brief Generates package assets for Articy import data.
	 *
	 * Creates new Articy package objects based on the definitions in the import data.
	 *
	 * @param Data The import data used for asset generation.
	 */
	static void GenerateAssets(class UArticyImportData* Data);

private:
	PackagesGenerator() {}
	virtual ~PackagesGenerator() {}
};

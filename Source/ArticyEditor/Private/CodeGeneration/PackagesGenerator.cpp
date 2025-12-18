//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "PackagesGenerator.h"
#include "ArticyImportData.h"

#define LOCTEXT_NAMESPACE "PackagesGenerator"

/**
 * @brief Generates package assets for Articy import data.
 *
 * This function creates new Articy package objects based on the definitions in the import data.
 *
 * @param Data The import data used for asset generation.
 */
void PackagesGenerator::GenerateAssets(UArticyImportData* Data)
{
	// Generate new Articy objects
	const auto& ArticyPackageDefs = Data->GetPackageDefs();
	ArticyPackageDefs.GenerateAssets(Data);
}

#undef LOCTEXT_NAMESPACE

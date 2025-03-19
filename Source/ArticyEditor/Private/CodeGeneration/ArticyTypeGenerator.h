//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

class UArticyImportData;
struct FArticyType;
class FString;

/**
 * @class ArticyTypeGenerator
 * @brief A utility class for generating ArticyType structures from import data.
 */
class ArticyTypeGenerator
{
public:
    /**
     * @brief Generates code for the ArticyType structure from the given import data.
     *
     * This function creates a header file for the ArticyTypeSystem using the provided
     * import data and sets the output file name.
     *
     * @param Data The import data used to generate the ArticyType structure.
     * @param OutFile The output file name for the generated code.
     */
    static void GenerateCode(const UArticyImportData* Data, FString& OutFile);

    /**
     * @brief Generates an Articy type asset from the given import data.
     *
     * This function creates a new ArticyTypeSystem asset based on the provided
     * import data and populates it with type information.
     *
     * @param Data The import data used to generate the Articy type asset.
     */
    static void GenerateAsset(UArticyImportData* Data);

private:
    /**
     * @brief Private default constructor to prevent instantiation.
     */
    ArticyTypeGenerator() {}

    /**
     * @brief Private destructor.
     */
    ~ArticyTypeGenerator() {}
};

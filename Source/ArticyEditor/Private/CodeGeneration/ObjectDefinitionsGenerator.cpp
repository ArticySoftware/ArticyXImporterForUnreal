//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ObjectDefinitionsGenerator.h"
#include "CodeFileGenerator.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"

/**
 * @brief Generates code for Articy object definitions based on import data.
 *
 * This function creates header files for object types defined in the Articy project.
 *
 * @param Data The import data used for code generation.
 * @param OutFile The output filename for the generated code.
 */
void ObjectDefinitionsGenerator::GenerateCode(const UArticyImportData* Data, FString& OutFile)
{
	OutFile = CodeGenerator::GetGeneratedTypesFilename(Data);
	CodeFileGenerator(OutFile + ".h", true, [&](CodeFileGenerator* header)
		{
			header->Line("#include \"CoreUObject.h\"");
			header->Line("#include \"ArticyBaseInclude.h\"");
			header->Line("#include \"" + CodeGenerator::GetGeneratedInterfacesFilename(Data) + ".h\"");
			header->Line("#include \"" + CodeGenerator::GetGeneratedTypesFilename(Data) + ".generated.h\"");
			header->Line();

			for (const auto& type : Data->GetObjectDefs().GetTypes())
				type.Value.GenerateCode(*header, Data);
		});
}

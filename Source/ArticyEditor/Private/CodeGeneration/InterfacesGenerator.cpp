//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "InterfacesGenerator.h"
#include "CodeFileGenerator.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"

/**
 * @brief Generates code for Articy interfaces based on import data.
 *
 * This function creates header files for interfaces associated with Articy features.
 *
 * @param Data The import data used for code generation.
 * @param OutFile The output filename for the generated code.
 */
void InterfacesGenerator::GenerateCode(const UArticyImportData* Data, FString& OutFile)
{
	OutFile = CodeGenerator::GetGeneratedInterfacesFilename(Data);
	CodeFileGenerator(OutFile + ".h", true, [&](CodeFileGenerator* header)
		{
			header->Line("#include \"CoreUObject.h\"");
			if (Data->GetObjectDefs().GetFeatures().Num() > 0)
				header->Line("#include \"" + CodeGenerator::GetGeneratedInterfacesFilename(Data) + ".generated.h\"");
			header->Line();

			for (auto pair : Data->GetObjectDefs().GetFeatures())
			{
				FArticyTemplateFeatureDef feature = pair.Value;

				header->Line();
				header->UInterface(CodeGenerator::GetFeatureInterfaceClassName(Data, feature, true),
					"MinimalAPI, BlueprintType, Category=\"" + Data->GetProject().TechnicalName + " Feature Interfaces\", meta=(CannotImplementInterfaceInBlueprint)",
					"UNINTERFACE generated from Articy " + feature.GetDisplayName() + " Feature", [&]
					{
						header->Line("public:", false, true, -1);
						header->Line();
						header->Method("virtual class " + feature.GetCppType(Data, true), "GetFeature" + feature.GetTechnicalName(), "", [&]
							{
								header->Line("return nullptr", true);
							}, "", true, "BlueprintCallable", "const");
					});
			}
		});
}

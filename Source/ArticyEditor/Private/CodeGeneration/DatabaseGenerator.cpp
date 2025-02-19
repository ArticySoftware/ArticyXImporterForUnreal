//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "DatabaseGenerator.h"
#include "CodeGenerator.h"
#include "ArticyDatabase.h"
#include "ArticyImportData.h"
#include "CodeFileGenerator.h"
#include "ExpressoScriptsGenerator.h"
#include "ArticyImporterHelpers.h"

/**
 * @brief Generates code for the Articy database class based on import data.
 *
 * This function creates a header file for the project-specific Articy database class.
 *
 * @param Data The import data used for code generation.
 * @param OutFile The output filename for the generated code.
 */
void DatabaseGenerator::GenerateCode(const UArticyImportData* Data, FString& OutFile)
{
	if (!ensure(Data))
		return;

	OutFile = CodeGenerator::GetDatabaseClassname(Data, true);
	CodeFileGenerator(OutFile + ".h", true, [&](CodeFileGenerator* header)
		{
			header->Line("#include \"CoreUObject.h\"");
			header->Line("#include \"ArticyDatabase.h\"");
			header->Line("#include \"" + ExpressoScriptsGenerator::GetFilename(Data) + "\"");
			header->Line("#include \"" + OutFile + ".generated.h\"");
			header->Line();

			const auto& className = CodeGenerator::GetDatabaseClassname(Data);
			header->Class(className + " : public UArticyDatabase", "", true, [&]
				{
					const auto& expressoClass = CodeGenerator::GetExpressoScriptsClassname(Data, false);

					header->AccessModifier("public");

					header->Line();
					header->Method("static " + className + "*", "Get", "const UObject* WorldContext", [&]
						{
							header->Line(FString::Printf(TEXT("return static_cast<%s*>(Super::Get(WorldContext));"), *className));
						}, "Get the instance (copy of the asset) of the database.", true,
						"BlueprintPure, Category = \"articy:draft\", meta=(HidePin=\"WorldContext\", DefaultToSelf=\"WorldContext\", DisplayName=\"GetArticyDB\", keywords=\"database\")");

					header->Line();

					const auto& globalVarsClass = CodeGenerator::GetGlobalVarsClassname(Data);
					header->Method(globalVarsClass + "*", "GetGVs", "", [&]
						{
							header->Line(FString::Printf(TEXT("return static_cast<%s*>(Super::GetGVs());"), *globalVarsClass));
						}, "Get the global variables.", true,
						"BlueprintPure, Category = \"articy:draft\", meta=(keywords=\"global variables\")", "const override");
					header->Method(globalVarsClass + "*", "GetRuntimeGVs", "UArticyAlternativeGlobalVariables* Asset", [&]
						{
							header->Line(FString::Printf(TEXT("return static_cast<%s*>(Super::GetRuntimeGVs(Asset));"), *globalVarsClass));
						}, "Gets the current runtime instance of a set of GVs.", true,
						"BlueprintPure, Category = \"articy:draft\", meta=(keywords=\"global variables\")", "const override");
				});
		});
}

/**
 * @brief Generates the Articy database asset based on import data.
 *
 * This function creates the project-specific Articy database asset.
 *
 * @param Data The import data used for asset generation.
 * @return A pointer to the generated Articy database asset.
 */
UArticyDatabase* DatabaseGenerator::GenerateAsset(const UArticyImportData* Data)
{
	const auto& className = CodeGenerator::GetDatabaseClassname(Data, true);
	return ArticyImporterHelpers::GenerateAsset<UArticyDatabase>(*className, FApp::GetProjectName(), "", "", RF_ArchetypeObject, true);
}

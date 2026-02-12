//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "GlobalVarsGenerator.h"
#include "CodeGenerator.h"
#include "CodeFileGenerator.h"
#include "ArticyImportData.h"
#include "ArticyGlobalVariables.h"
#include "ArticyImporterHelpers.h"

/**
 * @brief Generates code for Articy global variables based on import data.
 *
 * This function creates header files for global variables, organizing them into namespaces as needed.
 *
 * @param Data The import data used for code generation.
 * @param OutFile The output filename for the generated code.
 */
void GlobalVarsGenerator::GenerateCode(const UArticyImportData* Data, FString& OutFile)
{
	if (!ensure(Data))
		return;

	OutFile = CodeGenerator::GetGlobalVarsClassname(Data, true);
	CodeFileGenerator(OutFile + ".h", true, [&](CodeFileGenerator* header)
		{
			header->Line("#include \"CoreUObject.h\"");
			header->Line("#include \"ArticyGlobalVariables.h\"");
			header->Line("#include \"" + OutFile + ".generated.h\"");

			// Disable "optimization cannot be applied due to function size" compile error. This error is caused by the huge constructor when all expresso
			// scripts are added to the collection and this pragma disables the optimizations.
			header->Line("#if !((defined(PLATFORM_PS4) && PLATFORM_PS4) || (defined(PLATFORM_PS5) && PLATFORM_PS5))");
			header->Line("#pragma warning(push)");
			header->Line("#pragma warning(disable: 4883) //<disable \"optimization cannot be applied due to function size\" compile error.");
			header->Line("#endif");

			// Generate all the namespaces (with comment)
			for (const auto& ns : Data->GetGlobalVars().Namespaces)
			{
				header->Line();
				header->Class(ns.CppTypename + TEXT(" : public UArticyBaseVariableSet"), ns.Description, true, [&]
					{
						// Generate all the variables in public section
						header->Line("public:", false, true, -1);

						for (const FArticyGVar& var : ns.Variables)
							header->Variable(var.GetCPPTypeString() + "*", var.Variable, "nullptr", var.Description, true,
								FString::Printf(TEXT("VisibleAnywhere, BlueprintReadOnly, Category=\"%s\""), *ns.Namespace));

						header->Line();

						// In the constructor, create the subobject for all the variables
						header->Method("", ns.CppTypename, "", [&]
							{
								// Create subobject
								for (const auto& var : ns.Variables)
									header->Line(FString::Printf(TEXT("%s = CreateDefaultSubobject<%s>(\"%s\");"), *var.Variable, *var.GetCPPTypeString(), *var.Variable));
							});

						header->Line();

						// In the Init method, call all the variable's Init method
						header->Method("void", "Init", "UArticyGlobalVariables* const Store", [&]
							{
								header->Comment("initialize the variables");

								for (const auto& var : ns.Variables)
								{
									header->Line(FString::Printf(TEXT("%s->Init<%s>(this, Store, TEXT(\"%s.%s\"), %s);"), *var.Variable, *var.GetCPPTypeString(), *ns.Namespace, *var.Variable, *var.GetCPPValueString()));
									header->Line(FString::Printf(TEXT("this->Variables.Add(%s);"), *var.Variable));
								}
							});
					});
			}

			header->Line();

			// Now generate the UArticyGlobalVariables class
			const auto& type = CodeGenerator::GetGlobalVarsClassname(Data, false);
			header->Class(type + " : public UArticyGlobalVariables", TEXT("Global Articy Variables"), true, [&]
				{
					header->Line("public:", false, true, -1);

					// Generate all the namespaces
					for (const auto& ns : Data->GetGlobalVars().Namespaces)
					{
						header->Variable(ns.CppTypename + TEXT("*"), ns.Namespace, TEXT("nullptr"), ns.Description, true,
							FString::Printf(TEXT("VisibleAnywhere, BlueprintReadOnly, Category=\"%s\""), *ns.Namespace));
					}

					//---------------------------------------------------------------------------//
					header->Line();

					header->Method(TEXT(""), type, TEXT(""), [&]
						{
							header->Comment(TEXT("create the namespaces"));
							for (const auto& ns : Data->GetGlobalVars().Namespaces)
								header->Line(FString::Printf(TEXT("%s = CreateDefaultSubobject<%s>(\"%s\");"), *ns.Namespace, *ns.CppTypename, *ns.Namespace));

							header->Line();
							header->Line("Init();");
						});

					//---------------------------------------------------------------------------//
					header->Line();

					header->Method(TEXT("void"), TEXT("Init"), TEXT(""), [&]
						{
							header->Comment(TEXT("initialize the namespaces"));
							for (const auto& ns : Data->GetGlobalVars().Namespaces)
							{
								header->Line(FString::Printf(TEXT("%s->Init(this);"), *ns.Namespace));
								header->Line(FString::Printf(TEXT("this->VariableSets.Add(%s);"), *ns.Namespace));
							}
						});

					//---------------------------------------------------------------------------//
					header->Line();

					header->Method(TEXT("static ") + type + TEXT("*"), TEXT("GetDefault"), TEXT("const UObject* WorldContext"), [&]
						{
							header->Line(TEXT("return static_cast<") + type + TEXT("*>(UArticyGlobalVariables::GetDefault(WorldContext));"));
						}, TEXT("Get the default GlobalVariables (a copy of the asset)."), true,
							TEXT("BlueprintPure, Category=\"ArticyGlobalVariables\", meta=(HidePin=\"WorldContext\", DefaultToSelf=\"WorldContext\", DisplayName=\"GetArticyGV\", keywords=\"global variables\")"));
				});

			header->Line("#if !((defined(PLATFORM_PS4) && PLATFORM_PS4) || (defined(PLATFORM_PS5) && PLATFORM_PS5))");
			header->Line("#pragma warning(pop)");
			header->Line("#endif");
		});
}

/**
 * @brief Generates the Articy global variables asset based on import data.
 *
 * This function creates the project-specific global variables asset for Articy.
 *
 * @param Data The import data used for asset generation.
 */
void GlobalVarsGenerator::GenerateAsset(const UArticyImportData* Data)
{
	const auto& className = CodeGenerator::GetGlobalVarsClassname(Data, true);
	ArticyImporterHelpers::GenerateAsset<UArticyGlobalVariables>(*className, FApp::GetProjectName(), TEXT(""), TEXT(""), RF_ArchetypeObject);
}

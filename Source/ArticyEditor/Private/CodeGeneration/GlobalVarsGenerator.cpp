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
 * @brief Generates static code for Articy global variables.
 *
 * The generated file defines a static structure that supports namespaces,
 * with variables dynamically initialized from asset data.
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

            header->Line("#if !((defined(PLATFORM_PS4) && PLATFORM_PS4) || (defined(PLATFORM_PS5) && PLATFORM_PS5))");
            header->Line("#pragma warning(push)");
            header->Line("#pragma warning(disable: 4883)");
            header->Line("#endif");

            // Static global variables class with namespaces
            const auto type = CodeGenerator::GetGlobalVarsClassname(Data, false);
            header->Class(type + " : public UArticyBaseVariableSet", TEXT("Static Articy Global Variables"), true, [&]
                {
                    header->Line("public:", false, true, -1);

                    // Generic container for namespaces
                    header->Variable("TMap<FString, FArticyNamespaceVariables>", "Namespaces", "{}", 
                        "Container for namespaces and their variables.", true, 
                        "VisibleAnywhere, BlueprintReadOnly, Category=\"GlobalVariables\"");

                    header->Line();

                    // Constructor
                    header->Method("", type, "", [&]
                        {
                            header->Line("/* Initialization will be handled dynamically */");
                        });

                    // Init method
                    header->Method("void", "Init", "UArticyGlobalVariables* const Store", [&]
                        {
                            header->Comment("Initialize variables dynamically from the asset.");
                            header->Line("const auto* Asset = Cast<UArticyGlobalVariablesData>(GetDefault<UObject>());");
                            header->Line("if (!Asset) return;");

                            header->Line();
                            header->Line("for (const auto& NamespaceData : Asset->Namespaces)");
                            header->Line("{", false, true, 1);
                            header->Variable("FArticyNamespaceVariables", "NamespaceVariables");

                            // Iterate through variables in the namespace
                            header->Line("for (const auto& VariableData : NamespaceData.Value.Variables)");
                            header->Line("{", false, true, 1);
                            header->Line("UClass* VariableClass = FindObject<UClass>(ANY_PACKAGE, *VariableData.VariableTypeClass);");
                            header->Line("if (!VariableClass)");
                            header->Line("{", false, true, 1);
                            header->Line("    UE_LOG(LogTemp, Warning, TEXT(\"Variable type %s not found!\"), *VariableData.VariableTypeClass);");
                            header->Line("    continue;");
                            header->Line("}", false, true, -1);

                            header->Line("UArticyVariable* NewVariable = NewObject<UArticyVariable>(this, VariableClass, *VariableData.VariableName);");
                            header->Line("if (NewVariable)");
                            header->Line("{", false, true, 1);
                            header->Line("    NewVariable->Init<UArticyVariable>(this, Store, *VariableData.FullName, VariableData.DefaultValue);");
                            header->Line("    Variables.Add(VariableData.VariableName, NewVariable);");
                            header->Line("}", false, true, -1);

                            header->Line("}", false, true, -1);

                            header->Line("Namespaces.Add(NamespaceData.Key, NamespaceVariables);");
                            header->Line("}", false, true, -1);
                        });

                    header->Line();

                    // Static method to get default instance
                    header->Method("static " + type + "*", "GetDefault", "const UObject* WorldContext", [&]
                        {
                            header->Line("return reinterpret_cast<" + type + "*>(UArticyGlobalVariables::GetDefault(WorldContext));");
                        }, "Get the default GlobalVariables (a copy of the asset).", true,
                            "BlueprintPure, Category=\"ArticyGlobalVariables\", meta=(HidePin=\"WorldContext\", DefaultToSelf=\"WorldContext\", DisplayName=\"GetArticyGV\", keywords=\"global variables\")");
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
    const auto className = CodeGenerator::GetGlobalVarsClassname(Data, true);

    auto* Asset = ArticyImporterHelpers::GenerateAsset<UArticyGlobalVariablesData>(
        *className, FApp::GetProjectName(), TEXT(""), TEXT(""), RF_ArchetypeObject);

    if (Asset)
    {
        Asset->Namespaces.Empty();

        for (const auto& ns : Data->GetGlobalVars().Namespaces)
        {
            FArticyNamespaceData NamespaceData;
            NamespaceData.NamespaceName = ns.Namespace;

            for (const auto& var : ns.Variables)
            {
                FArticyVariableData VariableData;
                VariableData.VariableName = var.Variable;
                VariableData.VariableTypeClass = var.GetCPPTypeString();  // Store fully qualified class name.
                VariableData.FullName = FString::Printf(TEXT("%s.%s"), *ns.Namespace, *var.Variable);
                VariableData.DefaultValue = var.GetCPPValueString();  // Serialized default value.

                NamespaceData.Variables.Add(VariableData);
            }

            Asset->Namespaces.Add(NamespaceData.NamespaceName, NamespaceData);
        }

        Asset->MarkPackageDirty();  // Save changes to the asset.
    }
}

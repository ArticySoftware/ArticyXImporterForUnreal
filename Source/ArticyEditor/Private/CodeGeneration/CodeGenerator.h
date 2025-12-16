//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ArticyImportData.h"
#include "Misc/CompilationResult.h"


class UArticyImportData;

/**
 * @class CodeGenerator
 * @brief Manages the generation, compilation, and asset creation for Articy-imported data.
 */
class CodeGenerator
{
public:
	/**
	 * @brief Generates code files based on the provided import data.
	 *
	 * @param Data The import data used for code generation.
	 * @return true if code was generated successfully, false otherwise.
	 */
	static bool GenerateCode(UArticyImportData* Data);

	/**
	 * @brief Caches the content of code files in the source folder.
	 *
	 * Loads the content of each file into memory for later restoration if needed.
	 */
	static void CacheCodeFiles();

	/**
	 * @brief Restores cached files from previous import sessions.
	 *
	 * Attempts to write the cached file content back to disk.
	 *
	 * @return true if all files were restored successfully, false otherwise.
	 */
	static bool RestoreCachedFiles();

	/**
	 * @brief Generates assets based on the provided import data.
	 *
	 * Handles asset generation, including handling renaming and deletion of generated assets.
	 *
	 * @param Data The import data used for asset generation.
	 */
	static void GenerateAssets(UArticyImportData* Data);

	/**
	 * @brief Initiates the recompilation process for the generated code.
	 *
	 * @param Data The import data used for recompilation.
	 */
	static void Recompile(UArticyImportData* Data);

	/**
	 * @brief Returns the main source folder for all the generated code.
	 *
	 * @return The path to the source folder.
	 */
	static FString GetSourceFolder();

	/**
	 * @brief Helper methods for generated class/struct names.
	 */
	static FString GetGeneratedInterfacesFilename(const UArticyImportData* Data);
	static FString GetGeneratedTypesFilename(const UArticyImportData* Data);
	static FString GetGeneratedTypeInformationFilename(const UArticyImportData* Data);
	static FString GetGlobalVarsClassname(const UArticyImportData* Data, const bool bOmittPrefix = false);
	static FString GetGVNamespaceClassname(const UArticyImportData* Data, const FString& Namespace);
	static FString GetDatabaseClassname(const UArticyImportData* Data, const bool bOmittPrefix = false);
	static FString GetArticyTypeClassname(const UArticyImportData* Data, const bool bOmittPrefix = false);
	static FString GetArticyLocalizerClassname(const UArticyImportData* Data, const bool bOmittPrefix = false);
	static FString GetMethodsProviderClassname(const UArticyImportData* Data, const bool bOmittPrefix = false);
	static FString GetExpressoScriptsClassname(const UArticyImportData* Data, const bool bOmittPrefix = false);
	static FString GetFeatureInterfaceClassName(const UArticyImportData* Data, const FArticyTemplateFeatureDef& Feature, const bool bOmittPrefix = false);

	/**
	 * @brief Deletes a file inside the source folder (empty Filename deletes the whole folder).
	 *
	 * @param Filename The name of the file to delete, or empty to delete the whole folder.
	 * @return true if the deletion was successful, false otherwise.
	 */
	static bool DeleteGeneratedCode(const FString& Filename = "");

	/**
	 * @brief Deletes extra generated code files that are not in the provided list.
	 *
	 * Iterates through the source folder and deletes files not matching any prefix from the generated files list.
	 *
	 * @param GeneratedFiles A list of prefixes for the generated files to keep.
	 * @return true if all non-matching files were successfully deleted, false otherwise.
	 */
	static bool DeleteExtraCode(const TArray<FString>& GeneratedFiles);

	/**
	 * @brief Deletes generated assets based on package definitions.
	 *
	 * Removes assets not included in the import, handling invalid assets appropriately.
	 *
	 * @param PackageDefs The package definitions used to determine which assets to delete.
	 * @return true if all invalid assets were successfully deleted, false otherwise.
	 */
	static bool DeleteGeneratedAssets(FArticyPackageDefs& PackageDefs);

	/**
	 * @brief Renames generated assets based on package definitions.
	 *
	 * This function handles renaming of package assets when their names have changed.
	 *
	 * @param PackageDefs The package definitions containing the new asset names.
	 * @return true if all renaming operations succeeded, false otherwise.
	 */
	static bool RenameGeneratedAssets(FArticyPackageDefs& PackageDefs);

private:
	/**
	 * @brief Compiles the generated code using the hot-reload interface.
	 *
	 * This function handles the compilation process, including handling any failures.
	 *
	 * @param Data The import data used for compilation.
	 */
	static void Compile(UArticyImportData* Data);

	/**
	 * @brief Callback function called when compilation is completed.
	 *
	 * This function updates the settings and broadcasts a notification for completed compilation.
	 *
	 * @param Data The import data used for compilation.
	 */
	static void OnCompiled(UArticyImportData* Data);

	/**
	 * @brief Parses a log to detect errors related to Articy-generated code.
	 *
	 * @param Log The compilation log to parse.
	 * @return true if errors related to Articy-generated code were found, false otherwise.
	 */
	static bool ParseForError(const FString& Log);

	/**
	 * @brief Restores the previous import session (ImportData + Code).
	 *
	 * Returns true if the restoration resulted in functioning code + import data.
	 *
	 * @param Data The import data to restore.
	 * @param bNotifyUser Whether to notify the user about the restoration process.
	 * @param Reason The reason for the restoration (e.g., compilation error).
	 * @return true if the restoration was successful, false otherwise.
	 */
	static bool RestorePreviousImport(UArticyImportData* Data, const bool& bNotifyUser = true, ECompilationResult::Type Reason = ECompilationResult::Unknown);

	// Cached files, mapped from FileName to FileContent
	static TMap<FString, FString> CachedFiles;

	//========================================//

	CodeGenerator() {}
	~CodeGenerator() {}
};

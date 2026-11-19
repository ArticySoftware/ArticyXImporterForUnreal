//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#pragma once

#include "CoreMinimal.h"

/**
 * @brief The state of the articy import of the project.
 */
enum class EArticyImportState : uint8
{
	/** The import matches the export. */
	UpToDate,
	/** Nothing was imported, and there is no export to import. */
	NoExportFile,
	/** There is an export, but it was never imported. */
	NotImported,
	/** Generated code or assets of the last import are missing. */
	Incomplete,
	/** The last import was made with another version of the plugin. */
	PluginChanged,
	/** The export changed since the last import. */
	ExportChanged
};

/**
 * @brief The import that brings an import state up to date.
 */
enum class EArticyImportAction : uint8
{
	None,
	ImportChanges,
	FullReimport
};

/**
 * @brief What is known about the export, the import data and the generated files.
 */
struct ARTICYEDITOR_API FArticyImportFacts
{
	/** The articy directory, as shown to the user. */
	FString ArticyDirectory;
	/** The name of the imported articy project. */
	FString ProjectName;
	/** The technical name of the project, which prefixes the generated classes. */
	FString ProjectTechnicalName;
	/** The cultures of the imported languages. */
	TArray<FString> Languages;
	int32 NumExportFiles = 0;
	bool bHasImportData = false;
	/** Whether the import data was ever filled by an import. */
	bool bHasPriorImport = false;
	bool bHasGeneratedCode = false;
	bool bHasGeneratedAssets = false;
	/** Whether an export file is newer than the import, or was never imported. */
	bool bExportChanged = false;
	FString ImportedPluginVersion;
	FString PluginVersion;

	/** @brief Collects the facts from the project, without importing anything. */
	static FArticyImportFacts Gather();

	/** @brief Returns true if texts were imported for a culture, or if no languages are known. */
	bool HasLanguage(const FString& CultureName) const;
};

/**
 * @brief The import state of the project, with what to tell the user about it.
 */
struct ARTICYEDITOR_API FArticyImportStatus
{
	EArticyImportState State = EArticyImportState::UpToDate;
	FArticyImportFacts Facts;

	/** @brief Derives the state from the facts. */
	static FArticyImportStatus Evaluate(const FArticyImportFacts& InFacts);

	/** @brief Gets the state of the project. */
	static FArticyImportStatus Query() { return Evaluate(FArticyImportFacts::Gather()); }

	bool IsUpToDate() const { return State == EArticyImportState::UpToDate; }

	/** @brief Returns true if the import is broken rather than just behind. */
	bool IsError() const { return State == EArticyImportState::Incomplete; }

	/** @brief Gets the explanation shown to the user; empty when up to date. */
	FText GetMessage() const;

	/** @brief Gets the import that fixes the state. */
	EArticyImportAction GetAction() const;

	/** @brief Gets the label of the button running the action. */
	FText GetActionLabel() const;
};

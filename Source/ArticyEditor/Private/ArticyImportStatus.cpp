//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "ArticyImportStatus.h"
#include "ArticyDatabase.h"
#include "ArticyEditorFunctionLibrary.h"
#include "ArticyGlobalVariables.h"
#include "ArticyHelpers.h"
#include "ArticyImportData.h"
#include "ArticyPackage.h"
#include "ArticyPluginSettings.h"
#include "CodeGeneration/CodeGenerator.h"
#include "EditorFramework/AssetImportData.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif

#define LOCTEXT_NAMESPACE "ArticyImportStatus"

namespace
{
	/** Finds the import data asset without creating one. */
	UArticyImportData* FindImportData(IAssetRegistry& AssetRegistry)
	{
		TArray<FAssetData> Assets;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		AssetRegistry.GetAssetsByClass(UArticyImportData::StaticClass()->GetClassPathName(), Assets);
#else
		AssetRegistry.GetAssetsByClass(UArticyImportData::StaticClass()->GetFName(), Assets);
#endif
		return Assets.Num() > 0 ? Cast<UArticyImportData>(Assets[0].GetAsset()) : nullptr;
	}

	/** Returns true if the file differs from what was recorded when it was imported. */
	bool HasExportFileChanged(const UArticyImportData* ImportData, const FString& ExportFile)
	{
		if (!ImportData->ImportData)
		{
			return false;
		}

		TArray<FString> ImportedFiles;
		ImportData->ImportData->ExtractFilenames(ImportedFiles);
		const TArray<FAssetImportInfo::FSourceFile>& SourceFiles = ImportData->ImportData->SourceData.SourceFiles;

		for (int32 Index = 0; Index < ImportedFiles.Num() && Index < SourceFiles.Num(); ++Index)
		{
			if (!FPaths::IsSamePath(ImportedFiles[Index], ExportFile))
			{
				continue;
			}

			// A merged file is tracked by name only
			const FAssetImportInfo::FSourceFile& SourceFile = SourceFiles[Index];
			if (SourceFile.Timestamp == FDateTime(0) || SourceFile.Timestamp == IFileManager::Get().GetTimeStamp(*ExportFile))
			{
				return false;
			}

			return !SourceFile.FileHash.IsValid() || SourceFile.FileHash != FMD5Hash::HashFile(*ExportFile);
		}

		return true;
	}
}

FArticyImportFacts FArticyImportFacts::Gather()
{
	FArticyImportFacts Facts;
	Facts.ArticyDirectory = GetDefault<UArticyPluginSettings>()->ArticyDirectory;
	Facts.PluginVersion = FArticyEditorFunctionLibrary::GetCurrentPluginVersion();

	FString RelativeDirectory = Facts.ArticyDirectory;
	RelativeDirectory.RemoveFromStart(TEXT("/Game"));
	RelativeDirectory.RemoveFromStart(TEXT("/"));
	const FString ExportDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / RelativeDirectory);

	TArray<FString> ExportFiles;
	IFileManager::Get().FindFiles(ExportFiles, *ExportDirectory, TEXT("articyue"));
	Facts.NumExportFiles = ExportFiles.Num();

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

	if (const UArticyImportData* ImportData = FindImportData(AssetRegistry))
	{
		Facts.bHasImportData = true;
		Facts.bHasPriorImport = !ImportData->GetProject().Guid.IsEmpty();
		Facts.ProjectName = ImportData->GetProject().Name;
		Facts.ProjectTechnicalName = ImportData->GetProject().TechnicalName;
		Facts.ImportedPluginVersion = ImportData->GetLastImporterPluginVersion();

		for (const TPair<FString, FArticyLanguageDef>& Language : ImportData->Languages.Languages)
		{
			Facts.Languages.Add(Language.Value.CultureName);
		}
		Facts.Languages.Sort();

		for (const FString& ExportFile : ExportFiles)
		{
			Facts.bExportChanged |= HasExportFileChanged(ImportData, ExportDirectory / ExportFile);
		}
	}

	// if we have less than 5 code files we are missing at least one
	TArray<FString> CodeFiles;
	IFileManager::Get().FindFiles(CodeFiles, *CodeGenerator::GetSourceFolder());
	Facts.bHasGeneratedCode = CodeFiles.Num() >= 5;

	// An asset whose generated class is missing has no class to report
	TArray<FAssetData> GeneratedAssets;
	AssetRegistry.GetAssetsByPath(FName(*ArticyHelpers::GetArticyGeneratedFolder()), GeneratedAssets, true);

	bool bHasDatabase = false;
	bool bHasGlobalVariables = false;
	bool bHasPackage = false;
	for (const FAssetData& Asset : GeneratedAssets)
	{
		if (const UClass* Class = Asset.GetClass())
		{
			bHasDatabase |= Class->IsChildOf(UArticyDatabase::StaticClass());
			bHasGlobalVariables |= Class->IsChildOf(UArticyGlobalVariables::StaticClass());
			bHasPackage |= Class->IsChildOf(UArticyPackage::StaticClass());
		}
	}
	Facts.bHasGeneratedAssets = bHasDatabase && bHasGlobalVariables && bHasPackage;

	return Facts;
}

bool FArticyImportFacts::HasLanguage(const FString& CultureName) const
{
	if (Languages.Num() == 0)
	{
		return true;
	}

	// Like the generated localizer: the exact culture, or a regional export of its language
	const FString LanguageCode = CultureName.Left(2);
	for (const FString& Language : Languages)
	{
		if (Language.Equals(CultureName, ESearchCase::IgnoreCase)
			|| (Language.Len() > 2 && Language.Left(2).Equals(LanguageCode, ESearchCase::IgnoreCase)))
		{
			return true;
		}
	}

	return false;
}

FArticyImportStatus FArticyImportStatus::Evaluate(const FArticyImportFacts& InFacts)
{
	FArticyImportStatus Status;
	Status.Facts = InFacts;

	if (!InFacts.bHasImportData || !InFacts.bHasPriorImport)
	{
		Status.State = InFacts.NumExportFiles > 0 ? EArticyImportState::NotImported : EArticyImportState::NoExportFile;
	}
	else if (!InFacts.bHasGeneratedCode || !InFacts.bHasGeneratedAssets)
	{
		Status.State = EArticyImportState::Incomplete;
	}
	else if (!InFacts.ImportedPluginVersion.Equals(InFacts.PluginVersion))
	{
		Status.State = EArticyImportState::PluginChanged;
	}
	else if (InFacts.bExportChanged)
	{
		Status.State = EArticyImportState::ExportChanged;
	}

	return Status;
}

FText FArticyImportStatus::GetMessage() const
{
	switch (State)
	{
	case EArticyImportState::NoExportFile:
		return FText::Format(LOCTEXT("NoExportFile", "No articy:draft X export was found in {0}. Export your project into the Content folder of this project to import it."),
			FText::FromString(Facts.ArticyDirectory));
	case EArticyImportState::NotImported:
		return LOCTEXT("NotImported", "The articy:draft X export of this project has not been imported yet.");
	case EArticyImportState::Incomplete:
		return Facts.bHasGeneratedCode
			? LOCTEXT("AssetsMissing", "The last import did not complete: generated assets are missing.")
			: LOCTEXT("CodeMissing", "The last import did not complete: generated code is missing.");
	case EArticyImportState::PluginChanged:
		return Facts.ImportedPluginVersion.IsEmpty()
			? LOCTEXT("PluginChangedUnknown", "The project was imported with an older version of the importer.")
			: FText::Format(LOCTEXT("PluginChanged", "The project was imported with version {0} of the importer, version {1} is installed."),
				FText::FromString(Facts.ImportedPluginVersion), FText::FromString(Facts.PluginVersion));
	case EArticyImportState::ExportChanged:
		return LOCTEXT("ExportChanged", "The articy:draft X export has changed since the last import.");
	default:
		return FText::GetEmpty();
	}
}

EArticyImportAction FArticyImportStatus::GetAction() const
{
	switch (State)
	{
	case EArticyImportState::NotImported:
	case EArticyImportState::ExportChanged:
		return EArticyImportAction::ImportChanges;
	case EArticyImportState::Incomplete:
	case EArticyImportState::PluginChanged:
		return EArticyImportAction::FullReimport;
	default:
		return EArticyImportAction::None;
	}
}

FText FArticyImportStatus::GetActionLabel() const
{
	if (State == EArticyImportState::NotImported)
	{
		return LOCTEXT("Import", "Import");
	}

	switch (GetAction())
	{
	case EArticyImportAction::ImportChanges:
		return LOCTEXT("ImportChanges", "Import Changes");
	case EArticyImportAction::FullReimport:
		return LOCTEXT("FullReimport", "Full Reimport");
	default:
		return FText::GetEmpty();
	}
}

#undef LOCTEXT_NAMESPACE

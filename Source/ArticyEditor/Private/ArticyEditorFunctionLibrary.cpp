//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyEditorFunctionLibrary.h"
#include "ArticyPluginSettings.h"
#include "ArticyEditorModule.h"
#include "ArticyJSONFactory.h"
#include "CodeGeneration/CodeGenerator.h"
#include "ObjectTools.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

int32 FArticyEditorFunctionLibrary::ForceCompleteReimport(UArticyImportData* ImportData)
{
	const EImportDataEnsureResult Result = EnsureImportDataAsset(&ImportData);
	// if we generated the import data asset we will cause a full reimport, so stop here
	if (Result == EImportDataEnsureResult::Generation)
	{
		return 0;
	}
	if (Result == EImportDataEnsureResult::Failure)
	{
		return -1;
	}

	ImportData->Settings.GlobalVariablesHash.Reset();
	ImportData->Settings.ObjectDefinitionsHash.Reset();
	ImportData->Settings.ObjectDefinitionsTextHash.Reset();
	ImportData->Settings.ScriptFragmentsHash.Reset();
	ImportData->PackageDefs.ResetPackages();
	return ReimportChanges(ImportData);
}

int32 FArticyEditorFunctionLibrary::ReimportChanges(UArticyImportData* ImportData)
{
	const EImportDataEnsureResult Result = EnsureImportDataAsset(&ImportData);
	// if we generated the import data asset we will cause a full reimport, so stop here
	if (Result == EImportDataEnsureResult::Generation)
	{
		return 0;
	}
	if (Result == EImportDataEnsureResult::Failure)
	{
		return -1;
	}

	const auto Factory = NewObject<UArticyJSONFactory>();
	if (Factory)
	{
		return Factory->Reimport(ImportData) - 1;
		//GC will destroy factory
	}

	return -1;
}

int32 FArticyEditorFunctionLibrary::RegenerateAssets(UArticyImportData* ImportData)
{
	const EImportDataEnsureResult Result = EnsureImportDataAsset(&ImportData);
	// if we generated the import data asset we will cause a full reimport, so stop here
	if (Result == EImportDataEnsureResult::Generation)
	{
		return 0;
	}
	if (Result == EImportDataEnsureResult::Failure)
	{
		return -1;
	}

	CodeGenerator::GenerateAssets(ImportData);

	return -1;
}

EImportDataEnsureResult FArticyEditorFunctionLibrary::EnsureImportDataAsset(UArticyImportData** ImportData)
{
	EImportDataEnsureResult Result;

	if(*ImportData)
	{
		Result = Success;
	}
	else
	{
		TWeakObjectPtr<UArticyImportData> ImportDataAsset = UArticyImportData::GetImportData();

		if (!ImportDataAsset.IsValid())
		{
			UE_LOG(LogArticyEditor, Warning, TEXT("Attempting to create from .articyue export file"));
			ImportDataAsset = GenerateImportDataAsset();

			if(ImportDataAsset.IsValid())
			{
				*ImportData = ImportDataAsset.Get();
				Result = Generation;
			}
			else
			{
				Result = Failure;
			}			
		}
		else
		{
			*ImportData = ImportDataAsset.Get();
			Result = AssetRegistry;
		}
	}

	return Result;
}

UArticyImportData* FArticyEditorFunctionLibrary::GenerateImportDataAsset()
{
	UArticyImportData* ImportData = nullptr;
	
	UArticyJSONFactory* Factory = NewObject<UArticyJSONFactory>();

	TArray<FString> ArticyImportFiles;
	// path is virtual in the beginning
	const FString ArticyDirectory = GetDefault<UArticyPluginSettings>()->ArticyDirectory.Path;
	// remove /Game/ so that the non-virtual part remains
	FString ArticyDirectoryNonVirtual = ArticyDirectory;
	ArticyDirectoryNonVirtual.RemoveFromStart(TEXT("/Game"));
	ArticyDirectoryNonVirtual.RemoveFromStart(TEXT("/"));
	// attach the non-virtual path to the content directory, then convert it to absolute
	const FString AbsoluteDirectoryPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*(FPaths::ProjectContentDir() + ArticyDirectoryNonVirtual));
	IFileManager::Get().FindFiles(ArticyImportFiles, *AbsoluteDirectoryPath, TEXT("articyue"));
	if (ArticyImportFiles.Num() == 0)
	{
		UE_LOG(LogArticyEditor, Error, TEXT("Failed creation of import data asset. No .articyue file found in directory %s. Please check the plugin settings for the correct articy directory and try again."), *ArticyDirectory);
		return nullptr;
	}
	
	const FString FileName = FPaths::GetBaseFilename(ArticyImportFiles[0], false);

	const FString PackagePath = ArticyDirectory + TEXT("/") + FileName;

	const FString CleanedPackagePath = PackagePath.Replace(TEXT(" "), TEXT("_")).Replace(TEXT("."), TEXT("_"));

	// @TODO Engine Versioning
#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
	UPackage* Outer = CreatePackage(*CleanedPackagePath);
#else
	UPackage* Outer = CreatePackage(nullptr, *CleanedPackagePath);
#endif

	Outer->FullyLoad();

	const FString FullPath = AbsoluteDirectoryPath + TEXT("/") + ArticyImportFiles[0];
	bool bRequired = false;
	UObject* ImportDataAsset = Factory->ImportObject(UArticyImportData::StaticClass(), Outer, FName(*FPaths::GetBaseFilename(CleanedPackagePath)), EObjectFlags::RF_Standalone | EObjectFlags::RF_Public, FullPath, nullptr, bRequired);

	if (ImportDataAsset)
	{
		ImportData = Cast<UArticyImportData>(ImportDataAsset);

		// automatically save the import data asset
		TArray<UPackage*> FailedToSavePackages;
		FEditorFileUtils::PromptForCheckoutAndSave({ Outer }, false, false, &FailedToSavePackages);

		UE_LOG(LogArticyEditor, Warning, TEXT("Successfully created import data asset. Continuing process."));
	}
	else
	{
		// delete the package if we couldn't create the import asset
		ObjectTools::ForceDeleteObjects({ Outer });
		UE_LOG(LogArticyEditor, Error, TEXT("Failed creation import data asset. Aborting process."));
	}
	
	return ImportData;
}

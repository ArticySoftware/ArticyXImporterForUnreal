//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyJSONFactory.h"
#include "CoreMinimal.h"
#include "ArticyArchiveReader.h"
#include "ArticyImportData.h"
#include "Editor.h"
#include "ArticyEditorModule.h"
#include "ArticyImporterHelpers.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Runtime/Launch/Resources/Version.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/ConfigCacheIni.h"
#include "CodeGeneration/CodeGenerator.h"

#define LOCTEXT_NAMESPACE "ArticyJSONFactory"

/**
 * Constructor for UArticyJSONFactory.
 * Initializes the factory to import Articy JSON files.
 */
UArticyJSONFactory::UArticyJSONFactory()
{
    bEditorImport = true;
    Formats.Add(TEXT("articyue;A json file exported from articy:draft X"));
}

/**
 * Destructor for UArticyJSONFactory.
 */
UArticyJSONFactory::~UArticyJSONFactory()
{
}

/**
 * Checks if the factory can import the specified file.
 *
 * @param Filename The name of the file to check.
 * @return true if the factory can import the file, false otherwise.
 */
bool UArticyJSONFactory::FactoryCanImport(const FString& Filename)
{
    UE_LOG(LogArticyEditor, Log, TEXT("Gonna import %s"), *Filename);

    return true;
}

/**
 * Resolves the class supported by this factory.
 *
 * @return The class supported by this factory.
 */
UClass* UArticyJSONFactory::ResolveSupportedClass()
{
    return UArticyImportData::StaticClass();
}

/**
 * Creates an object from the specified file.
 *
 * @param InClass The class to create.
 * @param InParent The parent object.
 * @param InName The name of the object.
 * @param Flags Object flags.
 * @param Filename The name of the file to import.
 * @param Parms Additional parameters.
 * @param Warn Feedback context for warnings and errors.
 * @param bOutOperationCanceled Output flag indicating if the operation was canceled.
 * @return The created object.
 */
UObject* UArticyJSONFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
    FString Path = FPaths::GetPath(InParent->GetPathName());

    // Properly update the config file and delete previous import assets
    UArticyPluginSettings* CDO_Settings = GetMutableDefault<UArticyPluginSettings>();
    if (!CDO_Settings->ArticyDirectory.Path.Equals(Path))
    {
        // Update the directory path in the configuration
        CDO_Settings->ArticyDirectory.Path = Path;
        FString ConfigName = CDO_Settings->GetDefaultConfigFilename();
        GConfig->SetString(TEXT("/Script/ArticyRuntime.ArticyPluginSettings"), TEXT("ArticyDirectory"), *Path, ConfigName);
        GConfig->FindConfigFile(ConfigName)->Dirty = true;
        GConfig->Flush(false, ConfigName);
    }

    auto ArticyImportData = NewObject<UArticyImportData>(InParent, InName, Flags);

    const bool bImportQueued = HandleImportDuringPlay(ArticyImportData);

#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 22)
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, *FPaths::GetExtension(Filename));
#else
    FEditorDelegates::OnAssetPreImport.Broadcast(this, InClass, InParent, InName, *FPaths::GetExtension(Filename));
#endif

    ArticyImportData->ImportData->Update(GetCurrentFilename());

    if (!bImportQueued)
    {
        if (!ImportFromFile(Filename, ArticyImportData) && ArticyImportData)
        {
            bOutOperationCanceled = true;
            // The asset will be garbage collected because there are no references to it, no need to delete it
            ArticyImportData = nullptr;
        }
        // Else import should be ok 
    }


#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 22)
    GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, ArticyImportData);
#else
    FEditorDelegates::OnAssetPostImport.Broadcast(this, ArticyImportData);
#endif

    return ArticyImportData;
}

/**
 * Determines if the factory can reimport the specified object.
 *
 * @param Obj The object to reimport.
 * @param OutFilenames The list of filenames for reimporting.
 * @return true if the factory can reimport the object, false otherwise.
 */
bool UArticyJSONFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
    const auto& Asset = Cast<UArticyImportData>(Obj);

    if (!Asset)
    {
        return false;
    }

    const bool bImportQueued = HandleImportDuringPlay(Obj);
    if (bImportQueued)
    {
        return false;
    }

    Asset->ImportData->ExtractFilenames(OutFilenames);
    return true;
}

/**
 * Sets the reimport paths for the specified object.
 *
 * @param Obj The object to set reimport paths for.
 * @param NewReimportPaths The new reimport paths.
 */
void UArticyJSONFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
    auto Asset = Cast<UArticyImportData>(Obj);
    if (Asset)
        Asset->ImportData->UpdateFilenameOnly(NewReimportPaths[0]);
}

bool UArticyJSONFactory::IsFullArticyExport(const FString& FullArchivePath)
{
    UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
    if (!Archive->OpenArchive(*FullArchivePath))
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to open articy archive %s"), *FullArchivePath);
        return false;
    }

    FString ManifestJson;
    if (!Archive->ReadFile(TEXT("manifest.json"), ManifestJson))
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to read manifest.json from %s"), *FullArchivePath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<TCHAR>> Reader = TJsonReaderFactory<TCHAR>::Create(ManifestJson);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to parse manifest.json in %s"), *FullArchivePath);
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* PackagesJson = nullptr;
    if (!Root->TryGetArrayField(TEXT("Packages"), PackagesJson) || !PackagesJson)
    {
        // If there are no packages, treat this as not usable for our purposes
        UE_LOG(LogArticyEditor, Warning, TEXT("Manifest in %s has no 'Packages' array."), *FullArchivePath);
        return false;
    }

    // Full export requirement: every package in the manifest must be IsIncluded == true
    for (const TSharedPtr<FJsonValue>& PackageValue : *PackagesJson)
    {
        TSharedPtr<FJsonObject> PackageObject = PackageValue->AsObject();
        if (!PackageObject.IsValid())
        {
            continue;
        }

        bool bIsIncluded = false;
        PackageObject->TryGetBoolField(TEXT("IsIncluded"), bIsIncluded);

        if (!bIsIncluded)
        {
            // This is a partial export - not valid as the base import.
            return false;
        }
    }

    return true;
}

/**
 * Reimports the specified object.
 *
 * @param Obj The object to reimport.
 * @return The result of the reimport operation.
 */
EReimportResult::Type UArticyJSONFactory::Reimport(UObject* Obj)
{
    auto Asset = Cast<UArticyImportData>(Obj);
    if (!Asset || !Asset->ImportData)
    {
        return EReimportResult::Failed;
    }

    // Derive the directory from the original import file / plugin settings
    // 1) Try original import path
    FString FirstImportFilename = Asset->ImportData->GetFirstFilename();

    // Normalize old ".articyue4" extension if needed
    if (FirstImportFilename.EndsWith(TEXT("articyue4")))
    {
        FirstImportFilename.RemoveFromEnd(TEXT("4")); // -> ".articyue"
    }

    FString BaseDir;
    if (!FirstImportFilename.IsEmpty())
    {
        BaseDir = FPaths::GetPath(FirstImportFilename);
    }
    else
    {
        // Fallback: use plugin settings directory (same as GenerateImportDataAsset)
        const FString ArticyDirectory = GetDefault<UArticyPluginSettings>()->ArticyDirectory.Path;
        FString ArticyDirectoryNonVirtual = ArticyDirectory;
        ArticyDirectoryNonVirtual.RemoveFromStart(TEXT("/Game"));
        ArticyDirectoryNonVirtual.RemoveFromStart(TEXT("/"));

        BaseDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
            *(FPaths::ProjectContentDir() + ArticyDirectoryNonVirtual));
    }

    TArray<FString> ArticyFiles;
    IFileManager::Get().FindFiles(ArticyFiles, *BaseDir, TEXT("articyue"));

    if (ArticyFiles.Num() == 0)
    {
        UE_LOG(LogArticyEditor, Error,
            TEXT("Reimport failed: no .articyue files found in '%s'."), *BaseDir);
        return EReimportResult::Failed;
    }

    // Choose a base file (you can reuse IsFullArticyExport(...) here if you implemented it)
    FString BaseArticyFile;
    if (ArticyFiles.Num() == 1)
    {
        BaseArticyFile = ArticyFiles[0];
    }
    else
    {
        // Try to pick a "full export" as base; otherwise just the first one
        for (const FString& Candidate : ArticyFiles)
        {
            const FString FullCandidatePath = BaseDir / Candidate;
            if (IsFullArticyExport(FullCandidatePath))   // optional helper
            {
                BaseArticyFile = Candidate;
                break;
            }
        }

        if (BaseArticyFile.IsEmpty())
        {
            BaseArticyFile = ArticyFiles[0];
            UE_LOG(LogArticyEditor, Warning,
                TEXT("Multiple .articyue files found during reimport but no full export detected; using %s as base."),
                *BaseArticyFile);
        }
    }

    // 1) Import base file in "single-file" mode
    const FString BaseFullPath = BaseDir / BaseArticyFile;

    UE_LOG(LogArticyEditor, Log,
        TEXT("Reimport: using '%s' as base Articy export."), *BaseFullPath);

    Asset->bMultiFileMerge = false;
    if (!ImportFromFile(BaseFullPath, Asset))
    {
        UE_LOG(LogArticyEditor, Error,
            TEXT("Reimport failed: could not import base articy file '%s'."), *BaseFullPath);
        Asset->bMultiFileMerge = false;
        return EReimportResult::Failed;
    }

    // 2) Import remaining files as supplemental
    Asset->bMultiFileMerge = true;

    for (const FString& File : ArticyFiles)
    {
        if (File == BaseArticyFile)
            continue;

        const FString FullPath = BaseDir / File;

        UE_LOG(LogArticyEditor, Log,
            TEXT("Reimport: merging supplemental Articy export '%s'."), *FullPath);

        if (!ImportFromFile(FullPath, Asset))
        {
            UE_LOG(LogArticyEditor, Warning,
                TEXT("Reimport: failed to merge supplemental articy file '%s', continuing with others."),
                *FullPath);
        }
    }

    Asset->bMultiFileMerge = false;

    CodeGenerator::Recompile(Asset);

    return EReimportResult::Succeeded;
}

/**
 * Imports data from a JSON file into an Articy import data object.
 *
 * @param FileName The name of the file to import.
 * @param Asset The Articy import data object to import into.
 * @return true if the import was successful, false otherwise.
 */
bool UArticyJSONFactory::ImportFromFile(const FString& FileName, UArticyImportData* Asset) const
{
    UArticyArchiveReader* Archive = NewObject<UArticyArchiveReader>();
    Archive->OpenArchive(*FileName);

    // Load file as text file
    FString JSON;
    if (!Archive->ReadFile(TEXT("manifest.json"), JSON))
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to load file '%s' to string"), *FileName);
        return false;
    }

    // Parse outermost JSON object
    TSharedPtr<FJsonObject> JsonParsed;
    const TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(JSON);
    if (!FJsonSerializer::Deserialize(JsonReader, JsonParsed) || !JsonParsed.IsValid())
    {
        UE_LOG(LogArticyEditor, Error, TEXT("Failed to parse manifest.json for '%s'"), *FileName);
        return false;
    }

    const bool bOk = Asset->ImportFromJson(*Archive, JsonParsed);

#if WITH_EDITORONLY_DATA
    if (bOk && Asset->ImportData)
    {
        // Important for reimport / tracking: record the source file
        Asset->ImportData->Update(FileName);
    }
#endif

    return bOk;
}

/**
 * Handles the case where an import is attempted during play mode.
 *
 * @param Obj The object to import.
 * @return true if the import is queued for later, false otherwise.
 */
bool UArticyJSONFactory::HandleImportDuringPlay(UObject* Obj)
{
    const bool bIsPlaying = ArticyImporterHelpers::IsPlayInEditor();
    FArticyEditorModule& ArticyImporterModule = FModuleManager::Get().GetModuleChecked<FArticyEditorModule>("ArticyEditor");

    // If we are already queued, that means we just ended play mode. bIsPlaying will still be true in this case, so we need another check
    if (bIsPlaying && !ArticyImporterModule.IsImportQueued())
    {
        // We have to abuse the module to queue the import since this factory might not exist later on
        ArticyImporterModule.QueueImport();
        return true;
    }

    return false;
}

#undef LOCTEXT_NAMESPACE

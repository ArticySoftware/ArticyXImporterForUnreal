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

/**
 * Reimports the specified object.
 *
 * @param Obj The object to reimport.
 * @return The result of the reimport operation.
 */
EReimportResult::Type UArticyJSONFactory::Reimport(UObject* Obj)
{
    auto Asset = Cast<UArticyImportData>(Obj);
    if (Asset)
    {
        if (!Asset->ImportData)
            return EReimportResult::Failed;

        // Don't look for old .articyue4 files
        if (Asset->ImportData->SourceData.SourceFiles.Num() > 0)
            Asset->ImportData->SourceData.SourceFiles[0].RelativeFilename.RemoveFromEnd(TEXT("4"));

        const FString ImportFilename = Asset->ImportData->GetFirstFilename();

        if (ImportFilename.Len() == 0)
            return EReimportResult::Failed;

        if (ImportFromFile(ImportFilename, Asset))
            return EReimportResult::Succeeded;
    }

    return EReimportResult::Failed;
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
    if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
    {
        Asset->ImportFromJson(*Archive, JsonParsed);
    }

    return true;
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

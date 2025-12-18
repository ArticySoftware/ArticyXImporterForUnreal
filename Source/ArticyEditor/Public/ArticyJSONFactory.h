//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "Factories/Factory.h"
#include "EditorReimportHandler.h"

#include "ArticyJSONFactory.generated.h"

class UArticyImportData;

/**
 * Factory for importing JSON files exported from Articy:draft.
 */
UCLASS()
class UArticyJSONFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()

public:
    /** Constructor for UArticyJSONFactory. */
    UArticyJSONFactory();

    /** Destructor for UArticyJSONFactory. */
    virtual ~UArticyJSONFactory() override;

    /**
     * Determines if the factory can import the specified file.
     *
     * @param Filename The name of the file to check.
     * @return true if the factory can import the file, false otherwise.
     */
    virtual bool FactoryCanImport(const FString& Filename) override;

    /**
     * Resolves the class supported by this factory.
     *
     * @return The class supported by this factory.
     */
    virtual UClass* ResolveSupportedClass() override;

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
    virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

    // FReimportHandler

    /**
     * Determines if the factory can reimport the specified object.
     *
     * @param Obj The object to reimport.
     * @param OutFilenames The list of filenames for reimporting.
     * @return true if the factory can reimport the object, false otherwise.
     */
    virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;

    /**
     * Sets the reimport paths for the specified object.
     *
     * @param Obj The object to set reimport paths for.
     * @param NewReimportPaths The new reimport paths.
     */
    virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;

    /**
     * Reimports the specified object.
     *
     * @param Obj The object to reimport.
     * @return The result of the reimport operation.
     */
    virtual EReimportResult::Type Reimport(UObject* Obj) override;
    //~FReimportHandler

private:
    /**
     * Performs the actual import task, converting the JSON data into UArticyImportData.
     *
     * @param FileName The name of the file to import.
     * @param Asset The Articy import data object to import into.
     * @return true if the import was successful, false otherwise.
     */
    bool ImportFromFile(const FString& FileName, UArticyImportData* Asset) const;

    /**
     * Handles the case where an import or reimport is performed during play.
     *
     * @param Obj The object to import or reimport.
     * @return true if the import is queued for later, false otherwise.
     */
    bool HandleImportDuringPlay(UObject* Obj);
};

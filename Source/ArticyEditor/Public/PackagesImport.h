//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "ArticyAsset.h"
#include "ArticyBaseTypes.h"
#include "ArticyPackage.h"
#include "ArticyArchiveReader.h"
#include "ArticyTexts.h"

#include "PackagesImport.generated.h"


class UArticyImportData;
struct FAdiSettings;

/**
 * Represents a model definition in Articy with properties for asset reference, category, and JSON strings for properties and templates.
 */
USTRUCT(BlueprintType)
struct FArticyModelDef
{
	GENERATED_BODY()

public:

	/**
	 * Imports model definition data from a JSON object.
	 *
	 * @param JsonModel A shared pointer to the JSON object containing the model definition.
	 */
	void ImportFromJson(const TSharedPtr<FJsonObject> JsonModel);

	/**
	 * Gathers scripts from the model definition and adds them to the ArticyImportData.
	 *
	 * @param Data A pointer to the UArticyImportData object.
	 */
	void GatherScripts(UArticyImportData* Data) const;

	/**
	 * Generates a sub-asset from the model definition.
	 *
	 * @param Data A pointer to the UArticyImportData object.
	 * @param Outer The outer object for the sub-asset.
	 * @return A pointer to the generated UArticyObject sub-asset.
	 */
	UArticyObject* GenerateSubAsset(const UArticyImportData* Data, UObject* Outer) const;//MM_CHANGE

	/**
	 * Gets the type of the model.
	 *
	 * @return The model type as an FName.
	 */
	FName GetType() const { return Type; }

	/**
	 * Gets the technical name of the model.
	 *
	 * @return The technical name as a string.
	 */
	const FString& GetTechnicalName() const { return TechnicalName; }

	/**
	 * Gets the combined name and ID of the model.
	 *
	 * @return The name and ID as a string.
	 */
	const FString& GetNameAndId() const { return NameAndId; }

	/**
	 * Gets the ID of the model.
	 *
	 * @return The model ID as an FArticyId.
	 */
	const FArticyId& GetId() const { return Id; }

	/**
	 * Gets the parent ID of the model.
	 *
	 * @return The parent ID as an FArticyId.
	 */
	const FArticyId& GetParent() const { return Parent; }

	/**
	 * Gets the asset reference of the model.
	 *
	 * @return The asset reference as a string.
	 */
	const FString& GetAssetRef() const { return AssetRef; }

	/**
	 * Gets the asset category of the model.
	 *
	 * @return The asset category as an EArticyAssetCategory.
	 */
	const EArticyAssetCategory& GetAssetCat() const { return AssetCategory; }

	/**
	 * Gets the properties JSON object from the cached properties JSON string.
	 *
	 * @return A shared pointer to the properties JSON object.
	 */
	TSharedPtr<FJsonObject> GetPropertiesJson() const;

	/**
	 * Gets the templates JSON object from the cached templates JSON string.
	 *
	 * @return A shared pointer to the templates JSON object.
	 */
	TSharedPtr<FJsonObject> GetTemplatesJson() const;

private:

	/**
	 * Converts a category string to an EArticyAssetCategory enum value.
	 *
	 * @param Category The category string.
	 * @return The corresponding EArticyAssetCategory value.
	 */
	EArticyAssetCategory GetAssetCategoryFromString(const FString Category);

	/** The original type of the model as read in from json. */
	UPROPERTY(VisibleAnywhere, Category = "Model")
	FName Type;

	/** The TechnicalName of the model, extracted from the PropertiesJsonString. */
	UPROPERTY(VisibleAnywhere, Category = "Model")
	FString TechnicalName;

	/** The Id of the model, extracted from the PropertiesJsonString. */
	UPROPERTY(VisibleAnywhere, Category = "Model")
	FArticyId Id = -1;

	/** TechnicalName(Id) */
	UPROPERTY(VisibleAnywhere, Category = "Model")
	FString NameAndId;

	/** The Id of the parent of this model, extracted from the PropertiesJsonString. */
	UPROPERTY(VisibleAnywhere, Category = "Model")
	FArticyId Parent = 0;

	/** The asset reference, if this is an asset. */
	UPROPERTY(VisibleAnywhere, Category = "Model Meta")
	FString AssetRef;

	/** The asset category, if this is an asset. */
	UPROPERTY(VisibleAnywhere, Category = "Model Meta")
	EArticyAssetCategory AssetCategory = EArticyAssetCategory::None;

	UPROPERTY(VisibleAnywhere, Category = "Model")
	FString PropertiesJsonString;
	UPROPERTY(VisibleAnywhere, Category = "Model")
	FString TemplateJsonString;

	mutable TSharedPtr<FJsonObject> CachedPropertiesJson = nullptr;
	mutable TSharedPtr<FJsonObject> CachedTemplateJson = nullptr;
};

/**
 * Represents a package definition in Articy with properties for ID, name, description, and models.
 */
USTRUCT(BlueprintType)
struct FArticyPackageDef
{
	GENERATED_BODY()

public:

	/**
	 * Imports package definition data from a JSON object.
	 *
	 * @param Archive A reference to the ArticyArchiveReader object.
	 * @param JsonPackage A shared pointer to the JSON object containing the package definition.
	 */
	void ImportFromJson(const UArticyArchiveReader& Archive, const TSharedPtr<FJsonObject>& JsonPackage);

	/**
	 * Gathers scripts from the package definition and adds them to the ArticyImportData.
	 *
	 * @param Data A pointer to the UArticyImportData object.
	 */
	void GatherScripts(UArticyImportData* Data) const;

	/**
	 * Gathers text data from a JSON object and adds it to the package definition.
	 *
	 * @param Json A shared pointer to the JSON object containing the text data.
	 */
	void GatherText(const TSharedPtr<FJsonObject>& Json);

	/**
	 * Generates a package asset from the package definition.
	 *
	 * @param Data A pointer to the UArticyImportData object.
	 * @return A pointer to the generated UArticyPackage asset.
	 */
	UArticyPackage* GeneratePackageAsset(UArticyImportData* Data) const;//MM_CHANGE

	/**
	 * Gets the texts map from the package definition.
	 *
	 * @return A map of text data.
	 */
	TMap<FString, FArticyTexts> GetTexts() const;

	/**
	 * Gets the folder path for the package.
	 *
	 * @return The folder path as a string.
	 */
	FString GetFolder() const;

	/**
	 * Gets the folder name for the package.
	 *
	 * @return The folder name as a string.
	 */
	FString GetFolderName() const;

	/**
	 * Gets the name of the package.
	 *
	 * @return The package name as a string.
	 */
	const FString GetName() const;

	/**
	 * Gets the previous name of the package.
	 *
	 * @return The previous package name as a string.
	 */
	const FString GetPreviousName() const;

	/**
	 * Sets the name of the package.
	 *
	 * @param NewName The new name for the package.
	 */
	void SetName(const FString& NewName);

	/**
	 * Gets the ID of the package.
	 *
	 * @return The package ID as an FArticyId.
	 */
	FArticyId GetId() const;

	/**
	 * Checks if the package is included.
	 *
	 * @return True if the package is included, false otherwise.
	 */
	bool GetIsIncluded() const;

	/**
	 * Gets the script fragment hash for the package definition.
	 *
	 * @return The script fragment hash as a string.
	 */
	FString GetScriptFragmentHash() const;

	/**
	 * Equality operator for package definitions based on ID.
	 *
	 * @param Other The other package definition to compare with.
	 * @return True if the package IDs are equal, false otherwise.
	 */
	bool operator==(const FArticyPackageDef& Other) const
	{
		return Id == Other.Id;
	}

private:

	UPROPERTY(VisibleAnywhere, Category = "Package")
	FArticyId Id;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	FString Name;
	UPROPERTY(VisibleAnywhere, Category = "Package")
	FString Description;
	UPROPERTY(VisibleAnywhere, Category = "Package")
	bool IsDefaultPackage = false;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	TArray<FArticyModelDef> Models;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	TMap<FString, FArticyTexts> Texts;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	FString PackageObjectsHash;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	FString PackageTextsHash;

	UPROPERTY(VisibleAnywhere, Category = "Package")
	FString ScriptFragmentHash;

	bool IsIncluded = false;
	FString PreviousName = TEXT("");
};

/**
 * Contains information about all imported packages and provides methods for importing, validating, and generating package assets.
 */
USTRUCT(BlueprintType)
struct FArticyPackageDefs
{
	GENERATED_BODY()

public:

	/**
	 * Imports package definitions from a JSON array.
	 *
	 * @param Archive A reference to the ArticyArchiveReader object.
	 * @param Json A pointer to the JSON array containing the package definitions.
	 * @param Settings A reference to the FADISettings object.
	 */
	void ImportFromJson(const UArticyArchiveReader& Archive, const TArray<TSharedPtr<FJsonValue>>* Json, FAdiSettings& Settings);

	/**
	 * Validates the import of package definitions from a JSON array.
	 *
	 * @param Archive A reference to the ArticyArchiveReader object.
	 * @param Json A pointer to the JSON array containing the package definitions.
	 * @return True if the import is valid, false otherwise.
	 */
	bool ValidateImport(const UArticyArchiveReader& Archive, const TArray<TSharedPtr<FJsonValue>>* Json);

	/**
	 * Gathers scripts from all package definitions and adds them to the ArticyImportData.
	 *
	 * @param Data A pointer to the UArticyImportData object.
	 */
	void GatherScripts(UArticyImportData* Data) const;

	/**
	 * Generates assets for all package definitions and stores them in the ArticyImportData.
	 *
	 * @param Data A pointer to the UArticyImportData object.
	 */
	void GenerateAssets(UArticyImportData* Data) const;//MM_CHANGE

	/**
	 * Gets the texts map from a specific package definition.
	 *
	 * @param Package The package definition to retrieve texts from.
	 * @return A map of text data.
	 */
	static TMap<FString, FArticyTexts> GetTexts(const FArticyPackageDef& Package);

	/**
	 * Gets a set of package names from the package definitions.
	 *
	 * @return A set of package names.
	 */
	TSet<FString> GetPackageNames() const;

	/**
	 * Gets an array of package definitions.
	 *
	 * @return An array of package definitions.
	 */
	TArray<FArticyPackageDef> GetPackages() const;

	/**
	 * Resets the packages array, clearing all package definitions.
	 */
	void ResetPackages();
private:

	UPROPERTY(VisibleAnywhere, Category = "Packages")
	TArray<FArticyPackageDef> Packages;
};

//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "PackagesImport.h"
#include "ArticyEditorModule.h"
#include "ArticyImporterHelpers.h"
#include "ArticyImportData.h"
#include "CodeGeneration/CodeGenerator.h"
#include "ArticyObject.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UObject/ConstructorHelpers.h"
#include <string>

#include "ArticyArchiveReader.h"
#include "ArticyPluginSettings.h"

#define STRINGIFY(x) #x

/**
 * Imports model definition data from a JSON object.
 *
 * @param JsonModel A shared pointer to the JSON object containing the model definition.
 */
void FArticyModelDef::ImportFromJson(const TSharedPtr<FJsonObject> JsonModel)
{
	JSON_TRY_FNAME(JsonModel, Type);
	JSON_TRY_STRING(JsonModel, AssetRef);

	FString Category;
	JSON_TRY_STRING(JsonModel, Category);
	this->AssetCategory = GetAssetCategoryFromString(Category);

	{
		PropertiesJsonString = "";
		TSharedPtr<FJsonObject> Properties = JsonModel->GetObjectField(TEXT("Properties"));
		if (Properties.IsValid())
		{
			JSON_TRY_STRING(Properties, TechnicalName);
			JSON_TRY_HEX_ID(Properties, Id);
			JSON_TRY_HEX_ID(Properties, Parent);

			FString stringId;
			Properties->TryGetStringField(TEXT("Id"), stringId);
			NameAndId = FString::Printf(TEXT("%s_%s"), *TechnicalName, *stringId);

			// Serialize the Properties to string, using the condensed writer to save memory
			TSharedRef< TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> > Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&PropertiesJsonString);
			FJsonSerializer::Serialize(Properties.ToSharedRef(), Writer);
		}
	}

	{
		TemplateJsonString = "";
		const TSharedPtr<FJsonObject>* Template;
		if (JsonModel->TryGetObjectField(TEXT("Template"), Template))
		{
			TSharedRef< TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> > Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&TemplateJsonString);
			FJsonSerializer::Serialize(Template->ToSharedRef(), Writer);
		}
	}
}

/**
 * Gathers scripts from the model definition and adds them to the ArticyImportData.
 *
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyModelDef::GatherScripts(UArticyImportData* Data) const
{
	Data->GetObjectDefs().GatherScripts(*this, Data);
}

/**
 * Generates a sub-asset from the model definition.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @param Outer The outer object for the sub-asset.
 * @return A pointer to the generated UArticyObject sub-asset.
 */
UArticyObject* FArticyModelDef::GenerateSubAsset(const UArticyImportData* Data, UObject* Outer) const
{
	// The class is found by taking the CPP name and removing the first character
	auto className = Data->GetObjectDefs().GetCppType(Type, Data, false);
	className.RemoveAt(0);

	// Generate the asset
	auto fullClassName = FString::Printf(TEXT("Class'/Script/%s.%s'"), FApp::GetProjectName(), *className);
	auto uclass = ConstructorHelpersInternal::FindOrLoadClass(fullClassName, UArticyObject::StaticClass());
	if (uclass)
	{
		auto obj = ArticyImporterHelpers::GenerateSubAsset<UArticyObject>(*className, FApp::GetProjectName(), GetNameAndId(), Outer);
		FAssetRegistryModule::AssetCreated(Cast<UObject>(obj));
		if (ensure(obj))
		{
			obj->Initialize();
			Data->GetObjectDefs().InitializeModel(obj, *this, Data, Outer->GetName());

			// SAVE!!
			obj->MarkPackageDirty();
		}

		return obj;
	}
	return nullptr;
}

/**
 * Gets the properties JSON object from the cached properties JSON string.
 *
 * @return A shared pointer to the properties JSON object.
 */
TSharedPtr<FJsonObject> FArticyModelDef::GetPropertiesJson() const
{
	if (!CachedPropertiesJson.IsValid())
	{
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(PropertiesJsonString);
		FJsonSerializer::Deserialize(JsonReader, CachedPropertiesJson);

		if (!CachedPropertiesJson.IsValid())
			CachedPropertiesJson = MakeShareable(new FJsonObject);
	}

	return CachedPropertiesJson;
}

/**
 * Gets the templates JSON object from the cached templates JSON string.
 *
 * @return A shared pointer to the templates JSON object.
 */
TSharedPtr<FJsonObject> FArticyModelDef::GetTemplatesJson() const
{
	if (!CachedTemplateJson.IsValid())
	{
		TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(TemplateJsonString);
		FJsonSerializer::Deserialize(JsonReader, CachedTemplateJson);

		if (!CachedTemplateJson.IsValid())
			CachedTemplateJson = MakeShareable(new FJsonObject);
	}

	return CachedTemplateJson;
}

/**
 * Converts a category string to an EArticyAssetCategory enum value.
 *
 * @param Category The category string.
 * @return The corresponding EArticyAssetCategory value.
 */
EArticyAssetCategory FArticyModelDef::GetAssetCategoryFromString(const FString Category)
{
	if (Category == "Image") return EArticyAssetCategory::Image;
	else if (Category == "Video") return EArticyAssetCategory::Video;
	else if (Category == "Audio") return EArticyAssetCategory::Audio;
	else if (Category == "Document") return EArticyAssetCategory::Document;
	else if (Category == "Misc") return EArticyAssetCategory::Misc;
	else if (Category == "All") return EArticyAssetCategory::All;
	else return EArticyAssetCategory::None;
}

//---------------------------------------------------------------------------//

/**
 * Imports package definition data from a JSON object.
 *
 * @param Archive A reference to the ArticyArchiveReader object.
 * @param JsonPackage A shared pointer to the JSON object containing the package definition.
 */
void FArticyPackageDef::ImportFromJson(const UArticyArchiveReader& Archive, const TSharedPtr<FJsonObject>& JsonPackage)
{
	if (!JsonPackage.IsValid())
		return;

	JSON_TRY_HEX_ID(JsonPackage, Id);
	JSON_TRY_BOOL(JsonPackage, IsIncluded);

	if (!IsIncluded)
		return;

	JSON_TRY_STRING(JsonPackage, Name);
	JSON_TRY_STRING(JsonPackage, Description);
	JSON_TRY_BOOL(JsonPackage, IsDefaultPackage);
	JSON_TRY_STRING(JsonPackage, ScriptFragmentHash);

	TSharedPtr<FJsonObject> Files;
	JSON_TRY_OBJECT(JsonPackage, Files, {
		TSharedPtr<FJsonObject> Objects;
		if (!Archive.FetchJson(
			*obj,
			JSON_SUBSECTION_OBJECTS,
			PackageObjectsHash,
			Objects))
		{
			return;
		}

		Models.Reset();
		JSON_TRY_ARRAY(Objects, Objects,
		{
			auto innerObj = item->AsObject();
			if (innerObj.IsValid())
			{
				FArticyModelDef model;
				model.ImportFromJson(innerObj);
				Models.Add(model);
			}
		});

		TSharedPtr<FJsonObject> TextData;
		if (!Archive.FetchJson(
			*obj,
			JSON_SUBSECTION_TEXTS,
			PackageTextsHash,
			TextData))
		{
			return;
		}

		Texts.Reset();
		GatherText(TextData);
		});
}

/**
 * Gathers scripts from the package definition and adds them to the ArticyImportData.
 *
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyPackageDef::GatherScripts(UArticyImportData* Data) const
{
	for (const auto& model : Models)
		Data->GetObjectDefs().GatherScripts(model, Data);
}

/**
 * Generates a package asset from the package definition.
 *
 * @param Data A pointer to the UArticyImportData object.
 * @return A pointer to the generated UArticyPackage asset.
 */
UArticyPackage* FArticyPackageDef::GeneratePackageAsset(UArticyImportData* Data) const
{
	const FString PackageName = GetFolder();
	const FString PackagePath = ArticyHelpers::GetArticyGeneratedFolder() / PackageName;

	// @TODO Engine Versioning
#if ENGINE_MAJOR_VERSION >= 5 || (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 26)
	UPackage* AssetPackage = CreatePackage(*PackagePath);
#else
	UPackage* AssetPackage = CreatePackage(nullptr, *PackagePath);
#endif

	AssetPackage->FullyLoad();

	const FString AssetName = FPaths::GetBaseFilename(PackageName);

	UArticyPackage* ArticyPackage = ArticyImporterHelpers::GenerateAsset<UArticyPackage>(*UArticyPackage::StaticClass()->GetName(), TEXT("ArticyRuntime"), *AssetName, "Packages");

	ArticyPackage->Clear();
	ArticyPackage->Name = Name;
	ArticyPackage->Description = Description;
	ArticyPackage->bIsDefaultPackage = IsDefaultPackage;

	// Create all contained subassets and register them in the package
	for (const auto model : Models)
	{
		UArticyObject* asset = model.GenerateSubAsset(Data, ArticyPackage); //MM_CHANGE

		if (asset)
		{
			FString id = ArticyHelpers::Uint64ToHex(asset->GetId());
			ArticyPackage->AddAsset(asset);
			Data->AddChildToParentCache(model.GetParent(), model.GetId());
		}
	}

	FAssetRegistryModule::AssetCreated(ArticyPackage);

	AssetPackage->MarkPackageDirty();

	return ArticyPackage;
}

/**
 * Gets the folder path for the package.
 *
 * @return The folder path as a string.
 */
FString FArticyPackageDef::GetFolder() const
{
	return (FString(TEXT("Packages")) / Name).Replace(TEXT(" "), TEXT("_"));
}

/**
 * Gets the folder name for the package.
 *
 * @return The folder name as a string.
 */
FString FArticyPackageDef::GetFolderName() const
{
	int32 pathCutOffIndex = INDEX_NONE;
	FString folder = this->GetFolder();
	folder.FindLastChar('/', pathCutOffIndex);

	if (pathCutOffIndex != INDEX_NONE)
	{
		return this->GetFolder().RightChop(pathCutOffIndex);
	}
	else
	{
		UE_LOG(LogArticyEditor, Error, TEXT("Could not retrieve folder name for package %s! Did GetFolder() method change?"), *this->Name);
		return FString(TEXT("Invalid"));
	}
}

/**
 * Gets the name of the package.
 *
 * @return The package name as a string.
 */
const FString FArticyPackageDef::GetName() const
{
	return Name;
}

/**
 * Gets the previous name of the package.
 *
 * @return The previous package name as a string.
 */
const FString FArticyPackageDef::GetPreviousName() const
{
	if (PreviousName.Len() == 0)
	{
		return Name;
	}

	return PreviousName;
}

/**
 * Sets the name of the package.
 *
 * @param NewName The new name for the package.
 */
void FArticyPackageDef::SetName(const FString& NewName)
{
	PreviousName = Name;
	Name = NewName;
}

/**
 * Gets the ID of the package.
 *
 * @return The package ID as an FArticyId.
 */
FArticyId FArticyPackageDef::GetId() const
{
	return Id;
}

/**
 * Checks if the package is included.
 *
 * @return True if the package is included, false otherwise.
 */
bool FArticyPackageDef::GetIsIncluded() const
{
	return IsIncluded;
}

//---------------------------------------------------------------------------//

/**
 * Imports package definitions from a JSON array.
 *
 * @param Archive A reference to the ArticyArchiveReader object.
 * @param Json A pointer to the JSON array containing the package definitions.
 * @param Settings A reference to the FADISettings object.
 */
void FArticyPackageDefs::ImportFromJson(
	const UArticyArchiveReader& Archive,
	const TArray<TSharedPtr<FJsonValue>>* Json,
	FAdiSettings& Settings)
{
	if (!Json)
		return;

	TSet<FString> OldPackageScriptHashes;
	TArray<FArticyPackageDef> PackagesToRemove;

	// Iterate over existing packages
	for (auto& ExistingPackage : Packages)
	{
		OldPackageScriptHashes.Add(ExistingPackage.GetScriptFragmentHash());

		bool bExistingPackageFound = false;

		// Iterate over new package list
		for (const auto pack : *Json)
		{
			const auto obj = pack->AsObject();
			if (!obj.IsValid())
				continue;

			FArticyPackageDef package;
			package.ImportFromJson(Archive, obj);

			// If package with the same Id is found
			if (ExistingPackage.GetId() == package.GetId())
			{
				bExistingPackageFound = true;

				const FString& OldName = ExistingPackage.GetName();
				const FString& NewName = package.GetName();

				// If IsIncluded is set on the new package, replace the existing package
				if (package.GetIsIncluded())
				{
					ExistingPackage = package;

					// Useful if we ever decide to rename included packages 
					ExistingPackage.SetName(OldName);
				}

				if (!NewName.Equals(OldName))
				{
					// Name has changed
					ExistingPackage.SetName(NewName);
				}

				break;
			}
		}

		// If existing package was not found in the new package list, mark it for removal
		if (!bExistingPackageFound)
		{
			PackagesToRemove.Add(ExistingPackage);
		}
	}

	// Remove packages that don't exist in the new package list
	for (const auto& PackageToRemove : PackagesToRemove)
	{
		Packages.RemoveSingle(PackageToRemove);
	}

	// Iterate over new package list
	for (const auto pack : *Json)
	{
		const auto obj = pack->AsObject();
		if (!obj.IsValid())
			continue;

		FArticyPackageDef package;
		package.ImportFromJson(Archive, obj);

		bool bExistingPackageFound = false;

		// Check if package already exists in the Packages array
		for (const auto& ExistingPackage : Packages)
		{
			if (ExistingPackage.GetId() == package.GetId())
			{
				bExistingPackageFound = true;
				break;
			}
		}

		// If package doesn't exist, add it to the Packages array
		if (!bExistingPackageFound)
		{
			Packages.Add(package);
		}
	}

	// Check if set of hashes are the same
	if (OldPackageScriptHashes.Num() == Packages.Num())
	{
		bool bScriptFragmentsChanged = false;

		for (auto& Package : Packages)
		{
			if (!OldPackageScriptHashes.Contains(Package.GetScriptFragmentHash()))
			{
				bScriptFragmentsChanged = true;
				break;
			}
		}

		if (!bScriptFragmentsChanged)
		{
			// Skip rebuilding script fragments - they are the same
			return;
		}
	}

	Settings.SetScriptFragmentsNeedRebuild();
}

/**
 * Validates the import of package definitions from a JSON array.
 *
 * @param Archive A reference to the ArticyArchiveReader object.
 * @param Json A pointer to the JSON array containing the package definitions.
 * @return True if the import is valid, false otherwise.
 */
bool FArticyPackageDefs::ValidateImport(
	const UArticyArchiveReader& Archive,
	const TArray<TSharedPtr<FJsonValue>>* Json)
{
	if (!Json)
		return false;

	// Iterate over existing packages
	for (auto& ExistingPackage : Packages)
	{
		// Old package has data
		if (ExistingPackage.GetIsIncluded())
			continue;

		bool bPackageDataFound = false;

		// Iterate over new package list
		for (const auto pack : *Json)
		{
			const auto obj = pack->AsObject();
			if (!obj.IsValid())
				continue;

			FArticyPackageDef package;
			package.ImportFromJson(Archive, obj);

			// If package with the same Id is found
			if (ExistingPackage.GetId() == package.GetId())
			{
				// If IsIncluded is set on the new package, we are safe to continue
				if (package.GetIsIncluded())
				{
					bPackageDataFound = true;
				}

				break;
			}
		}

		if (!bPackageDataFound)
		{
			UE_LOG(LogArticyEditor, Error, TEXT("No data for package %s"), *ExistingPackage.GetName());
			return false;
		}
	}

	// Iterate over new package list
	for (const auto pack : *Json)
	{
		const auto obj = pack->AsObject();
		if (!obj.IsValid())
			continue;

		FArticyPackageDef package;
		package.ImportFromJson(Archive, obj);

		// New package has data
		if (package.GetIsIncluded())
			continue;

		bool bPackageDataFound = false;

		// Check if package already exists in the Packages array
		for (const auto& ExistingPackage : Packages)
		{
			if (ExistingPackage.GetId() == package.GetId())
			{
				// Old package has data
				if (ExistingPackage.GetIsIncluded())
				{
					bPackageDataFound = true;
				}
				break;
			}
		}

		if (!bPackageDataFound)
		{
			UE_LOG(LogArticyEditor, Error, TEXT("No data for package %s"), *package.GetName());
			return false;
		}
	}

	// All checks passed - safe to import
	return true;
}

/**
 * Gathers scripts from all package definitions and adds them to the ArticyImportData.
 *
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyPackageDefs::GatherScripts(UArticyImportData* Data) const
{
	for (const auto& pack : Packages)
		pack.GatherScripts(Data);
}

/**
 * Gathers text data from a JSON object and adds it to the package definition.
 *
 * @param Json A shared pointer to the JSON object containing the text data.
 */
void FArticyPackageDef::GatherText(const TSharedPtr<FJsonObject>& Json)
{
	for (auto JsonValue = Json->Values.CreateConstIterator(); JsonValue; ++JsonValue)
	{
		const FString KeyName = (*JsonValue).Key;
		const TSharedPtr<FJsonValue> Value = (*JsonValue).Value;

		FArticyTexts Text;
		Text.ImportFromJson(Value->AsObject());
		Texts.Add(KeyName, Text);
	}
}

/**
 * Gets the texts map from the package definition.
 *
 * @return A map of text data.
 */
TMap<FString, FArticyTexts> FArticyPackageDef::GetTexts() const
{
	return Texts;
}

/**
 * Gets the texts map from a specific package definition.
 *
 * @param Package The package definition to retrieve texts from.
 * @return A map of text data.
 */
TMap<FString, FArticyTexts> FArticyPackageDefs::GetTexts(const FArticyPackageDef& Package)
{
	return Package.GetTexts();
}

/**
 * Generates assets for all package definitions and stores them in the ArticyImportData.
 *
 * @param Data A pointer to the UArticyImportData object.
 */
void FArticyPackageDefs::GenerateAssets(UArticyImportData* Data) const
{
	auto& ArticyPackages = Data->GetPackages();

	ArticyPackages.Reset(Packages.Num());

	for (auto pack : Packages)
	{
		ArticyPackages.Add(pack.GeneratePackageAsset(Data));
	}

	// Store gathered information about who has which children in generated assets
	auto parentChildrenCache = Data->GetParentChildrenCache();
	const auto childrenProp = FName{ TEXT("Children") };
	for (auto pack : ArticyPackages)
	{
		for (auto obj : pack->GetAssets())
		{
			if (auto articyObj = Cast<UArticyObject>(obj))
			{
				if (auto children = parentChildrenCache.Find(articyObj->GetId()))
				{
					// If the setting is enabled, try to sort. Will only work with exported position properties.
					if (GetDefault<UArticyPluginSettings>()->bSortChildrenAtGeneration)
					{
						children->Values.Sort(ArticyImporterHelpers::FCompareArticyNodeXLocation());
					}
					articyObj->SetProp(childrenProp, children->Values);
				}
			}
		}
	}
}

/**
 * Gets a set of package names from the package definitions.
 *
 * @return A set of package names.
 */
TSet<FString> FArticyPackageDefs::GetPackageNames() const
{
	TSet<FString> outArray;
	for (FArticyPackageDef def : Packages)
	{
		outArray.Add(def.GetName());
	}

	return outArray;
}

/**
 * Gets an array of package definitions.
 *
 * @return An array of package definitions.
 */
TArray<FArticyPackageDef> FArticyPackageDefs::GetPackages() const
{
	return Packages;
}

/**
 * Resets the packages array, clearing all package definitions.
 */
void FArticyPackageDefs::ResetPackages()
{
	Packages.Empty();
}

/**
 * Gets the script fragment hash for the package definition.
 *
 * @return The script fragment hash as a string.
 */
FString FArticyPackageDef::GetScriptFragmentHash() const
{
	return ScriptFragmentHash;
}

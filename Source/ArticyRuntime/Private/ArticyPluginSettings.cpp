//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyPluginSettings.h"
#include "Modules/ModuleManager.h"
#include "ArticyDatabase.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0 
#include "AssetRegistry/AssetRegistryModule.h"
#else
#include "AssetRegistryModule.h"
#endif
#include "Misc/ConfigCacheIni.h"

UArticyPluginSettings::UArticyPluginSettings()
{
	bCreateBlueprintTypeForScriptMethods = true;
	bKeepDatabaseBetweenWorlds = true;
	bKeepGlobalVariablesBetweenWorlds = true;
	bConvertUnityToUnrealRichText = false;
	bVerifyArticyReferenceBeforeImport = true;
	bUseLegacyImporter = false;

	bSortChildrenAtGeneration = false;
	ArticyDirectory = TEXT("/Game");
	// update package load settings after all files have been loaded
	FAssetRegistryModule& AssetRegistry = FModuleManager::Get().GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistry.Get().OnFilesLoaded().AddUObject(this, &UArticyPluginSettings::UpdatePackageSettings);
}

bool UArticyPluginSettings::DoesPackageSettingExist(FString packageName)
{
	return PackageLoadSettings.Contains(packageName);
}

const UArticyPluginSettings* UArticyPluginSettings::Get()
{
	static TWeakObjectPtr<UArticyPluginSettings> Settings;

	if (!Settings.IsValid())
	{
		Settings = TWeakObjectPtr<UArticyPluginSettings>(NewObject<UArticyPluginSettings>());
	}

	return Settings.Get();
}

/**
 * Prunes overrides for removed packages and re-applies the survivors. Untoggled packages
 * are left alone so the manifest's IsDefaultPackage stays authoritative.
 */
void UArticyPluginSettings::UpdatePackageSettings()
{
	TWeakObjectPtr<UArticyDatabase> ArticyDatabase = UArticyDatabase::GetMutableOriginal();

	if (!ArticyDatabase.IsValid())
	{
		return;
	}

	TArray<FString> ImportedPackageNames = ArticyDatabase->GetImportedPackageNames();

	TArray<FString> CurrentNames;
	PackageLoadSettings.GenerateKeyArray(CurrentNames);

	bool bRemoved = false;
	for (const FString& Name : CurrentNames)
	{
		if (!ImportedPackageNames.Contains(Name))
		{
			PackageLoadSettings.Remove(Name);
			bRemoved = true;
		}
	}

	if (bRemoved)
	{
		SaveConfig();
	}

	ApplyPreviousSettings();
}

/**
 * Applies persisted overrides to UArticyPackage::bIsDefaultPackage. Packages without an
 * entry keep the manifest value written by FArticyPackageDef::GeneratePackageAsset.
 */
void UArticyPluginSettings::ApplyPreviousSettings() const
{
	TWeakObjectPtr<UArticyDatabase> OriginalDatabase = UArticyDatabase::GetMutableOriginal();
	if (!OriginalDatabase.IsValid())
		return;

	const UArticyPluginSettings* DefaultSettings = GetDefault<UArticyPluginSettings>();

	for (const TPair<FString, bool>& Override : DefaultSettings->PackageLoadSettings)
	{
		OriginalDatabase->ChangePackageDefault(FName(*Override.Key), Override.Value);
	}
}


#if WITH_EDITOR
void UArticyPluginSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	GConfig->Flush(false, GEngineIni);
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UArticyPluginSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	GConfig->Flush(false, GEngineIni);
}

void UArticyPluginSettings::PostInitProperties()
{
	Super::PostInitProperties();
	GConfig->Flush(false, GEngineIni);
}

void UArticyPluginSettings::PostTransacted(const FTransactionObjectEvent& TransactionEvent)
{
	Super::PostTransacted(TransactionEvent);
	GConfig->Flush(false, GEngineIni);
}
#endif

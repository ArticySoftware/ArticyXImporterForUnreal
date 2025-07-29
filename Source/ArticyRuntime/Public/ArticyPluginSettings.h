//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "Runtime/Launch/Resources/Version.h"
#include "ArticyPluginSettings.generated.h"

/**
 * UArticyPluginSettings is a class that manages configuration settings for the Articy plugin.
 * It handles settings related to import, runtime behavior, and other plugin-specific features.
 */
UCLASS(config = Engine, defaultconfig)
class ARTICYRUNTIME_API UArticyPluginSettings : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Default constructor for UArticyPluginSettings.
	 */
	UArticyPluginSettings();

	/**
	 * Exposes the generated method provider interface to Blueprint.
	 * INFO: Generated functions will be named according to the format <MethodName>_<ArgumentTypes>
	 * as a workaround to support overloaded functions in blueprint.
	 */
	UPROPERTY(EditAnywhere, config, Category = ImportSettings, meta = (DisplayName = "Create Blueprint type for script method interface"))
	bool bCreateBlueprintTypeForScriptMethods;

	/**
	 * If true, will attempt to sort the children using the exported position properties.
	 * This can be slower.
	 */
	UPROPERTY(EditAnywhere, config, Category = ImportSettings, meta = (DisplayName = "Sort children when importing"))
	bool bSortChildrenAtGeneration;

	/**
	 * If true, the importer will try to parse the Source/<ProjectName>/<ProjectName>.Build.cs file
	 * to find a reference to the ArticyRuntime inside it before importing.
	 * Proposes to the user to add it automatically if no reference is found.
	 */
	UPROPERTY(EditAnywhere, config, Category = ImportSettings, meta = (DisplayName = "Verify ArticyRuntime reference inside Unreal Build tools before importing"))
	bool bVerifyArticyReferenceBeforeImport;

	/**
	 * If true, the old importer process will be used (Articy version < 3.2.3).
	 */
	UPROPERTY(EditAnywhere, Config, Category = ImportSettings, meta = (DisplayName = "Use legacy importer (prev. Articy 3.2.3)"))
	bool bUseLegacyImporter;

	/**
	 * The directory where ArticyContent will be generated and assets are looked for
	 * (when using ArticyAsset). Also used to search for the .articyue file to regenerate
	 * the import asset. Automatically set to the location of the import asset during import.
	 * Change only to manually apply a fix!
	 */
	UPROPERTY(VisibleAnywhere, config, Category = ImportSettings, meta = (DisplayName = "Articy Directory", ContentDir, LongPackageName))
	FString ArticyDirectory;


	/**
	 * Keeps one instance of the database for the whole game alive, even if the world changes.
	 */
	UPROPERTY(EditAnywhere, config, Category = RuntimeSettings, meta = (DisplayName = "Keep database between worlds"))
	bool bKeepDatabaseBetweenWorlds;

	/**
	 * Keeps one instance of the global variables for the whole game alive, even if the world changes.
	 */
	UPROPERTY(EditAnywhere, config, Category = RuntimeSettings, meta = (DisplayName = "Keep global variables between worlds"))
	bool bKeepGlobalVariablesBetweenWorlds;

	/**
	 * If true, converts Unity formatting in the exported articy:draft project into Unreal's rich text format.
	 * Hit "Import Changes" anytime you change this setting.
	 */
	UPROPERTY(EditAnywhere, config, Category = RuntimeSettings, meta = (DisplayName = "Convert Unity formatting to Unreal Rich Text"))
	bool bConvertUnityToUnrealRichText;

	/**
	 * Internal cached data for data consistency between imports (setting restoration etc.).
	 */
	UPROPERTY()
	TMap<FString, bool> PackageLoadSettings;

	/**
	 * Checks if a package setting exists in the internal settings map.
	 *
	 * @param packageName The name of the package to check.
	 * @return True if the package setting exists, false otherwise.
	 */
	bool DoesPackageSettingExist(FString packageName);

	/**
	 * Retrieves the singleton instance of UArticyPluginSettings.
	 *
	 * @return A const pointer to the singleton instance of UArticyPluginSettings.
	 */
	static const UArticyPluginSettings* Get();

	/**
	 * Updates package settings based on the currently imported packages.
	 */
	void UpdatePackageSettings();

	/**
	 * Applies previously saved settings for packages to avoid resetting user-tweaked values.
	 */
	void ApplyPreviousSettings() const;

#if WITH_EDITOR
	/**
	 * Called after a property is edited in the editor.
	 *
	 * @param PropertyChangedEvent Contains details about the property that was changed.
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * Called after a config file is reloaded.
	 *
	 * @param PropertyThatWasLoaded The property that was reloaded.
	 */
	virtual void PostReloadConfig(class FProperty* PropertyThatWasLoaded) override;

	/**
	 * Called after properties are initialized.
	 */
	virtual void PostInitProperties() override;

	/**
	 * Called after a transaction occurs.
	 *
	 * @param TransactionEvent Contains details about the transaction.
	 */
	virtual void PostTransacted(const FTransactionObjectEvent& TransactionEvent) override;
#endif

};

//  
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.  
//

#pragma once

#include "CoreMinimal.h"
#include "ObjectDefinitionsImport.h"
#include "PackagesImport.h"
#include "ArticyPackage.h"
#include "ArticyArchiveReader.h"
#include "StringTableGenerator.h"
#include "Serialization/Csv/CsvParser.h"
#include "ArticyImportData.generated.h"

class UArticyImportData;

/**
 * The Settings object in the .json file.
 */
USTRUCT()
struct FAdiSettings
{
	GENERATED_BODY()

public:
	/** Name of the text formatter used by the articy project. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString set_TextFormatter = "";

	/** If this is false, no ExpressoScripts class is generated, and script fragments are not evaluated/executed. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	bool set_UseScriptSupport = false;

	/** Comma-separated list of node type names included in this export. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString set_IncludedNodes = "";

	/** The articy id of the rule set used by this export. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FArticyId RuleSetId;

	/** The articy:draft X export format version this import data was produced from. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString ExportVersion = "";

	/** Hash of the global variables section; used to decide whether GV code needs regeneration. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString GlobalVariablesHash = "";

	/** Hash of the object definitions section; used to decide whether object-def code needs regeneration. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString ObjectDefinitionsHash = "";

	/** Hash of the object-definitions localized texts section. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString ObjectDefinitionsTextHash = "";

	/** Hash of the script fragments section; used to decide whether expresso code needs regeneration. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString ScriptFragmentsHash = "";

	/** Hash of the hierarchy section; used to decide whether hierarchy-driven code needs regeneration. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString HierarchyHash = "";

	/** Hash of the script methods section; used to decide whether the user-methods interface needs regeneration. */
	UPROPERTY(VisibleAnywhere, Category = "Settings")
	FString ScriptMethodsHash = "";

	/**
	 * Populates this settings object from the Settings section of the articy JSON export.
	 *
	 * @param JsonRoot The JSON object containing the Settings section.
	 */
	void ImportFromJson(const TSharedPtr<FJsonObject> JsonRoot);

	/** Returns true if object definitions or global variables have changed since the last rebuild. */
	bool DidObjectDefsOrGVsChange() const { return bObjectDefsOrGVsChanged; }
	/** Returns true if the script fragments have changed since the last rebuild. */
	bool DidScriptFragmentsChange() const { return bScriptFragmentsChanged; }

	/** Marks object definitions and global variables as up-to-date with the generated code. */
	void SetObjectDefinitionsRebuilt() { bObjectDefsOrGVsChanged = false; }
	/** Marks script fragments as up-to-date with the generated code. */
	void SetScriptFragmentsRebuilt() { bScriptFragmentsChanged = false; }

	/** Marks object definitions and global variables as needing regeneration. */
	void SetObjectDefinitionsNeedRebuild() { bObjectDefsOrGVsChanged = true; }
	/** Marks script fragments as needing regeneration. */
	void SetScriptFragmentsNeedRebuild() { bScriptFragmentsChanged = true; }


protected:
	//unused in the UE plugin
	UPROPERTY(VisibleAnywhere, Category = "Settings", meta = (DisplayName = "set_Localization - unused in UE"))
	bool set_Localization = false;

private:
	bool bObjectDefsOrGVsChanged = false;

	bool bScriptFragmentsChanged = false;
};

/**
 * The Project object in the .json file.
 */
USTRUCT()
struct FArticyProjectDef
{
	GENERATED_BODY()

public:
	/** Display name of the articy:draft project. */
	UPROPERTY(VisibleAnywhere, Category = "Project")
	FString Name;
	/** Extended/detailed name of the project. */
	UPROPERTY(VisibleAnywhere, Category = "Project")
	FString DetailName;
	/** Unique GUID of the articy:draft project. */
	UPROPERTY(VisibleAnywhere, Category = "Project")
	FString Guid;
	/** Technical name of the project; used as the prefix for generated classes (e.g. <TechnicalName>Database). */
	UPROPERTY(VisibleAnywhere, Category = "Project")
	FString TechnicalName;

	/**
	 * Populates this project definition from the Project section of the articy JSON export.
	 *
	 * @param JsonRoot The JSON object containing the Project section.
	 * @param Settings The import settings, updated to reflect any relevant project-level changes.
	 */
	void ImportFromJson(const TSharedPtr<FJsonObject> JsonRoot, FAdiSettings& Settings);
};

/**
 * Enumeration for Articy data types.
 */
UENUM()
enum class EArticyType : uint8
{
	/** Boolean variable. */
	ADT_Boolean,
	/** 32-bit signed integer variable. */
	ADT_Integer,
	/** Single-language string variable. */
	ADT_String,
	/** String variable with per-language values. */
	ADT_MultiLanguageString
};

/**
 * A single global variable definition.
 */
USTRUCT()
struct FArticyGVar
{
	GENERATED_BODY()

public:
	/** The variable's name within its namespace. */
	UPROPERTY(VisibleAnywhere, Category = "Variable")
	FString Variable;
	/** The variable's articy type. */
	UPROPERTY(VisibleAnywhere, Category = "Variable")
	EArticyType Type = EArticyType::ADT_String;
	/** The variable's description, as authored in articy:draft. */
	UPROPERTY(VisibleAnywhere, Category = "Variable")
	FString Description;

	/** The initial value for a boolean variable (ADT_Boolean). */
	UPROPERTY(VisibleAnywhere, Category = "Variable")
	bool BoolValue = false;
	/** The initial value for an integer variable (ADT_Integer). */
	UPROPERTY(VisibleAnywhere, Category = "Variable")
	int IntValue = 0;
	/** The initial value for a string variable (ADT_String / ADT_MultiLanguageString). */
	UPROPERTY(VisibleAnywhere, Category = "Variable")
	FString StringValue;

	/** Returns the UArticyVariable type to be used for this variable. */
	FString GetCPPTypeString() const;
	/** Returns the C++ literal representation of this variable's initial value. */
	FString GetCPPValueString() const;

	/**
	 * Populates this variable from a JSON entry in the GlobalVariables section of the articy export.
	 *
	 * @param JsonVar The JSON value describing the variable.
	 */
	void ImportFromJson(const TSharedPtr<FJsonObject> JsonVar);
};

/**
 * A namespace containing global variables.
 */
USTRUCT()
struct FArticyGVNamespace
{
	GENERATED_BODY()

public:
	/** The name of this namespace */
	UPROPERTY(VisibleAnywhere, Category = "Namespace")
	FString Namespace;
	UPROPERTY(VisibleAnywhere, Category = "Namespace")
	FString Description;
	UPROPERTY(VisibleAnywhere, Category = "Namespace")
	TArray<FArticyGVar> Variables;

	UPROPERTY(VisibleAnywhere, Category = "Namespace")
	FString CppTypename;

	void ImportFromJson(const TSharedPtr<FJsonObject> JsonNamespace, const UArticyImportData* Data);
};

/**
 * Information about global variables.
 */
USTRUCT()
struct FArticyGVInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "GlobalVariables")
	TArray<FArticyGVNamespace> Namespaces;

	void ImportFromJson(const TArray<TSharedPtr<FJsonValue>>* Json, const UArticyImportData* Data);
};

//---------------------------------------------------------------------------//

/**
 * A parameter for a script method.
 */
USTRUCT()
struct FAIDScriptMethodParameter
{
	GENERATED_BODY()

	FAIDScriptMethodParameter() {}
	FAIDScriptMethodParameter(FString InType, FString InName) : Type(InType), Name(InName) {}

	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	FString Type;

	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	FString Name;
};

/**
 * A script method definition.
 */
USTRUCT()
struct FAIDScriptMethod
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	FString Name;
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	FString BlueprintName;
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	bool bIsOverloadedFunction = false;

	/** A list of parameters (type + parameter name), to be used in a method declaration. */
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	TArray<FAIDScriptMethodParameter> ParameterList;
	/** A list of arguments (values), including a leading comma, to be used when calling a method. */
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	TArray<FString> ArgumentList;
	/** A list of parameters (original types), used for generating the blueprint function display name. */
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	TArray<FString> OriginalParameterTypes;

	const FString& GetCPPReturnType() const;
	const FString& GetCPPDefaultReturn() const;
	const FString GetCPPParameters() const;
	const FString GetArguments() const;
	const FString GetOriginalParametersForDisplayName() const;

	void ImportFromJson(TSharedPtr<FJsonObject> Json, TSet<FString>& OverloadedMethods);

private:
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	FString ReturnType;
};

/**
 * A collection of user-defined script methods.
 */
USTRUCT()
struct FAIDUserMethods
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "ScriptMethods")
	TArray<FAIDScriptMethod> ScriptMethods;

	void ImportFromJson(const TArray<TSharedPtr<FJsonValue>>* Json);
};

/**
 * A single language definition
 */
USTRUCT()
struct FArticyLanguageDef
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Language")
	FString CultureName;
	UPROPERTY(VisibleAnywhere, Category = "Language")
	FString ArticyLanguageId;
	UPROPERTY(VisibleAnywhere, Category = "Language")
	FString LanguageName;
	UPROPERTY(VisibleAnywhere, Category = "Language")
	bool IsVoiceOver = false;

	void ImportFromJson(const TSharedPtr<FJsonObject>& JsonRoot);
};

/**
 * The Languages object in the manifest file.
 */
USTRUCT()
struct FArticyLanguages
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Languages")
	TMap<FString, FArticyLanguageDef> Languages;

	void ImportFromJson(const TSharedPtr<FJsonObject>& JsonRoot);
};

/*Used as a workaround to store an array in a map*/
USTRUCT()
struct FArticyIdArray
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FArticyId> Values;
};

//---------------------------------------------------------------------------//

/**
 * Represents a hierarchy object in the Articy import data.
 */
UCLASS(BlueprintType)
class UADIHierarchyObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "HierarchyObject")
	FString Id;
	UPROPERTY(VisibleAnywhere, Category = "HierarchyObject")
	FString TechnicalName;
	UPROPERTY(VisibleAnywhere, Category = "HierarchyObject")
	FString Type;

	UPROPERTY(VisibleAnywhere, Category = "HierarchyObject")
	TArray<UADIHierarchyObject*> Children;

	static UADIHierarchyObject* CreateFromJson(UObject* Outer, const TSharedPtr<FJsonObject> JsonObject);
};

/**
 * Represents a hierarchy of Articy objects.
 */
USTRUCT()
struct FADIHierarchy
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Hierarchy")
	UADIHierarchyObject* RootObject = nullptr;

	void ImportFromJson(UArticyImportData* ImportData, const TSharedPtr<FJsonObject> JsonRoot);
};

/**
 * A fragment of Expresso script code.
 */
USTRUCT()
struct FArticyExpressoFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Script")
	FString OriginalFragment = "";
	UPROPERTY(VisibleAnywhere, Category = "Script")
	FString ParsedFragment = "";
	UPROPERTY(VisibleAnywhere, Category = "Script")
	bool bIsInstruction = false;

	bool operator==(const FArticyExpressoFragment& Other) const
	{
		return bIsInstruction == Other.bIsInstruction && OriginalFragment == Other.OriginalFragment;
	}
};

inline uint32 GetTypeHash(const FArticyExpressoFragment& A)
{
	return GetTypeHash(A.OriginalFragment) ^ GetTypeHash(A.bIsInstruction);
}

/**
 * Structure for Articy import data.
 */
USTRUCT()
struct ARTICYEDITOR_API FArticyImportDataStruct
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FAdiSettings Settings;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyProjectDef Project;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyGVInfo GlobalVariables;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyObjectDefinitions ObjectDefinitions;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyPackageDefs PackageDefs;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FAIDUserMethods UserMethods;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FADIHierarchy Hierarchy;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyLanguages Languages;

	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	TSet<FArticyExpressoFragment> ScriptFragments;

	UPROPERTY(VisibleAnywhere, Category = "Imported")
	TArray<TSoftObjectPtr<UArticyPackage>> ImportedPackages;

	UPROPERTY(VisibleAnywhere, Category = "Imported")
	TMap<FArticyId, FArticyIdArray> ParentChildrenCache;
};

USTRUCT()
struct ARTICYEDITOR_API FArticyCsvRow
{
	GENERATED_BODY()

	FString Key;
	FString SourceString;
	FString PackageId;
};

/**
 * Main class for handling Articy import data.
 */
UCLASS()
class ARTICYEDITOR_API UArticyImportData : public UDataAsset
{
	GENERATED_BODY()

public:
	void PostInitProperties() override;
	void PostLoad() override;

	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	class UAssetImportData* ImportData;

	void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;

	void PostImport();

	bool ImportFromJson(const UArticyArchiveReader& Archive, const TSharedPtr<FJsonObject> RootObject);

	const static TWeakObjectPtr<UArticyImportData> GetImportData();
	const FAdiSettings& GetSettings() const { return Settings; }
	FAdiSettings& GetSettings() { return Settings; }
	const FArticyProjectDef& GetProject() const { return Project; }
	const FArticyGVInfo& GetGlobalVars() const { return GlobalVariables; }
	const FADIHierarchy& GetHierarchy() const { return Hierarchy; }
	const FArticyObjectDefinitions& GetObjectDefs() const { return ObjectDefinitions; }
	const FArticyPackageDefs& GetPackageDefs() const { return PackageDefs; }

	TArray<TSoftObjectPtr<UArticyPackage>>& GetPackages() { return ImportedPackages; }
	TArray<UArticyPackage*> GetPackagesDirect();
	const TArray<TSoftObjectPtr<UArticyPackage>>& GetPackages() const { return ImportedPackages; }

	const TArray<FAIDScriptMethod>& GetUserMethods() const { return UserMethods.ScriptMethods; }

	void GatherScripts();
	void AddScriptFragment(const FString& Fragment, const bool bIsInstruction);
	const TSet<FArticyExpressoFragment>& GetScriptFragments() const { return ScriptFragments; }

	void AddChildToParentCache(FArticyId Parent, FArticyId Child);
	const TMap<FArticyId, FArticyIdArray>& GetParentChildrenCache() const { return ParentChildrenCache; }

	void BuildCachedVersion();
	void ResolveCachedVersion();
	bool HasCachedVersion() const { return bHasCachedVersion; }

	void SetInitialImportComplete() { bHasCachedVersion = true; }

	bool FinalizeImport(bool bAllowRemovalFinal);
	void SetLatestOwnerByObjectId(TMap<FArticyId, FArticyId>&& InMap) { LatestOwnerByObjectId = MoveTemp(InMap); }
	const TMap<FArticyId, FArticyId>& GetLatestOwnerByObjectId() const { return LatestOwnerByObjectId; }

	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyLanguages Languages;

	UPROPERTY(Transient)
	bool bDeferGeneration = false;

	bool bMultiFileMerge = false;

protected:

	UPROPERTY(VisibleAnywhere, Category = "Articy")
	FArticyImportDataStruct CachedData;

	// indicates whether we've had at least one working import. Used to determine if we want to re
	UPROPERTY()
	bool bHasCachedVersion = false;

private:

	friend class FArticyEditorFunctionLibrary;

	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FAdiSettings Settings;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyProjectDef Project;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyGVInfo GlobalVariables;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyObjectDefinitions ObjectDefinitions;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FArticyPackageDefs PackageDefs;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FAIDUserMethods UserMethods;
	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	FADIHierarchy Hierarchy;

	UPROPERTY(VisibleAnywhere, Category = "ImportData")
	TSet<FArticyExpressoFragment> ScriptFragments;

	UPROPERTY(VisibleAnywhere, Category = "Imported")
	TArray<TSoftObjectPtr<UArticyPackage>> ImportedPackages;

	UPROPERTY(VisibleAnywhere, Category = "Imported")
	TMap<FArticyId, FArticyIdArray> ParentChildrenCache;

	UPROPERTY(Transient)
	TMap<FArticyId, FArticyId> LatestOwnerByObjectId;

	void ImportAudioAssets(const FString& BaseContentDir);
	int ProcessStrings(TArray<FArticyCsvRow>& OutRows, const TMap<FString, FArticyTexts>& Data, const TPair<FString, FArticyLanguageDef>& Language, const FString& PackageId);

	static void LoadExistingRows(
		const FString& CsvPath,
		TMap<FString, FArticyCsvRow>& OutRows)
	{
		if (!FPaths::FileExists(CsvPath))
			return;

		FString FileContent;
		if (!FFileHelper::LoadFileToString(FileContent, *CsvPath))
			return;

		FCsvParser Parser(FileContent);
		const auto& Rows = Parser.GetRows();

		for (int32 i = 1; i < Rows.Num(); i++) // skip header
		{
			const auto& Row = Rows[i];

			if (Row.Num() < 3)
				continue;

			FArticyCsvRow CsvRow;
			CsvRow.Key = Row[0];
			CsvRow.SourceString = Row[1];
			CsvRow.PackageId = Row[2];

			OutRows.Add(CsvRow.Key, CsvRow);
		}
	}

	static void AddRowsUnique(
		TMap<FString, FArticyCsvRow>& Out,
		const TArray<FArticyCsvRow>& InRows,
		bool bOverwriteExisting = false)
	{
		for (const auto& R : InRows)
		{
			if (R.Key.IsEmpty())
				continue;

			if (bOverwriteExisting)
			{
				Out.Add(R.Key, R);
			}
			else
			{
				Out.FindOrAdd(R.Key, R);
			}
		}
	}

};

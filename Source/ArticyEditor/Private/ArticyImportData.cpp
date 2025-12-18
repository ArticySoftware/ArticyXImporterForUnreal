//  
// Copyright (c) 2023 articy Software GmbH & Co. KG. All rights reserved.  
//

#include "ArticyImportData.h"
#include "EditorFramework/AssetImportData.h"
#include "CodeGeneration/CodeGenerator.h"
#include "ArticyPluginSettings.h"
#include "Internationalization/Regex.h"
#include "ArticyEditorModule.h"
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 24
#include "Dialogs/Dialogs.h"
#else
#include "Misc/MessageDialog.h"
#endif
#include "ArticyArchiveReader.h"
#include "ISourceControlModule.h"
#include "SourceControlHelpers.h"
#include "StringTableGenerator.h"
#include "BuildToolParser/BuildToolParser.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Factories/SoundFactory.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "ArticyImportData"

/**
 * Imports settings from a JSON object.
 *
 * @param Json The JSON object to import from.
 */
void FAdiSettings::ImportFromJson(TSharedPtr<FJsonObject> Json)
{
	if (!Json.IsValid())
		return;

	JSON_TRY_STRING(Json, set_IncludedNodes);
	if (!set_IncludedNodes.Contains(TEXT("Settings")))
		return;

	const FArticyId OldRuleSetId = RuleSetId;
	JSON_TRY_HEX_ID(Json, RuleSetId);
	if (RuleSetId != OldRuleSetId)
	{
		// Different rule set, start over
		GlobalVariablesHash.Reset();
		ObjectDefinitionsHash.Reset();
		ObjectDefinitionsTextHash.Reset();
		ScriptFragmentsHash.Reset();
	}

	JSON_TRY_BOOL(Json, set_Localization);
	JSON_TRY_STRING(Json, set_TextFormatter);
	JSON_TRY_BOOL(Json, set_UseScriptSupport);
	JSON_TRY_STRING(Json, ExportVersion);
}

/**
 * Imports project definition from a JSON object.
 *
 * @param Json The JSON object to import from.
 * @param Settings The settings to be updated.
 */
void FArticyProjectDef::ImportFromJson(const TSharedPtr<FJsonObject> Json, FAdiSettings& Settings)
{
	if (!Json.IsValid())
		return;

	const FString OldGuid = Guid;
	const FString OldTechnicalName = TechnicalName;
	JSON_TRY_STRING(Json, Guid);
	JSON_TRY_STRING(Json, TechnicalName);

	if (!Guid.Equals(OldGuid) || !TechnicalName.Equals(OldTechnicalName))
	{
		// Treat as different export
		Settings.GlobalVariablesHash.Reset();
		Settings.ObjectDefinitionsHash.Reset();
		Settings.ObjectDefinitionsTextHash.Reset();
		Settings.ScriptFragmentsHash.Reset();
	}

	JSON_TRY_STRING(Json, Name);
	JSON_TRY_STRING(Json, DetailName);
}

/**
 * Retrieves the C++ type string representation for the global variable.
 *
 * @return The C++ type string.
 */
FString FArticyGVar::GetCPPTypeString() const
{
	switch (Type)
	{
	case EArticyType::ADT_Boolean:
		return TEXT("UArticyBool");

	case EArticyType::ADT_Integer:
		return TEXT("UArticyInt");

	case EArticyType::ADT_String:
		return TEXT("UArticyString");

	default:
		return TEXT("Cannot get CPP type string, unknown type!");
	}
}

/**
 * Retrieves the C++ value string representation for the global variable.
 *
 * @return The C++ value string.
 */
FString FArticyGVar::GetCPPValueString() const
{
	FString value;
	switch (Type)
	{
	case EArticyType::ADT_Boolean:
		value = FString::Printf(TEXT("%s"), BoolValue ? TEXT("true") : TEXT("false"));
		break;

	case EArticyType::ADT_Integer:
		value = FString::Printf(TEXT("%d"), IntValue);
		break;

	case EArticyType::ADT_String:
		value = FString::Printf(TEXT("\"%s\""), *StringValue);
		break;

	case EArticyType::ADT_MultiLanguageString:
		value = FString::Printf(TEXT("\"%s\""), *StringValue);
		break;

	default:
		value = TEXT("Cannot get CPP init string, unknown type!");
	}

	return value;
}

/**
 * Imports global variable data from a JSON object.
 *
 * @param JsonVar The JSON object representing the variable.
 */
void FArticyGVar::ImportFromJson(const TSharedPtr<FJsonObject> JsonVar)
{
	if (!JsonVar.IsValid())
		return;

	JSON_TRY_STRING(JsonVar, Variable);
	JSON_TRY_STRING(JsonVar, Description);

	FString typeString;
	if (JsonVar->TryGetStringField(TEXT("Type"), typeString))
	{
		if (typeString == TEXT("Boolean"))
			Type = EArticyType::ADT_Boolean;
		else if (typeString == TEXT("Integer"))
			Type = EArticyType::ADT_Integer;
		else
		{
			if (typeString != TEXT("String"))
				UE_LOG(LogArticyEditor, Error, TEXT("Unknown GlobalVariable type '%s', falling back to String."),
					*typeString);

			Type = EArticyType::ADT_String;
		}
	}

	switch (Type)
	{
	case EArticyType::ADT_Boolean: JsonVar->TryGetBoolField(TEXT("Value"), BoolValue);
		break;
	case EArticyType::ADT_Integer: JsonVar->TryGetNumberField(TEXT("Value"), IntValue);
		break;
	case EArticyType::ADT_String: JsonVar->TryGetStringField(TEXT("Value"), StringValue);
		break;
	case EArticyType::ADT_MultiLanguageString: JsonVar->TryGetStringField(TEXT("Value"), StringValue);
		break;
	default: break;
	}
}

/**
 * Imports namespace data from a JSON object.
 *
 * @param JsonNamespace The JSON object representing the namespace.
 * @param Data The import data object.
 */
void FArticyGVNamespace::ImportFromJson(const TSharedPtr<FJsonObject> JsonNamespace, const UArticyImportData* Data)
{
	if (!JsonNamespace.IsValid())
		return;

	JSON_TRY_STRING(JsonNamespace, Namespace);
	CppTypename = CodeGenerator::GetGVNamespaceClassname(Data, Namespace);
	JSON_TRY_STRING(JsonNamespace, Description);

	const TArray<TSharedPtr<FJsonValue>>* varsJson;
	if (!JsonNamespace->TryGetArrayField(TEXT("Variables"), varsJson))
		return;
	for (const auto& varJson : *varsJson)
	{
		const auto& obj = varJson->AsObject();
		if (!obj.IsValid())
			continue;

		FArticyGVar var;
		var.ImportFromJson(obj);
		Variables.Add(var);
	}
}

/**
 * Imports global variable information from a JSON array.
 *
 * @param Json The JSON array representing the global variables.
 * @param Data The import data object.
 */
void FArticyGVInfo::ImportFromJson(const TArray<TSharedPtr<FJsonValue>>* Json, const UArticyImportData* Data)
{
	Namespaces.Reset(Json ? Json->Num() : 0);

	if (!Json)
		return;

	for (const auto& nsJson : *Json)
	{
		const auto& obj = nsJson->AsObject();
		if (!obj.IsValid())
			continue;

		FArticyGVNamespace ns;
		ns.ImportFromJson(obj, Data);
		Namespaces.Add(ns);
	}
}

//---------------------------------------------------------------------------//

/**
 * Retrieves the C++ return type string for the script method.
 *
 * @return The C++ return type string.
 */
const FString& FAIDScriptMethod::GetCPPReturnType() const
{
	//TODO change this once the ReturnType is changed from C#-style ('System.Void' ecc.) to something more generic!
	if (ReturnType == "string")
	{
		const static auto String = FString{ "const FString" };
		return String;
	}
	if (ReturnType == "object")
	{
		// object is pretty much all encompassing. We only support them as UArticyPrimitives right now, which means GetObj(...) and self works.
		const static auto ArticyPrimitive = FString{ "UArticyPrimitive*" };
		return ArticyPrimitive;
	}

	return ReturnType;
}

/**
 * Retrieves the default C++ return value string for the script method.
 *
 * @return The default C++ return value string.
 */
const FString& FAIDScriptMethod::GetCPPDefaultReturn() const
{
	//TODO change this once the ReturnType is changed from C#-style ('System.Void' ecc.) to something more generic!
	if (ReturnType == "bool")
	{
		const static auto True = FString{ "true" };
		return True;
	}
	if (ReturnType == "int" || ReturnType == "float")
	{
		const static auto Zero = FString{ "0" };
		return Zero;
	}
	if (ReturnType == "string")
	{
		const static auto EmptyString = FString{ "\"\"" };
		return EmptyString;
	}
	if (ReturnType == "ArticyObject")
	{
		const static auto ArticyObject = FString{ "nullptr" };
		return ArticyObject;
	}
	if (ReturnType == "ArticyString")
	{
		const static auto ArticyObject = FString{ "nullptr" };
		return ArticyObject;
	}
	if (ReturnType == "ArticyMultiLanguageString")
	{
		const static auto ArticyObject = FString{ "nullptr" };
		return ArticyObject;
	}

	const static auto Nothing = FString{ "" };
	return Nothing;
}

/**
 * Retrieves the C++ parameters string for the script method.
 *
 * @return The C++ parameters string.
 */
const FString FAIDScriptMethod::GetCPPParameters() const
{
	FString Parameters = "";

	for (const auto& Parameter : ParameterList)
	{
		FString Type = Parameter.Type;

		if (Type.Equals("string"))
		{
			Type = TEXT("const FString&");
		}
		else if (Type.Equals("object"))
		{
			Type = TEXT("UArticyPrimitive*");
		}

		Parameters += Type + TEXT(" ") + Parameter.Name + TEXT(", ");
	}

	return Parameters.LeftChop(2);
}

/**
 * Retrieves the argument string for the script method.
 *
 * @return The argument string.
 */
const FString FAIDScriptMethod::GetArguments() const
{
	FString Parameters = "";

	for (const auto& Argument : ArgumentList)
	{
		Parameters += Argument + TEXT(", ");
	}

	return Parameters.LeftChop(2);
}

/**
 * Retrieves the original parameters string for display name generation.
 *
 * @return The original parameters string.
 */
const FString FAIDScriptMethod::GetOriginalParametersForDisplayName() const
{
	FString DisplayNameSuffix = "";

	for (const auto& OriginalParameterType : OriginalParameterTypes)
	{
		DisplayNameSuffix += OriginalParameterType + TEXT(", ");
	}

	return DisplayNameSuffix.LeftChop(2);
}

/**
 * Imports script method data from a JSON object.
 *
 * @param Json The JSON object representing the script method.
 * @param OverloadedMethods Set of overloaded method names.
 */
void FAIDScriptMethod::ImportFromJson(TSharedPtr<FJsonObject> Json, TSet<FString>& OverloadedMethods)
{
	JSON_TRY_STRING(Json, Name);
	JSON_TRY_STRING(Json, ReturnType);

	BlueprintName = Name + TEXT("_");
	ParameterList.Empty();
	OriginalParameterTypes.Empty();

	const TArray<TSharedPtr<FJsonValue>>* items;

	if (Json->TryGetArrayField(TEXT("Parameters"), items))
	{
		for (const auto& item : *items)
		{
			const TSharedPtr<FJsonObject>* obj;
			if (!ensure(item->TryGetObject(obj))) continue;

			//import parameter name and type
			FString Param, Type;
			JSON_TRY_STRING((*obj), Param);
			JSON_TRY_STRING((*obj), Type);

			// append param types to blueprint names
			FString formattedType = Type;
			formattedType[0] = FText::FromString(Type).ToUpper().ToString()[0];
			BlueprintName += formattedType;

			OriginalParameterTypes.Add(Type);

			//append to parameter list
			ParameterList.Emplace(Type, Param);
			ArgumentList.Add(Param);
		}
	}

	if (BlueprintName.EndsWith("_"))
		BlueprintName.RemoveAt(BlueprintName.Len() - 1);

	// determine if this is an overloaded blueprint function
	static TMap<FString, FString> UsedBlueprintMethodsNames;
	if (UsedBlueprintMethodsNames.Contains(Name))
	{
		if (UsedBlueprintMethodsNames[Name] != BlueprintName)
			OverloadedMethods.Add(Name);
	}
	else
	{
		UsedBlueprintMethodsNames.Add(Name, BlueprintName);
	}
}

/**
 * Imports user-defined script methods from a JSON array.
 *
 * @param Json The JSON array representing the script methods.
 */
void FAIDUserMethods::ImportFromJson(const TArray<TSharedPtr<FJsonValue>>* Json)
{
	ScriptMethods.Reset(Json ? Json->Num() : 0);

	if (!Json)
		return;

	TSet<FString> OverloadedMethods;

	for (const auto& smJson : *Json)
	{
		const auto& obj = smJson->AsObject();
		if (!obj.IsValid())
			continue;

		FAIDScriptMethod sm;
		sm.ImportFromJson(obj, OverloadedMethods);
		ScriptMethods.Add(sm);
	}

	// mark overloaded methods
	for (auto& scriptMethod : ScriptMethods)
		scriptMethod.bIsOverloadedFunction = OverloadedMethods.Contains(scriptMethod.Name);
}

//---------------------------------------------------------------------------//

/**
 * Creates a hierarchy object from a JSON object.
 *
 * @param Outer The outer object.
 * @param JsonObject The JSON object representing the hierarchy.
 * @return The created hierarchy object.
 */
UADIHierarchyObject* UADIHierarchyObject::CreateFromJson(UObject* Outer, const TSharedPtr<FJsonObject> JsonObject)
{
	if (!JsonObject.IsValid())
		return nullptr;

	//extract properties
	FString Id;
	JSON_TRY_STRING(JsonObject, Id);
	FString TechnicalName;
	JSON_TRY_STRING(JsonObject, TechnicalName);
	FString Type;
	JSON_TRY_STRING(JsonObject, Type);

	//create new object, referenced by Outer (otherwise it is transient!), with name TechnicalName
	auto obj = NewObject<UADIHierarchyObject>(Outer, *TechnicalName);
	obj->Id = Id;
	obj->TechnicalName = TechnicalName;
	obj->Type = Type;

	//fill in children
	const TArray<TSharedPtr<FJsonValue>>* jsonChildren;
	if (JsonObject->TryGetArrayField(TEXT("Children"), jsonChildren) && jsonChildren)
	{
		obj->Children.Reset(jsonChildren->Num());
		for (auto& jsonChild : *jsonChildren)
		{
			auto child = CreateFromJson(obj, jsonChild->AsObject());
			obj->Children.Add(child);
		}
	}

	return obj;
}

/**
 * Imports hierarchy data from a JSON object.
 *
 * @param ImportData The import data object.
 * @param Json The JSON object representing the hierarchy.
 */
void FADIHierarchy::ImportFromJson(UArticyImportData* ImportData, const TSharedPtr<FJsonObject> Json)
{
	RootObject = nullptr;

	//find the "Hierarchy" section
	if (!Json.IsValid())
		return;

	RootObject = UADIHierarchyObject::CreateFromJson(ImportData, Json);
}

/**
 * Imports language definition data from a JSON object.
 *
 * @param Json The JSON object representing the language definition.
 */
void FArticyLanguageDef::ImportFromJson(const TSharedPtr<FJsonObject>& Json)
{
	if (!Json.IsValid())
		return;

	JSON_TRY_STRING(Json, CultureName);
	JSON_TRY_STRING(Json, ArticyLanguageId);
	JSON_TRY_STRING(Json, LanguageName);
	JSON_TRY_BOOL(Json, IsVoiceOver);
}

/**
 * Imports language data from a JSON object.
 *
 * @param Json The JSON object representing the languages.
 */
void FArticyLanguages::ImportFromJson(const TSharedPtr<FJsonObject>& Json)
{
	if (!Json.IsValid())
		return;

	JSON_TRY_ARRAY(Json, Languages, {
		FArticyLanguageDef def;
		def.ImportFromJson(item->AsObject());
		Languages.Add(def.CultureName, def);
		});
}

//---------------------------------------------------------------------------//
//---------------------------------------------------------------------------//

/**
 * Initializes properties after construction.
 */
void UArticyImportData::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		ImportData = NewObject<UAssetImportData>(this, TEXT("ImportData"));
	}
#endif
}

#if WITH_EDITORONLY_DATA
/**
 * Retrieves asset registry tags for the import data.
 *
 * @param OutTags The array to fill with asset registry tags.
 */
void UArticyImportData::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	if (ImportData)
	{
		OutTags.Add(FAssetRegistryTag(SourceFileTagName(), ImportData->GetSourceData().ToJson(),
			FAssetRegistryTag::TT_Hidden));
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Super::GetAssetRegistryTags(OutTags);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}
#endif

/**
 * Handles actions to perform after importing data.
 */
void UArticyImportData::PostImport()
{
	FArticyEditorModule& ArticyEditorModule = FModuleManager::Get().GetModuleChecked<FArticyEditorModule>(
		"ArticyEditor");
	ArticyEditorModule.OnImportFinished.Broadcast();
}

/**
 * Imports data from a JSON object using the specified archive.
 *
 * @param Archive The archive reader.
 * @param RootObject The root JSON object.
 * @return True if import was successful, false otherwise.
 */
bool UArticyImportData::ImportFromJson(const UArticyArchiveReader& Archive, const TSharedPtr<FJsonObject> RootObject)
{
	// Abort if we will have broken packages
	if (!PackageDefs.ValidateImport(Archive, &RootObject->GetArrayField(JSON_SECTION_PACKAGES)))
		return false;

	// Record old script fragments hash
	const FString& OldScriptFragmentsHash = Settings.ScriptFragmentsHash;

	// import the main sections
	Settings.ImportFromJson(RootObject->GetObjectField(JSON_SECTION_SETTINGS));

	if (Settings.set_IncludedNodes.Contains(TEXT("Project")))
		Project.ImportFromJson(RootObject->GetObjectField(JSON_SECTION_PROJECT), Settings);

	Languages.ImportFromJson(RootObject);

	if (Settings.set_IncludedNodes.Contains(TEXT("Packages")))
		PackageDefs.ImportFromJson(Archive, &RootObject->GetArrayField(JSON_SECTION_PACKAGES), Settings);

	if (Settings.set_IncludedNodes.Contains(TEXT("Hierarchy")))
	{
		TSharedPtr<FJsonObject> HierarchyObject;
		if (
			Archive.FetchJson(
				RootObject,
				JSON_SECTION_HIERARCHY,
				Settings.HierarchyHash,
				HierarchyObject))
		{
			Hierarchy.ImportFromJson(this, HierarchyObject);
		}
	}

	TSharedPtr<FJsonObject> UserMethodsObject;
	if (
		Archive.FetchJson(
			RootObject,
			JSON_SECTION_SCRIPTMEETHODS,
			Settings.ScriptMethodsHash,
			UserMethodsObject))
	{
		UserMethods.ImportFromJson(&UserMethodsObject->GetArrayField(JSON_SECTION_SCRIPTMEETHODS));
		Settings.SetScriptFragmentsNeedRebuild();
	}

	bool bNeedsCodeGeneration = false;

	ParentChildrenCache.Empty();

	TSharedPtr<FJsonObject> GvObject;
	if (Archive.FetchJson(
		RootObject,
		JSON_SECTION_GLOBALVARS,
		Settings.GlobalVariablesHash,
		GvObject))
	{
		GlobalVariables.ImportFromJson(&GvObject->GetArrayField(JSON_SECTION_GLOBALVARS), this);
		Settings.SetObjectDefinitionsNeedRebuild();
		bNeedsCodeGeneration = true;
	}

	const TSharedPtr<FJsonObject> ObjectDefs = RootObject->GetObjectField(JSON_SECTION_OBJECTDEFS);
	TSharedPtr<FJsonObject> ObjTypes;
	if (Archive.FetchJson(
		RootObject->GetObjectField(JSON_SECTION_OBJECTDEFS),
		JSON_SUBSECTION_TYPES,
		Settings.ObjectDefinitionsHash,
		ObjTypes))
	{
		ObjectDefinitions.ImportFromJson(&ObjTypes->GetArrayField(JSON_SECTION_OBJECTDEFS), this);
		Settings.SetObjectDefinitionsNeedRebuild();
		bNeedsCodeGeneration = true;
	}

	const FString OldObjectDefintionsTextHash = Settings.ObjectDefinitionsTextHash;
	TSharedPtr<FJsonObject> ObjTexts;
	if (Archive.FetchJson(
		RootObject->GetObjectField(JSON_SECTION_OBJECTDEFS),
		JSON_SUBSECTION_TEXTS,
		Settings.ObjectDefinitionsTextHash,
		ObjTexts))
	{
		ObjectDefinitions.GatherText(ObjTexts);
		Settings.SetObjectDefinitionsNeedRebuild();
		bNeedsCodeGeneration = true;
	}

	if (Settings.ScriptFragmentsHash.IsEmpty() || !Settings.ScriptFragmentsHash.Equals(OldScriptFragmentsHash))
	{
		Settings.SetScriptFragmentsNeedRebuild();
	}

	if (Settings.DidScriptFragmentsChange() && this->GetSettings().set_UseScriptSupport)
	{
		this->GatherScripts();
		bNeedsCodeGeneration = true;
	}
	//===================================//

	// ArticyRuntime reference check, ask user to add "ArticyRuntime" Reference to Unreal build tool if needed.
	if (UArticyPluginSettings::Get()->bVerifyArticyReferenceBeforeImport)
	{
		FString path = FPaths::GameSourceDir() / FApp::GetProjectName() / FApp::GetProjectName() + TEXT(".Build.cs");
		// TEXT("");
		BuildToolParser RefVerifier = BuildToolParser(path);
		if (!RefVerifier.VerifyArticyRuntimeRef())
		{
			const FText RuntimeRefNotFoundTitle = FText::FromString(TEXT("ArticyRuntime reference not found."));
			const FText RuntimeRefNotFound = LOCTEXT("ArticyRuntimeReferenceNotFound",
				"The \"ArticyRuntime\" reference needs to be added inside the Unreal build tool.\nDo you want to add the reference automatically ?\nIf you use a custom build system or a custom build file, you can disable automatic reference verification inside the Articy Plugin settings from the Project settings.\n");
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 24
			EAppReturnType::Type ReturnType = OpenMsgDlgInt(EAppMsgType::Ok, RuntimeRefNotFound, RuntimeRefNotFoundTitle);
#elif ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
			EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNoCancel, RuntimeRefNotFound,
				RuntimeRefNotFoundTitle);
#else
			EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::YesNoCancel, RuntimeRefNotFound,
				&RuntimeRefNotFoundTitle);
#endif
			if (ReturnType == EAppReturnType::Yes)
			{
				RefVerifier.AddArticyRuntimmeRef();
			}
			else if (ReturnType == EAppReturnType::Cancel)
			{
				// Abort code generation
				bNeedsCodeGeneration = false;
			}
		}
	}

	// Add invariant if it does not exist
	if (!Languages.Languages.Contains(TEXT("")))
	{
		const auto& Iterator = Languages.Languages.CreateConstIterator();
		const auto& Elem = *Iterator;
		Languages.Languages.Add(TEXT(""), Elem.Value);
	}

	// Create string tables
	if (!OldObjectDefintionsTextHash.Equals(Settings.ObjectDefinitionsTextHash))
	{
		const auto& ObjectDefsText = GetObjectDefs().GetTexts();
		for (const auto& Language : Languages.Languages)
		{
			StringTableGenerator(TEXT("ARTICY"), Language.Key, [&](StringTableGenerator* CsvOutput)
				{
					return ProcessStrings(CsvOutput, ObjectDefsText, Language);
				});
		}
	}

	for (const auto& Language : Languages.Languages)
	{
		// Handle packages
		for (const auto& Package : GetPackageDefs().GetPackages())
		{
			const FString PackageName = Package.GetName();
			const FString StringTableFileName = PackageName.Replace(TEXT(" "), TEXT("_"));
			if (!Package.GetName().Equals(Package.GetPreviousName()))
			{
				// Needs rename
				const FString OldStringTableFileName = Package.GetPreviousName().Replace(TEXT(" "), TEXT("_"));
				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				ISourceControlModule& SCModule = ISourceControlModule::Get();

				bool bCheckOutEnabled = false;
				if (SCModule.IsEnabled())
				{
					bCheckOutEnabled = ISourceControlModule::Get().GetProvider().UsesCheckout();
				}

				// Work out the old and new file paths
				FString OldPath, NewPath;
				const FString OldFilePath = TEXT("ArticyContent/Generated") / OldStringTableFileName;
				const FString NewFilePath = TEXT("ArticyContent/Generated") / StringTableFileName;
				if (Language.Key.IsEmpty())
				{
					OldPath = FPaths::ProjectContentDir() / OldFilePath;
					NewPath = FPaths::ProjectContentDir() / NewFilePath;
				}
				else {
					OldPath = FPaths::ProjectContentDir() / TEXT("L10N") / Language.Key / OldFilePath;
					NewPath = FPaths::ProjectContentDir() / TEXT("L10N") / Language.Key / NewFilePath;
				}
				OldPath += TEXT(".csv");
				NewPath += TEXT(".csv");

				// Check out and rename
				if (PlatformFile.FileExists(*OldPath))
				{
					if (bCheckOutEnabled)
						USourceControlHelpers::CheckOutFile(*OldPath);

					// Rename the file
					PlatformFile.MoveFile(*NewPath, *OldPath);

					if (bCheckOutEnabled)
					{
						USourceControlHelpers::MarkFileForAdd(*NewPath);
						USourceControlHelpers::MarkFileForDelete(*OldPath);
					}
				}
			}

			if (!Package.GetIsIncluded())
				continue;

			StringTableGenerator(StringTableFileName, Language.Key,
				[&](StringTableGenerator* CsvOutput)
				{
					return ProcessStrings(CsvOutput, Package.GetTexts(), Language);
				});
		}
	}

	// Import Unreal audio assets
	FString AssetBaseDirectory = FPaths::ProjectContentDir() + TEXT("ArticyContent/Resources/Assets/");
	ImportAudioAssets(AssetBaseDirectory);

	// if we are generating code, generate and compile it; after it has finished, generate assets and perform post import logic
	if (bNeedsCodeGeneration)
	{
		const bool bAnyCodeGenerated = CodeGenerator::GenerateCode(this);

		if (bAnyCodeGenerated)
		{
			static FDelegateHandle PostImportHandle;

			if (PostImportHandle.IsValid())
			{
				FArticyEditorModule::Get().OnCompilationFinished.Remove(PostImportHandle);
				PostImportHandle.Reset();
			}

			// this will have either the current import data or the cached version
			PostImportHandle = FArticyEditorModule::Get().OnCompilationFinished.AddLambda(
				[this](UArticyImportData* Data)
				{
					BuildCachedVersion();
					CodeGenerator::GenerateAssets(Data);
					PostImport();
				});

			CodeGenerator::Recompile(this);
		}
	}
	// if we are importing but no code needed to be generated, generate assets immediately and perform post import
	else
	{
		BuildCachedVersion();
		CodeGenerator::GenerateAssets(this);
		PostImport();
	}

	return true;
}

/**
 * Processes strings and writes them to a CSV output.
 *
 * @param CsvOutput The CSV output generator.
 * @param Data The map of strings and their associated text data.
 * @param Language The language information.
 * @return The number of processed strings.
 */
int UArticyImportData::ProcessStrings(StringTableGenerator* CsvOutput, const TMap<FString, FArticyTexts>& Data, const TPair<FString, FArticyLanguageDef>& Language)
{
	int Counter = 0;

	// Handle object defs
	for (const auto& Text : Data)
	{
		// Send localized data or key, depending on whether data is available
		if (Text.Value.Content.Num() > 0)
		{
			if (Text.Value.Content.Contains(Language.Key))
			{
				// Specific language data
				CsvOutput->Line(Text.Key, Text.Value.Content[Language.Key].Text);
				if (!Text.Value.Content[Language.Key].VoAsset.IsEmpty())
				{
					CsvOutput->Line(Text.Key + ".VOAsset", Text.Value.Content[Language.Key].VoAsset);
				}
			}
			else
			{
				// Infer default from iterator
				const auto& Iterator = Text.Value.Content.CreateConstIterator();
				const auto& Elem = *Iterator;
				CsvOutput->Line(Text.Key, Elem.Value.Text);
				if (!Elem.Value.VoAsset.IsEmpty())
				{
					CsvOutput->Line(Text.Key + ".VOAsset", Elem.Value.VoAsset);
				}
			}
			Counter++;
		}
	}

	return Counter > 0;
}

/**
 * Imports audio assets from a directory.
 *
 * @param BaseContentDir The base content directory.
 */
void UArticyImportData::ImportAudioAssets(const FString& BaseContentDir)
{
    TArray<FString> FilesToImport;

    // Recursively find all .wav and .ogg files in the base directory
    IFileManager& FileManager = IFileManager::Get();
    FileManager.FindFilesRecursive(FilesToImport, *BaseContentDir, TEXT("*.wav"), true, false, false);
    FileManager.FindFilesRecursive(FilesToImport, *BaseContentDir, TEXT("*.ogg"), true, false, false);

    IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    for (const FString& FilePath : FilesToImport)
    {
        // Calculate the relative path from the base directory
        FString RelativePath = FilePath;
        FPaths::MakePathRelativeTo(RelativePath, *BaseContentDir);

        // Determine the package path based on the relative path
        FString PackagePath = TEXT("/Game/ArticyContent/Resources/Assets/") + FPaths::GetPath(RelativePath);
        FString FileName = FPaths::GetBaseFilename(FilePath);
        FString PackageFileName = FPaths::Combine(PackagePath, FileName + TEXT(".uasset"));

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1
        FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(*PackageFileName));
#else
        FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FName(*PackageFileName));
#endif

        if (!AssetData.IsValid())
        {
            // Check if the .uasset file exists on disk and delete it if stale
            FString PackageFilename;
            if (FPackageName::TryConvertLongPackageNameToFilename(PackageFileName, PackageFilename))
            {
                FString FullPackageFilename = PackageFilename;
                if (FPaths::FileExists(FullPackageFilename))
                {
                    UE_LOG(LogArticyEditor, Warning, TEXT("Deleting stale .uasset: %s"), *FullPackageFilename);
                    IFileManager::Get().Delete(*FullPackageFilename);
                }
            }
        }

        // Create a new package
        UPackage* Package = CreatePackage(*FPaths::Combine(PackagePath, FileName));
        if (!Package)
        {
            UE_LOG(LogArticyEditor, Error, TEXT("Failed to create package for: %s"), *FileName);
            continue;
        }

        Package->FullyLoad();

        // Create a new USoundWave object
        USoundWave* NewSoundWave = NewObject<USoundWave>(Package, FName(*FileName), RF_Public | RF_Standalone);
        if (!NewSoundWave)
        {
            UE_LOG(LogArticyEditor, Error, TEXT("Failed to create USoundWave for: %s"), *FileName);
            continue;
        }

        // Import the sound file
        bool bCancelled = false;
        USoundFactory* Factory = NewObject<USoundFactory>();
        if (!Factory)
        {
            UE_LOG(LogArticyEditor, Error, TEXT("Failed to create USoundFactory for: %s"), *FileName);
            continue;
        }

        Factory->SuppressImportDialogs(); // Suppress overwrite prompts
        Factory->bAutoCreateCue = false;

        UObject* ImportedAsset = Factory->ImportObject(NewSoundWave->GetClass(), Package, FName(*FileName), RF_Public | RF_Standalone, FilePath, nullptr, bCancelled);
        if (!ImportedAsset || bCancelled)
        {
            UE_LOG(LogArticyEditor, Error, TEXT("Failed to import sound file: %s"), *FilePath);
            continue;
        }

        // Notify the asset registry
        FAssetRegistryModule::AssetCreated(NewSoundWave);
        Package->MarkPackageDirty();

        // Save the package
        FString PackageOutFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

#if (ENGINE_MAJOR_VERSION >= 5)
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        SaveArgs.Error = GError;
        SaveArgs.bForceByteSwapping = false;
        SaveArgs.bWarnOfLongFilename = false;
        if (!UPackage::SavePackage(Package, NewSoundWave, *PackageOutFileName, SaveArgs))
#else
        if (!UPackage::SavePackage(Package, NewSoundWave, RF_Public | RF_Standalone, *PackageOutFileName, GError))
#endif
        {
            UE_LOG(LogArticyEditor, Error, TEXT("Failed to save package: %s"), *PackageOutFileName);
            continue;
        }

        UE_LOG(LogArticyEditor, Log, TEXT("Successfully imported and saved sound asset: %s"), *FileName);
    }
}

/**
 * Retrieves the import data object.
 *
 * @return The import data object.
 */
const TWeakObjectPtr<UArticyImportData> UArticyImportData::GetImportData()
{
	static TWeakObjectPtr<UArticyImportData> ImportData = nullptr;

	if (!ImportData.IsValid())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			"AssetRegistry");
		TArray<FAssetData> AssetData;

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION > 0
		AssetRegistryModule.Get().GetAssetsByClass(UArticyImportData::StaticClass()->GetClassPathName(), AssetData);
#else
		AssetRegistryModule.Get().GetAssetsByClass(UArticyImportData::StaticClass()->GetFName(), AssetData);
#endif	

		if (!AssetData.Num())
		{
			UE_LOG(LogArticyEditor, Warning, TEXT("Could not find articy import data asset."));
		}
		else
		{
			ImportData = Cast<UArticyImportData>(AssetData[0].GetAsset());

			if (AssetData.Num() > 1)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >0
				UE_LOG(LogArticyEditor, Error,
					TEXT(
						"Found more than one import file. This is not supported by the plugin. Using the first found file for now: %s"
					),
					*AssetData[0].GetObjectPathString());
#else
				UE_LOG(LogArticyEditor, Error,
					TEXT("Found more than one import file. This is not supported by the plugin. Using the first found file for now: %s"),
					*AssetData[0].ObjectPath.ToString());
#endif
		}
	}

	return ImportData;
}

/**
 * Retrieves the list of packages directly.
 *
 * @return The list of packages.
 */
TArray<UArticyPackage*> UArticyImportData::GetPackagesDirect()
{
	TArray<UArticyPackage*> Packages;

	for (auto& Package : ImportedPackages)
	{
		Packages.Add(Package.Get());
	}
	return Packages;
}

/**
 * Gathers scripts from the import data.
 */
void UArticyImportData::GatherScripts()
{
	ScriptFragments.Empty();
	PackageDefs.GatherScripts(this);
}

/**
 * Adds a script fragment to the import data.
 *
 * @param Fragment The script fragment to add.
 * @param bIsInstruction Whether the fragment is an instruction.
 */
void UArticyImportData::AddScriptFragment(const FString& Fragment, const bool bIsInstruction)
{
	//match any group of two words separated by a dot, that does not start with a double quote
	// (?<!["a-zA-Z])(\w+\.\w+)
	//NOTE: static is no good here! crashes on application quit...
	const FRegexPattern unquotedWordDotWord(TEXT("(?<![\"a-zA-Z_])([a-zA-Z_]{1}\\w+\\.\\w+)"));
	//match an assignment operator (an = sign that does not have any of [ = < > ] before it, and no = after it)
	const FRegexPattern assignmentOperator(TEXT("(?<![=<>])=(?!=)"));

	// regex pattern to find literal string, even if they contain escaped quotes (looks nasty if string escaped...): "([^"\\]|\\[\s\S])*" 
	const FRegexPattern literalStringPattern(TEXT("\"([^\"\\\\]|\\\\[\\s\\S])*\""));

	// regex patterns to match exact words "seen", "unseen" and "seenCounter"
	const FRegexPattern seenPattern(TEXT("\\bseen\\b"));
	const FRegexPattern unseenPattern(TEXT("\\bunseen\\b"));
	const FRegexPattern seenCounterPattern(TEXT("\\bseenCounter\\b"));

	bool bCreateBlueprintableUserMethods = UArticyPluginSettings::Get()->bCreateBlueprintTypeForScriptMethods;

	FString string = Fragment; //Fragment.Replace(TEXT("\n"), TEXT(""));

	if (string.Len() > 0)
	{
		static TArray<FString> lines;
		lines.Reset();
		//split into lines
		string.ParseIntoArray(lines, TEXT("\n"));

		string = TEXT("");
		FString comments = TEXT("");
		for (auto& line : lines)
		{
			//remove comment
			//NOTE: this breaks once // is allowed in a string (i.e. in an object name)
			auto doubleSlashPos = line.Find(TEXT("//"));
			if (doubleSlashPos != INDEX_NONE)
			{
				comments += line.Mid(doubleSlashPos) + TEXT("\n");
				line = line.Left(doubleSlashPos);
			}

			//re-compose lines
			string += line + TEXT(" ");
		}

		//now, split at semicolons, i.e. into statements
		string.TrimEndInline();
		string.ParseIntoArray(lines, TEXT(";"));

		//a script condition must not have more than one statement (semicolon)!
		ensure(bIsInstruction || lines.Num() <= 1);

		//re-assemble the string, putting all comments at the top
		string = comments;
		for (auto l = 0; l < lines.Num(); ++l)
		{
			auto& line = lines[l];

			// since "line" gets modified after the literalStrings matcher was created
			// we need to offset the values from the matcher based on the changes done to "line" in the loop
			auto offset = 0;

			// create FStrings from literal strings
			FRegexMatcher literalStrings(literalStringPattern, line);
			while (literalStrings.FindNext())
			{
				auto literalStart = literalStrings.GetMatchBeginning() + offset;
				auto literalEnd = literalStrings.GetMatchEnding() + offset;

				line = line.Left(literalStart) + TEXT("FString(TEXT(") + line.Mid(
					literalStart, literalEnd - literalStart) + TEXT("))") + line.Mid(literalEnd);
				offset += strlen("FString(TEXT(") + strlen("))");
			}

			//find all GV accesses (Namespace.Variable)
			FRegexMatcher gvAccess(unquotedWordDotWord, line);

			//find the last assignment operator in the line
			auto assignments = FRegexMatcher(assignmentOperator, line);
			auto lastAssignment = line.Len();
			while (assignments.FindNext())
				lastAssignment = assignments.GetMatchBeginning();

			offset = 0;

			//replace all remaining Namespace.Variable with *Namespace->Variable
			//note: if the variable appears to the right of an assignment operator,
			//write Namespace->Variable->Get() instead
			while (gvAccess.FindNext())
			{
				auto start = gvAccess.GetMatchBeginning() + offset;
				auto end = gvAccess.GetMatchEnding() + offset;

				literalStrings = FRegexMatcher(literalStringPattern, line);
				auto inLiteral = false;
				while (literalStrings.FindNext())
				{
					// no offset, since this line copy is the current up-to-date one
					auto literalStart = literalStrings.GetMatchBeginning();
					auto literalEnd = literalStrings.GetMatchEnding();

					if (start >= literalStart && end <= literalEnd)
					{
						inLiteral = true;
						break;
					}
				}

				if (!inLiteral)
				{
					// only to GV replacement if we are not within a literal string
					if (lastAssignment < start)
					{
						//there is an assignment operator to the left of this, thus get the raw value
						line = line.Left(start) + line.Mid(start, end - start).Replace(TEXT("."), TEXT("->")) +
							TEXT("->Get()") + line.Mid(end);

						offset += strlen(">") + strlen("->Get()");
					}
					else
					{
						//get the dereferenced variable
						line = line.Left(start) + TEXT("(*") + line.Mid(start, end - start).Replace(
							TEXT("."), TEXT("->")) + ")" + line.Mid(end);
						offset += strlen(".") + strlen(">") + strlen("()");
					}
				} // !inLiteral
			} // GV matching

			// replace "seen" and "unseen" with corresponding expressions
			FRegexMatcher seenMatcher(seenPattern, line);
			FRegexMatcher unseenMatcher(unseenPattern, line);

			while (seenMatcher.FindNext())
			{
				auto start = seenMatcher.GetMatchBeginning() + offset;
				auto end = seenMatcher.GetMatchEnding() + offset;
				line = line.Left(start) + TEXT("seenCounter > 0") + line.Mid(end);
				offset += strlen("seenCounter > 0") - (end - start);
			}

			offset = 0; // reset offset for unseen replacement

			while (unseenMatcher.FindNext())
			{
				auto start = unseenMatcher.GetMatchBeginning() + offset;
				auto end = unseenMatcher.GetMatchEnding() + offset;
				line = line.Left(start) + TEXT("seenCounter == 0") + line.Mid(end);
				offset += strlen("seenCounter == 0") - (end - start);
			}

			FRegexMatcher seenCounterMatcher(seenCounterPattern, line);

			offset = 0; // reset offset for seen counter replacement

			while (seenCounterMatcher.FindNext())
			{
				auto start = seenCounterMatcher.GetMatchBeginning() + offset;
				auto end = seenCounterMatcher.GetMatchEnding() + offset;
				line = line.Left(start) + TEXT("getSeenCounter()") + line.Mid(end);
				offset += strlen("getSeenCounter()") - (end - start);
			}

			//re-compose the string
			string += line;

			//script conditions don't have semicolons!
			if (bIsInstruction)
				string += TEXT(";");

			//the last statement does not need a newline
			if (l < lines.Num() - 1)
				string += TEXT("\n");
		}
	}

	FArticyExpressoFragment frag;
	frag.bIsInstruction = bIsInstruction;
	frag.OriginalFragment = *Fragment;
	frag.ParsedFragment = string;
	ScriptFragments.Add(frag);
}

/**
 * Adds a child to the parent-child cache.
 *
 * @param Parent The parent Articy ID.
 * @param Child The child Articy ID.
 */
void UArticyImportData::AddChildToParentCache(const FArticyId Parent, const FArticyId Child)
{
	// Changed because of the way Map.Find works. In original version there were cases, when first element wasn't added because children was declared out of the scope.
	// It lead to situation when first element of children array wasn't added and for example Codex Locations weren't properly initialized
	auto& childrenRef = ParentChildrenCache.FindOrAdd(Parent);
	childrenRef.Values.AddUnique(Child);
}

/**
 * Builds a cached version of the import data.
 */
void UArticyImportData::BuildCachedVersion()
{
	CachedData.Settings = this->Settings;
	CachedData.Project = this->Project;
	CachedData.GlobalVariables = this->GlobalVariables;
	CachedData.ObjectDefinitions = this->ObjectDefinitions;
	CachedData.PackageDefs = this->PackageDefs;
	CachedData.UserMethods = this->UserMethods;
	CachedData.Hierarchy = this->Hierarchy;
	CachedData.Languages = this->Languages;
	CachedData.ScriptFragments = this->ScriptFragments;
	CachedData.ImportedPackages = this->ImportedPackages;
	CachedData.ParentChildrenCache = this->ParentChildrenCache;
}

/**
 * Resolves the cached version of the import data.
 */
void UArticyImportData::ResolveCachedVersion()
{
	ensure(HasCachedVersion());

	this->Settings = CachedData.Settings;
	this->Project = CachedData.Project;
	this->GlobalVariables = CachedData.GlobalVariables;
	this->ObjectDefinitions = CachedData.ObjectDefinitions;
	this->PackageDefs = CachedData.PackageDefs;
	this->UserMethods = CachedData.UserMethods;
	this->Hierarchy = CachedData.Hierarchy;
	this->Languages = CachedData.Languages;
	this->ScriptFragments = CachedData.ScriptFragments;
	this->ImportedPackages = CachedData.ImportedPackages;
	this->ParentChildrenCache = CachedData.ParentChildrenCache;
	this->CachedData = FArticyImportDataStruct();
	this->bHasCachedVersion = false;
}

#undef LOCTEXT_NAMESPACE

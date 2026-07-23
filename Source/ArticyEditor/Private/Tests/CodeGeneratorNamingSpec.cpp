//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "CodeGeneration/CodeGenerator.h"
#include "ArticyImportData.h"
#include "ObjectDefinitionsImport.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// An import data object carrying just the project technical name, which is all the
	// naming helpers read.
	UArticyImportData* MakeImportData(const FString& TechnicalName)
	{
		UArticyImportData* Data = NewObject<UArticyImportData>();
		Data->Test_GetProject().TechnicalName = TechnicalName;
		return Data;
	}
}

// Every generated class and file is named from the project's technical name, and both the
// generated C++ and the generated assets look each other up by exactly these strings. A
// change here silently orphans an existing project's generated code, so the shapes are
// pinned: the prefix, the suffix, and the bOmittPrefix variant used at call sites that
// need the bare name.
BEGIN_DEFINE_SPEC(FArticyCodeGeneratorNamingSpec, "Articy.Editor.CodeGenerator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyCodeGeneratorNamingSpec)

void FArticyCodeGeneratorNamingSpec::Define()
{
	Describe("generated class names", [this]()
	{
		It("prefixes each class with U and the project technical name", [this]()
		{
			UArticyImportData* Data = MakeImportData(TEXT("MyProject"));

			TestEqual(TEXT("global variables"), CodeGenerator::GetGlobalVarsClassname(Data),
				FString(TEXT("UMyProjectGlobalVariables")));
			TestEqual(TEXT("database"), CodeGenerator::GetDatabaseClassname(Data),
				FString(TEXT("UMyProjectDatabase")));
			TestEqual(TEXT("methods provider"), CodeGenerator::GetMethodsProviderClassname(Data),
				FString(TEXT("UMyProjectMethodsProvider")));
			TestEqual(TEXT("expresso scripts"), CodeGenerator::GetExpressoScriptsClassname(Data),
				FString(TEXT("UMyProjectExpressoScripts")));
			TestEqual(TEXT("type system"), CodeGenerator::GetArticyTypeClassname(Data),
				FString(TEXT("UMyProjectTypeSystem")));
			TestEqual(TEXT("localizer"), CodeGenerator::GetArticyLocalizerClassname(Data),
				FString(TEXT("UMyProjectLocalizerSystem")));
		});

		It("drops the U prefix when asked", [this]()
		{
			UArticyImportData* Data = MakeImportData(TEXT("MyProject"));

			TestEqual(TEXT("global variables"), CodeGenerator::GetGlobalVarsClassname(Data, true),
				FString(TEXT("MyProjectGlobalVariables")));
			TestEqual(TEXT("database"), CodeGenerator::GetDatabaseClassname(Data, true),
				FString(TEXT("MyProjectDatabase")));
			TestEqual(TEXT("expresso scripts"), CodeGenerator::GetExpressoScriptsClassname(Data, true),
				FString(TEXT("MyProjectExpressoScripts")));
			TestEqual(TEXT("type system"), CodeGenerator::GetArticyTypeClassname(Data, true),
				FString(TEXT("MyProjectTypeSystem")));
			TestEqual(TEXT("localizer"), CodeGenerator::GetArticyLocalizerClassname(Data, true),
				FString(TEXT("MyProjectLocalizerSystem")));
		});

		It("names a GV namespace class after the namespace, always with the U prefix", [this]()
		{
			UArticyImportData* Data = MakeImportData(TEXT("MyProject"));
			TestEqual(TEXT("namespace class"),
				CodeGenerator::GetGVNamespaceClassname(Data, TEXT("GameState")),
				FString(TEXT("UMyProjectGameStateVariables")));
		});

		It("names a feature interface with an I prefix and a Feature suffix", [this]()
		{
			UArticyImportData* Data = MakeImportData(TEXT("MyProject"));

			TSharedPtr<FJsonObject> FeatureJson = MakeShared<FJsonObject>();
			FeatureJson->SetStringField(TEXT("TechnicalName"), TEXT("Audio"));
			FArticyTemplateFeatureDef Feature;
			Feature.ImportFromJson(FeatureJson, Data);

			TestEqual(TEXT("interface"), CodeGenerator::GetFeatureInterfaceClassName(Data, Feature),
				FString(TEXT("IMyProjectObjectWithAudioFeature")));
			TestEqual(TEXT("without prefix"), CodeGenerator::GetFeatureInterfaceClassName(Data, Feature, true),
				FString(TEXT("MyProjectObjectWithAudioFeature")));
		});
	});

	Describe("generated file names", [this]()
	{
		It("suffixes the generated headers with their role", [this]()
		{
			UArticyImportData* Data = MakeImportData(TEXT("MyProject"));
			TestEqual(TEXT("interfaces"), CodeGenerator::GetGeneratedInterfacesFilename(Data),
				FString(TEXT("MyProjectInterfaces")));
			TestEqual(TEXT("types"), CodeGenerator::GetGeneratedTypesFilename(Data),
				FString(TEXT("MyProjectArticyTypes")));
		});

		It("writes generated code into the project's ArticyGenerated folder", [this]()
		{
			const FString Folder = CodeGenerator::GetSourceFolder();
			TestTrue(TEXT("under Source"), Folder.Contains(TEXT("Source")));
			TestTrue(TEXT("ArticyGenerated leaf"), Folder.EndsWith(TEXT("ArticyGenerated")));
		});
	});

	Describe("technical names that need no escaping", [this]()
	{
		It("passes the technical name through verbatim", [this]()
		{
			// The technical name is used as a raw C++ identifier fragment - it is not
			// sanitised here, so whatever articy exported has to already be valid.
			UArticyImportData* Data = MakeImportData(TEXT("Project_2"));
			TestEqual(TEXT("database"), CodeGenerator::GetDatabaseClassname(Data),
				FString(TEXT("UProject_2Database")));
		});

		It("still produces the bare suffixes for an empty technical name", [this]()
		{
			UArticyImportData* Data = MakeImportData(FString());
			TestEqual(TEXT("database"), CodeGenerator::GetDatabaseClassname(Data),
				FString(TEXT("UDatabase")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

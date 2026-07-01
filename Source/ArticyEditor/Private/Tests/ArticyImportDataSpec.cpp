//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyImportData.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// Builds a JSON object describing a single global variable.
	TSharedPtr<FJsonObject> MakeGVarJson(const FString& Name, const FString& TypeName)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Variable"), Name);
		Json->SetStringField(TEXT("Type"), TypeName);
		return Json;
	}
}

BEGIN_DEFINE_SPEC(FArticyImportDataSpec, "Articy.Editor.ImportData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyImportDataSpec)

void FArticyImportDataSpec::Define()
{
	Describe("FAdiSettings::ImportFromJson", [this]()
	{
		It("parses settings when the Settings node is included", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("set_IncludedNodes"), TEXT("Settings"));
			Json->SetStringField(TEXT("set_TextFormatter"), TEXT("ArticyTextFormatter"));
			Json->SetBoolField(TEXT("set_UseScriptSupport"), true);
			Json->SetStringField(TEXT("ExportVersion"), TEXT("1.2.3"));

			FAdiSettings Settings;
			Settings.ImportFromJson(Json);

			TestEqual(TEXT("text formatter"), Settings.set_TextFormatter, FString(TEXT("ArticyTextFormatter")));
			TestTrue(TEXT("script support"), Settings.set_UseScriptSupport);
			TestEqual(TEXT("export version"), Settings.ExportVersion, FString(TEXT("1.2.3")));
		});

		It("ignores the input when the Settings node is absent", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("set_IncludedNodes"), TEXT("ObjectDefinitions"));
			Json->SetStringField(TEXT("ExportVersion"), TEXT("9.9.9"));

			FAdiSettings Settings;
			Settings.ImportFromJson(Json);

			TestTrue(TEXT("export version untouched"), Settings.ExportVersion.IsEmpty());
		});
	});

	Describe("FArticyProjectDef::ImportFromJson", [this]()
	{
		It("parses the project name, technical name and guid", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Guid"), TEXT("abc-123"));
			Json->SetStringField(TEXT("TechnicalName"), TEXT("MyProject"));
			Json->SetStringField(TEXT("Name"), TEXT("My Project"));
			Json->SetStringField(TEXT("DetailName"), TEXT("My Project (detail)"));

			FAdiSettings Settings;
			FArticyProjectDef Project;
			Project.ImportFromJson(Json, Settings);

			TestEqual(TEXT("name"), Project.Name, FString(TEXT("My Project")));
			TestEqual(TEXT("technical name"), Project.TechnicalName, FString(TEXT("MyProject")));
			TestEqual(TEXT("guid"), Project.Guid, FString(TEXT("abc-123")));
		});
	});

	Describe("FArticyGVNamespace::ImportFromJson", [this]()
	{
		It("parses the namespace, description and its variables", [this]()
		{
			TArray<TSharedPtr<FJsonValue>> Vars;
			Vars.Add(MakeShared<FJsonValueObject>(MakeGVarJson(TEXT("Score"), TEXT("Integer"))));
			Vars.Add(MakeShared<FJsonValueObject>(MakeGVarJson(TEXT("IsDead"), TEXT("Boolean"))));

			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Namespace"), TEXT("Game"));
			Json->SetStringField(TEXT("Description"), TEXT("Game vars"));
			Json->SetArrayField(TEXT("Variables"), Vars);

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyGVNamespace Ns;
			Ns.ImportFromJson(Json, Data);

			TestEqual(TEXT("namespace"), Ns.Namespace, FString(TEXT("Game")));
			TestEqual(TEXT("description"), Ns.Description, FString(TEXT("Game vars")));
			TestEqual(TEXT("variable count"), Ns.Variables.Num(), 2);
			// Classname is derived from the (empty) project technical name + namespace.
			TestEqual(TEXT("cpp typename"), Ns.CppTypename, FString(TEXT("UGameVariables")));
		});
	});

	Describe("FArticyGVInfo::ImportFromJson", [this]()
	{
		It("parses each namespace in the array", [this]()
		{
			TSharedPtr<FJsonObject> NsJson = MakeShared<FJsonObject>();
			NsJson->SetStringField(TEXT("Namespace"), TEXT("Game"));

			TArray<TSharedPtr<FJsonValue>> Namespaces;
			Namespaces.Add(MakeShared<FJsonValueObject>(NsJson));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyGVInfo Info;
			Info.ImportFromJson(&Namespaces, Data);

			TestEqual(TEXT("namespace count"), Info.Namespaces.Num(), 1);
		});
	});

	Describe("FArticyGVar::ImportFromJson", [this]()
	{
		It("parses a boolean variable and its CPP strings", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeGVarJson(TEXT("IsDead"), TEXT("Boolean"));
			Json->SetBoolField(TEXT("Value"), true);

			FArticyGVar Var;
			Var.ImportFromJson(Json);

			TestEqual(TEXT("name"), Var.Variable, FString(TEXT("IsDead")));
			TestTrue(TEXT("bool value"), Var.BoolValue);
			TestEqual(TEXT("cpp type"), Var.GetCPPTypeString(), FString(TEXT("UArticyBool")));
			TestEqual(TEXT("cpp value"), Var.GetCPPValueString(), FString(TEXT("true")));
		});

		It("parses an integer variable and its CPP strings", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeGVarJson(TEXT("Score"), TEXT("Integer"));
			Json->SetNumberField(TEXT("Value"), 42);

			FArticyGVar Var;
			Var.ImportFromJson(Json);

			TestEqual(TEXT("int value"), Var.IntValue, 42);
			TestEqual(TEXT("cpp type"), Var.GetCPPTypeString(), FString(TEXT("UArticyInt")));
			TestEqual(TEXT("cpp value"), Var.GetCPPValueString(), FString(TEXT("42")));
		});

		It("parses a string variable and quotes its CPP value", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeGVarJson(TEXT("PlayerName"), TEXT("String"));
			Json->SetStringField(TEXT("Value"), TEXT("Bob"));

			FArticyGVar Var;
			Var.ImportFromJson(Json);

			TestEqual(TEXT("string value"), Var.StringValue, FString(TEXT("Bob")));
			TestEqual(TEXT("cpp type"), Var.GetCPPTypeString(), FString(TEXT("UArticyString")));
			TestEqual(TEXT("cpp value"), Var.GetCPPValueString(), FString(TEXT("\"Bob\"")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

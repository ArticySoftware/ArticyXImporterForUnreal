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

	// Runs a raw Expresso fragment through the importer's parser and returns the
	// generated C++ (ParsedFragment) so tests can assert on the transformation.
	FString ParseExpresso(const FString& Fragment, bool bIsInstruction)
	{
		UArticyImportData* Data = NewObject<UArticyImportData>();
		Data->AddScriptFragment(Fragment, bIsInstruction);
		for (const FArticyExpressoFragment& F : Data->GetScriptFragments())
		{
			if (F.OriginalFragment == Fragment && F.bIsInstruction == bIsInstruction)
				return F.ParsedFragment;
		}
		return FString();
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

	Describe("AddScriptFragment: seen/unseen/seenCounter keywords", [this]()
	{
		It("expands bare seen and unseen to boolean getSeenCounter() checks", [this]()
		{
			TestEqual(TEXT("seen"), ParseExpresso(TEXT("seen"), false),
				FString(TEXT("(getSeenCounter() > 0)")));
			TestEqual(TEXT("unseen"), ParseExpresso(TEXT("unseen"), false),
				FString(TEXT("(getSeenCounter() == 0)")));
		});

		It("expands bare seenCounter to the integer getSeenCounter()", [this]()
		{
			TestEqual(TEXT("seenCounter"), ParseExpresso(TEXT("seenCounter"), false),
				FString(TEXT("getSeenCounter()")));
		});

		It("carries an object argument into getSeenCounter (name/id overload)", [this]()
		{
			// Regression: the old two-stage replacement orphaned the argument, e.g.
			// seen("Sally") produced getSeenCounter() > 0("Sally") which failed to compile.
			TestEqual(TEXT("seen(arg)"), ParseExpresso(TEXT("seen(\"Sally\")"), false),
				FString(TEXT("(getSeenCounter(FString(TEXT(\"Sally\"))) > 0)")));
			TestEqual(TEXT("unseen(arg)"), ParseExpresso(TEXT("unseen(\"Sally\")"), false),
				FString(TEXT("(getSeenCounter(FString(TEXT(\"Sally\"))) == 0)")));
			TestEqual(TEXT("seenCounter(arg)"), ParseExpresso(TEXT("seenCounter(\"Sally\")"), false),
				FString(TEXT("getSeenCounter(FString(TEXT(\"Sally\")))")));
		});

		It("keeps comparison operators against the integer seenCounter", [this]()
		{
			// The correct way to compare a count: seenCounter, not seen.
			TestEqual(TEXT("seenCounter > 10"), ParseExpresso(TEXT("seenCounter > 10"), false),
				FString(TEXT("getSeenCounter() > 10")));
			TestEqual(TEXT("seenCounter(arg) >= 10"), ParseExpresso(TEXT("seenCounter(\"Sally\") >= 10"), false),
				FString(TEXT("getSeenCounter(FString(TEXT(\"Sally\"))) >= 10")));
		});

		It("does not touch keywords that appear inside a string literal", [this]()
		{
			// Regression: the keyword replacement used to run over literals too, so a
			// print message containing seen/unseen/seenCounter was corrupted (and could
			// break the import). All three keywords must be left alone inside quotes.
			TestEqual(TEXT("print with seen in text"),
				ParseExpresso(TEXT("print(\"You have seen Sally\")"), true),
				FString(TEXT("print(FString(TEXT(\"You have seen Sally\")));")));
			TestEqual(TEXT("print with unseen in text"),
				ParseExpresso(TEXT("print(\"Sally is unseen\")"), true),
				FString(TEXT("print(FString(TEXT(\"Sally is unseen\")));")));
			TestEqual(TEXT("print with seenCounter in text"),
				ParseExpresso(TEXT("print(\"seenCounter debug\")"), true),
				FString(TEXT("print(FString(TEXT(\"seenCounter debug\")));")));
		});

		It("expands a keyword sitting next to a global-variable access", [this]()
		{
			// Regression: the seen loop reused the leftover offset from GV replacement,
			// corrupting positions when both appeared on the same line.
			TestEqual(TEXT("gv && seen"), ParseExpresso(TEXT("Game.IsDead && seen"), false),
				FString(TEXT("(*Game->IsDead) && (getSeenCounter() > 0)")));
		});

		It("handles multi-statement instructions, expanding keywords per statement", [this]()
		{
			// Instructions may hold several ';'-separated statements; each is emitted on
			// its own line with a trailing ';'. Keyword expansion runs independently per
			// statement, and the real setSeenCounter() function is left untouched.
			TestEqual(TEXT("keyword then gv"),
				ParseExpresso(TEXT("seenCounter(\"Sally\");Game.Score = 5"), true),
				FString(TEXT("getSeenCounter(FString(TEXT(\"Sally\")));\n(*Game->Score) = 5;")));
			TestEqual(TEXT("setSeenCounter preserved, bare seenCounter expanded"),
				ParseExpresso(TEXT("Game.Score = 5;setSeenCounter(\"Sally\", seenCounter)"), true),
				FString(TEXT("(*Game->Score) = 5;\nsetSeenCounter(FString(TEXT(\"Sally\")), getSeenCounter());")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

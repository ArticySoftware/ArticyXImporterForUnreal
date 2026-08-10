//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "ObjectDefinitionsTestJson.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyObjectDefSpec, "Articy.Editor.ObjectDefinitions.ObjectDef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyObjectDefSpec)

void FArticyObjectDefSpec::Define()
{
	//block scope, so the shorthand cannot leak into other files of a unity build
	namespace TestJson = ArticyObjectDefTestJson;

	Describe("ImportFromJson", [this]()
	{
		It("builds a U-prefixed type for a model definition", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("MyEntity"));
			Json->SetStringField(TEXT("Class"), TEXT("Entity"));
			Json->SetArrayField(TEXT("Properties"), TestJson::OneObject(TestJson::PropJson(TEXT("Health"), TEXT("int"))));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("original type"), Def.GetOriginalType().ToString(), FString(TEXT("MyEntity")));
			TestEqual(TEXT("cpp type"), Def.GetCppType(Data, false), FString(TEXT("UMyEntity")));
		});

		It("builds an E-prefixed type for an enum definition", [this]()
		{
			TSharedPtr<FJsonObject> Values = MakeShared<FJsonObject>();
			Values->SetNumberField(TEXT("Happy"), 0);
			Values->SetNumberField(TEXT("Sad"), 1);

			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("Mood"));
			Json->SetObjectField(TEXT("Values"), Values);

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("cpp type"), Def.GetCppType(Data, false), FString(TEXT("EMood")));
		});

		It("parses a template definition", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("NPC"));
			Json->SetStringField(TEXT("Class"), TEXT("Entity"));
			Json->SetObjectField(TEXT("Template"), TestJson::TemplateJson(TEXT("NPCTemplate"), TEXT("NPC")));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("cpp type"), Def.GetCppType(Data, false), FString(TEXT("UNPC")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

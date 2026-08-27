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

		It("records each property's articy type in the type system", [this]()
		{
			TArray<TSharedPtr<FJsonValue>> Properties = TestJson::OneObject(TestJson::PropJson(TEXT("DisplayName"), TEXT("ArticyString")));
			Properties.Add(MakeShared<FJsonValueObject>(TestJson::PropJson(TEXT("Text"), TEXT("ArticyMultiLanguageString"))));

			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("Hub"));
			Json->SetStringField(TEXT("Class"), TEXT("Hub"));
			Json->SetArrayField(TEXT("Properties"), Properties);

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			// What an [Object.Property.$Type] token and Blueprint code use to tell the kinds apart.
			TestEqual(TEXT("plain"), Def.ArticyType.GetProperty(TEXT("DisplayName")).PropertyType, FString(TEXT("ArticyString")));
			TestEqual(TEXT("localized"), Def.ArticyType.GetProperty(TEXT("Text")).PropertyType, FString(TEXT("ArticyMultiLanguageString")));
			TestEqual(TEXT("technical name"), Def.ArticyType.GetProperty(TEXT("Text")).TechnicalName, FString(TEXT("Text")));
			TestFalse(TEXT("not a template property"), Def.ArticyType.GetProperty(TEXT("Text")).IsTemplateProperty);
		});
	});

	// What the importer hands to the type system, and therefore what a
	// [$Type.<Type>.<Property>] token and UArticyBaseObject::GetArticyType() see.
	Describe("ArticyType", [this]()
	{
		It("names the type and describes its own properties", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("MyEntity"));
			Json->SetStringField(TEXT("Class"), TEXT("Entity"));
			Json->SetArrayField(TEXT("Properties"), TestJson::OneObject(TestJson::PropJson(TEXT("Health"), TEXT("int"))));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			// The technical name has to match the key the type is registered under.
			TestEqual(TEXT("technical name"), Def.ArticyType.TechnicalName, Def.GetOriginalType().ToString());
			TestEqual(TEXT("cpp type"), Def.ArticyType.CPPType, FString(TEXT("UMyEntity")));
			TestEqual(TEXT("property type"), Def.ArticyType.GetProperty(TEXT("Health")).PropertyType, FString(TEXT("int")));
		});

		It("describes both its own and its template's properties", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("NPC"));
			Json->SetStringField(TEXT("Class"), TEXT("Entity"));
			Json->SetArrayField(TEXT("Properties"), TestJson::OneObject(TestJson::PropJson(TEXT("DisplayName"), TEXT("string"))));
			Json->SetObjectField(TEXT("Template"), TestJson::TemplateJson(TEXT("NPCTemplate"), TEXT("NPC")));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("own property"), Def.ArticyType.GetProperty(TEXT("DisplayName")).PropertyType, FString(TEXT("string")));
			TestEqual(TEXT("feature property"), Def.ArticyType.GetProperty(TEXT("Stats.HP")).PropertyType, FString(TEXT("int")));
			TestEqual(TEXT("property count"), Def.ArticyType.GetProperties().Num(), 2);
			TestEqual(TEXT("feature count"), Def.ArticyType.Features.Num(), 1);
			TestTrue(TEXT("feature listed by technical name"), Def.ArticyType.Features.Contains(TEXT("Stats")));
		});

		It("describes enum values by technical name", [this]()
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

			TestTrue(TEXT("is enum"), Def.ArticyType.IsEnum);
			TestEqual(TEXT("by name"), Def.ArticyType.GetEnumValue(FString(TEXT("Sad"))).Value, 1);
			TestEqual(TEXT("by value"), Def.ArticyType.GetEnumValue(0).TechnicalName, FString(TEXT("Happy")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

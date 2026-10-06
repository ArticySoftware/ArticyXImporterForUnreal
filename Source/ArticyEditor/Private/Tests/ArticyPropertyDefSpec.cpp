//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyPropertyDefSpec, "Articy.Editor.ObjectDefinitions.PropertyDef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyPropertyDefSpec)

void FArticyPropertyDefSpec::Define()
{
	Describe("ImportFromJson", [this]()
	{
		It("parses a non-localized property and keeps its type", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("CustomStr"));
			Json->SetStringField(TEXT("Type"), TEXT("string"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("property name"), Def.GetPropetyName().ToString(), FString(TEXT("CustomStr")));
			TestEqual(TEXT("type kept"), Def.GetOriginalType().ToString(), FString(TEXT("string")));
		});

		It("promotes a localized string property (e.g. Text) to FText", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("Text"));
			Json->SetStringField(TEXT("Type"), TEXT("string"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("promoted to FText"), Def.GetOriginalType().ToString(), FString(TEXT("FText")));
		});
	});

	// The type system describes properties in articy's own terms; this is what a
	// [$Type.<Type>.<Property>] token resolves to.
	Describe("GetPropertyInfo", [this]()
	{
		It("describes the property by its technical name and declared type", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("Health"));
			Json->SetStringField(TEXT("Type"), TEXT("int"));
			Json->SetStringField(TEXT("DisplayName"), TEXT("NPC.Health.DisplayName"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			const FArticyPropertyInfo& Info = Def.GetPropertyInfo();
			TestEqual(TEXT("technical name"), Info.TechnicalName, FString(TEXT("Health")));
			TestEqual(TEXT("property type"), Info.PropertyType, FString(TEXT("int")));
			TestEqual(TEXT("loca key"), Info.LocaKey_DisplayName, FString(TEXT("NPC.Health.DisplayName")));
			TestFalse(TEXT("not a template property"), Info.IsTemplateProperty);
		});

		It("reports the articy type of a localized property, not its C++ type", [this]()
		{
			// The def itself promotes localized strings to FText so the generated code gets
			// the right C++ type; the type system keeps reporting what articy declared.
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("DisplayName"));
			Json->SetStringField(TEXT("Type"), TEXT("string"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("cpp type promoted"), Def.GetOriginalType().ToString(), FString(TEXT("FText")));
			TestEqual(TEXT("articy type kept"), Def.GetPropertyInfo().PropertyType, FString(TEXT("string")));
		});

		It("falls back to the property name when no display name is given", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("Health"));
			Json->SetStringField(TEXT("Type"), TEXT("int"));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyPropertyDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("loca key"), Def.GetPropertyInfo().LocaKey_DisplayName, FString(TEXT("Health")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

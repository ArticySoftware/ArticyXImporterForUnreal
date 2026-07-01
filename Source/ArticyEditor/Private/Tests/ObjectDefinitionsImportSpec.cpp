//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyObjectDefinitionsImportSpec, "Articy.Editor.ObjectDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyObjectDefinitionsImportSpec)

void FArticyObjectDefinitionsImportSpec::Define()
{
	Describe("FArticyTemplateConstraint::ImportFromJson", [this]()
	{
		It("parses the property, type and localization flag", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Property"), TEXT("MyText"));
			Json->SetStringField(TEXT("Type"), TEXT("string"));
			Json->SetBoolField(TEXT("IsLocalized"), true);

			FArticyTemplateConstraint Constraint;
			Constraint.ImportFromJson(Json);

			TestEqual(TEXT("property"), Constraint.Property, FString(TEXT("MyText")));
			TestEqual(TEXT("type"), Constraint.Type, FString(TEXT("string")));
			TestTrue(TEXT("localized"), Constraint.IsLocalized);
		});
	});

	Describe("FArticyEnumValue::ImportFromJson", [this]()
	{
		It("takes the name from the key and the value from the number", [this]()
		{
			const TPair<FString, TSharedPtr<FJsonValue>> Entry(TEXT("Green"), MakeShared<FJsonValueNumber>(2));

			FArticyEnumValue EnumValue;
			EnumValue.ImportFromJson(Entry);

			TestEqual(TEXT("name"), EnumValue.Name, FString(TEXT("Green")));
			TestEqual(TEXT("value"), static_cast<int32>(EnumValue.Value), 2);
		});
	});

	Describe("FArticyPropertyDef::ImportFromJson", [this]()
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
}

#endif // WITH_AUTOMATION_TESTS

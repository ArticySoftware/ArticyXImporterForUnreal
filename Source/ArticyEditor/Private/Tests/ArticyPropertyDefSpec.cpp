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
}

#endif // WITH_AUTOMATION_TESTS

//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// Wraps a single JSON object into a one-element JSON array (explicit TArray to
	// avoid brace-init template deduction issues with SetArrayField).
	TArray<TSharedPtr<FJsonValue>> OneObject(const TSharedPtr<FJsonObject>& Obj)
	{
		TArray<TSharedPtr<FJsonValue>> Array;
		Array.Add(MakeShared<FJsonValueObject>(Obj));
		return Array;
	}

	TSharedPtr<FJsonObject> PropJson(const FString& Property, const FString& Type)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Property"), Property);
		Json->SetStringField(TEXT("Type"), Type);
		return Json;
	}

	TSharedPtr<FJsonObject> ConstraintJson(const FString& Property, const FString& Type, bool bLocalized)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("Property"), Property);
		Json->SetStringField(TEXT("Type"), Type);
		Json->SetBoolField(TEXT("IsLocalized"), bLocalized);
		return Json;
	}

	// A feature with a single "HP" int property and a matching constraint (a
	// constraint is required for every property, or the parser ensures).
	TSharedPtr<FJsonObject> FeatureJson(const FString& TechnicalName, const FString& DisplayName)
	{
		TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
		Json->SetStringField(TEXT("TechnicalName"), TechnicalName);
		Json->SetStringField(TEXT("DisplayName"), DisplayName);
		Json->SetArrayField(TEXT("Constraints"), OneObject(ConstraintJson(TEXT("HP"), TEXT("int"), false)));
		Json->SetArrayField(TEXT("Properties"), OneObject(PropJson(TEXT("HP"), TEXT("int"))));
		return Json;
	}
}

BEGIN_DEFINE_SPEC(FArticyObjectDefCompositeSpec, "Articy.Editor.ObjectDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyObjectDefCompositeSpec)

void FArticyObjectDefCompositeSpec::Define()
{
	Describe("FArticyTemplateFeatureDef::ImportFromJson", [this]()
	{
		It("parses the feature name and builds its C++ type", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateFeatureDef Feature;
			Feature.ImportFromJson(FeatureJson(TEXT("Stats"), TEXT("Stats Feature")), Data);

			TestEqual(TEXT("technical name"), Feature.GetTechnicalName(), FString(TEXT("Stats")));
			TestEqual(TEXT("display name"), Feature.GetDisplayName(), FString(TEXT("Stats Feature")));
			TestEqual(TEXT("cpp type"), Feature.GetCppType(Data, false), FString(TEXT("UStatsFeature")));
		});
	});

	Describe("FArticyTemplateDef::ImportFromJson", [this]()
	{
		It("parses nested features", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("TechnicalName"), TEXT("NPCTemplate"));
			Json->SetStringField(TEXT("DisplayName"), TEXT("NPC"));
			Json->SetArrayField(TEXT("Features"), OneObject(FeatureJson(TEXT("Stats"), TEXT("Stats Feature"))));

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateDef Template;
			Template.ImportFromJson(Json, Data);

			TestEqual(TEXT("display name"), Template.GetDisplayName(), FString(TEXT("NPC")));
			TestEqual(TEXT("feature count"), Template.GetFeatures().Num(), 1);
			if (Template.GetFeatures().Num() == 1)
			{
				TestEqual(TEXT("feature name"), Template.GetFeatures()[0].GetTechnicalName(), FString(TEXT("Stats")));
			}
		});
	});

	Describe("FArticyObjectDef::ImportFromJson", [this]()
	{
		It("builds a U-prefixed type for a model definition", [this]()
		{
			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("MyEntity"));
			Json->SetStringField(TEXT("Class"), TEXT("Entity"));
			Json->SetArrayField(TEXT("Properties"), OneObject(PropJson(TEXT("Health"), TEXT("int"))));

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
			TSharedPtr<FJsonObject> TemplateJson = MakeShared<FJsonObject>();
			TemplateJson->SetStringField(TEXT("TechnicalName"), TEXT("NPCTemplate"));
			TemplateJson->SetStringField(TEXT("DisplayName"), TEXT("NPC"));
			TemplateJson->SetArrayField(TEXT("Features"), OneObject(FeatureJson(TEXT("Stats"), TEXT("Stats Feature"))));

			TSharedPtr<FJsonObject> Json = MakeShared<FJsonObject>();
			Json->SetStringField(TEXT("Type"), TEXT("NPC"));
			Json->SetStringField(TEXT("Class"), TEXT("Entity"));
			Json->SetObjectField(TEXT("Template"), TemplateJson);

			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyObjectDef Def;
			Def.ImportFromJson(Json, Data);

			TestEqual(TEXT("cpp type"), Def.GetCppType(Data, false), FString(TEXT("UNPC")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "ObjectDefinitionsTestJson.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyTemplateDefSpec, "Articy.Editor.ObjectDefinitions.TemplateDef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTemplateDefSpec)

void FArticyTemplateDefSpec::Define()
{
	//block scope, so the shorthand cannot leak into other files of a unity build
	namespace TestJson = ArticyObjectDefTestJson;

	Describe("ImportFromJson", [this]()
	{
		It("parses nested features", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateDef Template;
			Template.ImportFromJson(TestJson::TemplateJson(TEXT("NPCTemplate"), TEXT("NPC")), Data);

			TestEqual(TEXT("display name"), Template.GetDisplayName(), FString(TEXT("NPC")));
			TestEqual(TEXT("feature count"), Template.GetFeatures().Num(), 1);
			if (Template.GetFeatures().Num() == 1)
			{
				TestEqual(TEXT("feature name"), Template.GetFeatures()[0].GetTechnicalName(), FString(TEXT("Stats")));
			}
		});

		It("lists its features by technical name in the type", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateDef Template;
			Template.ImportFromJson(TestJson::TemplateJson(TEXT("NPCTemplate"), TEXT("NPC")), Data);

			// Technical names, because that is how tokens and generated properties
			// address a feature.
			TestEqual(TEXT("feature count"), Template.ArticyType.Features.Num(), 1);
			TestTrue(TEXT("listed by technical name"), Template.ArticyType.Features.Contains(TEXT("Stats")));
			TestTrue(TEXT("has template"), Template.ArticyType.HasTemplate);
		});

		It("adopts the feature properties under their qualified names", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateDef Template;
			Template.ImportFromJson(TestJson::TemplateJson(TEXT("NPCTemplate"), TEXT("NPC")), Data);

			const FArticyPropertyInfo Info = Template.ArticyType.GetProperty(TEXT("Stats.HP"));
			TestEqual(TEXT("qualified name"), Info.TechnicalName, FString(TEXT("Stats.HP")));
			TestEqual(TEXT("property type"), Info.PropertyType, FString(TEXT("int")));

			const TArray<FArticyPropertyInfo> InFeature = Template.ArticyType.GetPropertiesInFeature(TEXT("Stats"));
			TestEqual(TEXT("properties in feature"), InFeature.Num(), 1);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

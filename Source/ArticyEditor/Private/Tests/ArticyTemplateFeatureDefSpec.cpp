//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "ArticyImportData.h"
#include "ObjectDefinitionsTestJson.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyTemplateFeatureDefSpec, "Articy.Editor.ObjectDefinitions.TemplateFeatureDef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTemplateFeatureDefSpec)

void FArticyTemplateFeatureDefSpec::Define()
{
	//block scope, so the shorthand cannot leak into other files of a unity build
	namespace TestJson = ArticyObjectDefTestJson;

	Describe("ImportFromJson", [this]()
	{
		It("parses the feature name and builds its C++ type", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateFeatureDef Feature;
			Feature.ImportFromJson(TestJson::FeatureJson(TEXT("Stats"), TEXT("Stats Feature")), Data);

			TestEqual(TEXT("technical name"), Feature.GetTechnicalName(), FString(TEXT("Stats")));
			TestEqual(TEXT("display name"), Feature.GetDisplayName(), FString(TEXT("Stats Feature")));
			TestEqual(TEXT("cpp type"), Feature.GetCppType(Data, false), FString(TEXT("UStatsFeature")));
		});

		It("describes its properties as template properties", [this]()
		{
			UArticyImportData* Data = NewObject<UArticyImportData>();
			FArticyTemplateFeatureDef Feature;
			Feature.ImportFromJson(TestJson::FeatureJson(TEXT("Stats"), TEXT("Stats Feature")), Data);

			// Names are unqualified here; the owning template is what prefixes them with
			// the feature name.
			const FArticyPropertyInfo Info = Feature.GetArticyType().GetProperty(TEXT("HP"));
			TestEqual(TEXT("property type"), Info.PropertyType, FString(TEXT("int")));
			TestTrue(TEXT("is a template property"), Info.IsTemplateProperty);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

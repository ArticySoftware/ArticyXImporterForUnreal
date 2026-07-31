//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "Dom/JsonObject.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyTemplateConstraintSpec, "Articy.Editor.ObjectDefinitions.TemplateConstraint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyTemplateConstraintSpec)

void FArticyTemplateConstraintSpec::Define()
{
	Describe("ImportFromJson", [this]()
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
}

#endif // WITH_AUTOMATION_TESTS

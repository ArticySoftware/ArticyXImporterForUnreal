//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ObjectDefinitionsImport.h"
#include "Dom/JsonValue.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyEnumValueSpec, "Articy.Editor.ObjectDefinitions.EnumValue",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyEnumValueSpec)

void FArticyEnumValueSpec::Define()
{
	Describe("ImportFromJson", [this]()
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
}

#endif // WITH_AUTOMATION_TESTS

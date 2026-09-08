//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "Slate/UserInterfaceHelperFunctions.h"
#include "ArticyTestTextModel.h"

#if WITH_AUTOMATION_TESTS

// The asset picker and tooltips name an object through this helper; with a plain FString
// DisplayName it used to reinterpret the property as FText and crash the editor.
BEGIN_DEFINE_SPEC(FArticyUserInterfaceHelpersSpec, "Articy.Editor.UserInterfaceHelpers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyUserInterfaceHelpersSpec)

void FArticyUserInterfaceHelpersSpec::Define()
{
	Describe("GetDisplayName", [this]()
	{
		It("shows a display name stored as FString", [this]()
		{
			UArticyTestPlainNamedObject* Object = NewObject<UArticyTestPlainNamedObject>();
			Object->DisplayName = TEXT("Questions");

			TestEqual(TEXT("display name"), UserInterfaceHelperFunctions::GetDisplayName(Object), FString(TEXT("Questions")));
		});

		It("falls back to a preview of a plain text", [this]()
		{
			UArticyTestPlainNamedObject* Object = NewObject<UArticyTestPlainNamedObject>();
			Object->Text = TEXT("Manfred wakes up in a padded cell.");

			TestEqual(TEXT("preview"), UserInterfaceHelperFunctions::GetDisplayName(Object), FString(TEXT("Manfred wakes ...")));
		});

		It("returns an empty name when display name and text are empty", [this]()
		{
			UArticyTestPlainNamedObject* Object = NewObject<UArticyTestPlainNamedObject>();

			TestTrue(TEXT("empty"), UserInterfaceHelperFunctions::GetDisplayName(Object).IsEmpty());
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

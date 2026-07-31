//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyLocalizerSystem.h"
#include "ArticyTestLocalizer.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// A key no generated string table can contain, so the lookup always misses.
	const TCHAR* MissingKey = TEXT("Articy.Tests.KeyThatIsNotInAnyStringTable");
}

// The lookup falls back through several stages, and the caller's backup text is the last
// one. It used to be unreachable whenever text extension was on: the key was resolved and
// returned instead, so a real project showed the raw key where the fallback was expected.
BEGIN_DEFINE_SPEC(FArticyLocalizerSystemSpec, "Articy.Runtime.LocalizerSystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyLocalizerSystemSpec)

void FArticyLocalizerSystemSpec::Define()
{
	Describe("LocalizeString before the string table is loaded", [this]()
	{
		It("returns the backup text when one is given", [this]()
		{
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(false);

			const FText Key = FText::FromString(MissingKey);
			const FText Backup = FText::FromString(TEXT("fallback"));

			TestEqual(TEXT("backup"), Localizer->LocalizeString(nullptr, Key, true, &Backup).ToString(),
				FString(TEXT("fallback")));
		});

		It("returns the key when no backup text is given", [this]()
		{
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(false);

			const FText Key = FText::FromString(MissingKey);

			TestEqual(TEXT("key"), Localizer->LocalizeString(nullptr, Key, true).ToString(),
				FString(MissingKey));
		});
	});

	Describe("LocalizeString on a key with no table entry", [this]()
	{
		It("prefers the backup text over the key", [this]()
		{
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);

			const FText Key = FText::FromString(MissingKey);
			const FText Backup = FText::FromString(TEXT("fallback"));

			TestEqual(TEXT("backup"), Localizer->LocalizeString(nullptr, Key, true, &Backup).ToString(),
				FString(TEXT("fallback")));
		});

		It("prefers the backup text with text extension turned off", [this]()
		{
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);

			const FText Key = FText::FromString(MissingKey);
			const FText Backup = FText::FromString(TEXT("fallback"));

			TestEqual(TEXT("backup"), Localizer->LocalizeString(nullptr, Key, false, &Backup).ToString(),
				FString(TEXT("fallback")));
		});

		It("falls back to the key when no backup text is given", [this]()
		{
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);

			const FText Key = FText::FromString(MissingKey);

			TestEqual(TEXT("key"), Localizer->LocalizeString(nullptr, Key, true).ToString(),
				FString(MissingKey));
		});

		It("returns a PreviewText key unresolved", [this]()
		{
			// PreviewText is excluded from text extension, so it never goes through Resolve.
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);

			const FText Key = FText::FromString(TEXT("Articy.Tests.Missing.PreviewText"));
			const FText Backup = FText::FromString(TEXT("fallback"));

			TestEqual(TEXT("backup"), Localizer->LocalizeString(nullptr, Key, true, &Backup).ToString(),
				FString(TEXT("fallback")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyLocalizerSystem.h"
#include "ArticyPluginSettings.h"
#include "ArticyTestLocalizer.h"
#include "Misc/EngineVersionComparison.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	// A key no generated string table can contain, so the lookup always misses.
	const TCHAR* MissingKey = TEXT("Articy.Tests.KeyThatIsNotInAnyStringTable");

	// The key the tables registered by RegisterArticyTable use.
	const TCHAR* TableKey = TEXT("Articy.Tests.TableKey");
	const FName ArticyTableId(TEXT("ARTICY"));

	struct FTableEntry
	{
		const TCHAR* Key;
		const TCHAR* Value;
	};

	/** Replaces the articy string table, as loading another language does. */
	void RegisterArticyTable(std::initializer_list<FTableEntry> Entries)
	{
		FStringTableRegistry::Get().UnregisterStringTable(ArticyTableId);

		FStringTableRef Table = FStringTable::NewStringTable();
		Table->SetNamespace(TEXT("ARTICY"));
		for (const FTableEntry& Entry : Entries)
		{
			// UE 5.8 added the dev notes to the editor's string tables
#if WITH_EDITORONLY_DATA && (ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8))
			Table->SetSourceString(FTextKey(FString(Entry.Key)), FString(Entry.Value), FString());
#else
			Table->SetSourceString(FTextKey(FString(Entry.Key)), FString(Entry.Value));
#endif
		}
		FStringTableRegistry::Get().RegisterStringTable(ArticyTableId, Table);
	}

	/** Removes the test table and puts back the one of the project, if there is one. */
	void RestoreArticyTable()
	{
		FStringTableRegistry::Get().UnregisterStringTable(ArticyTableId);
		if (UArticyLocalizerSystem* ProjectLocalizer = UArticyLocalizerSystem::Get())
		{
			ProjectLocalizer->Reload();
		}
	}
}

// The lookup falls back through several stages, and the caller's backup text is the last
// one. It used to be unreachable whenever text extension was on: the key was resolved and
// returned instead, so a real project showed the raw key where the fallback was expected.
BEGIN_DEFINE_SPEC(FArticyLocalizerSystemSpec, "Articy.Runtime.LocalizerSystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	bool bSavedClearOnSwitch = true;
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

	Describe("LocalizeString across a language switch", [this]()
	{
		BeforeEach([this]()
		{
			bSavedClearOnSwitch = GetDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch;
		});

		AfterEach([this]()
		{
			GetMutableDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch = bSavedClearOnSwitch;
			RestoreArticyTable();
		});

		It("keeps the previous text for a key the new language lacks, when the database is not cleared", [this]()
		{
			GetMutableDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch = false;
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);
			const FText Key = FText::FromString(TableKey);

			RegisterArticyTable({ { TableKey, TEXT("Hello") } });
			TestEqual(TEXT("first language"), Localizer->LocalizeString(nullptr, Key, false).ToString(), FString(TEXT("Hello")));

			RegisterArticyTable({});
			Localizer->OnLanguageChanged();

			TestEqual(TEXT("second language"), Localizer->LocalizeString(nullptr, Key, false).ToString(), FString(TEXT("Hello")));
		});

		It("prefers the text of the new language", [this]()
		{
			GetMutableDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch = false;
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);
			const FText Key = FText::FromString(TableKey);

			RegisterArticyTable({ { TableKey, TEXT("Hello") } });
			Localizer->LocalizeString(nullptr, Key, false);

			RegisterArticyTable({ { TableKey, TEXT("Hallo") } });
			Localizer->OnLanguageChanged();

			TestEqual(TEXT("second language"), Localizer->LocalizeString(nullptr, Key, false).ToString(), FString(TEXT("Hallo")));
		});

		It("forgets the previous texts when the database is cleared", [this]()
		{
			GetMutableDefault<UArticyPluginSettings>()->bClearLocalizationOnLanguageSwitch = true;
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);
			const FText Key = FText::FromString(TableKey);

			RegisterArticyTable({ { TableKey, TEXT("Hello") } });
			Localizer->LocalizeString(nullptr, Key, false);

			RegisterArticyTable({});
			Localizer->OnLanguageChanged();

			TestEqual(TEXT("retained texts"), Localizer->GetNumRetainedTexts(), 0);
			TestEqual(TEXT("second language"), Localizer->LocalizeString(nullptr, Key, false).ToString(), FString(TableKey));
		});
	});

	Describe("A missing text", [this]()
	{
		AfterEach([this]()
		{
			RestoreArticyTable();
		});

		It("is reported once per key", [this]()
		{
			AddExpectedError(TEXT("no string table entry for 'Articy.Tests.KeyThatIsNotInAnyStringTable'"), EAutomationExpectedErrorFlags::Contains, 1);

			RegisterArticyTable({});
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);
			Localizer->bReportMissingTexts = true;

			Localizer->LocalizeString(nullptr, FText::FromString(MissingKey), false);
			Localizer->LocalizeString(nullptr, FText::FromString(MissingKey), false);
		});

		It("is reported again after a language switch", [this]()
		{
			AddExpectedError(TEXT("no string table entry"), EAutomationExpectedErrorFlags::Contains, 2);

			RegisterArticyTable({});
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);
			Localizer->bReportMissingTexts = true;

			Localizer->LocalizeString(nullptr, FText::FromString(MissingKey), false);
			Localizer->OnLanguageChanged();
			Localizer->LocalizeString(nullptr, FText::FromString(MissingKey), false);
		});

		It("is not reported unless asked for", [this]()
		{
			RegisterArticyTable({});
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);

			TestEqual(TEXT("key"), Localizer->LocalizeString(nullptr, FText::FromString(MissingKey), false).ToString(), FString(MissingKey));
		});

		It("is not reported for a text that is found, or that is no key", [this]()
		{
			RegisterArticyTable({ { TableKey, TEXT("Hello") } });
			UArticyTestLocalizer* Localizer = NewObject<UArticyTestLocalizer>();
			Localizer->SetDataLoaded(true);
			Localizer->bReportMissingTexts = true;

			Localizer->LocalizeString(nullptr, FText::FromString(TableKey), false);
			Localizer->LocalizeString(nullptr, FText::FromString(TEXT("A plain sentence")), false);
			Localizer->LocalizeString(nullptr, FText::GetEmpty(), false);
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

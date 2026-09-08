//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyDatabase.h"
#include "ArticyTestFilterObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	UArticyTestFilterObject* MakeFilterObject(uint64 Id, const TCHAR* TechnicalName, const TCHAR* DisplayName, const TCHAR* Text)
	{
		UArticyTestFilterObject* Object = NewObject<UArticyTestFilterObject>();
		Object->SetTestId(FArticyId(Id));
		Object->SetTestTechnicalName(TechnicalName);
		Object->DisplayName = FText::FromString(DisplayName);
		Object->Text = FText::FromString(Text);
		return Object;
	}

	bool ContainsObject(const TArray<UArticyObject*>& Objects, const UArticyObject* Object)
	{
		return Objects.Contains(const_cast<UArticyObject*>(Object));
	}
}

// Mirrors ArticyDatabase.FilterObjects / FilterObjectsBasedOn of the Unity importer: one string
// that is either an object id or a part of the technical name, display name or text.
BEGIN_DEFINE_SPEC(FArticyDatabaseFilterSpec, "Articy.Runtime.DatabaseFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyDatabaseFilterSpec)

void FArticyDatabaseFilterSpec::Define()
{
	Describe("FilterObjectsBasedOn", [this]()
	{
		It("returns every object for an empty filter", [this]()
		{
			TArray<UArticyObject*> Objects;
			Objects.Add(MakeFilterObject(0x0100000000000001, TEXT("Chr_Hamster"), TEXT("Hamster"), TEXT("A rodent.")));
			UArticyTestUnnamedObject* Unnamed = NewObject<UArticyTestUnnamedObject>();
			Unnamed->SetTestTechnicalName(TEXT("Jmp_Exit"));
			Objects.Add(Unnamed);

			TestEqual(TEXT("all"), UArticyDatabase::FilterObjectsBasedOn(Objects, FString()).Num(), 2);
		});

		It("matches a hexadecimal object id with or without the 0x prefix", [this]()
		{
			UArticyObject* Hamster = MakeFilterObject(0x0100000000000ABC, TEXT("Chr_Hamster"), TEXT("Hamster"), TEXT(""));
			UArticyObject* Manfred = MakeFilterObject(0x0100000000000DEF, TEXT("Chr_Manfred"), TEXT("Manfred"), TEXT(""));
			const TArray<UArticyObject*> Objects{ Hamster, Manfred };

			TArray<UArticyObject*> Found = UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("0x0100000000000ABC"));
			TestEqual(TEXT("prefixed"), Found.Num(), 1);
			TestTrue(TEXT("prefixed hit"), ContainsObject(Found, Hamster));

			Found = UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("0100000000000abc"));
			TestEqual(TEXT("bare, lower case"), Found.Num(), 1);
			TestTrue(TEXT("bare hit"), ContainsObject(Found, Hamster));
		});

		It("matches a part of the technical name regardless of case", [this]()
		{
			UArticyObject* Hamster = MakeFilterObject(1, TEXT("Chr_Hamster"), TEXT(""), TEXT(""));
			UArticyObject* Manfred = MakeFilterObject(2, TEXT("Chr_Manfred"), TEXT(""), TEXT(""));
			const TArray<UArticyObject*> Objects{ Hamster, Manfred };

			TArray<UArticyObject*> Found = UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("hams"));
			TestEqual(TEXT("one hit"), Found.Num(), 1);
			TestTrue(TEXT("hamster"), ContainsObject(Found, Hamster));

			TestEqual(TEXT("shared prefix"), UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("CHR_")).Num(), 2);
		});

		It("matches a part of the display name", [this]()
		{
			UArticyObject* Hamster = MakeFilterObject(1, TEXT("Chr_1"), TEXT("The Hamster"), TEXT(""));
			UArticyObject* Manfred = MakeFilterObject(2, TEXT("Chr_2"), TEXT("Manfred"), TEXT(""));
			const TArray<UArticyObject*> Objects{ Hamster, Manfred };

			TArray<UArticyObject*> Found = UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("hamster"));
			TestEqual(TEXT("one hit"), Found.Num(), 1);
			TestTrue(TEXT("hamster"), ContainsObject(Found, Hamster));
		});

		It("matches a part of the text", [this]()
		{
			UArticyObject* Cell = MakeFilterObject(1, TEXT("FFr_1"), TEXT("Cell"), TEXT("Manfred wakes up in a padded cell."));
			UArticyObject* Lobby = MakeFilterObject(2, TEXT("FFr_2"), TEXT("Lobby"), TEXT("The lobby is empty."));
			const TArray<UArticyObject*> Objects{ Cell, Lobby };

			TArray<UArticyObject*> Found = UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("padded cell"));
			TestEqual(TEXT("one hit"), Found.Num(), 1);
			TestTrue(TEXT("cell"), ContainsObject(Found, Cell));
		});

		It("only checks the display name and text of objects that have them", [this]()
		{
			UArticyTestUnnamedObject* Jump = NewObject<UArticyTestUnnamedObject>();
			Jump->SetTestTechnicalName(TEXT("Jmp_ToHamster"));
			UArticyObject* Hamster = MakeFilterObject(1, TEXT("Chr_1"), TEXT("Hamster"), TEXT(""));
			const TArray<UArticyObject*> Objects{ Jump, Hamster };

			TestEqual(TEXT("by name"), UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("Hamster")).Num(), 2);
			TestEqual(TEXT("by technical name only"), UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("Jmp")).Num(), 1);
		});

		It("returns nothing when nothing matches", [this]()
		{
			const TArray<UArticyObject*> Objects{ MakeFilterObject(1, TEXT("Chr_Hamster"), TEXT("Hamster"), TEXT("A rodent.")) };

			TestEqual(TEXT("no hit"), UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("Manfred")).Num(), 0);
			TestEqual(TEXT("no id hit"), UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("0x2")).Num(), 0);
		});

		It("drops null entries", [this]()
		{
			const TArray<UArticyObject*> Objects{ nullptr, MakeFilterObject(1, TEXT("Chr_Hamster"), TEXT(""), TEXT("")) };

			TestEqual(TEXT("empty filter"), UArticyDatabase::FilterObjectsBasedOn(Objects, FString()).Num(), 1);
			TestEqual(TEXT("name filter"), UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("Hamster")).Num(), 1);
		});

		It("keeps the element type of a typed array", [this]()
		{
			TArray<UArticyTestFilterObject*> Objects;
			Objects.Add(MakeFilterObject(1, TEXT("Chr_Hamster"), TEXT(""), TEXT("")));
			Objects.Add(MakeFilterObject(2, TEXT("Chr_Manfred"), TEXT(""), TEXT("")));

			const TArray<UArticyTestFilterObject*> Found = UArticyDatabase::FilterObjectsBasedOn(Objects, TEXT("Manfred"));
			TestEqual(TEXT("one hit"), Found.Num(), 1);
			TestEqual(TEXT("typed"), Found[0]->GetTechnicalName().ToString(), FString(TEXT("Chr_Manfred")));
		});
	});

	Describe("MatchesFilter", [this]()
	{
		It("treats a hexadecimal filter as an id and as text", [this]()
		{
			// A filter made of hex digits is still a substring of e.g. a technical name.
			UArticyObject* DeadEnd = MakeFilterObject(0xABC, TEXT("Dlg_DeadEnd"), TEXT(""), TEXT(""));

			TestTrue(TEXT("by id"), UArticyDatabase::MatchesFilter(DeadEnd, TEXT("0xABC")));
			TestTrue(TEXT("by name"), UArticyDatabase::MatchesFilter(DeadEnd, TEXT("dead")));
			TestFalse(TEXT("other id"), UArticyDatabase::MatchesFilter(DeadEnd, TEXT("0xABD")));
		});

		It("never matches a null object", [this]()
		{
			TestFalse(TEXT("empty filter"), UArticyDatabase::MatchesFilter(nullptr, FString()));
			TestFalse(TEXT("name filter"), UArticyDatabase::MatchesFilter(nullptr, TEXT("Hamster")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

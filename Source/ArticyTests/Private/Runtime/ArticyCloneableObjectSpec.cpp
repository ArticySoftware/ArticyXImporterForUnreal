//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyDatabase.h"
#include "ArticyObject.h"

#if WITH_AUTOMATION_TESTS

namespace
{
	/** The clone ids in use, as a comma separated list, so a mismatch names the ids it found. */
	FString CloneIdList(const UArticyCloneableObject* Object)
	{
		TArray<FString> Parts;
		for (int32 Id : Object->GetCloneIds())
		{
			Parts.Add(FString::FromInt(Id));
		}

		return FString::Join(Parts, TEXT(","));
	}

	/** The clone id of a clone, or -1 if there is no clone. */
	int32 CloneIdOf(const UArticyObject* Clone)
	{
		return Clone ? static_cast<int32>(Clone->GetCloneId()) : -1;
	}
}

// Clone ids are handed out two ways that are easy to confuse: ArticyCloneId::NextFree fills the
// lowest gap, while CloneWithConsecutiveId always goes past the highest id in use. These specs
// pin both, and the rules around removing clones.
BEGIN_DEFINE_SPEC(FArticyCloneableObjectSpec, "Articy.Runtime.Clones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	UArticyDatabase* DB = nullptr;
	UArticyCloneableObject* Cloneable = nullptr;
END_DEFINE_SPEC(FArticyCloneableObjectSpec)

void FArticyCloneableObjectSpec::Define()
{
	// The database is only needed as the shadow state manager the clones are taken against
	BeforeEach([this]()
	{
		DB = NewObject<UArticyDatabase>(GetTransientPackage());
		Cloneable = NewObject<UArticyCloneableObject>(GetTransientPackage());
		Cloneable->Init(NewObject<UArticyObject>(GetTransientPackage()));
	});

	Describe("A freshly imported object", [this]()
	{
		It("has nothing but the original", [this]()
		{
			TestEqual(TEXT("clone ids"), CloneIdList(Cloneable), FString(TEXT("0")));
			TestEqual(TEXT("highest clone id"), Cloneable->GetHighestCloneId(), ArticyCloneId::Original);
			TestEqual(TEXT("next free clone id"), Cloneable->GetNextFreeCloneId(), 1);
		});
	});

	Describe("Clone", [this]()
	{
		It("creates the clone with the requested id", [this]()
		{
			UArticyObject* Clone = Cloneable->Clone(DB, 7);

			TestNotNull(TEXT("clone"), Clone);
			TestEqual(TEXT("its clone id"), CloneIdOf(Clone), 7);
			TestEqual(TEXT("clone ids"), CloneIdList(Cloneable), FString(TEXT("0,7")));
		});

		It("refuses to overwrite an existing clone", [this]()
		{
			TestNotNull(TEXT("first clone"), Cloneable->Clone(DB, 1));
			TestNull(TEXT("second clone with the same id"), Cloneable->Clone(DB, 1));
		});

		It("returns the existing clone when it may", [this]()
		{
			UArticyObject* First = Cloneable->Clone(DB, 1);
			UArticyObject* Again = Cloneable->Clone(DB, 1, false);

			TestTrue(TEXT("same clone"), First != nullptr && First == Again);
		});

		It("fills the lowest gap for ArticyCloneId::NextFree", [this]()
		{
			Cloneable->Clone(DB, 1);
			Cloneable->Clone(DB, 3);

			UArticyObject* Filler = Cloneable->Clone(DB, ArticyCloneId::NextFree);

			TestEqual(TEXT("filled the gap at 2"), CloneIdOf(Filler), 2);
			TestEqual(TEXT("clone ids"), CloneIdList(Cloneable), FString(TEXT("0,1,2,3")));
		});
	});

	Describe("CloneWithConsecutiveId", [this]()
	{
		It("starts at the id after the original", [this]()
		{
			TestEqual(TEXT("its clone id"), CloneIdOf(Cloneable->CloneWithConsecutiveId(DB)), 1);
		});

		It("counts up from the highest id in use, leaving gaps alone", [this]()
		{
			Cloneable->Clone(DB, 1);
			Cloneable->Clone(DB, 3);

			TestEqual(TEXT("past the highest, not into the gap at 2"), CloneIdOf(Cloneable->CloneWithConsecutiveId(DB)), 4);
		});

		It("skips a gap a removed clone left below the highest id", [this]()
		{
			Cloneable->Clone(DB, 1);
			Cloneable->Clone(DB, 2);
			Cloneable->RemoveClone(1);

			TestEqual(TEXT("past the highest, not into the freed 1"), CloneIdOf(Cloneable->CloneWithConsecutiveId(DB)), 3);
		});

		It("uses the id of a removed clone again once it is the highest one", [this]()
		{
			Cloneable->Clone(DB, 1);
			Cloneable->Clone(DB, 2);
			Cloneable->RemoveClone(2);

			// Only the ids still in use are counted from, so the freed top id comes back
			TestEqual(TEXT("the freed top id"), CloneIdOf(Cloneable->CloneWithConsecutiveId(DB)), 2);
		});
	});

	Describe("RemoveClone", [this]()
	{
		It("removes the clone and frees its id", [this]()
		{
			Cloneable->Clone(DB, 1);

			TestTrue(TEXT("removed"), Cloneable->RemoveClone(1));
			TestEqual(TEXT("clone ids"), CloneIdList(Cloneable), FString(TEXT("0")));
			TestNull(TEXT("no longer retrievable"), Cloneable->Get(DB, 1));
			TestEqual(TEXT("id is free again"), Cloneable->GetNextFreeCloneId(), 1);
		});

		It("reports nothing removed for an id that is not in use", [this]()
		{
			TestFalse(TEXT("removed"), Cloneable->RemoveClone(5));
		});

		It("refuses to remove the imported original", [this]()
		{
			AddExpectedError(TEXT("it is the object imported from articy"), EAutomationExpectedErrorFlags::Contains, 1);

			TestFalse(TEXT("removed"), Cloneable->RemoveClone(ArticyCloneId::Original));
			TestNotNull(TEXT("original is still there"), Cloneable->Get(DB, ArticyCloneId::Original));
		});
	});

	Describe("RemoveAllClones", [this]()
	{
		It("keeps the original and reports how many went", [this]()
		{
			Cloneable->Clone(DB, 1);
			Cloneable->Clone(DB, 4);

			TestEqual(TEXT("removed"), Cloneable->RemoveAllClones(), 2);
			TestEqual(TEXT("clone ids"), CloneIdList(Cloneable), FString(TEXT("0")));
			TestNotNull(TEXT("original is still there"), Cloneable->Get(DB, ArticyCloneId::Original));
		});

		It("removes nothing when there are no clones", [this]()
		{
			TestEqual(TEXT("removed"), Cloneable->RemoveAllClones(), 0);
			TestEqual(TEXT("clone ids"), CloneIdList(Cloneable), FString(TEXT("0")));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS

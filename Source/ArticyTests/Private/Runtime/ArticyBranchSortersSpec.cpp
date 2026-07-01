//
// Copyright (c) 2026 articy Software GmbH & Co. KG. All rights reserved.
//

#include "Misc/AutomationTest.h"
#include "ArticyBranchSorters.h"
#include "ArticyTestPositional.h"

#if WITH_AUTOMATION_TESTS

BEGIN_DEFINE_SPEC(FArticyBranchSortersSpec, "Articy.Runtime.BranchSorters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
END_DEFINE_SPEC(FArticyBranchSortersSpec)

void FArticyBranchSortersSpec::Define()
{
	using FSorter = ArticyBranchSorters::FArticyBranchPositionSorter;

	Describe("IsPositionSmaller", [this]()
	{
		It("orders by X first", [this]()
		{
			UArticyTestPositional* A = NewObject<UArticyTestPositional>();
			A->TestPosition = FVector2D(1, 5);
			UArticyTestPositional* B = NewObject<UArticyTestPositional>();
			B->TestPosition = FVector2D(2, 0);

			TestTrue(TEXT("A before B"), FSorter::IsPositionSmaller(A, B));
			TestFalse(TEXT("B not before A"), FSorter::IsPositionSmaller(B, A));
		});

		It("falls back to Y when X is equal", [this]()
		{
			UArticyTestPositional* A = NewObject<UArticyTestPositional>();
			A->TestPosition = FVector2D(1, 2);
			UArticyTestPositional* B = NewObject<UArticyTestPositional>();
			B->TestPosition = FVector2D(1, 5);

			TestTrue(TEXT("A before B by Y"), FSorter::IsPositionSmaller(A, B));
			TestFalse(TEXT("B not before A by Y"), FSorter::IsPositionSmaller(B, A));
		});
	});
}

#endif // WITH_AUTOMATION_TESTS
